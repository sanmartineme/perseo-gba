/* =====================================================================
   pal_gba_video.c — Modo 0, streaming de "mapa grande" y sprite OBJ
   =====================================================================
   Implementación real de la interfaz de vídeo de la PAL (ver
   docs/PLAN_MIGRACION_GBA_C.md, sección 2 y la "Fase 2" del plan).

   Distribución de VRAM elegida:
     - Charblock 0 (CBB 0): gráficos de tile de fondo (placeholders de
       color sólido — el arte real de grit llega en la Fase 3).
     - Screenblock 8  (SBB 8): tilemap de BG2 (el nivel colisionable).
     - Screenblock 9  (SBB 9): tilemap de BG1 (paralaje, relleno único).
     (SBB 0-7 quedan reservados para no pisar los datos de CBB 0: un
     screenblock son 2KB y un charblock 16KB = 8 screenblocks.)

   Técnica de "mapa grande" (Tonc, "big maps"): un screenblock de
   hardware sólo tiene 32x32 entradas. Los niveles reales (hasta 200
   tiles de ancho) son mucho más grandes que eso, así que BG2 se trata
   como un buffer CIRCULAR de 32x32: la entrada para el tile de mundo
   (tx,ty) siempre vive en se_mem[SBB][(ty&31)*32+(tx&31)], sin importar
   cuántas veces haya dado la vuelta el scroll. Sólo hace falta
   mantener escrita la ventana [visible ± 1 tile de margen] en todo
   momento; a medida que la cámara se mueve, se reescriben nada más las
   columnas/filas que quedan recién expuestas (ver sync_columns/rows).
   ===================================================================== */
#include <tonc.h>
#include "../pal.h"
#include "../../core/camera.h" /* SCREEN_W/SCREEN_H */

#define BG_CBB_INDEX 0
#define BG2_SBB_INDEX 8
#define BG1_SBB_INDEX 9

#define MAP_TILES 32 /* tamaño de un screenblock, en tiles */
#define MAP_MASK  (MAP_TILES - 1)

/* Índices de tile ya cargados en el charblock (ver pal_video_init). */
enum { TILEG_EMPTY = 0, TILEG_SOLID = 1, TILEG_PLATFORM = 2, TILEG_PARALLAX = 3 };

/* Traduce un id de tile del nivel (core/level.h) al índice de gráfico
   ya cargado en VRAM. Es la única función que conoce ambos mundos. */
static u16 tile_graphic_for(uint8_t level_id) {
    switch (level_id) {
        case TILE_BRICK:
        case TILE_RUST:
        case TILE_GRATE:
        case TILE_DOOR_FRAME:
            return TILEG_SOLID;
        case TILE_PLATFORM:
            return TILEG_PLATFORM;
        default:
            return TILEG_EMPTY;
    }
}

static void set_map_entry(int sbb, int world_tx, int world_ty, u16 tile_graphic) {
    int bx = world_tx & MAP_MASK;
    int by = world_ty & MAP_MASK;
    se_mem[sbb][by * MAP_TILES + bx] = SE_ID(tile_graphic);
}

/* ---------- Carga de un tile placeholder de color sólido ----------
   Un tile 4bpp son 8 filas x 4 bytes (2 píxeles por byte). Para un
   color sólido, cada byte es (idx<<4)|idx repetido. */
static void load_solid_tile(int cbb, int tile_index, u8 pal_index) {
    TILE t;
    u32 word = pal_index | (pal_index << 4);
    word |= word << 8;
    word |= word << 16;
    for (int i = 0; i < 8; i++) t.data[i] = word;
    tile_mem[cbb][tile_index] = t;
}

/* ---------- Estado de sincronización del "mapa grande" de BG2 ----------
   Rango de tiles de MUNDO que el buffer circular tiene actualmente al
   día. INT32_MIN a propósito como centinela de "todavía no sincronizado
   nada" para forzar un llenado completo la primera vez. */
static int s_left = 1, s_right = 0;   /* rango vacío: left > right */
static int s_top = 1, s_bottom = 0;

static void sync_column(const Level *lv, int world_tx, int top, int bottom) {
    for (int ty = top; ty <= bottom; ty++) {
        set_map_entry(BG2_SBB_INDEX, world_tx, ty, tile_graphic_for(level_tile_at(lv, world_tx, ty)));
    }
}
static void sync_row(const Level *lv, int world_ty, int left, int right) {
    for (int tx = left; tx <= right; tx++) {
        set_map_entry(BG2_SBB_INDEX, tx, world_ty, tile_graphic_for(level_tile_at(lv, tx, world_ty)));
    }
}

void pal_video_init(void) {
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG1 | DCNT_BG2 | DCNT_OBJ | DCNT_OBJ_1D;

    /* Paleta de fondo: 0 sin usar (índice 0 de cualquier tile es
       transparente), 1 = sólido (paredes/piso/rejillas), 2 = plataforma
       de un sentido, 3 = capa de paralaje. Mismos tonos que el debug de
       la Fase 1 para continuidad visual mientras no hay arte real. */
    pal_bg_mem[1] = RGB15(14, 17, 22); /* T: gris azulado (muro/tubería) */
    pal_bg_mem[2] = RGB15(18, 21, 12); /* plataforma: un tono distinguible */
    pal_bg_mem[3] = RGB15(8, 10, 15);  /* paralaje: más oscuro que el fondo, más claro que el vacío */

    load_solid_tile(BG_CBB_INDEX, TILEG_SOLID, 1);
    load_solid_tile(BG_CBB_INDEX, TILEG_PLATFORM, 2);
    load_solid_tile(BG_CBB_INDEX, TILEG_PARALLAX, 3);
    /* TILEG_EMPTY (0) se deja en ceros: es el tile "todo transparente"
       por defecto de VRAM tras el arranque. */

    SBB_CLEAR(BG2_SBB_INDEX);
    SBB_CLEAR(BG1_SBB_INDEX);
    /* BG1 es un único tile de relleno repetido en todo el screenblock:
       no necesita streaming, siempre es igual. */
    for (int i = 0; i < MAP_TILES * MAP_TILES; i++) se_mem[BG1_SBB_INDEX][i] = SE_ID(TILEG_PARALLAX);

    REG_BG2CNT = BG_CBB(BG_CBB_INDEX) | BG_SBB(BG2_SBB_INDEX) | BG_4BPP | BG_REG_32x32 | BG_PRIO(0);
    REG_BG1CNT = BG_CBB(BG_CBB_INDEX) | BG_SBB(BG1_SBB_INDEX) | BG_4BPP | BG_REG_32x32 | BG_PRIO(1);

    /* Sprite del jugador: placeholder 16x16 de color sólido (el ciclo
       de animación real llega en la Fase 3). Un sprite de 16x16 usa 4
       tiles de 8x8 en memoria de OBJ; con mapeo 1D (DCNT_OBJ_1D) basta
       con rellenar 4 tiles consecutivos a partir del índice 0. */
    pal_obj_mem[1] = RGB15(31, 20, 2); /* Y: dorado, silueta de Perseo */
    u32 word = 1 | (1 << 4);
    word |= word << 8;
    word |= word << 16;
    for (int i = 0; i < 4; i++) {
        TILE t;
        for (int j = 0; j < 8; j++) t.data[j] = word;
        tile_mem_obj[0][i] = t;
    }

    oam_init(oam_mem, 128);
    obj_set_attr(&oam_mem[0], ATTR0_SQUARE, ATTR1_SIZE_16x16, ATTR2_ID(0) | ATTR2_PALBANK(0));

    s_left = 1; s_right = 0; /* rango vacío: fuerza un llenado completo en el primer sync */
    s_top = 1; s_bottom = 0;
}

void pal_video_sync_level(const Level *lv, int cam_x, int cam_y) {
    /* Ventana deseada: lo visible en pantalla + 1 tile de margen a cada
       lado, para que nunca se vea un tile sin sincronizar al hacer
       scroll un par de píxeles. */
    int new_left   = (cam_x >> 3) - 1;
    int new_right  = ((cam_x + SCREEN_W - 1) >> 3) + 1;
    int new_top    = (cam_y >> 3) - 1;
    int new_bottom = ((cam_y + SCREEN_H - 1) >> 3) + 1;

    if (s_left > s_right) {
        /* Primera sincronización: no hay nada previamente válido. */
        for (int tx = new_left; tx <= new_right; tx++) sync_column(lv, tx, new_top, new_bottom);
    } else {
        if (new_left < s_left)  for (int tx = new_left; tx < s_left; tx++)      sync_column(lv, tx, s_top, s_bottom);
        if (new_right > s_right) for (int tx = s_right + 1; tx <= new_right; tx++) sync_column(lv, tx, s_top, s_bottom);
        s_left = new_left; s_right = new_right;

        if (new_top < s_top)     for (int ty = new_top; ty < s_top; ty++)         sync_row(lv, ty, s_left, s_right);
        if (new_bottom > s_bottom) for (int ty = s_bottom + 1; ty <= new_bottom; ty++) sync_row(lv, ty, s_left, s_right);
    }
    s_left = new_left; s_right = new_right;
    s_top = new_top; s_bottom = new_bottom;

    REG_BG2HOFS = cam_x;
    REG_BG2VOFS = cam_y;
}

void pal_video_set_parallax_scroll(int cam_x, int cam_y) {
    /* Se mueve a la mitad de la velocidad de la cámara: sensación de
       profundidad barata mientras no hay capas de paralaje reales
       (drawParallax() del prototipo, Fase 3). */
    REG_BG1HOFS = cam_x / 2;
    REG_BG1VOFS = cam_y / 2;
}

void pal_video_set_player_sprite(int scr_x, int scr_y) {
    obj_set_pos(&oam_mem[0], scr_x, scr_y);
}
