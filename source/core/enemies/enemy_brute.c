/* El Sicario: el enemigo único de la Madriguera Subterránea y el más
   agresivo del juego. Patrulla como un matón, pero en cuanto detecta a
   Perseo cerca y a su misma altura embiste a toda velocidad en línea
   recta hasta chocar; sólo tras un respiro vuelve a patrullar.
   Traducción del case 'brute' de updateEnts(). */
#include "enemy_common.h"

#define BRUTE_SPEED        FX_C(0.3)
#define BRUTE_FRICTION     FX_C(0.85)
#define BRUTE_CHARGE_SPEED FX_C(2.6)
#define BRUTE_SIGHT_X      80
#define BRUTE_SIGHT_Y      18
#define BRUTE_CHARGE_MAX   50
#define BRUTE_RECOVER      40

enum { BRUTE_PATROL = 0, BRUTE_CHARGE, BRUTE_RECOVERING };

void enemy_brute_update(Entity *e, World *w) {
    if (!e->started) {
        e->started = true;
        if (e->dir == 0) e->dir = -1;
        e->hp = HP_BRUTE;
        e->vx = 0;
        e->state = BRUTE_PATROL;
        e->cooldown = 0;
        e->y = fx_sub(world_ground_y(w, fx_add(e->x, fx_from_int(8)), e->y),
                      fx_from_int(10));
    }

    fx_t dx = fx_sub(w->player.x, e->x);

    if (e->state == BRUTE_PATROL) {
        e->vx = fx_mul(e->vx, BRUTE_FRICTION);
        if (fx_abs(e->vx) < FX_C(0.1)) e->vx = 0;
        world_patrol(w, e, BRUTE_SPEED, 16, 4, 11);

        if (fx_abs(dx) < fx_from_int(BRUTE_SIGHT_X)
            && fx_abs(fx_sub(w->player.y, e->y)) < fx_from_int(BRUTE_SIGHT_Y)) {
            e->state = BRUTE_CHARGE;
            e->dir = dx < 0 ? -1 : 1;
            e->face = e->dir;
            e->cooldown = 0;
            if (w->shake < 2) w->shake = 2;
        }
    } else if (e->state == BRUTE_CHARGE) {
        e->cooldown++;
        fx_t nx = fx_add(e->x, fx_mul(fx_from_int(e->dir), BRUTE_CHARGE_SPEED));
        fx_t front = e->dir < 0 ? nx : fx_add(nx, fx_from_int(16));
        int16_t ftx = fx_to_tile(front);
        bool wall  = tile_is_solid(level_tile_at(w->lv, ftx,
                        fx_to_tile(fx_add(e->y, fx_from_int(4)))));
        bool floor = tile_is_solid(level_tile_at(w->lv, ftx,
                        fx_to_tile(fx_add(e->y, fx_from_int(11)))));
        /* Frena contra un muro, ante un precipicio, o cuando se le acaba
           el envión. */
        if (wall || !floor || e->cooldown > BRUTE_CHARGE_MAX) {
            e->state = BRUTE_RECOVERING;
            e->cooldown = 0;
        } else {
            e->x = nx;
        }
    } else {
        e->cooldown++;
        if (e->cooldown > BRUTE_RECOVER) e->state = BRUTE_PATROL;
    }

    world_touch_player(w, e);
}
