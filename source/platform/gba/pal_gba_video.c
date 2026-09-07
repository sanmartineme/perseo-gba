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

#include "tileset_tuneles.h"
#include "perseo.h"

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

#define PLAYER_OBJ 0
#define PLAYER_TILES_PER_FRAME 4 /* 16x16 = 2x2 tiles */

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

    /* Sprites: hoja de frames de Perseo + su paleta (banco 0 de OBJ). */
    memcpy32(&tile_mem_obj[0][0], perseoTiles, perseoTilesLen / 4);
    memcpy32(pal_obj_mem, perseoPal, perseoPalLen / 4);

    SBB_CLEAR(BG2_SBB_INDEX);
    SBB_CLEAR(BG1_SBB_INDEX);
    /* BG1 es un único tile de relleno repetido: no necesita streaming. */
    for (int i = 0; i < MAP_TILES * MAP_TILES; i++) {
        se_mem[BG1_SBB_INDEX][i] = SE_ID(TILEG_PARALLAX);
    }

    REG_BG2CNT = BG_CBB(BG_CBB_INDEX) | BG_SBB(BG2_SBB_INDEX) | BG_4BPP | BG_REG_32x32 | BG_PRIO(0);
    REG_BG1CNT = BG_CBB(BG_CBB_INDEX) | BG_SBB(BG1_SBB_INDEX) | BG_4BPP | BG_REG_32x32 | BG_PRIO(1);

    oam_init(oam_mem, 128);
    obj_set_attr(&oam_mem[PLAYER_OBJ], ATTR0_SQUARE, ATTR1_SIZE_16x16,
                 ATTR2_ID(0) | ATTR2_PALBANK(0));

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

void pal_video_draw_player(const PlayerAnim *anim, int scr_x, int scr_y) {
    OBJ_ATTR *obj = &oam_mem[PLAYER_OBJ];
    if (anim->hidden) {
        obj_hide(obj);
        return;
    }
    obj_unhide(obj, ATTR0_REG);
    /* Cada frame de 16x16 ocupa 4 tiles consecutivos gracias al meta-tiling
       de grit (-Mw2 -Mh2) y al mapeo 1D de OBJ. */
    BFN_SET(obj->attr2, anim->frame * PLAYER_TILES_PER_FRAME, ATTR2_ID);
    if (anim->flip_h) obj->attr1 |= ATTR1_HFLIP;
    else              obj->attr1 &= ~ATTR1_HFLIP;
    obj_set_pos(obj, scr_x + PLAYER_SPRITE_OFFSET_X,
                     scr_y + PLAYER_SPRITE_OFFSET_Y + anim->bob);
}
