#include "player.h"
#include "world.h"
#include "entity_pool.h"
#include "particles.h"
#include "projectiles.h"
#include "audio.h"
#include "relics.h"

const char *const ABILITY_NAMES[ABIL_COUNT] = {
    "SALTO DOBLE", "DASH SOMBRÍO", "GARRA FELINA"
};

bool player_has_ability(const Player *p, AbilityId id) {
    switch (id) {
        case ABIL_DJUMP: return p->ab.double_jump;
        case ABIL_DASH:  return p->ab.dash;
        case ABIL_CLIMB: return p->ab.climb;
        default: return false;
    }
}

bool player_grant_ability(Player *p, AbilityId id) {
    if (player_has_ability(p, id)) return false;
    switch (id) {
        case ABIL_DJUMP: p->ab.double_jump = true; break;
        case ABIL_DASH:  p->ab.dash = true; break;
        case ABIL_CLIMB: p->ab.climb = true; break;
        default: return false;
    }
    return true;
}

/* Constantes de física — traducción valor-por-valor de las constantes
   locales de updatePlayer() en el prototipo (SPD, ACC, GRV, JV, MAXF) y
   de los literales usados en su cuerpo (0.8, 2.6, 3.8, 1.6, 3.4, 0.7).
   Se mantienen con magnitud positiva, igual que el original, y se
   niegan en el punto de uso (ej. "P.vy=-JV") para que el port se pueda
   comparar línea a línea contra la fuente. */
#define PLAYER_SPEED          FX_C(1.35)  /* SPD */
#define PLAYER_ACCEL          FX_C(0.30)  /* ACC */
#define PLAYER_GRAVITY        FX_C(0.22)  /* GRV */
#define PLAYER_JUMP_V         FX_C(4.2)   /* JV  */
#define PLAYER_MAX_FALL       FX_C(4.0)   /* MAXF */
#define PLAYER_FRICTION       FX_C(0.7)
#define PLAYER_WALLSLIDE_MAXV FX_C(0.8)
#define PLAYER_WALLJUMP_VY    FX_C(4.0)
#define PLAYER_WALLJUMP_VX    FX_C(2.6)
#define PLAYER_DJUMP_VY       FX_C(3.8)
#define PLAYER_VARJUMP_CAP    FX_C(1.6)
#define PLAYER_DASH_VX        FX_C(3.4)

#define PLAYER_ATK_FRAMES     12
#define PLAYER_ATK_COOLDOWN   18
#define PLAYER_THROW_FRAMES   10
#define PLAYER_THROW_COOLDOWN 34  /* más largo que el ganchito: pega de
                                     lejos, no debe volver trivial el
                                     cuerpo a cuerpo */
#define PLAYER_INV_FRAMES     90

#define PLAYER_JUMP_BUFFER_FRAMES 7
#define PLAYER_COYOTE_FRAMES      7
#define PLAYER_DASH_FRAMES        11
#define PLAYER_DASH_COOLDOWN      32

void player_init(Player *p, fx_t spawn_x, fx_t spawn_y) {
    Player zero = {0};
    *p = zero;
    p->x = spawn_x;
    p->y = spawn_y;
    p->w = 10;
    p->h = 13;
    p->face = 1;
    p->can_double_jump = true;
    p->hp = p->max_hp = 5;
}

void player_update(Player *p, World *w, const PlayerInput *in) {
    const Level *lv = w->lv;

    /* --- Jump buffer: ventana de 7 frames para que un salto pulsado un
       poco antes de tocar el suelo/muro igual cuente. --- */
    /* Pata de la Suerte: 1 corazón tras 12 s (720 frames) sin recibir
       daño. El contador se reinicia mientras dure la invulnerabilidad,
       que es justo después de un golpe. */
    if ((p->relics_equipped & RELIC_BIT(RELIC_PATA)) && p->hp < p->max_hp && p->inv == 0) {
        if (++p->regen_t > 720) {
            p->regen_t = 0;
            p->hp++;
            audio_play_sfx(SFX_PICK);
            particles_burst(fx_add(p->x, fx_from_int(5)), fx_add(p->y, fx_from_int(2)),
                            PCOL_RED, 4, FX_C(1.5));
        }
    } else if (p->inv > 0) {
        p->regen_t = 0;
    }

    if (in->jump_pressed) p->jump_buffer = PLAYER_JUMP_BUFFER_FRAMES;
    else if (p->jump_buffer > 0) p->jump_buffer--;

    /* El cooldown del dash se descuenta ANTES de evaluar si se puede
       arrancar uno nuevo este mismo frame — mismo orden que el
       prototipo (dashCd-- ocurre antes del "if(dPress&&...)"). */
    if (p->dash_cd > 0) p->dash_cd--;
    if (p->inv > 0) p->inv--;
    if (p->atk_cd > 0) p->atk_cd--;
    if (p->throw_cd > 0) p->throw_cd--;
    if (p->throw_t > 0) p->throw_t--;

    /* --- Dash Sombrío --- */
    if (in->dash_pressed && p->ab.dash && p->dash_cd == 0 && p->dash_t == 0) {
        p->dash_t = PLAYER_DASH_FRAMES;
        p->dash_cd = PLAYER_DASH_COOLDOWN;
        p->vy = 0;
        audio_play_sfx(SFX_DASH);
    }

    if (p->dash_t > 0) {
        p->dash_t--;
        p->vx = fx_mul(fx_from_int(p->face), PLAYER_DASH_VX);
        p->vy = 0;
    } else {
        /* Movimiento horizontal */
        if (in->left) {
            p->vx = fx_max(fx_sub(p->vx, PLAYER_ACCEL), fx_neg(PLAYER_SPEED));
            p->face = -1;
        } else if (in->right) {
            p->vx = fx_min(fx_add(p->vx, PLAYER_ACCEL), PLAYER_SPEED);
            p->face = 1;
        } else {
            p->vx = fx_mul(p->vx, PLAYER_FRICTION);
        }
        /* Gravedad */
        p->vy = fx_min(fx_add(p->vy, PLAYER_GRAVITY), PLAYER_MAX_FALL);
    }

    /* --- Muro (Garra Felina) --- */
    p->wall = 0;
    if (p->ab.climb && !p->on_ground && p->dash_t == 0) {
        int8_t dir = in->left ? -1 : (in->right ? 1 : 0);
        if (dir != 0) {
            fx_t probe_x = fx_add(p->x, fx_from_int((int32_t)dir * 2));
            fx_t probe_y = fx_add(p->y, fx_from_int(2));
            if (level_rect_solid(lv, probe_x, probe_y, p->w, p->h - 4)) {
                p->wall = dir;
                if (p->vy > PLAYER_WALLSLIDE_MAXV) p->vy = PLAYER_WALLSLIDE_MAXV;
                p->can_double_jump = true;
            }
        }
    }

    /* --- Saltos: suelo/coyote, muro o Salto Doble, por prioridad --- */
    if (p->jump_buffer > 0) {
        if (p->on_ground || p->coyote > 0) {
            p->vy = fx_neg(PLAYER_JUMP_V);
            p->on_ground = false;
            p->coyote = 0;
            p->jump_buffer = 0;
            p->spun = false;
            audio_play_sfx(SFX_JUMP);
        } else if (p->wall != 0) {
            p->vy = fx_neg(PLAYER_WALLJUMP_VY);
            p->vx = fx_neg(fx_mul(fx_from_int((int32_t)p->wall), PLAYER_WALLJUMP_VX));
            p->face = (int8_t)-p->wall;
            p->jump_buffer = 0;
            p->spun = false;
            audio_play_sfx(SFX_JUMP);
        } else if (p->ab.double_jump && p->can_double_jump) {
            p->vy = fx_neg(PLAYER_DJUMP_VY);
            p->can_double_jump = false;
            p->jump_buffer = 0;
            p->spun = true;
            audio_play_sfx(SFX_DJUMP);
        }
    }
    /* Salto de altura variable: soltar el botón corta el impulso hacia
       arriba (no aplica en pleno dash). */
    if (!in->jump_held && p->vy < fx_neg(PLAYER_VARJUMP_CAP) && p->dash_t == 0) {
        p->vy = fx_neg(PLAYER_VARJUMP_CAP);
    }

    /* --- Colisión horizontal ---
       Dashear rompe las rejillas oxidadas: es la cerradura que abre el
       Dash Sombrío, y por eso el tilemap del nivel vive en RAM (ver
       world_load). */
    p->x = fx_add(p->x, p->vx);
    {
        int16_t tx0 = fx_to_tile(p->x);
        int16_t tx1 = fx_to_tile(fx_add(p->x, fx_from_int(p->w - 1)));
        int16_t ty0 = fx_to_tile(p->y);
        int16_t ty1 = fx_to_tile(fx_add(p->y, fx_from_int(p->h - 1)));
        for (int16_t ty = ty0; ty <= ty1; ty++) {
            for (int16_t tx = tx0; tx <= tx1; tx++) {
                uint8_t id = level_tile_at(lv, tx, ty);
                if (id == TILE_RUST && p->dash_t > 0) {
                    world_break_tile(w, tx, ty);
                    particles_burst(fx_from_int((int32_t)tx * TILE_SIZE + 4),
                                    fx_from_int((int32_t)ty * TILE_SIZE + 4),
                                    PCOL_BROWN, 6, FX_C(3.0));
                    if (w->shake < 4) w->shake = 4;
                    audio_play_sfx(SFX_BREAK);
                    continue;
                }
                if (tile_is_solid(id)) {
                    if (p->vx > 0) p->x = fx_sub(fx_from_int((int32_t)tx * TILE_SIZE), fx_from_int(p->w));
                    else if (p->vx < 0) p->x = fx_from_int((int32_t)tx * TILE_SIZE + TILE_SIZE);
                    p->vx = 0;
                }
            }
        }
    }

    /* --- Colisión vertical --- */
    fx_t prev_feet = fx_add(p->y, fx_from_int(p->h));
    p->y = fx_add(p->y, p->vy);
    p->on_ground = false;
    {
        int16_t tx0 = fx_to_tile(p->x);
        int16_t tx1 = fx_to_tile(fx_add(p->x, fx_from_int(p->w - 1)));
        if (p->vy >= 0) {
            /* Se usa (y+h)/8 y NO (y+h-1)/8 — ver la nota del prototipo:
               con "-1" el aterrizaje se detecta un píxel tarde y
               produce parpadeo entre los estados de caminar/caer. */
            int16_t ty = fx_to_tile(fx_add(p->y, fx_from_int(p->h)));
            for (int16_t tx = tx0; tx <= tx1; tx++) {
                uint8_t id = level_tile_at(lv, tx, ty);
                bool plat = id == TILE_PLATFORM
                    && fx_to_int(prev_feet) <= (int32_t)ty * TILE_SIZE + 1
                    && !in->down;
                if (tile_is_solid(id) || plat) {
                    p->y = fx_sub(fx_from_int((int32_t)ty * TILE_SIZE), fx_from_int(p->h));
                    p->vy = 0;
                    p->on_ground = true;
                    p->can_double_jump = true;
                    p->coyote = PLAYER_COYOTE_FRAMES;
                    p->spun = false;
                }
            }
        } else {
            int16_t ty = fx_to_tile(p->y);
            for (int16_t tx = tx0; tx <= tx1; tx++) {
                if (tile_is_solid(level_tile_at(lv, tx, ty))) {
                    p->y = fx_from_int((int32_t)ty * TILE_SIZE + TILE_SIZE);
                    p->vy = 0;
                }
            }
        }
    }
    if (!p->on_ground && p->coyote > 0) p->coyote--;

    /* --- Animación de caminata (bandera; los frames de arte real
       llegan en la Fase 3) --- */
    p->walking = p->on_ground && p->dash_t == 0 && (in->left || in->right);
    if (p->walking) p->walk_anim++; else p->walk_anim = 0;

    /* --- Tiles dañinos: pinchos, lodo y vapor ---
       Se muestrea el interior de la caja (1 px de margen) y gana el
       último peligro encontrado, igual que el prototipo. */
    {
        int16_t tx0 = fx_to_tile(fx_add(p->x, fx_from_int(1)));
        int16_t tx1 = fx_to_tile(fx_add(p->x, fx_from_int(p->w - 2)));
        int16_t ty0 = fx_to_tile(fx_add(p->y, fx_from_int(2)));
        int16_t ty1 = fx_to_tile(fx_add(p->y, fx_from_int(p->h - 1)));
        uint8_t hazard = 0;
        for (int16_t ty = ty0; ty <= ty1; ty++) {
            for (int16_t tx = tx0; tx <= tx1; tx++) {
                uint8_t id = level_tile_at(lv, tx, ty);
                if (id == TILE_SPIKE || id == TILE_MUD) hazard = id;
                /* El vapor sólo quema mientras sale: alterna cada 40 frames. */
                if (id == TILE_STEAM && ((w->tick / 40) & 1) == 0) hazard = id;
            }
        }
        if (hazard == TILE_SPIKE || hazard == TILE_STEAM) {
            world_damage_player(w, 1, (int8_t)-p->face);
        } else if (hazard == TILE_MUD) {
            if (p->inv == 0) {
                world_damage_player(w, 1, (int8_t)-p->face);
                p->vy = fx_neg(FX_C(3.2));
            } else {
                p->vy = fx_min(p->vy, fx_neg(FX_C(1.0)));
            }
        }
    }

    /* --- Caída al vacío --- */
    if (p->y > fx_from_int((int32_t)lv->h * TILE_SIZE + 40)) {
        world_damage_player(w, 1, 1);
        if (!p->dead) {
            p->x = fx_from_int((int32_t)lv->spawn_tx * TILE_SIZE);
            p->y = fx_from_int((int32_t)lv->spawn_ty * TILE_SIZE);
            p->vx = p->vy = 0;
        }
    }

    /* --- Habilidad pasiva 1: Ganchito Mortal (cuerpo a cuerpo) --- */
    if (in->attack_pressed && p->atk_cd == 0 && p->dash_t == 0) {
        p->atk_t = PLAYER_ATK_FRAMES;
        p->atk_cd = PLAYER_ATK_COOLDOWN;
        p->swing++;
        audio_play_sfx(SFX_ATK);
    }
    if (p->atk_t > 0) {
        p->atk_t--;
        /* Caja de golpe por delante de la zarpa. */
        Rect hb = {
            p->face > 0 ? fx_add(p->x, fx_from_int(p->w - 2))
                        : fx_sub(p->x, fx_from_int(13)),
            fx_sub(p->y, fx_from_int(1)), 15, 15
        };
        for (int i = 0; i < ENTITY_POOL_CAPACITY; i++) {
            Entity *e = entity_pool_at(i);
            if (!e->alive || e->hit_swing == p->swing) continue;
            Rect box = entity_box(e);
            if (box.w == 0) continue; /* no es golpeable */
            if (rect_overlap(hb, box)) {
                world_hit_enemy(w, e, 1);
                e->hit_swing = p->swing;
            }
        }
    }

    /* --- Habilidad pasiva 2: Lanzamiento de Traza (a distancia) --- */
    if (in->throw_pressed && p->throw_cd == 0 && p->dash_t == 0) {
        p->throw_t = PLAYER_THROW_FRAMES;
        p->throw_cd = PLAYER_THROW_COOLDOWN;
        fx_t sx = p->face > 0 ? fx_add(p->x, fx_from_int(p->w - 2))
                              : fx_sub(p->x, fx_from_int(6));
        proj_spawn_traza(sx, fx_add(p->y, fx_from_int(3)), p->face);
        audio_play_sfx(SFX_TRAZA);
        particles_burst(fx_add(p->x, fx_from_int(p->face > 0 ? p->w : 0)),
                        fx_add(p->y, fx_from_int(6)), PCOL_BROWN, 4, FX_C(2.0));
    }

    /* Reliquias (Pata de la Suerte, etc.) siguen siendo de la Fase 7. */
}

/* Traducción directa de drawPlayer() del prototipo (línea ~2318): misma
   prioridad de estados y mismos tiempos. Las poses de ataque ya están en la
   prioridad de estados y mismos tiempos. */
PlayerAnim player_get_anim(const Player *p, uint32_t tick) {
    PlayerAnim a = { PFRAME_IDLE1, 0, p->face < 0, false };

    /* Parpadeo de invulnerabilidad: se oculta 2 de cada 4 frames. */
    a.hidden = (p->inv > 0) && ((tick >> 2) & 1);

    if (p->dash_t > 0 || p->spun) {
        a.frame = PFRAME_DASH;
    } else if (p->atk_t > 0) {
        /* El zarpazo dura 12 frames: preparación, impacto con las garras
           fuera y recobro — que se vea la patita lanzarse y volver, en vez
           de una pose fija. */
        a.frame = p->atk_t > 8 ? PFRAME_ATK1 : (p->atk_t > 4 ? PFRAME_ATK2 : PFRAME_ATK3);
    } else if (!p->on_ground) {
        a.frame = (p->vy < 0) ? PFRAME_JUMP : PFRAME_FALL;
    } else if (p->walking) {
        /* Ciclo de 4 fases, una cada 4 frames, con "contoneo" de 1 px en las
           fases de contacto (1 y 3) — el vaivén de peso que pide
           docs/Estilo_Grafico_Perseo.md. */
        uint32_t wf = (p->walk_anim >> 2) & 3;
        a.frame = (uint8_t)(PFRAME_WALK1 + wf);
        a.bob = (wf == 1 || wf == 3) ? 1 : 0;
    } else {
        a.frame = ((tick >> 5) & 1) ? PFRAME_IDLE1 : PFRAME_IDLE2;
    }
    return a;
}
