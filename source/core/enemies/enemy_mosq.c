/* Mosquito: ronda su puesto flotando en una figura de ocho y, cuando
   Perseo se le acerca, se lanza en línea recta contra él; después vuelve
   despacio a su sitio. Tres estados, como en el prototipo (case 'mosq').

   Los senos del original (sin(t/40)*12 y sin(t/25)*4) se calculan con la
   tabla de core/trig.h: mismo movimiento, sin coma flotante. */
#include "enemy_common.h"
#include "../trig.h"

#define MOSQ_DIVE_SPEED FX_C(1.7)
#define MOSQ_RETURN     FX_C(0.05)

enum { MOSQ_HOVER = 0, MOSQ_DIVE, MOSQ_RETREAT };

void enemy_mosq_update(Entity *e, World *w) {
    if (!e->started) {
        e->started = true;
        e->hp = HP_MOSQ;
        e->home_x = e->x;
        e->home_y = e->y;
        e->state = MOSQ_HOVER;
        e->timer = 0;
    }
    e->timer++;

    if (e->state == MOSQ_HOVER) {
        uint32_t t = (uint32_t)e->timer;
        e->x = fx_add(e->home_x, fx_mul(fx_sin((t * FX_SIN_RATE(40)) >> 8), fx_from_int(12)));
        e->y = fx_add(e->home_y, fx_mul(fx_sin((t * FX_SIN_RATE(25)) >> 8), fx_from_int(4)));

        fx_t dx = fx_sub(w->player.x, e->x);
        fx_t dy = fx_sub(w->player.y, e->y);
        if (fx_abs(dx) < fx_from_int(60) && fx_abs(dy) < fx_from_int(50) && e->timer > 90) {
            /* Normaliza el vector hacia Perseo para lanzarse a velocidad
               constante venga de donde venga. */
            uint32_t d = isqrt32((uint32_t)(fx_to_int(fx_abs(dx)) * fx_to_int(fx_abs(dx))
                                          + fx_to_int(fx_abs(dy)) * fx_to_int(fx_abs(dy))));
            if (d == 0) d = 1;
            e->vx = fx_mul(fx_div(dx, fx_from_int((int32_t)d)), MOSQ_DIVE_SPEED);
            e->vy = fx_mul(fx_div(dy, fx_from_int((int32_t)d)), MOSQ_DIVE_SPEED);
            e->state = MOSQ_DIVE;
            e->timer = 0;
        }
    } else if (e->state == MOSQ_DIVE) {
        e->x = fx_add(e->x, e->vx);
        e->y = fx_add(e->y, e->vy);
        if (e->timer > 45 || level_rect_solid(w->lv, e->x, e->y, 8, 8)) {
            e->state = MOSQ_RETREAT;
            e->timer = 0;
        }
    } else {
        e->x = fx_add(e->x, fx_mul(fx_sub(e->home_x, e->x), MOSQ_RETURN));
        e->y = fx_add(e->y, fx_mul(fx_sub(e->home_y, e->y), MOSQ_RETURN));
        if (e->timer > 60) {
            e->state = MOSQ_HOVER;
            e->timer = 0;
        }
    }
    e->face = w->player.x < e->x ? -1 : 1;
    world_touch_player(w, e);
}
