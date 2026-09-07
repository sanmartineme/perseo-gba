/* Matón: rata grande de la banda de Betty. Patrulla lento pero aguanta
   cuatro golpes, así que obliga a comprometerse o a esquivarlo.
   Traducción del case 'thug' de updateEnts(). */
#include "enemy_common.h"

#define THUG_SPEED    FX_C(0.35)
#define THUG_FRICTION FX_C(0.85)

void enemy_thug_update(Entity *e, World *w) {
    if (!e->started) {
        e->started = true;
        e->dir = -1;
        e->hp = HP_THUG;
        e->vx = 0;
        e->y = fx_sub(world_ground_y(w, fx_add(e->x, fx_from_int(8)), e->y),
                      fx_from_int(10));
    }
    e->vx = fx_mul(e->vx, THUG_FRICTION);
    if (fx_abs(e->vx) < FX_C(0.1)) e->vx = 0;

    world_patrol(w, e, THUG_SPEED, 16, 4, 11);
    world_touch_player(w, e);
}
