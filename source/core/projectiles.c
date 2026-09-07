#include "projectiles.h"
#include "entity_pool.h"
#include "particles.h"
#include "world.h"

#define TRAZA_SPEED  FX_C(3.4)
#define TRAZA_LIFE   85
#define JUNK_GRAVITY FX_C(0.15)
#define SHOCK_LIFE   140

static Entity *spawn(EntityType type, fx_t x, fx_t y, fx_t vx, fx_t vy) {
    Entity *e = entity_pool_alloc(type);
    if (!e) return 0;
    e->x = x; e->y = y;
    e->vx = vx; e->vy = vy;
    e->timer = 0;
    return e;
}

Entity *proj_spawn_traza(fx_t x, fx_t y, int8_t face) {
    Entity *e = spawn(ENT_PROJ_TRAZA, x, y, fx_mul(fx_from_int(face), TRAZA_SPEED), 0);
    if (e) e->friendly = true;
    return e;
}

Entity *proj_spawn_junk(fx_t x, fx_t y, fx_t vx, fx_t vy) {
    return spawn(ENT_PROJ_JUNK, x, y, vx, vy);
}

Entity *proj_spawn_shock(fx_t x, fx_t y, fx_t vx) {
    return spawn(ENT_PROJ_SHOCK, x, y, vx, 0);
}

void proj_update(Entity *e, World *w) {
    e->timer++;

    if (e->friendly) {
        /* Traza de Perseo: vuela recta, se deshace contra un muro y
           revienta contra el primer enemigo que toque. Nunca comprueba
           colisión con el jugador — no puede dañarse a sí mismo. */
        e->x = fx_add(e->x, e->vx);
        if (level_rect_solid(w->lv, fx_add(e->x, fx_from_int(1)),
                             fx_add(e->y, fx_from_int(1)), 6, 4)
            || e->timer > TRAZA_LIFE) {
            particles_burst(fx_add(e->x, fx_from_int(4)), fx_add(e->y, fx_from_int(3)),
                            PCOL_BROWN, 5, FX_C(2.0));
            entity_pool_free(e);
            return;
        }
        Rect hit = { e->x, e->y, 8, 6 };
        for (int i = 0; i < ENTITY_POOL_CAPACITY; i++) {
            Entity *other = entity_pool_at(i);
            if (!other->alive || other == e) continue;
            Rect box = entity_box(other);
            if (box.w == 0) continue;
            if (rect_overlap(hit, box)) {
                world_hit_enemy(w, other, 1);
                /* Salpicadura: pedazos cafés claros y oscuros + sacudida. */
                particles_burst(fx_add(e->x, fx_from_int(4)), fx_add(e->y, fx_from_int(3)),
                                PCOL_BROWN, 10, FX_C(3.2));
                if (w->shake < 3) w->shake = 3;
                entity_pool_free(e);
                return;
            }
        }
        return;
    }

    if (e->type == ENT_PROJ_JUNK) {
        e->vy = fx_add(e->vy, JUNK_GRAVITY);
        e->x = fx_add(e->x, e->vx);
        e->y = fx_add(e->y, e->vy);
        if (level_rect_solid(w->lv, fx_add(e->x, fx_from_int(1)),
                             fx_add(e->y, fx_from_int(1)), 6, 4)) {
            particles_burst(fx_add(e->x, fx_from_int(3)), fx_add(e->y, fx_from_int(3)),
                            PCOL_GREY, 4, FX_C(2.0));
            entity_pool_free(e);
            return;
        }
    } else {
        /* Onda de choque: corre pegada al suelo hasta chocar o caducar. */
        e->x = fx_add(e->x, e->vx);
        if (level_rect_solid(w->lv, e->x, e->y, 8, 5) || e->timer > SHOCK_LIFE) {
            entity_pool_free(e);
            return;
        }
        e->y = fx_sub(world_ground_y(w, fx_add(e->x, fx_from_int(4)), e->y),
                      fx_from_int(6));
    }

    Rect hit = { e->x, e->y, 7, 6 };
    if (rect_overlap(hit, player_box(&w->player))) {
        world_damage_player(w, 1, e->vx > 0 ? 1 : -1);
        entity_pool_free(e);
    }
}
