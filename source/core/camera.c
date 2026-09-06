#include "camera.h"

void camera_init(Camera *cam) {
    cam->x = 0;
    cam->y = 0;
}

void camera_update(Camera *cam, fx_t target_x, fx_t target_y, const Level *lv) {
    /* cam.x = max(0, min(P.x+5-SW/2, LV.w*8-SW)) — se replica el orden
       exacto max(0, min(...)) del prototipo, y no un fx_clamp(lo,hi)
       genérico: con un nivel más angosto que la pantalla, LV.w*8-SW da
       negativo, y max(0,min(x,negativo)) da 0 (no el límite negativo,
       que es lo que devolvería un clamp(lo=0,hi=negativo) normal). */
    fx_t want_x = fx_sub(fx_add(target_x, fx_from_int(5)), fx_from_int(SCREEN_W / 2));
    fx_t want_y = fx_sub(fx_add(target_y, fx_from_int(6)), fx_from_int(SCREEN_H / 2));
    fx_t max_x = fx_sub(fx_from_int((int32_t)lv->w * TILE_SIZE), fx_from_int(SCREEN_W));
    fx_t max_y = fx_sub(fx_from_int((int32_t)lv->h * TILE_SIZE), fx_from_int(SCREEN_H));

    cam->x = fx_max(0, fx_min(want_x, max_x));
    cam->y = fx_max(0, fx_min(want_y, max_y));
}
