#include "world.h"
#include "entity_pool.h"
#include "particles.h"
#include "projectiles.h"
#include "rng.h"
#include "enemies/enemy_common.h"
#include "boss/boss_fsm.h"
#include "audio.h"
#include "relics.h"
#include <string.h>

/* ---------- Cajas de colisión ---------- */

bool rect_overlap(Rect a, Rect b) {
    return a.x < fx_add(b.x, fx_from_int(b.w))
        && fx_add(a.x, fx_from_int(a.w)) > b.x
        && a.y < fx_add(b.y, fx_from_int(b.h))
        && fx_add(a.y, fx_from_int(a.h)) > b.y;
}

Rect player_box(const Player *p) {
    Rect r = { p->x, p->y, p->w, p->h };
    return r;
}

/* Tamaños tomados de entBox() del prototipo. Un ancho 0 significa
   "no golpea ni se le puede pegar": decoración, recogibles, proyectiles. */
Rect entity_box(const Entity *e) {
    Rect r = { e->x, e->y, 0, 0 };
    switch (e->type) {
        case ENT_RAT:    r.w = 14; r.h = 8;  break;
        case ENT_ROACH:  r.w = 8;  r.h = 6;  break;
        case ENT_MOSQ:   r.w = 8;  r.h = 8;  break;
        case ENT_BAT:    r.w = 8;  r.h = 8;  break;
        case ENT_THUG:   r.w = 16; r.h = 10; break;
        case ENT_GUNNER: r.w = 10; r.h = 8;  break;
        case ENT_BRUTE:  r.w = 16; r.h = 10; break;
        case ENT_BOSS: {
            const BossConfig *cfg = boss_config_of(e);
            r.x = fx_add(e->x, fx_from_int(1));
            r.y = fx_add(e->y, fx_from_int(2));
            r.w = cfg->w; r.h = cfg->h;
            break;
        }
        default: break;
    }
    return r;
}

fx_t world_ground_y(const World *w, fx_t px, fx_t py) {
    int16_t tx = fx_to_tile(px);
    int16_t ty = fx_to_tile(py);
    while (ty < w->lv->h && !tile_is_solid(level_tile_at(w->lv, tx, ty))) ty++;
    return fx_from_int((int32_t)ty * TILE_SIZE);
}

/* ---------- Patrulla compartida ---------- */

void world_patrol(const World *w, Entity *e, fx_t speed, int16_t width,
                  int16_t wall_probe_y, int16_t floor_probe_y) {
    /* Si viene empujado por un golpe manda el empujón; si no, su paso. */
    fx_t step = (e->vx != 0) ? e->vx : fx_mul(fx_from_int(e->dir), speed);
    fx_t nx = fx_add(e->x, step);
    fx_t front = e->dir < 0 ? nx : fx_add(nx, fx_from_int(width));

    int16_t ftx = fx_to_tile(front);
    bool wall  = tile_is_solid(level_tile_at(w->lv, ftx,
                     fx_to_tile(fx_add(e->y, fx_from_int(wall_probe_y)))));
    bool floor = tile_is_solid(level_tile_at(w->lv, ftx,
                     fx_to_tile(fx_add(e->y, fx_from_int(floor_probe_y)))));

    /* Se da la vuelta ante un muro o ante un precipicio: nunca se tira. */
    if (wall || !floor) e->dir = (int8_t)-e->dir;
    else e->x = nx;
    e->face = e->dir;
}

/* ---------- Daño ---------- */

void world_damage_player(World *w, int amount, int8_t dir) {
    Player *p = &w->player;
    /* En pleno dash Perseo es intocable: es parte de para qué sirve. */
    if (p->inv > 0 || p->dash_t > 0 || p->dead) return;

    if (amount < 1) amount = 1;
    /* Escalado por dificultad, con la misma cuenta que el prototipo:
       max(1, round(n * dmgTaken)). */
    if (w->dmg_den) {
        amount = (amount * w->dmg_num + w->dmg_den / 2) / w->dmg_den;
        if (amount < 1) amount = 1;
    }
    /* Bigotes de Acero: -1, nunca por debajo de 1. */
    if (p->relics_equipped & RELIC_BIT(RELIC_BIGOTES)) {
        amount = amount > 1 ? amount - 1 : 1;
    }
    p->regen_t = 0;
    p->hp -= (int16_t)amount;
    p->inv = 90;
    p->vx = fx_mul(fx_from_int(dir ? dir : 1), FX_C(2.0));
    p->vy = fx_neg(FX_C(2.5));
    w->shake = 6;
    particles_burst(fx_add(p->x, fx_from_int(5)), fx_add(p->y, fx_from_int(6)),
                    PCOL_RED, 6, FX_C(2.5));
    audio_play_sfx(SFX_HURT);
    if (p->hp <= 0) {
        p->hp = 0;
        p->dead = true;
    }
}

void world_hit_enemy(World *w, Entity *e, int amount) {
    if (e->type == ENT_BOSS) {
        if (e->state == BOSS_DIE) return;
        /* Aturdido vale el doble: premia esperar a que se estrelle en vez
           de cambiar golpes de frente. */
        if (e->state == BOSS_STUN) amount *= 2;
    }
    /* Colmillo Afilado. Va acá y no en el ataque cuerpo a cuerpo para
       copiar al prototipo, que lo suma dentro de hitEnemy(): la traza
       lanzada también se beneficia. */
    if (w->player.relics_equipped & RELIC_BIT(RELIC_COLMILLO)) amount += 1;
    e->hp -= (int16_t)amount;
    e->flash = 8;
    Rect box = entity_box(e);
    fx_t cx = fx_add(box.x, fx_from_int(box.w / 2));
    fx_t cy = fx_add(box.y, fx_from_int(box.h / 2));
    particles_burst(cx, cy, PCOL_WHITE, 5, FX_C(2.5));
    audio_play_sfx(SFX_HIT_ENEMY);
    /* Empujón hacia el lado contrario al jugador; a un jefe no lo mueve. */
    if (e->type != ENT_BOSS) {
        e->vx = fx_mul(fx_from_int(e->x < w->player.x ? -1 : 1), FX_C(1.5));
    }

    if (e->hp <= 0) {
        if (e->type == ENT_BOSS) {
            /* No se libera acá: la agonía es un estado más de la FSM. */
            e->state = BOSS_DIE;
            e->timer = 0;
            return;
        }
        particles_burst(cx, cy, PCOL_RED, 10, FX_C(3.0));
        audio_play_sfx(SFX_SPLAT);
        /* Uno de cada cuatro suelta un corazón. */
        if (rng_chance(25)) {
            Entity *drop = entity_pool_alloc(ENT_HP);
            if (drop) { drop->x = e->x; drop->y = e->y; }
        }
        entity_pool_free(e);
    }
}

void world_touch_player(World *w, Entity *e) {
    if (rect_overlap(entity_box(e), player_box(&w->player))) {
        world_damage_player(w, 1, w->player.x < e->x ? -1 : 1);
    }
}

/* ---------- Recogibles ---------- */

static void update_pickup(World *w, Entity *e) {
    Player *p = &w->player;
    Rect box = { e->x, e->y, 8, e->type == ENT_HP ? 7 : 8 };
    if (!rect_overlap(box, player_box(p))) return;

    fx_t cx = fx_add(e->x, fx_from_int(4));
    fx_t cy = fx_add(e->y, fx_from_int(4));
    if (e->type == ENT_CHAPA) {
        w->chapas++;
        particles_burst(cx, cy, PCOL_GOLD, 6, FX_C(2.0));
    } else {
        if (p->hp < p->max_hp) p->hp++;
        particles_burst(cx, cy, PCOL_RED, 6, FX_C(2.0));
    }
    audio_play_sfx(SFX_PICK);
    entity_pool_free(e);
}

/* ---------- Carga y ciclo ---------- */

static uint8_t s_tiles[LEVEL_MAX_TILES];

void world_carve(World *w, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t id) {
    level_carve(&w->level_rt, x0, y0, x1, y1, id);
    w->tiles_dirty = true;
}

/* Al caer un jefe: se abre el paso que había sellado y aparece la puerta
   al nivel siguiente, justo donde los datos del nivel dicen (`exit`). La
   de Betty es distinta — no lleva a otro nivel, abre el desenlace — y por
   eso es una ENT_VDOOR. Sin esto el juego se quedaba encerrado en la
   arena del jefe, que es exactamente lo que faltaba para poder recorrer
   los siete niveles de una sentada. */
/* Declarada acá porque world_boss_defeated() la necesita y su
   definición vive más abajo, junto al resto de las interacciones. */
static void post_event(World *w, WorldEventKind kind,
                       const char *title, const char *desc, int16_t param);

void world_boss_defeated(World *w) {
    Entity *boss = w->boss;
    w->boss_active = false;
    w->boss = 0;

    if (w->boss_gate && w->boss_gate->spawn) {
        const int16_t *p = w->boss_gate->spawn->passage;
        world_carve(w, p[0], p[1], p[2], p[3], TILE_EMPTY);
    }
    w->boss_gate = 0;

    if (!boss || !boss->spawn) return;
    const BossConfig *cfg = boss_config_of(boss);
    Entity *door = entity_pool_alloc(cfg->is_final ? ENT_VDOOR : ENT_DOOR);
    if (door) {
        door->x = fx_from_int((int32_t)boss->spawn->exit_tx * TILE_SIZE);
        door->y = fx_from_int((int32_t)boss->spawn->exit_ty * TILE_SIZE);
        door->param = cfg->next_level;
    }

    audio_play_sfx(SFX_DOOR);
    if (!cfg->is_final) audio_play_song((SongId)w->lv->song);
    post_event(w, WEV_BANNER, cfg->victory_title, cfg->victory_desc, 0);
}

void world_load(World *w, const Level *lv) {
    /* El tilemap generado vive en ROM; el juego necesita poder cambiarlo
       (romper rejillas, sellar y abrir la arena del jefe), asi que se
       trabaja sobre una copia en RAM. */
    int32_t count = (int32_t)lv->w * lv->h;
    if (count > LEVEL_MAX_TILES) count = LEVEL_MAX_TILES;
    memcpy(s_tiles, lv->tiles, (size_t)count);
    w->level_rt = *lv;
    w->level_rt.tiles = s_tiles;
    w->lv = &w->level_rt;
    /* El nivel entra limpio: el cambio de nivel ya fuerza su propio
       redibujado completo. */
    w->tiles_dirty = false;
    w->tick = 0;
    w->shake = 0;
    w->chapas = 0;
    if (!w->dmg_den) { w->dmg_num = 1; w->dmg_den = 1; }

    entity_pool_reset();
    particles_reset();
    rng_seed(0xC0FFEEu);
    w->boss = 0;
    w->boss_gate = 0;
    w->boss_active = false;

    player_init(&w->player,
                fx_from_int((int32_t)lv->spawn_tx * TILE_SIZE),
                fx_from_int((int32_t)lv->spawn_ty * TILE_SIZE));

    w->event.kind = WEV_NONE;
    w->sign_text = 0;

    /* Instancia las entidades que trae el nivel. */
    for (int i = 0; i < lv->entity_count; i++) {
        const LevelEntitySpawn *sp = &lv->entities[i];
        switch (sp->type) {
            case ENT_RAT: case ENT_ROACH: case ENT_MOSQ: case ENT_GUNNER:
            case ENT_BAT: case ENT_THUG: case ENT_BRUTE:
            case ENT_CHAPA: case ENT_HP: case ENT_RELIC:
            case ENT_SHRINE: case ENT_LAMP: case ENT_SIGN:
            case ENT_DOOR: case ENT_VDOOR:
            case ENT_BOSS: case ENT_BOSSGATE:
                break;
            default:
                continue;
        }
        Entity *e = entity_pool_alloc((EntityType)sp->type);
        if (!e) break; /* pool lleno: el resto del nivel se queda sin poblar */
        e->x = fx_from_int((int32_t)sp->tx * TILE_SIZE);
        e->y = fx_from_int((int32_t)sp->ty * TILE_SIZE);
        e->param = sp->param;
        e->dir = sp->param < 0 ? -1 : 1;
        e->face = e->dir;
        e->spawn = sp;

        if (e->type == ENT_BOSS) {
            w->boss = e;
            boss_init(e, w);
        } else if (e->type == ENT_BOSSGATE) {
            w->boss_gate = e;
        }
    }
}

/* Deja un aviso para la capa de partida. El primero del frame gana: si
   dos interacciones coincidieran, la segunda seguiría estando ahí el
   frame siguiente, porque las entidades no se consumen hasta que su
   interacción se resuelve. */
static void post_event(World *w, WorldEventKind kind,
                       const char *title, const char *desc, int16_t param) {
    if (w->event.kind != WEV_NONE) return;
    w->event.kind = (uint8_t)kind;
    w->event.title = title;
    w->event.desc = desc;
    w->event.param = param;
    w->event.tx = w->event.ty = 0;
}

/* Santuario: entrega una de las tres habilidades llave. La caja es más
   angosta que el sprite (el santuario tiene una base ancha que no debe
   contar) — mismos números que el prototipo. */
static void update_shrine(World *w, Entity *e) {
    Rect box = { fx_add(e->x, fx_from_int(3)), e->y, 10, 16 };
    if (!rect_overlap(box, player_box(&w->player))) return;
    AbilityId id = (AbilityId)e->param;
    if (!player_grant_ability(&w->player, id)) return;

    audio_play_sfx(SFX_ABILITY);
    particles_burst(fx_add(e->x, fx_from_int(8)), fx_add(e->y, fx_from_int(4)),
                    PCOL_WHITE, 16, FX_C(3.5));
    w->shake = 4;
    const char *name = e->spawn && e->spawn->text ? e->spawn->text : ABILITY_NAMES[id];
    post_event(w, WEV_BANNER, name, e->spawn ? e->spawn->desc : 0, 0);
    entity_pool_free(e);
}

static void update_relic(World *w, Entity *e) {
    Rect box = { e->x, e->y, 8, 8 };
    if (!rect_overlap(box, player_box(&w->player))) return;
    RelicId id = (RelicId)e->param;
    if (id >= RELIC_COUNT) return;
    uint8_t bit = RELIC_BIT(id);
    if (w->player.relics_found & bit) return;

    w->player.relics_found |= bit;
    /* Se equipa sola si queda hueco: la primera reliquia del juego no
       debería obligar a abrir el inventario para que sirva de algo. */
    if (relic_equipped_count(w->player.relics_equipped) < RELIC_MAX_EQUIPPED) {
        w->player.relics_equipped |= bit;
    }
    audio_play_sfx(SFX_ABILITY);
    particles_burst(fx_add(e->x, fx_from_int(4)), fx_add(e->y, fx_from_int(4)),
                    PCOL_GOLD, 12, FX_C(2.5));
    w->shake = 3;
    post_event(w, WEV_BANNER, RELICS[id].name, RELICS[id].desc, 0);
    entity_pool_free(e);
}

/* Lámpara: punto de control. Sólo una encendida a la vez, como en el
   prototipo — la nueva apaga a las demás. */
static void update_lamp(World *w, Entity *e) {
    if (e->state) return;
    Rect box = { e->x, e->y, 8, 16 };
    if (!rect_overlap(box, player_box(&w->player))) return;

    for (int i = 0; i < ENTITY_POOL_CAPACITY; i++) {
        Entity *o = entity_pool_at(i);
        if (o->alive && o->type == ENT_LAMP) o->state = 0;
    }
    e->state = 1;
    audio_play_sfx(SFX_CHECK);
    particles_burst(fx_add(e->x, fx_from_int(4)), fx_add(e->y, fx_from_int(2)),
                    PCOL_GOLD, 8, FX_C(2.0));
    post_event(w, WEV_CHECKPOINT, 0, 0, 0);
    w->event.tx = (int16_t)(fx_to_int(e->x) / TILE_SIZE);
    w->event.ty = (int16_t)(fx_to_int(e->y) / TILE_SIZE - 1);
}

static void update_sign(World *w, Entity *e) {
    if (!e->spawn || !e->spawn->text) return;
    fx_t dx = fx_sub(w->player.x, e->x);
    fx_t dy = fx_sub(w->player.y, e->y);
    if (dx < 0) dx = fx_neg(dx);
    if (dy < 0) dy = fx_neg(dy);
    if (dx < fx_from_int(26) && dy < fx_from_int(30)) w->sign_text = e->spawn->text;
}

static void update_door(World *w, Entity *e) {
    Rect box = { e->x, e->y, 16, 24 };
    if (!rect_overlap(box, player_box(&w->player))) return;
    if (e->type == ENT_VDOOR) {
        post_event(w, WEV_ENDING, 0, 0, 0);
        return;
    }
    audio_play_sfx(SFX_DOOR);
    post_event(w, WEV_DOOR, 0, 0, e->param);
}

/* Disparador de la pelea: sella la arena cuando el jugador la cruza.

   OJO con la condicion — el prototipo tuvo aca un bug que vale la pena no
   repetir: comparaba solo P.x y una banda vertical demasiado ancha, asi
   que el disparador saltaba tambien en el punto de aparicion del nivel
   (misma columna, pero en el pasillo de abajo) y sellaba el paso ANTES de
   recorrer el nivel. Por eso ademas de la posicion horizontal se exige
   estar dentro de la franja vertical real de la arena. */
static void update_bossgate(World *w, Entity *gate) {
    if (w->boss_active || !w->boss || !w->boss->alive || !gate->spawn) return;

    const LevelEntitySpawn *sp = gate->spawn;
    const Player *p = &w->player;
    fx_t right = fx_add(p->x, fx_from_int(p->w));
    if (right >= fx_sub(gate->x, fx_from_int(4))) return;
    if (p->y <= fx_from_int(sp->yband[0]) || p->y >= fx_from_int(sp->yband[1])) return;

    w->boss_active = true;
    world_carve(w, sp->gate[0], sp->gate[1], sp->gate[2], sp->gate[3], TILE_BRICK);

    /* Gracia al entrar: en una arena chica Perseo puede quedar pegado al
       jefe justo cuando la puerta se cierra, y sin esto se comeria un
       golpe de contacto invisible en el mismo instante en que empieza la
       pelea. */
    if (w->player.inv < 90) w->player.inv = 90;

    /* El jefe espera quieto mientras se cruzan las réplicas; entra en
       escena cuando la partida cierra el diálogo (GS_BOSSDIALOG). */
    w->boss->state = BOSS_WAIT;
    w->boss->timer = 0;
    audio_play_sfx(SFX_CHECK);
    audio_play_song(SONG_BOSS);
    post_event(w, WEV_BOSS_INTRO, 0, 0, w->boss->param);
}

/* ---------- Progreso que sobrevive a recargar el nivel ---------- */

void world_take_progress(const World *w, PlayerProgress *pr) {
    pr->ab = w->player.ab;
    pr->relics_found = w->player.relics_found;
    pr->relics_equipped = w->player.relics_equipped;
    pr->chapas = w->chapas;
}

void world_place_player(World *w, int16_t tx, int16_t ty) {
    Player *p = &w->player;
    p->x = fx_from_int((int32_t)tx * TILE_SIZE);
    p->y = fx_from_int((int32_t)ty * TILE_SIZE);
    p->vx = p->vy = 0;
    p->dash_t = 0;
    p->on_ground = false;
}

void world_apply_progress(World *w, const PlayerProgress *pr) {
    w->player.ab = pr->ab;
    w->player.relics_found = pr->relics_found;
    w->player.relics_equipped = pr->relics_equipped;
    w->chapas = pr->chapas;

    /* Lo ya conseguido no vuelve a aparecer. Sin esto, morir en el nivel
       1 dejaría los dos santuarios otra vez de pie, con su cartel y su
       fanfarria, entregando algo que Perseo ya tiene. */
    for (int i = 0; i < ENTITY_POOL_CAPACITY; i++) {
        Entity *e = entity_pool_at(i);
        if (!e->alive) continue;
        if (e->type == ENT_SHRINE && player_has_ability(&w->player, (AbilityId)e->param)) {
            entity_pool_free(e);
        } else if (e->type == ENT_RELIC && e->param < RELIC_COUNT &&
                   (w->player.relics_found & RELIC_BIT(e->param))) {
            entity_pool_free(e);
        }
    }
}

void world_update(World *w, const PlayerInput *in) {
    /* Muerto no se actualiza nada: la pantalla de muerte y la
       reaparición las lleva core/game_state.c. */
    if (w->player.dead) return;
    w->tick++;
    if (w->shake > 0) w->shake--;
    w->sign_text = 0;   /* estado, no evento: vale sólo para este frame */

    player_update(&w->player, w, in);

    for (int i = 0; i < ENTITY_POOL_CAPACITY; i++) {
        Entity *e = entity_pool_at(i);
        if (!e->alive) continue;
        if (e->flash > 0) e->flash--;

        switch (e->type) {
            case ENT_RAT:    enemy_rat_update(e, w);    break;
            case ENT_ROACH:  enemy_roach_update(e, w);  break;
            case ENT_MOSQ:   enemy_mosq_update(e, w);   break;
            case ENT_BAT:    enemy_bat_update(e, w);    break;
            case ENT_THUG:   enemy_thug_update(e, w);   break;
            case ENT_GUNNER: enemy_gunner_update(e, w); break;
            case ENT_BRUTE:  enemy_brute_update(e, w);  break;
            case ENT_BOSS:     boss_update(e, w);     break;
            case ENT_BOSSGATE: update_bossgate(w, e); break;

            case ENT_PROJ_TRAZA:
            case ENT_PROJ_JUNK:
            case ENT_PROJ_SHOCK:
                proj_update(e, w);
                break;

            case ENT_CHAPA:
            case ENT_HP:
                update_pickup(w, e);
                break;

            case ENT_SHRINE: update_shrine(w, e); break;
            case ENT_RELIC:  update_relic(w, e);  break;
            case ENT_LAMP:   update_lamp(w, e);   break;
            case ENT_SIGN:   update_sign(w, e);   break;
            case ENT_DOOR:
            case ENT_VDOOR:  update_door(w, e);   break;

            default: break;
        }
    }

    particles_update();
}
