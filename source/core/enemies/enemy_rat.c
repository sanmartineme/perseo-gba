/* Rata pequeña: el enemigo básico de las cloacas. Patrulla de ida y
   vuelta sin más, pero encaja dos golpes y su empujón al ser herida se
   nota (rebota antes de retomar la marcha).
   Traducción del case 'rat' de updateEnts() en el prototipo. */
#include "enemy_common.h"

#define RAT_SPEED    FX_C(0.45)
#define RAT_FRICTION FX_C(0.8)

void enemy_rat_update(Entity *e, World *w) {
    if (!e->started) {
        e->started = true;
        e->dir = -1;
        e->hp = HP_RAT;
        e->vx = 0;
        e->y = fx_sub(world_ground_y(w, fx_add(e->x, fx_from_int(7)), e->y),
                      fx_from_int(8));
    }
    /* El empujón de un golpe se va apagando hasta que retoma su paso. */
    e->vx = fx_mul(e->vx, RAT_FRICTION);
    if (fx_abs(e->vx) < FX_C(0.1)) e->vx = 0;

    world_patrol(w, e, RAT_SPEED, 14, 4, 9);
    world_touch_player(w, e);
}
