/* Cucaracha: rápida, frágil (muere de un golpe) y siempre a ras de
   suelo. No la empuja nada — se muere antes de que se note.
   Traducción del case 'roach' de updateEnts(). */
#include "enemy_common.h"

#define ROACH_SPEED FX_C(0.9)

void enemy_roach_update(Entity *e, World *w) {
    if (!e->started) {
        e->started = true;
        e->dir = 1;
        e->hp = HP_ROACH;
        e->y = fx_sub(world_ground_y(w, fx_add(e->x, fx_from_int(4)), e->y),
                      fx_from_int(6));
    }
    e->vx = 0; /* ignora el empujón, igual que el original */
    world_patrol(w, e, ROACH_SPEED, 8, 3, 7);
    world_touch_player(w, e);
}
