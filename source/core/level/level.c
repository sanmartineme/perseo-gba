#include "level.h"

bool tile_is_solid(uint8_t id) {
    switch (id) {
        case TILE_BRICK:
        case TILE_RUST:
        case TILE_GRATE:
        case TILE_DOOR_FRAME:
            return true;
        default:
            return false;
    }
}

uint8_t level_tile_at(const Level *lv, int16_t tx, int16_t ty) {
    if (!lv) return TILE_BRICK;
    if (tx < 0 || tx >= lv->w || ty < 0) return TILE_BRICK;
    if (ty >= lv->h) return TILE_EMPTY;
    return lv->tiles[(int32_t)ty * lv->w + tx];
}

bool level_rect_solid(const Level *lv, fx_t x, fx_t y, int16_t w, int16_t h) {
    int16_t tx0 = fx_to_tile(x);
    int16_t tx1 = fx_to_tile(fx_add(x, fx_from_int(w - 1)));
    int16_t ty0 = fx_to_tile(y);
    int16_t ty1 = fx_to_tile(fx_add(y, fx_from_int(h - 1)));
    for (int16_t ty = ty0; ty <= ty1; ty++)
        for (int16_t tx = tx0; tx <= tx1; tx++)
            if (tile_is_solid(level_tile_at(lv, tx, ty)))
                return true;
    return false;
}

/* ---------- Habitación de prueba de la Fase 1 ----------
   30x20 tiles = 240x160 px, exactamente la pantalla de GBA: así el
   render de depuración (source/main.c) no necesita cámara/scroll
   todavía (eso es trabajo de la Fase 2). Se construye una sola vez con
   el mismo patrón "rectángulo relleno" que carve() en el prototipo,
   pensado para poder probar cada mecánica de movimiento:
     - paredes izquierda/derecha completas -> Garra Felina
     - plataforma flotante central          -> salto y Salto Doble
     - saliente pegada a la pared derecha   -> Dash Sombrío horizontal
   ===================================================================== */
#define TEST_ROOM_W 30
#define TEST_ROOM_H 20

static uint8_t s_test_room_tiles[TEST_ROOM_W * TEST_ROOM_H];
static Level   s_test_room;
static bool    s_test_room_ready = false;

static void carve(uint8_t *tiles, int16_t w, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t id) {
    for (int16_t y = y0; y <= y1; y++)
        for (int16_t x = x0; x <= x1; x++)
            tiles[(int32_t)y * w + x] = id;
}

const Level *level_get_test_room(void) {
    if (s_test_room_ready) return &s_test_room;

    carve(s_test_room_tiles, TEST_ROOM_W, 0, 0, TEST_ROOM_W - 1, TEST_ROOM_H - 1, TILE_EMPTY);
    /* Paredes laterales completas */
    carve(s_test_room_tiles, TEST_ROOM_W, 0, 0, 0, TEST_ROOM_H - 1, TILE_BRICK);
    carve(s_test_room_tiles, TEST_ROOM_W, TEST_ROOM_W - 1, 0, TEST_ROOM_W - 1, TEST_ROOM_H - 1, TILE_BRICK);
    /* Suelo */
    carve(s_test_room_tiles, TEST_ROOM_W, 1, TEST_ROOM_H - 3, TEST_ROOM_W - 2, TEST_ROOM_H - 1, TILE_BRICK);
    /* Plataforma flotante central (salto / Salto Doble) */
    carve(s_test_room_tiles, TEST_ROOM_W, 10, TEST_ROOM_H - 8, 16, TEST_ROOM_H - 8, TILE_BRICK);
    /* Saliente a media altura junto a la pared derecha (Dash Sombrío) */
    carve(s_test_room_tiles, TEST_ROOM_W, TEST_ROOM_W - 6, TEST_ROOM_H - 12, TEST_ROOM_W - 2, TEST_ROOM_H - 12, TILE_BRICK);

    s_test_room.w = TEST_ROOM_W;
    s_test_room.h = TEST_ROOM_H;
    s_test_room.tiles = s_test_room_tiles;
    s_test_room.spawn_tx = 3;
    s_test_room.spawn_ty = TEST_ROOM_H - 4;
    s_test_room_ready = true;
    return &s_test_room;
}
