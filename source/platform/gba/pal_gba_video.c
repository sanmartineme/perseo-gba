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

#define TILES_OF(len)   ((len) / 32) /* un tile 4bpp son 32 bytes */
#define TB_PERSEO       0
#define TB_E16X8        (TB_PERSEO  + TILES_OF(perseoTilesLen))
#define TB_E8X8         (TB_E16X8   + TILES_OF(enemies_16x8TilesLen))
#define TB_E16X16       (TB_E8X8    + TILES_OF(enemies_8x8TilesLen))
#define TB_FX           (TB_E16X16  + TILES_OF(enemies_16x16TilesLen))
#define TB_PARTICLES    (TB_FX      + TILES_OF(fx_8x8TilesLen))
#define TB_BOSSES       (TB_PARTICLES + TILES_OF(fx_particlesTilesLen))

#define PALBANK_PERSEO  0
#define PALBANK_ENEMY   1
#define PALBANK_FX      2
#define PALBANK_BOSS    3

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

    memcpy32(&pal_obj_mem[PALBANK_PERSEO * 16], perseoPal,       perseoPalLen / 4);
    memcpy32(&pal_obj_mem[PALBANK_ENEMY * 16],  enemies_16x8Pal, enemies_16x8PalLen / 4);
    memcpy32(&pal_obj_mem[PALBANK_FX * 16],     fx_8x8Pal,       fx_8x8PalLen / 4);
    memcpy32(&pal_obj_mem[PALBANK_BOSS * 16],   bossesPal,       bossesPalLen / 4);

    SBB_CLEAR(BG2_SBB_INDEX);
    SBB_CLEAR(BG1_SBB_INDEX);
    /* BG1 es un único tile de relleno repetido: no necesita streaming. */
    for (int i = 0; i < MAP_TILES * MAP_TILES; i++) {
        se_mem[BG1_SBB_INDEX][i] = SE_ID(TILEG_PARALLAX);
    }

    REG_BG2CNT = BG_CBB(BG_CBB_INDEX) | BG_SBB(BG2_SBB_INDEX) | BG_4BPP | BG_REG_32x32 | BG_PRIO(0);
    REG_BG1CNT = BG_CBB(BG_CBB_INDEX) | BG_SBB(BG1_SBB_INDEX) | BG_4BPP | BG_REG_32x32 | BG_PRIO(1);

    oam_init(oam_mem, 128);

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

    if (s_left > s_right) {
        /* Primera sincronización: no hay nada previamente válido. */
        for (int tx = new_left; tx <= new_right; tx++) sync_column(lv, tx, new_top, new_bottom);
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

void pal_video_set_parallax_scroll(int cam_x, int cam_y) {
    /* Se mueve a la mitad de la velocidad de la cámara: profundidad barata
       mientras no existan las capas de paralaje completas del prototipo
       (drawParallax(), pendiente). */
    REG_BG1HOFS = cam_x / 2;
    REG_BG1VOFS = cam_y / 2;
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
    obj->attr0 = ATTR0_Y(scr_y & 0xFF) | shape;
    obj->attr1 = ATTR1_X(scr_x & 0x1FF) | size | (flip_h ? ATTR1_HFLIP : 0);
    obj->attr2 = ATTR2_ID(tile) | ATTR2_PALBANK(palbank);
}

/* Elige la hoja y el frame base de cada tipo de entidad. Devuelve NULL
   para lo que todavía no se dibuja (decoración sin sistema propio). */
static const SpriteDef *sprite_for(const Entity *e, int *first_frame) {
    switch (e->type) {
        case ENT_RAT:    *first_frame = 0; return &SPRITE_16X8;
        case ENT_GUNNER: *first_frame = 2; return &SPRITE_16X8;
        case ENT_ROACH:  *first_frame = 0; return &SPRITE_8X8;
        case ENT_MOSQ:   *first_frame = 2; return &SPRITE_8X8;
        case ENT_BAT:    *first_frame = 4; return &SPRITE_8X8;
        case ENT_THUG:   *first_frame = 0; return &SPRITE_16X16;
        case ENT_BRUTE:  *first_frame = 2; return &SPRITE_16X16;
        case ENT_PROJ_TRAZA: *first_frame = FX_TRAZA; return &SPRITE_FX;
        case ENT_PROJ_JUNK:  *first_frame = FX_JUNK;  return &SPRITE_FX;
        case ENT_PROJ_SHOCK: *first_frame = FX_SHOCK; return &SPRITE_FX;
        case ENT_CHAPA:      *first_frame = FX_CHAPA; return &SPRITE_FX;
        case ENT_HP:         *first_frame = FX_HEART; return &SPRITE_FX;
        default: return 0;
    }
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

    /* 3. Partículas: un objeto cada una, con un frame por color. */
    for (int i = 0; i < particles_count(); i++) {
        const Particle *p = particles_get(i);
        if (!p->alive) continue;
        obj_put(fx_to_int(p->x) - cam_x, fx_to_int(p->y) - cam_y,
                (uint16_t)(TB_PARTICLES + p->color),
                ATTR0_SQUARE, ATTR1_SIZE_8x8, PALBANK_FX, false);
    }

    /* 4. HUD de vida: en coordenadas de pantalla, sin cámara. */
    for (int i = 0; i < w->player.hp; i++) {
        obj_put(4 + i * 9, 4, (uint16_t)(TB_FX + FX_HEART),
                ATTR0_SQUARE, ATTR1_SIZE_8x8, PALBANK_FX, false);
    }

    /* Esconde las ranuras que no se usaron este frame. */
    for (int i = s_obj_next; i < 128; i++) obj_hide(&oam_mem[i]);
}
