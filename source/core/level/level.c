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

void level_carve(Level *lv, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t id) {
    uint8_t *tiles = (uint8_t *)lv->tiles;
    for (int16_t y = y0; y <= y1; y++) {
        if (y < 0 || y >= lv->h) continue;
        for (int16_t x = x0; x <= x1; x++) {
            if (x < 0 || x >= lv->w) continue;
            tiles[(int32_t)y * lv->w + x] = id;
        }
    }
}
