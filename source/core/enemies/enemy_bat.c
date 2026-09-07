/* Murciélago: va y viene en horizontal sobre un tramo fijo, subiendo y
   bajando con un aleteo senoidal. No persigue: molesta por estar donde
   uno quiere saltar. Traducción del case 'bat' de updateEnts(). */
#include "enemy_common.h"
#include "../trig.h"

#define BAT_SPEED FX_C(0.6)
#define BAT_RANGE 28

void enemy_bat_update(Entity *e, World *w) {
    if (!e->started) {
        e->started = true;
        e->hp = HP_BAT;
        if (e->dir == 0) e->dir = 1;
        e->home_x = e->x;
        e->home_y = e->y;
        e->timer = 0;
    }
    e->timer++;

    e->x = fx_add(e->x, fx_mul(fx_from_int(e->dir), BAT_SPEED));
    e->y = fx_add(e->home_y,
                  fx_mul(fx_sin(((uint32_t)e->timer * FX_SIN_RATE(18)) >> 8), fx_from_int(3)));
    if (fx_abs(fx_sub(e->x, e->home_x)) > fx_from_int(BAT_RANGE)) e->dir = (int8_t)-e->dir;
    e->face = e->dir;

    world_touch_player(w, e);
}
