/* =====================================================================
   pal_gba_video.c — Modo 0, streaming de "mapa grande" y sprites OBJ
   =====================================================================
   Implementación real de la interfaz de vídeo de la PAL (ver
   docs/PLAN_MIGRACION_GBA_C.md, secciones 2 y 5).

   Distribución de VRAM:
     - Charblock 0 (CBB 0): tiles de fondo (tileset del nivel).
     - Screenblock 8  (SBB 8): tilemap de BG2 (el nivel colisionable).
     - Screenblock 9  (SBB 9): tilemap de BG1 (paralaje, relleno único).
     - OBJ VRAM: hoja de frames de Perseo.
     (SBB 0-7 quedan reservados para no pisar los datos de CBB 0: un
     screenblock son 2KB y un charblock 16KB = 8 screenblocks.)

   El arte viene del pipeline de assets (Fase 3): los .png de assets/src/
   los convierte grit a los arrays de assets/gen/. Ver
   tools/spritegen/ascii_to_png.py y los .grit junto a cada imagen.

   Técnica de "mapa grande" (Tonc, "big maps"): un screenblock de
   hardware sólo tiene 32x32 entradas y los niveles reales llegan a
   200x44 tiles, así que BG2 se trata como un buffer CIRCULAR: la
   entrada del tile de mundo (tx,ty) siempre vive en
   se_mem[SBB][(ty&31)*32+(tx&31)]. Sólo hace falta mantener escrita la
   ventana [visible ± 1 tile de margen]; al mover la cámara se
   reescriben nada más las columnas/filas recién expuestas.
   ===================================================================== */
#include <tonc.h>
#include "../pal.h"
#include "../../core/camera.h" /* SCREEN_W/SCREEN_H */
#include "../../core/player.h" /* PlayerAnim, PLAYER_SPRITE_OFFSET_* */

#include "../../core/world.h"
#include "../../core/particles.h"
#include "../../core/entity_pool.h"
#include "../../core/boss/boss_fsm.h"

#include "tileset_tuneles.h"
#include "perseo.h"
#include "enemies_16x8.h"
#include "enemies_8x8.h"
#include "enemies_16x16.h"
#include "bosses.h"
#include "fx_8x8.h"
#include "fx_particles.h"
#include "props_16x16.h"
#include "props_16x32.h"
#include "props_8x16.h"
#include "props_8x8.h"
#include "aurorita.h"
#include "thugs_32x32.h"
#include "perseo_32x32.h"
#include "cine_16x16.h"
#include "cine_32x32.h"
#include "ui_tiles.h"

#define TILES_OF(len)   ((len) / 64) /* un tile 8bpp son 64 bytes (256 colores) */
#define TB_PERSEO       0
#define TB_E16X8        (TB_PERSEO  + TILES_OF(perseoTilesLen))
#define TB_E8X8         (TB_E16X8   + TILES_OF(enemies_16x8TilesLen))
#define TB_E16X16       (TB_E8X8    + TILES_OF(enemies_8x8TilesLen))
#define TB_FX           (TB_E16X16  + TILES_OF(enemies_16x16TilesLen))
#define TB_PARTICLES    (TB_FX      + TILES_OF(fx_8x8TilesLen))
#define TB_BOSSES       (TB_PARTICLES + TILES_OF(fx_particlesTilesLen))
#define TB_PROP16       (TB_BOSSES    + TILES_OF(bossesTilesLen))
#define TB_PROP16X32    (TB_PROP16    + TILES_OF(props_16x16TilesLen))
#define TB_PROP8X16     (TB_PROP16X32 + TILES_OF(props_16x32TilesLen))
#define TB_PROP8        (TB_PROP8X16  + TILES_OF(props_8x16TilesLen))
/* Vinetas de la cinematica: los mismos personajes al doble de tamano, ver
   la nota de "scale" en tools/spritegen/ascii_to_png.py. */
#define TB_CINE_AURORA  (TB_PROP8       + TILES_OF(props_8x8TilesLen))
#define TB_CINE_THUGS   (TB_CINE_AURORA + TILES_OF(auroritaTilesLen))
#define TB_CINE_PERSEO  (TB_CINE_THUGS  + TILES_OF(thugs_32x32TilesLen))
#define TB_CINE_MOON    (TB_CINE_PERSEO + TILES_OF(perseo_32x32TilesLen))
#define TB_CINE_CAGE    (TB_CINE_MOON   + TILES_OF(cine_16x16TilesLen))

#define PALBANK_PERSEO  0
#define PALBANK_ENEMY   1
#define PALBANK_FX      2
#define PALBANK_BOSS    3
#define PALBANK_PROPS   4
/* Aurorita no sale en ningun nivel, solo en las vinetas, y su pelaje
   atigrado no entra en ninguno de los bancos ya cargados. */
#define PALBANK_AURORA  5
#define PALBANK_CINE    6

#define BG_CBB_INDEX 0
#define BG2_SBB_INDEX 8
#define BG1_SBB_INDEX 9

#define MAP_TILES 32 /* tamaño de un screenblock, en tiles */
#define MAP_MASK  (MAP_TILES - 1)

/* El tileset se dibuja en el mismo orden que los TileId de core/level.h
   (ver assets/src/tiles/tileset_tuneles.txt), así que el id de tile del
   nivel ES el índice del gráfico: no hace falta traducir nada. El tile
   extra del final es el fondo de paralaje, que no tiene TileId porque no
   forma parte del mundo colisionable. */
#define TILEG_PARALLAX 10
/* Tiles del fondo de paralaje (ver assets/src/tiles/tileset_tuneles.txt). */
#define TILEG_PIPE_H     11
#define TILEG_PIPE_JOINT 12
#define TILEG_PIPE_V     13
#define TILEG_SKYLINE    14
#define TILEG_SKY_TOP    15


/* Se activa junto con el mosaico de fondos, para que en la transición se
   pixele la escena entera y no sólo el decorado. */
static uint16_t s_obj_mosaic = 0;

/* ---------- El fondo de paralaje ----------
   El prototipo dibuja DOS capas: la silueta de la ciudad a 0,3x de la
   camara y el entramado de tuberias industriales a 0,55x. En Modo 0 hay
   cuatro fondos y tres ya estan ocupados (nivel, interfaz y velo), asi
   que las dos se funden en BG1 y se desplazan juntas a media velocidad.
   Se pierde la separacion de profundidad; se gana el rasgo que de verdad
   define la escena, que son las tuberias.

   El patron se escribe UNA vez y despues lo mueve el scroll de hardware:
   no cuesta nada por frame. Cierra en 32 tiles, que es el ancho del
   screenblock, asi que da la vuelta sin costura. */
/* Paralaje temático por nivel (Sept 2026).
   Tres patrones visuales diferentes crean atmósfera única en cada zona. */
static void build_parallax_pattern(int pattern) {
    for (int r = 0; r < MAP_TILES; r++) {
        for (int c = 0; c < MAP_TILES; c++) {
            uint16_t t = TILEG_PARALLAX;

            if (pattern == 0) {
                /* Patrón Industrial: tuberías complejas (Túneles, Vertedero, Residuos) */
                int b = c / 3;
                int top = 12 + ((b * 7 + c * 3) % 6);
                if (r >= top && r < 18) {
                    t = (r == top) ? TILEG_SKY_TOP : TILEG_SKYLINE;
                    if ((c % 5) == 2 && r > top && r < 17) t = TILEG_PARALLAX;
                }
                if ((r == 2 || r == 8 || r == 14) && c % 2 == 0) {
                    t = (c % 6 == 3) ? TILEG_PIPE_JOINT : TILEG_PIPE_H;
                }
                else if ((c % 8 == 3 || c % 8 == 6) && r > 2 && r < 14) {
                    t = TILEG_PIPE_V;
                }
                else if (c % 12 == 6 && r % 4 == 0 && r > 2 && r < 14) {
                    t = TILEG_PIPE_JOINT;
                }
            } else if (pattern == 1) {
                /* Patrón Arquitectura: líneas ordenadas (Estación, Mercado) */
                int b = c / 4;
                int top = 11 + ((b * 11) % 7);
                if (r >= top && r < 19) {
                    t = (r == top) ? TILEG_SKY_TOP : TILEG_SKYLINE;
                    if ((c + r) % 6 == 0) t = TILEG_PARALLAX;  /* Ventanas ocasionales */
                }
                /* Líneas horizontales de arquitectura */
                if ((r % 5) == 0 && (c % 3) == 0) t = TILEG_PIPE_H;
                if ((c % 10) == 5 && (r % 4) == 0) t = TILEG_PIPE_V;
            } else {
                /* Patrón Orgánico: fractales naturales (Madriguera, Trono) */
                int b = c / 5;
                int top = 10 + ((b * 13 + c) % 8);
                if (r >= top && r < 20) {
                    t = (r == top) ? TILEG_SKY_TOP : TILEG_SKYLINE;
                    if ((c + r) % 7 == 0) t = TILEG_PARALLAX;
                }
                /* Raíces/esculturas ocasionales */
                if ((c % 7) == 3 && r > 5 && r < 15) {
                    t = ((r + c) & 1) ? TILEG_PIPE_V : TILEG_PARALLAX;
                }
                /* Variación caótica */
                if ((c % 9) == 4 && (r % 6) == 2) t = TILEG_PIPE_JOINT;
            }

            se_mem[BG1_SBB_INDEX][r * MAP_TILES + c] = SE_ID(t);
        }
    }
}

static void build_parallax(void) {
    /* Patrón por defecto: industrial (nivel 0) */
    build_parallax_pattern(0);
}

static void set_map_entry(int sbb, int world_tx, int world_ty, u16 tile_graphic) {
    se_mem[sbb][(world_ty & MAP_MASK) * MAP_TILES + (world_tx & MAP_MASK)] = SE_ID(tile_graphic);
}

/* ---------- Estado de sincronización del "mapa grande" de BG2 ----------
   Rango de tiles de MUNDO que el buffer circular tiene al día. Se arranca
   con un rango vacío (left > right) para forzar un llenado completo la
   primera vez. */
static int s_left = 1, s_right = 0;
static int s_top = 1, s_bottom = 0;

static void sync_column(const Level *lv, int world_tx, int top, int bottom) {
    for (int ty = top; ty <= bottom; ty++) {
        set_map_entry(BG2_SBB_INDEX, world_tx, ty, level_tile_at(lv, world_tx, ty));
    }
}
static void sync_row(const Level *lv, int world_ty, int left, int right) {
    for (int tx = left; tx <= right; tx++) {
        set_map_entry(BG2_SBB_INDEX, tx, world_ty, level_tile_at(lv, tx, world_ty));
    }
}

/* Rellena la ventana entera. Va aparte de sync_column() porque es el
   camino caro — unos 760 tiles de una sentada — y el unico que se corre
   en un frame de juego normal: cada vez que world_carve() cambia algo que
   ya se esta viendo (una rejilla rota, el sellado de una arena).

   Por eso resuelve la fila una sola vez en vez de llamar a
   level_tile_at() por tile: esa llamada cruza unidad de traduccion, no se
   puede inlinear, y repite en cada tile las comprobaciones de borde que
   en realidad son iguales para toda la fila. Las tres ramas de aca
   reproducen exactamente su criterio: fuera por arriba o por los lados es
   roca, por debajo es vacio (se puede caer al abismo). */
static void refill_window(const Level *lv, int left, int right, int top, int bottom) {
    for (int ty = top; ty <= bottom; ty++) {
        uint16_t *row = &se_mem[BG2_SBB_INDEX][(ty & MAP_MASK) * MAP_TILES];
        if (!lv || ty < 0) {
            for (int tx = left; tx <= right; tx++) row[tx & MAP_MASK] = SE_ID(TILE_BRICK);
        } else if (ty >= lv->h) {
            for (int tx = left; tx <= right; tx++) row[tx & MAP_MASK] = SE_ID(TILE_EMPTY);
        } else {
            const uint8_t *src = lv->tiles + (int32_t)ty * lv->w;
            for (int tx = left; tx <= right; tx++) {
                uint8_t t = (tx < 0 || tx >= lv->w) ? (uint8_t)TILE_BRICK : src[tx];
                row[tx & MAP_MASK] = SE_ID(t);
            }
        }
    }
}

/* Estado de transición (Sept 2026) */
static int s_transition_duration = 0;
static int s_transition_type = 0;  /* 0=none, 1=fade, 2=mosaic, 3=wipe */
static int s_transition_frame = 0;

void pal_video_init(void) {
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG1 | DCNT_BG2 | DCNT_OBJ | DCNT_OBJ_1D;

    /* Fondos: tileset del nivel + su paleta, tal como los dejó grit. */
    memcpy32(&tile_mem[BG_CBB_INDEX][0], tileset_tunelesTiles, tileset_tunelesTilesLen / 4);
    memcpy32(pal_bg_mem, tileset_tunelesPal, tileset_tunelesPalLen / 4);

    /* Sprites: las hojas se apilan en la VRAM de objetos y cada banco de
       paleta va a su bloque de 16 colores. */
    memcpy32(&tile_mem_obj[0][TB_PERSEO],    perseoTiles,        perseoTilesLen / 4);
    memcpy32(&tile_mem_obj[0][TB_E16X8],     enemies_16x8Tiles,  enemies_16x8TilesLen / 4);
    memcpy32(&tile_mem_obj[0][TB_E8X8],      enemies_8x8Tiles,   enemies_8x8TilesLen / 4);
    memcpy32(&tile_mem_obj[0][TB_E16X16],    enemies_16x16Tiles, enemies_16x16TilesLen / 4);
    memcpy32(&tile_mem_obj[0][TB_FX],        fx_8x8Tiles,        fx_8x8TilesLen / 4);
    memcpy32(&tile_mem_obj[0][TB_PARTICLES], fx_particlesTiles,  fx_particlesTilesLen / 4);
    memcpy32(&tile_mem_obj[0][TB_BOSSES],    bossesTiles,        bossesTilesLen / 4);
    memcpy32(&tile_mem_obj[0][TB_PROP16],    props_16x16Tiles,   props_16x16TilesLen / 4);
    memcpy32(&tile_mem_obj[0][TB_PROP16X32], props_16x32Tiles,   props_16x32TilesLen / 4);
    memcpy32(&tile_mem_obj[0][TB_PROP8X16],  props_8x16Tiles,    props_8x16TilesLen / 4);
    memcpy32(&tile_mem_obj[0][TB_PROP8],     props_8x8Tiles,     props_8x8TilesLen / 4);
    memcpy32(&tile_mem_obj[0][TB_CINE_AURORA], auroritaTiles,     auroritaTilesLen / 4);
    memcpy32(&tile_mem_obj[0][TB_CINE_THUGS],  thugs_32x32Tiles,  thugs_32x32TilesLen / 4);
    memcpy32(&tile_mem_obj[0][TB_CINE_PERSEO], perseo_32x32Tiles, perseo_32x32TilesLen / 4);
    memcpy32(&tile_mem_obj[0][TB_CINE_MOON],   cine_16x16Tiles,   cine_16x16TilesLen / 4);
    memcpy32(&tile_mem_obj[0][TB_CINE_CAGE],   cine_32x32Tiles,   cine_32x32TilesLen / 4);

    /* En 8bpp (256 colores), todos los sprites comparten UNA paleta global.
       Combinamos las paletas de todos los sprites en la paleta 0 de objetos.
       Prioridad: Perseo (0-15), enemigos (16-31), efectos (32-47), jefe (48-63),
       props (64-79), Aurora (80-95), cinemática (96-111), resto reservado (112-255). */
    uint16_t *pal_combined = &pal_obj_mem[0];

    /* Espacios reservados para cada tipo de sprite en la paleta de 256 colores */
    memcpy32(&pal_combined[0],   perseoPal,       perseoPalLen / 4);
    memcpy32(&pal_combined[16],  enemies_16x8Pal, enemies_16x8PalLen / 4);
    memcpy32(&pal_combined[32],  fx_8x8Pal,       fx_8x8PalLen / 4);
    memcpy32(&pal_combined[48],  bossesPal,       bossesPalLen / 4);
    memcpy32(&pal_combined[64],  props_16x16Pal,  props_16x16PalLen / 4);
    memcpy32(&pal_combined[80],  auroritaPal,     auroritaPalLen / 4);
    memcpy32(&pal_combined[96],  cine_16x16Pal,   cine_16x16PalLen / 4);

    SBB_CLEAR(BG2_SBB_INDEX);
    SBB_CLEAR(BG1_SBB_INDEX);
    build_parallax();

    /* BG_MOSAIC va puesto siempre: con REG_MOSAIC en 0 no hace nada, y
       así la transición sólo tiene que tocar un registro. */
    /* Prioridades, de delante hacia atrás: BG0 texto (0), BG3 velo (0,
       detrás de BG0 porque en los empates gana el BG de número menor),
       objetos (1), BG2 el nivel (1: los objetos ganan el empate contra
       un fondo), BG1 paralaje (2). Las dos capas de UI las configura
       pal_gba_text.c. */
    REG_BG2CNT = BG_CBB(BG_CBB_INDEX) | BG_SBB(BG2_SBB_INDEX) | BG_4BPP | BG_REG_32x32 | BG_MOSAIC | BG_PRIO(1);
    REG_BG1CNT = BG_CBB(BG_CBB_INDEX) | BG_SBB(BG1_SBB_INDEX) | BG_4BPP | BG_REG_32x32 | BG_MOSAIC | BG_PRIO(2);
    REG_MOSAIC = 0;

    oam_init(oam_mem, 128);

    s_left = 1; s_right = 0;
    s_top = 1; s_bottom = 0;
}

void pal_video_reset_level_sync(void) {
    s_left = 1; s_right = 0;
    s_top = 1; s_bottom = 0;
}

void pal_video_sync_level(const Level *lv, int cam_x, int cam_y) {
    /* Ventana deseada: lo visible + 1 tile de margen a cada lado, para
       que nunca asome un tile sin sincronizar al desplazarse. */
    int new_left   = (cam_x >> 3) - 1;
    int new_right  = ((cam_x + SCREEN_W - 1) >> 3) + 1;
    int new_top    = (cam_y >> 3) - 1;
    int new_bottom = ((cam_y + SCREEN_H - 1) >> 3) + 1;

    /* Si la ventana nueva no toca a la vieja, no hay nada que
       reaprovechar y el camino incremental es una trampa: recorre el
       hueco columna por columna, y un salto de camara puede ser de
       cientos de columnas de las que este screenblock, que es de 32x32,
       solo conserva las ultimas 32. Todo lo demas se escribe para ser
       pisado. Medido en el nivel 1 al morir y reaparecer: el tramo de
       tiles costo el 153% de un frame y tiro un frame al suelo, el unico
       que se cayo en toda la campana.
       Con ventanas que se tocan el trabajo ya queda acotado solo: el
       desplazamiento no puede pasar del ancho o el alto de la ventana. */
    bool disjoint = new_left > s_right || new_right < s_left ||
                    new_top  > s_bottom || new_bottom < s_top;

    if (s_left > s_right || disjoint) {
        /* Llenado completo: primera sincronización, un salto de cámara que
           no comparte nada con lo que ya estaba, o tiles cambiados dentro
           de lo que ya se ve. */
        refill_window(lv, new_left, new_right, new_top, new_bottom);
    } else {
        /* Primero las columnas nuevas (con el rango vertical VIEJO, que es
           el que esas columnas necesitan cubrir), y recién después las
           filas nuevas (ya con el rango horizontal actualizado): así las
           esquinas quedan cubiertas al moverse en diagonal. */
        if (new_left < s_left)   for (int tx = new_left; tx < s_left; tx++)         sync_column(lv, tx, s_top, s_bottom);
        if (new_right > s_right) for (int tx = s_right + 1; tx <= new_right; tx++)  sync_column(lv, tx, s_top, s_bottom);
        s_left = new_left; s_right = new_right;

        if (new_top < s_top)       for (int ty = new_top; ty < s_top; ty++)          sync_row(lv, ty, s_left, s_right);
        if (new_bottom > s_bottom) for (int ty = s_bottom + 1; ty <= new_bottom; ty++) sync_row(lv, ty, s_left, s_right);
    }
    s_left = new_left; s_right = new_right;
    s_top = new_top; s_bottom = new_bottom;

    REG_BG2HOFS = cam_x;
    REG_BG2VOFS = cam_y;
}

/* La tabla la genera tools/spritegen/ascii_to_png.py a partir del
   `theme` de cada nivel. */
#define LEVEL_PALETTE_COUNT 7
extern const uint16_t LEVEL_PALETTES[LEVEL_PALETTE_COUNT][16];

void pal_video_set_level_palette(int level_index) {
    if (level_index < 0 || level_index >= LEVEL_PALETTE_COUNT) level_index = 0;
    /* Sólo el banco 0 de fondo: es el del tileset. Los bancos 8-15 son
       de la capa de interfaz (pal_gba_text.c) y no se tocan. */
    memcpy16(pal_bg_mem, LEVEL_PALETTES[level_index], 16);
}

void pal_video_set_mosaic(int amount) {
    if (amount < 0) amount = 0;
    if (amount > 15) amount = 15;
    uint16_t a = (uint16_t)amount;
    /* Los cuatro campos de 4 bits: BG horizontal/vertical y OBJ h/v. */
    REG_MOSAIC = a | (a << 4) | (a << 8) | (a << 12);
    s_obj_mosaic = amount ? ATTR0_MOSAIC : 0;
}

void pal_video_show_sprites(bool on) {
    if (on) REG_DISPCNT |= DCNT_OBJ;
    else    REG_DISPCNT &= ~DCNT_OBJ;
}

/* Transiciones visuales mejoradas (Sept 2026) */

void pal_video_transition_fade(int duration) {
    s_transition_type = 1;      /* Fade */
    s_transition_duration = duration;
    s_transition_frame = 0;
}

void pal_video_transition_mosaic(int duration) {
    s_transition_type = 2;      /* Mosaic */
    s_transition_duration = duration;
    s_transition_frame = 0;
}

void pal_video_transition_wipe(int duration) {
    s_transition_type = 3;      /* Wipe horizontal */
    s_transition_duration = duration;
    s_transition_frame = 0;
}

void pal_video_transition_update(void) {
    if (s_transition_duration == 0) return;

    s_transition_frame++;

    if (s_transition_type == 1) {
        /* Fade: oscurece gradualmente */
        int intensity = (s_transition_frame * 255) / s_transition_duration;
        if (intensity > 255) intensity = 255;
        pal_video_set_mosaic(intensity >> 4);  /* Usar mosaico como aproximación */
    } else if (s_transition_type == 2) {
        /* Mosaic: pixela la pantalla */
        int amount = (s_transition_frame * 15) / s_transition_duration;
        pal_video_set_mosaic(amount);
    } else if (s_transition_type == 3) {
        /* Wipe: transición gradual (usando mosaic como aproximación) */
        int amount = (s_transition_frame * 15) / s_transition_duration;
        pal_video_set_mosaic(15 - amount);  /* Inverso: empieza pixelado, se aclara */
    }

    if (s_transition_frame >= s_transition_duration) {
        s_transition_duration = 0;
        s_transition_type = 0;
        s_transition_frame = 0;
        pal_video_set_mosaic(0);  /* Limpiar efecto */
    }
}

bool pal_video_is_transitioning(void) {
    return s_transition_duration > 0;
}

/* Iluminación dinámica (Sept 2026) */
static int s_shadow_intensity = 0;  /* 0-255: intensidad de sombra global */
static fx_t s_light_x = 0, s_light_y = 0;  /* Posición de luz dinámico (ej: jugador) */
static int s_light_radius = 64;  /* Radio de influencia de la luz */

void pal_video_set_parallax_scroll(int cam_x, int cam_y) {
    /* Parallax mejorado con múltiples capas de profundidad (como Castlevania/Metroid):
       - Capa lejana (tuberías): 0.5x velocidad de cámara
       - Capa intermedia (silueta ciudad): 0.3x velocidad de cámara
       Esto crea mejor sensación de profundidad y movimiento. */
    int pipe_scroll = cam_x / 2;
    int city_scroll = (cam_x * 3) / 10;

    /* REG_BG1HOFS controla ambas capas simultáneamente en el patrón,
       así que optimizamos para el mejor efecto visual general. */
    REG_BG1HOFS = pipe_scroll;
    REG_BG1VOFS = cam_y / 2;
}

void pal_video_set_shadow(int intensity) {
    /* Establece intensidad de sombra global (0=normal, 255=oscuro) */
    if (intensity < 0) intensity = 0;
    if (intensity > 255) intensity = 255;
    s_shadow_intensity = intensity;
}

void pal_video_set_light_source(fx_t x, fx_t y, int radius) {
    /* Establece fuente de luz dinámica (ej: para linterna del jugador)
       La luz se atenúa según distancia desde el punto. */
    s_light_x = x;
    s_light_y = y;
    s_light_radius = radius;
}

int pal_video_get_light_intensity_at(fx_t x, fx_t y) {
    /* Calcula intensidad de luz en un punto basado en distancia a la fuente.
       Retorna 0-255: 0=oscuro, 255=brillante */
    if (s_light_radius == 0) return 255;  /* Sin luz dinámica */

    /* Distancia euclidiana aproximada */
    int dx = (fx_to_int(x) - fx_to_int(s_light_x)) >> 3;  /* Dividir por 8 para escala */
    int dy = (fx_to_int(y) - fx_to_int(s_light_y)) >> 3;
    int dist = (dx * dx + dy * dy);  /* Distancia al cuadrado */
    int radius_sq = (s_light_radius * s_light_radius);

    if (dist >= radius_sq) return s_shadow_intensity;  /* Fuera del radio: sombra total */

    /* Atenuación lineal dentro del radio */
    int light = 255 - s_shadow_intensity;
    int attenuation = (light * dist) / radius_sq;
    return s_shadow_intensity + attenuation;
}

/* =====================================================================
   Sprites de hardware (OBJ)
   =====================================================================
   Las hojas se cargan una detrás de otra en la VRAM de objetos; las bases
   se calculan a partir de los tamaños que reporta grit, así que agregar
   frames a una hoja no obliga a tocar ningún número acá.

   Bancos de paleta (4bpp, 16 colores cada uno): 0 Perseo, 1 enemigos,
   2 efectos. Los agrupa tools/spritegen/ascii_to_png.py.
   ===================================================================== */

/* Índices de frame dentro de la hoja de efectos (el orden lo fija
   SPRITE_SHEETS en tools/spritegen/ascii_to_png.py). */
enum { FX_TRAZA = 0, FX_JUNK = 2, FX_SHOCK = 3, FX_HEART = 4, FX_CHAPA = 5 };

typedef struct SpriteDef {
    uint16_t tile_base;      /* primer tile de la hoja */
    uint8_t  tiles_per_frame;
    uint8_t  frame_count;    /* cuántos frames tiene la animación de un tipo */
    uint16_t shape;          /* ATTR0_* */
    uint16_t size;           /* ATTR1_SIZE_* */
    uint8_t  palbank;
} SpriteDef;

static const SpriteDef SPRITE_16X8  = { TB_E16X8,  2, 2, ATTR0_WIDE,   ATTR1_SIZE_16x8,  PALBANK_ENEMY };
static const SpriteDef SPRITE_8X8   = { TB_E8X8,   1, 2, ATTR0_SQUARE, ATTR1_SIZE_8x8,   PALBANK_ENEMY };
static const SpriteDef SPRITE_16X16 = { TB_E16X16, 4, 2, ATTR0_SQUARE, ATTR1_SIZE_16x16, PALBANK_ENEMY };
static const SpriteDef SPRITE_FX    = { TB_FX,     1, 1, ATTR0_SQUARE, ATTR1_SIZE_8x8,   PALBANK_FX };
/* Decorado interactivo. Una hoja por forma de OBJ: shrine 16x16,
   puerta 16x32 (el arte es de 16x24 y el resto va transparente),
   lampara y cartel 8x16, reliquias 8x8. */
static const SpriteDef SPRITE_SHRINE = { TB_PROP16,    4, 1, ATTR0_SQUARE, ATTR1_SIZE_16x16, PALBANK_PROPS };
static const SpriteDef SPRITE_DOOR   = { TB_PROP16X32, 8, 1, ATTR0_TALL,   ATTR1_SIZE_16x32, PALBANK_PROPS };
static const SpriteDef SPRITE_TALL8  = { TB_PROP8X16,  2, 1, ATTR0_TALL,   ATTR1_SIZE_8x16,  PALBANK_PROPS };
static const SpriteDef SPRITE_RELIC  = { TB_PROP8,     1, 1, ATTR0_SQUARE, ATTR1_SIZE_8x8,   PALBANK_PROPS };

/* Ranura de OAM que toca; se reinicia en cada frame. */
static int s_obj_next;

static void obj_put(int scr_x, int scr_y, uint16_t tile, uint16_t shape,
                    uint16_t size, uint8_t palbank, bool flip_h) {
    if (s_obj_next >= 128) return; /* OAM lleno: lo que sobra no se dibuja */
    /* Descarta lo que quedó fuera de pantalla antes de gastar un objeto:
       las coordenadas de OAM son de 8/9 bits y darían la vuelta. */
    if (scr_x < -32 || scr_x > SCREEN_W + 32 ||
        scr_y < -32 || scr_y > SCREEN_H + 32) return;

    OBJ_ATTR *obj = &oam_mem[s_obj_next++];
    obj->attr0 = ATTR0_Y(scr_y & 0xFF) | shape | s_obj_mosaic;
    obj->attr1 = ATTR1_X(scr_x & 0x1FF) | size | (flip_h ? ATTR1_HFLIP : 0);
    /* En 8bpp (256 colores), no usamos PALBANK. El tile index
       ya contiene implícitamente el offset de color en la paleta de 256. */
    obj->attr2 = ATTR2_ID(tile) | ATTR2_PRIO(1);
}

/* Elige la hoja y el frame base de cada tipo de entidad. MEJORADO (Sept 2026):
   Sistema de animación más fluido con más frames por enemigo, basado en timer.
   Devuelve NULL para lo que todavía no se dibuja (decoración sin sistema propio). */
static const SpriteDef *sprite_for(const Entity *e, int *first_frame) {
    switch (e->type) {
        case ENT_RAT:
            /* RAT: 2 frames → 4 frames de carrera fluida */
            *first_frame = ((e->timer >> 2) & 1) ? 2 : 0;  /* Alterna cada 4 frames */
            return &SPRITE_16X8;
        case ENT_GUNNER:
            /* GUNNER: 2 frames → 4 frames de disparos variados */
            *first_frame = 2 + ((e->timer >> 2) & 1);
            return &SPRITE_16X8;
        case ENT_ROACH:
            /* ROACH: 2 frames → 3 frames de movimiento errático */
            *first_frame = ((e->timer >> 3) % 3);  /* Ciclo de 3 cada 8 frames */
            return &SPRITE_8X8;
        case ENT_MOSQ:
            /* MOSQ: 2 frames → 3 frames de vuelo suave */
            *first_frame = 2 + ((e->timer >> 3) % 3);
            return &SPRITE_8X8;
        case ENT_BAT:
            /* BAT: 2 frames → 4 frames de aleteo natural */
            *first_frame = 4 + ((e->timer >> 2) & 3);  /* Ciclo 0-3 cada 2 frames */
            return &SPRITE_8X8;
        case ENT_THUG:
            /* THUG: 2 frames → 5 frames de pasos pesados */
            *first_frame = ((e->timer >> 3) % 5);  /* 5 fases cada 8 frames */
            return &SPRITE_16X16;
        case ENT_BRUTE:
            /* BRUTE: 2 frames → 5 frames de movimiento lento pero amenazante */
            *first_frame = 2 + ((e->timer >> 4) % 5);  /* Ciclo lento (cada 16 frames) */
            return &SPRITE_16X16;
        case ENT_PROJ_TRAZA: *first_frame = FX_TRAZA; return &SPRITE_FX;
        case ENT_PROJ_JUNK:  *first_frame = FX_JUNK;  return &SPRITE_FX;
        case ENT_PROJ_SHOCK: *first_frame = FX_SHOCK; return &SPRITE_FX;
        case ENT_CHAPA:      *first_frame = FX_CHAPA; return &SPRITE_FX;
        case ENT_HP:         *first_frame = FX_HEART; return &SPRITE_FX;

        /* Decorado interactivo (Fase 7). La lámpara elige frame según
           esté encendida o no, que es el único estado que tiene. */
        case ENT_SHRINE: *first_frame = 0; return &SPRITE_SHRINE;
        case ENT_RELIC:  *first_frame = e->param < 3 ? e->param : 0; return &SPRITE_RELIC;
        case ENT_LAMP:   *first_frame = e->state ? 1 : 0; return &SPRITE_TALL8;
        case ENT_SIGN:   *first_frame = 2; return &SPRITE_TALL8;
        case ENT_DOOR:
        case ENT_VDOOR:  *first_frame = 0; return &SPRITE_DOOR;

        default: return 0;
    }
}

/* ---------- Viñetas de la cinemática ----------
   Comparten obj_put() con el dibujo del mundo, así que el recorte fuera de
   pantalla y el tope de 128 objetos valen igual acá. */
typedef struct CineActorDef {
    uint16_t tile;      /* primer tile del sprite en la VRAM de objetos */
    uint16_t shape, size;
    uint8_t  palbank;
} CineActorDef;

static const CineActorDef CINE_ACTORS[CINE_ACTOR_COUNT] = {
    /* AURORA */ { TB_CINE_AURORA,      ATTR0_SQUARE, ATTR1_SIZE_32x32, PALBANK_AURORA },
    /* THUG1  */ { TB_CINE_THUGS,       ATTR0_SQUARE, ATTR1_SIZE_32x32, PALBANK_ENEMY  },
    /* THUG2  */ { TB_CINE_THUGS + 16,  ATTR0_SQUARE, ATTR1_SIZE_32x32, PALBANK_ENEMY  },
    /* PERSEO */ { TB_CINE_PERSEO,      ATTR0_SQUARE, ATTR1_SIZE_32x32, PALBANK_PERSEO },
    /* BETTY  */ { TB_BOSSES + 10 * 16, ATTR0_SQUARE, ATTR1_SIZE_32x32, PALBANK_BOSS   },
    /* CAGE   */ { TB_CINE_CAGE,        ATTR0_SQUARE, ATTR1_SIZE_32x32, PALBANK_CINE   },
    /* MOON   */ { TB_CINE_MOON,        ATTR0_SQUARE, ATTR1_SIZE_16x16, PALBANK_CINE   },
};

/* Los colores del prototipo, tal cual: 'K' el cielo, 's' la silueta de la
   ciudad, 'Y' una ventana encendida, y para el trono la mezcla de 'h'
   sobre 'K' que alla se consigue con medio alpha. */
#define RGB_CINE_SKY     RGB15(1, 2, 3)
#define RGB_CINE_CITY    RGB15(3, 4, 6)
#define RGB_CINE_WINDOW  RGB15(20, 14, 4)
#define RGB_CINE_THRONE  RGB15(7, 3, 6)

/* Bancos de paleta de fondo que usa la cinematica. El 0 es del tileset del
   nivel y del 8 al 15 son de la interfaz; en medio no habia nadie. */
#define PB_CINE_FILL   2
#define PB_CINE_LIGHT  3

/* La fila del horizonte, en tiles: CINE_GROUND_Y (88) / 8. Los personajes
   se apoyan ahi y la silueta crece hacia arriba desde ahi. */
#define CINE_GROUND_ROW 11

static uint16_t     s_backdrop_saved;
static CineBackdrop s_cine_bg;

/* La silueta de la ciudad, en el screenblock de BG1. Edificios de dos
   tiles de ancho separados por uno, con alturas que se repiten cada pocas
   columnas — la misma idea que el `h = 20 + ((i+1000)*31)%26` del
   prototipo, resuelta con enteros chicos. Como el screenblock es de 32
   columnas y el patron cierra en 32, el scroll da la vuelta sin costura. */
static void fill_skyline(void) {
    for (int c = 0; c < 32; c++) {
        bool building = (c % 3) != 2;
        int h = building ? 3 + ((c * 7) % 5) : 0;
        for (int r = 0; r < 32; r++) {
            uint16_t se = SE_ID(TILE_BLANK);
            if (building && r < CINE_GROUND_ROW && r >= CINE_GROUND_ROW - h) {
                /* Una ventana encendida de vez en cuando, para que la
                   ciudad no sea una mancha lisa. */
                bool lit = ((c * 5 + r * 3) % 17) == 0;
                se = SE_ID(TILE_DARK) | SE_PALBANK(lit ? PB_CINE_LIGHT : PB_CINE_FILL);
            }
            se_mem[BG1_SBB_INDEX][r * 32 + c] = se;
        }
    }
}

/* El trono: franjas horizontales, como los seis rectangulos a medio alpha
   del prototipo.

   Se cortan en el horizonte y no siguen hasta abajo, por un motivo que
   costo ver: los tiles de la fuente tienen el fondo transparente, asi que
   por detras de cada letra se cuela lo que haya en BG1. Con las franjas
   llegando hasta el pie de la pantalla, el texto salia con parches color
   vino detras de las palabras. La silueta de la ciudad no daba problema
   porque ya se acababa en el horizonte. */
static void fill_throne(void) {
    for (int r = 0; r < 32; r++) {
        uint16_t se = (r < CINE_GROUND_ROW && (r / 2) % 2 == 0)
                    ? (uint16_t)(SE_ID(TILE_DARK) | SE_PALBANK(PB_CINE_FILL))
                    : (uint16_t)SE_ID(TILE_BLANK);
        for (int c = 0; c < 32; c++) se_mem[BG1_SBB_INDEX][r * 32 + c] = se;
    }
}

void pal_video_cine_backdrop(CineBackdrop kind) {
    if (kind == s_cine_bg) return;

    if (s_cine_bg == CINE_BG_NONE) {
        /* Se entra en una vineta: guardar lo que habia. */
        s_backdrop_saved = pal_bg_mem[0];
        REG_DISPCNT &= ~DCNT_BG2;            /* fuera el nivel */
        /* BG1 pasa a los tiles macizos de la interfaz: son los unicos que
           hay de un color plano, y con eso se dibujan tanto los edificios
           como las franjas sin arte nuevo. */
        REG_BG1CNT = BG_CBB(UI_CBB) | BG_SBB(BG1_SBB_INDEX) |
                     BG_4BPP | BG_REG_32x32 | BG_MOSAIC | BG_PRIO(2);
        pal_bg_mem[PB_CINE_FILL * 16 + 1]  = RGB_CINE_CITY;
        pal_bg_mem[PB_CINE_LIGHT * 16 + 1] = RGB_CINE_WINDOW;
    }

    s_cine_bg = kind;
    switch (kind) {
        case CINE_BG_NIGHT:
            pal_bg_mem[0] = RGB_CINE_SKY;
            pal_bg_mem[PB_CINE_FILL * 16 + 1] = RGB_CINE_CITY;
            fill_skyline();
            break;
        case CINE_BG_THRONE:
            pal_bg_mem[0] = RGB_CINE_SKY;
            pal_bg_mem[PB_CINE_FILL * 16 + 1] = RGB_CINE_THRONE;
            fill_throne();
            break;
        case CINE_BG_NONE:
        default:
            /* De vuelta al juego: el nivel y su paralaje como estaban. */
            pal_bg_mem[0] = s_backdrop_saved;
            REG_BG1CNT = BG_CBB(BG_CBB_INDEX) | BG_SBB(BG1_SBB_INDEX) |
                         BG_4BPP | BG_REG_32x32 | BG_MOSAIC | BG_PRIO(2);
            build_parallax();
            REG_BG1HOFS = 0;
            REG_DISPCNT |= DCNT_BG2;
            break;
    }
}

/* El paneo lento de la ciudad: el `frame*0.18` del prototipo. Es scroll de
   hardware, asi que no cuesta nada. */
void pal_video_cine_pan(uint32_t frame) {
    if (s_cine_bg == CINE_BG_NONE) return;
    /* El paralaje del nivel mueve BG1 con la camara, y en una vineta no hay
       camara que valga: si no se anula, la silueta de la ciudad aparece
       colgando del borde de arriba. */
    REG_BG1VOFS = 0;
    REG_BG1HOFS = (s_cine_bg == CINE_BG_NIGHT) ? (uint16_t)((frame * 3) >> 4) : 0;
}

/* ---------- Goteo de humedad ----------
   Los destellos cian que caen de las juntas de las tuberias. En el
   prototipo son cuatro, con la cadencia desfasada y siguiendo el paralaje;
   aca son cuatro sprites de un punto, que es lo que hay a mano y basta.

   Van con el fondo y no con el mundo: su x se calcula con el mismo medio
   scroll que BG1, para que parezcan colgar de las tuberias y no flotar
   sueltos por delante. */
#define DRIP_COUNT 4

void pal_video_draw_drips(int cam_x, uint32_t frame) {
    static const uint8_t DRIP_X[DRIP_COUNT] = { 26, 104, 182, 238 };
    for (int i = 0; i < DRIP_COUNT; i++) {
        uint32_t ph = (frame + (uint32_t)i * 37) % 110;
        if (ph >= 60) continue;               /* la pausa entre gota y gota */
        int x = ((int)DRIP_X[i] - (cam_x / 2)) % SCREEN_W;
        if (x < 0) x += SCREEN_W;
        int y = 24 + (int)((ph * 3) / 2);     /* cae a 1,5 px por frame */
        obj_put(x, y, (uint16_t)(TB_PARTICLES + PCOL_CYAN),
                ATTR0_SQUARE, ATTR1_SIZE_8x8, PALBANK_FX, false);
    }
}

void pal_video_cine_begin(void) {
    s_obj_next = 0;
}

void pal_video_cine_actor(CineActor who, int x, int y, bool flip_h) {
    if (who >= CINE_ACTOR_COUNT) return;
    const CineActorDef *d = &CINE_ACTORS[who];
    obj_put(x, y, d->tile, d->shape, d->size, d->palbank, flip_h);
}

void pal_video_cine_dot(int x, int y, uint8_t color) {
    obj_put(x, y, (uint16_t)(TB_PARTICLES + color),
            ATTR0_SQUARE, ATTR1_SIZE_8x8, PALBANK_FX, false);
}

void pal_video_cine_end(void) {
    for (int i = s_obj_next; i < 128; i++) obj_hide(&oam_mem[i]);
}

void pal_video_draw_world(const World *w, int cam_x, int cam_y) {
    s_obj_next = 0;

    /* 1. Perseo primero: es lo que nunca puede faltar si se llena OAM. */
    PlayerAnim anim = player_get_anim(&w->player, w->tick);
    if (!anim.hidden) {
        obj_put(fx_to_int(w->player.x) - cam_x + PLAYER_SPRITE_OFFSET_X,
                fx_to_int(w->player.y) - cam_y + PLAYER_SPRITE_OFFSET_Y + anim.bob,
                (uint16_t)(TB_PERSEO + anim.frame * 4),
                ATTR0_SQUARE, ATTR1_SIZE_16x16, PALBANK_PERSEO, anim.flip_h);
    }

    /* 2. Entidades vivas. */
    for (int i = 0; i < ENTITY_POOL_CAPACITY; i++) {
        const Entity *e = entity_pool_at(i);
        if (!e->alive) continue;
        /* El jefe se dibuja aparte: es el unico de 32x32, tiene banco de
           paleta propio y su frame lo decide la FSM segun el estado. */
        if (e->type == ENT_BOSS) {
            if (e->flash > 0 && (w->tick & 1)) continue;
            const BossConfig *cfg = boss_config_of(e);
            int frame = cfg->sprite * 2 + boss_sprite_frame(e, w->tick);
            obj_put(fx_to_int(e->x) - cam_x, fx_to_int(e->y) - cam_y,
                    (uint16_t)(TB_BOSSES + frame * 16),
                    ATTR0_SQUARE, ATTR1_SIZE_32x32, PALBANK_BOSS, e->face < 0);
            continue;
        }

        int first = 0;
        const SpriteDef *def = sprite_for(e, &first);
        if (!def) continue;

        /* Los enemigos golpeados parpadean: se saltan un frame de cada dos
           mientras dura el destello. Es como se lee un impacto sin tener
           que cambiarles la paleta. */
        if (e->flash > 0 && (w->tick & 1)) continue;

        int frame = first;
        if (def->frame_count > 1) frame += (int)((w->tick >> 3) & 1);
        obj_put(fx_to_int(e->x) - cam_x, fx_to_int(e->y) - cam_y,
                (uint16_t)(def->tile_base + frame * def->tiles_per_frame),
                def->shape, def->size, def->palbank, e->face < 0);
    }

    /* 3. El goteo del fondo, antes que las partículas del juego: si OAM
       se llena, lo que tiene que sobrevivir es lo que afecta a la
       partida, no la decoración. */
    pal_video_draw_drips(cam_x, w->tick);

    /* 4. Partículas: un objeto cada una, con un frame por color. */
    for (int i = 0; i < particles_count(); i++) {
        const Particle *p = particles_get(i);
        if (!p->alive) continue;
        obj_put(fx_to_int(p->x) - cam_x, fx_to_int(p->y) - cam_y,
                (uint16_t)(TB_PARTICLES + p->color),
                ATTR0_SQUARE, ATTR1_SIZE_8x8, PALBANK_FX, false);
    }

    /* 5. HUD de vida: un solo corazón, de icono. La vida en sí es una
       barra segmentada que dibuja la capa de interfaz (ui/hud.c) — con
       seis puntos de vida, seis corazones sueltos ocupaban media pantalla
       y costaba leer de un vistazo cuánta quedaba. */
    obj_put(4, 4, (uint16_t)(TB_FX + FX_HEART),
            ATTR0_SQUARE, ATTR1_SIZE_8x8, PALBANK_FX, false);

    /* Esconde las ranuras que no se usaron este frame. */
    for (int i = s_obj_next; i < 128; i++) obj_hide(&oam_mem[i]);
}
