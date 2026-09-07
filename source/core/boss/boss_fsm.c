#include "boss_fsm.h"
#include "../world.h"
#include "../entity_pool.h"
#include "../particles.h"
#include "../projectiles.h"
#include "../rng.h"

/* Tiempos y velocidades, todos tomados de updateBoss() del prototipo. */
#define INTRO_FRAMES     70
#define CHOOSE_FRAMES    30
#define TELE_FRAMES      26
#define TELE_FRAMES_P2   18   /* segunda fase: avisa menos, aprieta más */
#define STUN_FRAMES      65
#define STUN_FRAMES_P2   45
#define CHARGE_SPEED     FX_C(2.3)
#define LEAP_VY          FX_C(5.2)
#define LEAP_VX_MAX      FX_C(2.6)
#define LEAP_GRAVITY     FX_C(0.22)
#define DIE_FRAMES       110
#define PHASE2_HP        8    /* por debajo de esto se acelera todo */

#define BOSS_FOOT_OFFSET 23   /* del origen del sprite a sus patas */

void boss_init(Entity *e, World *w) {
    const BossConfig *cfg = boss_config_of(e);
    e->hp = cfg->hp;
    e->w = cfg->w;
    e->h = cfg->h;
    e->state = BOSS_WAIT;
    e->timer = 0;
    e->face = 1;
    e->vx = e->vy = 0;
    e->flash = 0;
    e->next_attack = e->last_attack = -1;
    /* Se apoya en el suelo que tenga debajo, igual que en el prototipo. */
    e->y = fx_sub(world_ground_y(w, fx_add(e->x, fx_from_int(15)),
                                 fx_add(e->y, fx_from_int(2))),
                  fx_from_int(BOSS_FOOT_OFFSET));
    e->home_x = e->x;
    e->home_y = e->y;
    e->started = true;
}

/* Elige el próximo ataque evitando repetir el anterior: es lo que hace
   que el patrón no se vuelva predecible sin tener que escribir una
   secuencia a mano. */
static void choose_attack(Entity *e) {
    const BossConfig *cfg = boss_config_of(e);
    int8_t pool[8];
    int n = 0;
    pool[n++] = BOSS_CHARGE;
    pool[n++] = BOSS_LEAP;
    pool[n++] = BOSS_THROW;
    /* Betty, pasado el 40% de su vida, también llama refuerzos: la fase
       extra que distingue al enfrentamiento final. */
    if (cfg->is_final && e->hp <= (cfg->hp * 2) / 5) {
        pool[n++] = BOSS_SUMMON;
        pool[n++] = BOSS_SUMMON;
    }
    if (cfg->is_final) pool[n++] = BOSS_TRASHBLAST;

    int8_t opts[8];
    int m = 0;
    for (int i = 0; i < n; i++) {
        if (pool[i] != e->last_attack) opts[m++] = pool[i];
    }
    if (m == 0) { opts[0] = pool[0]; m = 1; }
    e->next_attack = opts[rng_below((uint32_t)m)];
    e->last_attack = e->next_attack;
}

static void enter_state(Entity *e, BossState st) {
    e->state = st;
    e->timer = 0;
}

/* Choque de frente contra un muro, medido en el borde delantero del
   cuerpo — igual que el prototipo. */
static bool blocked_ahead(const World *w, const Entity *e, fx_t nx) {
    fx_t front = e->vx < 0 ? nx : fx_add(nx, fx_from_int(30));
    return level_rect_solid(w->lv, front, fx_add(e->y, fx_from_int(4)), 1, 16);
}

void boss_update(Entity *e, World *w) {
    /* Duerme hasta que el jugador entra en la arena. */
    if (e->state == BOSS_WAIT) return;

    e->timer++;
    bool phase2 = e->hp <= PHASE2_HP;
    fx_t speed_mul = phase2 ? FX_C(1.3) : FX_C(1.0);
    fx_t dx = fx_sub(w->player.x, fx_add(e->x, fx_from_int(15)));

    switch (e->state) {
        case BOSS_INTRO:
            /* Rugido, polvo bajo las patas y temblor que crece. */
            if (e->timer == 1) {
                w->shake = 8;
                particles_burst(fx_add(e->x, fx_from_int(15)),
                                fx_add(e->y, fx_from_int(22)), PCOL_GREY, 10, FX_C(3.0));
            }
            if (e->timer % 10 == 0) {
                int16_t s = (int16_t)(3 + e->timer / 9);
                w->shake = s > 9 ? 9 : s;
                particles_burst(fx_add(e->x, fx_from_int(8 + (int)rng_below(16))),
                                fx_add(e->y, fx_from_int(22)), PCOL_GREY, 3, FX_C(1.5));
            }
            if (e->timer > INTRO_FRAMES) enter_state(e, BOSS_CHOOSE);
            break;

        case BOSS_CHOOSE:
            e->face = dx < 0 ? -1 : 1;
            if (e->timer > CHOOSE_FRAMES) {
                choose_attack(e);
                enter_state(e, BOSS_TELE);
            }
            break;

        case BOSS_TELE:
            if (e->timer > (phase2 ? TELE_FRAMES_P2 : TELE_FRAMES)) {
                BossState next = (BossState)e->next_attack;
                if (next == BOSS_CHARGE) {
                    e->vx = fx_mul(fx_mul(fx_from_int(e->face), CHARGE_SPEED), speed_mul);
                } else if (next == BOSS_LEAP) {
                    e->vy = fx_neg(LEAP_VY);
                    fx_t aim = fx_div(dx, fx_from_int(34));
                    e->vx = fx_mul(fx_clamp(aim, fx_neg(LEAP_VX_MAX), LEAP_VX_MAX), speed_mul);
                }
                enter_state(e, next);
            }
            break;

        case BOSS_CHARGE: {
            fx_t nx = fx_add(e->x, e->vx);
            if (blocked_ahead(w, e, nx)) {
                /* Se estrella y queda aturdido: ésa es la ventana para
                   castigarlo, y ahí el daño se duplica. */
                enter_state(e, BOSS_STUN);
                w->shake = 6;
            } else {
                e->x = nx;
            }
            break;
        }

        case BOSS_STUN:
            if (e->timer > (phase2 ? STUN_FRAMES_P2 : STUN_FRAMES)) enter_state(e, BOSS_CHOOSE);
            break;

        case BOSS_LEAP: {
            e->vy = fx_add(e->vy, LEAP_GRAVITY);
            e->x = fx_add(e->x, e->vx);
            e->y = fx_add(e->y, e->vy);
            if (e->vx < 0 && level_rect_solid(w->lv, e->x, fx_add(e->y, fx_from_int(4)), 1, 16)) e->vx = 0;
            if (e->vx > 0 && level_rect_solid(w->lv, fx_add(e->x, fx_from_int(30)),
                                              fx_add(e->y, fx_from_int(4)), 1, 16)) e->vx = 0;
            if (e->vy > 0 && level_rect_solid(w->lv, fx_add(e->x, fx_from_int(2)),
                                              fx_add(e->y, fx_from_int(BOSS_FOOT_OFFSET)), 26, 1)) {
                /* Al aterrizar suelta dos ondas a ras de suelo: saltar el
                   impacto no alcanza, hay que salirse del camino. */
                int32_t feet = fx_to_int(fx_add(e->y, fx_from_int(BOSS_FOOT_OFFSET)));
                e->y = fx_from_int((feet / TILE_SIZE) * TILE_SIZE - BOSS_FOOT_OFFSET);
                e->vy = 0;
                w->shake = 8;
                proj_spawn_shock(fx_sub(e->x, fx_from_int(4)),
                                 fx_add(e->y, fx_from_int(16)), fx_neg(FX_C(1.8)));
                proj_spawn_shock(fx_add(e->x, fx_from_int(30)),
                                 fx_add(e->y, fx_from_int(16)), FX_C(1.8));
                enter_state(e, BOSS_CHOOSE);
            }
            break;
        }

        case BOSS_THROW: {
            int n = phase2 ? 4 : 3;
            if (e->timer % 20 == 1 && e->timer < 20 * n + 1) {
                fx_t vx = fx_add(fx_div(dx, fx_from_int(55)),
                                 (fx_t)((int32_t)rng_below(256) - 128));
                fx_t vy = fx_neg(fx_add(FX_C(3.6), (fx_t)rng_below(256)));
                proj_spawn_junk(fx_add(e->x, fx_from_int(12)),
                                fx_add(e->y, fx_from_int(4)), vx, vy);
            }
            if (e->timer > 20 * n + 25) enter_state(e, BOSS_CHOOSE);
            break;
        }

        case BOSS_TRASHBLAST:
            /* Exclusivo de Betty: en vez de lanzar de a uno, revienta un
               abanico de escombros a su alrededor de una sola vez. */
            if (e->timer == 1) {
                w->shake = 9;
                particles_burst(fx_add(e->x, fx_from_int(15)),
                                fx_add(e->y, fx_from_int(10)), PCOL_GREY, 18, FX_C(4.0));
                for (int i = -2; i <= 2; i++) {
                    proj_spawn_junk(fx_add(e->x, fx_from_int(15)),
                                    fx_add(e->y, fx_from_int(6)),
                                    fx_mul(fx_from_int(i), FX_C(1.4)),
                                    fx_neg(fx_add(FX_C(3.4), (fx_t)rng_below(333))));
                }
            }
            if (e->timer > 50) enter_state(e, BOSS_CHOOSE);
            break;

        case BOSS_SUMMON:
            if (e->timer == 1) {
                for (int i = 0; i < 2; i++) {
                    Entity *r = entity_pool_alloc(ENT_RAT);
                    if (!r) break;
                    r->x = fx_add(e->x, fx_from_int(i == 0 ? -24 : 34));
                    r->y = fx_add(e->y, fx_from_int(8));
                    r->dir = i == 0 ? -1 : 1;
                    r->face = r->dir;
                }
                w->shake = 5;
                particles_burst(fx_add(e->x, fx_from_int(15)),
                                fx_add(e->y, fx_from_int(10)), PCOL_WHITE, 12, FX_C(3.0));
            }
            if (e->timer > 40) enter_state(e, BOSS_CHOOSE);
            break;

        case BOSS_DIE: {
            static const ParticleColor DEATH_COLORS[4] = {
                PCOL_BROWN, PCOL_RED, PCOL_GOLD, PCOL_GREY
            };
            if (e->timer % 6 == 0) {
                particles_burst(fx_add(e->x, fx_from_int((int)rng_below(30))),
                                fx_add(e->y, fx_from_int((int)rng_below(22))),
                                DEATH_COLORS[rng_below(4)], 5, FX_C(3.0));
            }
            if (e->timer % 10 == 0) w->shake = 5;
            if (e->timer > DIE_FRAMES) {
                /* Deja dos corazones y abre el paso que había sellado. */
                for (int i = 0; i < 2; i++) {
                    Entity *drop = entity_pool_alloc(ENT_HP);
                    if (!drop) break;
                    drop->x = fx_add(e->x, fx_from_int(i == 0 ? 4 : 20));
                    drop->y = fx_add(e->y, fx_from_int(14));
                }
                world_boss_defeated(w);
                entity_pool_free(e);
            }
            return; /* agonizando ya no hace daño por contacto */
        }

        default: break;
    }

    /* El contacto sólo duele cuando el jefe pelea de verdad: durante la
       entrada cinemática Perseo puede acercarse sin castigo. */
    if (e->state != BOSS_INTRO) {
        world_touch_player(w, e);
    }
}

int boss_sprite_frame(const Entity *e, uint32_t tick) {
    /* Dos frames de marcha; aturdido o telegrafiando se queda quieto en
       el primero, que es justamente cuando conviene que se lea como
       vulnerable o a punto de atacar. */
    if (e->state == BOSS_STUN || e->state == BOSS_TELE) return 0;
    return (int)((tick >> 3) & 1);
}
