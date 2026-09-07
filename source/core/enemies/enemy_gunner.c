/* Rata tiradora: patrulla como una rata normal, pero además arquea
   proyectiles de chatarra hacia Perseo cuando lo tiene cerca en
   horizontal. Es la que rompe la regla de "si no lo toco no me pasa
   nada" y obliga a moverse.
   Traducción del case 'gunner' de updateEnts(). */
#include "enemy_common.h"
#include "../projectiles.h"
#include "../rng.h"

#define GUNNER_SPEED    FX_C(0.35)
#define GUNNER_FRICTION FX_C(0.8)
#define GUNNER_SHOT_VX  FX_C(1.7)
#define GUNNER_SHOT_VY  FX_C(3.4)

void enemy_gunner_update(Entity *e, World *w) {
    if (!e->started) {
        e->started = true;
        if (e->dir == 0) e->dir = -1;
        e->hp = HP_GUNNER;
        e->vx = 0;
        e->y = fx_sub(world_ground_y(w, fx_add(e->x, fx_from_int(5)), e->y),
                      fx_from_int(8));
        /* Cadencia inicial repartida para que dos gunners cercanos no
           disparen a la vez. */
        e->cooldown = (int16_t)(60 + rng_below(60));
    }
    e->vx = fx_mul(e->vx, GUNNER_FRICTION);
    if (fx_abs(e->vx) < FX_C(0.1)) e->vx = 0;

    world_patrol(w, e, GUNNER_SPEED, 10, 4, 9);

    fx_t dx = fx_sub(w->player.x, e->x);
    if (fx_abs(dx) < fx_from_int(100) && fx_abs(fx_sub(w->player.y, e->y)) < fx_from_int(40)) {
        /* Al tener a Perseo a tiro se gira hacia él y cuenta para tirar. */
        e->dir = dx < 0 ? -1 : 1;
        e->face = e->dir;
        if (e->cooldown > 0) e->cooldown--;
        if (e->cooldown <= 0) {
            e->cooldown = (int16_t)(100 + rng_below(50));
            proj_spawn_junk(fx_add(e->x, fx_from_int(5)), fx_add(e->y, fx_from_int(2)),
                            fx_mul(fx_from_int(e->dir), GUNNER_SHOT_VX),
                            fx_neg(GUNNER_SHOT_VY));
        }
    }
    world_touch_player(w, e);
}
