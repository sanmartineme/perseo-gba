#include "game_state.h"
#include "audio.h"
#include "entity_pool.h"
#include "relics.h"
#include "boss/boss_fsm.h"
#include "save.h"
#include "../platform/pal.h"
#include "level/level01_tuneles.h"
#include "level/level02_vertedero.h"

/* DIFF_CFG del prototipo. Ojo con "fácil": su dmgTaken de 0.5 no reduce
   nada cuando el golpe vale 1 (Math.round(0.5) es 1), que es el caso de
   casi todo el juego; lo que de verdad cambia es el corazón de más. Se
   conserva igual para no alterar el balance del original. */
const DiffCfg DIFF_CFGS[DIFF_COUNT] = {
    [DIFF_EASY]   = { 6, 1, 2 },
    [DIFF_NORMAL] = { 5, 1, 1 },
    [DIFF_HARD]   = { 4, 8, 5 },   /* 1.6 */
};

const char *const DIFF_LABELS[DIFF_COUNT] = { "FÁCIL", "NORMAL", "DIFÍCIL" };

/* Los niveles que existen hoy. La tabla es el único sitio que hay que
   tocar cuando la Fase 8 genere los cinco que faltan. */
static const Level *const LEVELS[GAME_LEVEL_COUNT] = {
    &level01_tuneles,
    &level02_vertedero,
};

#define TRANS_FRAMES 26   /* por cada mitad: cerrar y abrir */
#define BANNER_FRAMES 170 /* igual que el prototipo */
#define DEAD_FRAMES 60    /* antes de aceptar el botón de reintentar */

const Level *game_level(uint8_t i) {
    return i < GAME_LEVEL_COUNT ? LEVELS[i] : 0;
}

int game_scale_damage(const Game *g, int amount) {
    const DiffCfg *d = &DIFF_CFGS[g->difficulty];
    /* round(amount * num / den) con enteros. */
    int n = (amount * d->dmg_num + d->dmg_den / 2) / d->dmg_den;
    return n < 1 ? 1 : n;
}

/* --------------------------------------------------------------------
   Guardado
   --------------------------------------------------------------------
   El struct se arma acá, en core, y la PAL sólo mueve bytes. Guardar es
   automático: ocurre al encender una lámpara.
   -------------------------------------------------------------------- */
static void game_to_save(const Game *g, SaveData *s) {
    const Player *p = &g->world.player;
    s->difficulty = (uint8_t)g->difficulty;
    s->muted = g->muted ? 1 : 0;
    s->abilities = (uint8_t)((g->progress.ab.double_jump ? 1u : 0u) |
                             (g->progress.ab.dash        ? 2u : 0u) |
                             (g->progress.ab.climb       ? 4u : 0u));
    s->relics_found = g->progress.relics_found;
    s->relics_equipped = g->progress.relics_equipped;
    s->level = g->level_index;
    s->has_checkpoint = g->has_checkpoint ? 1 : 0;
    s->cp_level = g->cp_level;
    s->cp_tx = g->cp_tx;
    s->cp_ty = g->cp_ty;
    s->chapas = g->progress.chapas;
    s->deaths = g->deaths;
    s->play_frames = g->play_frames;
    (void)p;
    save_seal(s);
}

bool game_save(const Game *g) {
    SaveData s = { 0 };
    game_to_save(g, &s);
    return pal_save_write(&s, sizeof s);
}

bool game_has_save(void) {
    SaveData s;
    return pal_save_read(&s, sizeof s) && save_is_valid(&s);
}

static void enter(Game *g, GameState st) {
    g->state = st;
    g->state_t = 0;
}

void game_load_level(Game *g, uint8_t index) {
    const Level *lv = game_level(index);
    if (!lv) return;
    g->level_index = index;
    /* world_load() ya vacía el pool de entidades (y con él los
       proyectiles) y las partículas: no hace falta limpiarlos acá. */
    world_load(&g->world, lv);
    world_apply_progress(&g->world, &g->progress);
    g->world.dmg_num = DIFF_CFGS[g->difficulty].dmg_num;
    g->world.dmg_den = DIFF_CFGS[g->difficulty].dmg_den;
    g->world.player.max_hp = DIFF_CFGS[g->difficulty].start_hp;
    g->world.player.hp = g->world.player.max_hp;
    /* camera_update() es instantánea (no interpola), así que llamarla
       una vez ya deja la cámara encuadrada sin barrido inicial. */
    camera_init(&g->cam);
    camera_update(&g->cam, g->world.player.x, g->world.player.y, g->world.lv);
    g->zone_t = 150;   /* 2,5 s, como el zoneT del prototipo */
    audio_play_song((SongId)lv->song);
}

bool game_load(Game *g) {
    SaveData s;
    if (!pal_save_read(&s, sizeof s) || !save_is_valid(&s)) return false;

    g->difficulty = s.difficulty < DIFF_COUNT ? (Difficulty)s.difficulty : DIFF_NORMAL;
    g->muted = s.muted != 0;
    audio_set_muted(g->muted);

    g->progress.ab.double_jump = (s.abilities & 1u) != 0;
    g->progress.ab.dash        = (s.abilities & 2u) != 0;
    g->progress.ab.climb       = (s.abilities & 4u) != 0;
    g->progress.relics_found = s.relics_found;
    g->progress.relics_equipped = s.relics_equipped;
    g->progress.chapas = s.chapas;
    g->deaths = s.deaths;
    g->play_frames = s.play_frames;
    g->has_checkpoint = s.has_checkpoint != 0;
    g->cp_level = s.cp_level;
    g->cp_tx = s.cp_tx;
    g->cp_ty = s.cp_ty;

    uint8_t lv = g->has_checkpoint ? s.cp_level : s.level;
    if (!game_level(lv)) lv = 0;
    game_load_level(g, lv);
    if (g->has_checkpoint) {
        world_place_player(&g->world, g->cp_tx, g->cp_ty);
        camera_update(&g->cam, g->world.player.x, g->world.player.y, g->world.lv);
    }
    enter(g, GS_PLAY);
    return true;
}

/* Presenta el nivel con sus páginas de historia la primera vez que se
   entra; las siguientes veces (morir y volver) entra directo al juego,
   igual que `storyShown` en el prototipo. */
static void enter_level_or_story(Game *g) {
    const Level *lv = g->world.lv;
    uint8_t bit = (uint8_t)(1u << (g->level_index & 7));
    if (lv->story_count && !(g->story_shown_mask & bit)) {
        g->story_shown_mask |= bit;
        g->story_pages = lv->story;
        g->story_scenes = 0;
        g->story_count = lv->story_count;
        g->story_idx = 0;
        g->story_next = GS_PLAY;
        enter(g, GS_STORY);
    } else {
        enter(g, GS_PLAY);
    }
}

void game_begin_transition(Game *g, uint8_t to_level) {
    g->trans_to = to_level;
    g->trans_closing = true;
    enter(g, GS_TRANS);
}

void game_start_new(Game *g) {
    g->deaths = 0;
    g->play_frames = 0;
    /* Partida nueva: Perseo empieza sin ninguna habilidad llave. Las tres
       se ganan en santuarios (F7-09), que es de donde salían en el
       prototipo — hasta la Fase 6 estaban forzadas en main.c para poder
       probar la física. */
    PlayerProgress zero = { { false, false, false }, 0, 0, 0 };
    g->progress = zero;
    g->has_checkpoint = false;
    g->story_shown_mask = 0;
    game_load_level(g, 0);
    enter_level_or_story(g);
}

void game_init(Game *g) {
    g->state = GS_TITLE;
    g->state_t = 0;
    g->frame = 0;
    g->difficulty = DIFF_NORMAL;
    g->muted = false;
    g->level_index = 0;
    g->title_sel = 0;
    g->zone_t = 0;
    g->inv_tab = 0;
    g->inv_sel = 0;
    g->story_pages = 0;
    g->story_scenes = 0;
    g->story_count = g->story_idx = 0;
    g->story_next = GS_PLAY;
    g->story_shown_mask = 0;
    g->banner_title = g->banner_desc = 0;
    g->banner_t = 0;
    g->trans_to = 0;
    g->trans_closing = false;
    g->deaths = 0;
    g->play_frames = 0;
    PlayerProgress zero = { { false, false, false }, 0, 0, 0 };
    g->progress = zero;
    g->has_checkpoint = false;
    g->cp_level = 0;
    g->cp_tx = g->cp_ty = 0;

    /* El título se dibuja sobre un nivel de verdad desplazándose de
       fondo, como en el prototipo; cargarlo acá evita tener un caso
       especial de "todavía no hay mundo" en el resto del código. */
    game_load_level(g, 0);
    audio_play_song(SONG_TITLE);
}

/* ---------------------------------------------------------------------
   Actualización por estado
   --------------------------------------------------------------------- */
static void update_title(Game *g, const GameInput *in) {
    /* Con partida guardada el menú tiene una opción más, CONTINUAR, y
       es la primera: si volviste al juego, es lo que querías hacer. */
    int n = game_has_save() ? 4 : 3;
    if (in->down_pressed)  g->title_sel = (int8_t)((g->title_sel + 1) % n);
    if (in->up_pressed)    g->title_sel = (int8_t)((g->title_sel + n - 1) % n);

    /* Índice del menú -> qué es. Sin guardado, la fila 0 es JUGAR. */
    int item = (n == 4) ? g->title_sel : g->title_sel + 1;

    if (item == 0 && (in->confirm_pressed || in->start_pressed)) {
        if (game_load(g)) { audio_play_sfx(SFX_ABILITY); return; }
        audio_play_sfx(SFX_DENY);
        return;
    }

    int8_t delta = (in->right_pressed ? 1 : 0) - (in->left_pressed ? 1 : 0);
    if (item == 2 && (delta || in->confirm_pressed)) {
        g->muted = !g->muted;
        audio_set_muted(g->muted);
        if (!g->muted) audio_play_sfx(SFX_CHECK);
        return;
    }
    if (item == 3 && delta) {
        int d = (int)g->difficulty + delta;
        if (d < 0) d = DIFF_COUNT - 1;
        if (d >= DIFF_COUNT) d = 0;
        g->difficulty = (Difficulty)d;
        audio_play_sfx(SFX_CHECK);
        return;
    }
    if (item == 1 && (in->confirm_pressed || in->start_pressed)) {
        audio_play_sfx(SFX_ABILITY);
        /* La cinemática de apertura va antes de la partida: es la que
           cuenta por qué Perseo baja a las cloacas. */
        g->story_pages = 0;
        g->story_scenes = story_intro(&g->story_count);
        g->story_idx = 0;
        g->story_next = GS_STATE_COUNT;   /* al acabar: empezar la partida */
        enter(g, GS_STORY);
    }
}

static void update_story(Game *g, const GameInput *in) {
    if (!(in->confirm_pressed || in->start_pressed)) return;
    audio_play_sfx(SFX_CHECK);
    g->story_idx++;
    if (g->story_idx < g->story_count) return;

    /* Se acabaron las páginas. GS_STATE_COUNT es el caso especial "esto
       era la intro": lo que sigue no es una pantalla, es empezar. */
    if (g->story_next == GS_STATE_COUNT) game_start_new(g);
    else if (g->story_next == GS_CREDITS) {
        g->story_idx = 0;
        enter(g, GS_CREDITS);
    } else enter(g, (GameState)g->story_next);
}

/* Diálogo previo a la pelea. El jefe espera quieto (BOSS_WAIT) hasta que
   se acaba, y recién ahí entra en escena — igual que en el prototipo. */
static void update_boss_dialog(Game *g, const GameInput *in) {
    if (!(in->confirm_pressed || in->start_pressed)) return;
    audio_play_sfx(SFX_CHECK);
    if (++g->story_idx < g->story_count) return;

    Entity *b = g->world.boss;
    if (b && b->alive) {
        b->state = BOSS_INTRO;
        b->timer = 0;
    }
    g->world.shake = 5;
    audio_play_sfx(SFX_BOSS);
    enter(g, GS_PLAY);
}

/* Créditos: pasan solos, una pantalla cada tres segundos, y el botón
   adelanta. Al final se vuelve al título. */
#define CREDIT_FRAMES 180

static void update_credits(Game *g, const GameInput *in) {
    uint8_t n;
    story_credits(&n);
    if (in->confirm_pressed || in->start_pressed || g->state_t >= CREDIT_FRAMES) {
        g->state_t = 0;
        if (++g->story_idx >= n) {
            game_init(g);   /* de vuelta al título, partida limpia */
        }
    }
}

/* Recoge el aviso que dejó el mundo este frame (ver WorldEvent en
   core/world.h) y lo convierte en un cambio de pantalla. */
static void consume_world_event(Game *g) {
    WorldEvent ev = g->world.event;
    g->world.event.kind = WEV_NONE;
    if (ev.kind == WEV_NONE) return;

    /* Cualquier cosa que se recoja pasa a formar parte del progreso, o se
       perdería al morir. */
    world_take_progress(&g->world, &g->progress);

    switch (ev.kind) {
        case WEV_BANNER:
            g->banner_title = ev.title;
            g->banner_desc = ev.desc;
            g->banner_t = BANNER_FRAMES;
            enter(g, GS_BANNER);
            break;
        case WEV_CHECKPOINT:
            g->has_checkpoint = true;
            g->cp_level = g->level_index;
            g->cp_tx = ev.tx;
            g->cp_ty = ev.ty;
            game_save(g);   /* guardado automático, como el prototipo */
            break;
        case WEV_DOOR:
            if (game_level((uint8_t)ev.param)) game_begin_transition(g, (uint8_t)ev.param);
            break;
        case WEV_BOSS_INTRO: {
            const BossConfig *cfg = g->world.boss ? boss_config_of(g->world.boss) : 0;
            (void)cfg;
            g->story_pages = 0;
            g->story_scenes = boss_dialogue((BossId)ev.param, &g->story_count);
            g->story_idx = 0;
            enter(g, GS_BOSSDIALOG);
            break;
        }
        case WEV_ENDING:
            audio_play_song(SONG_END);
            g->story_pages = 0;
            g->story_scenes = story_ending(&g->story_count);
            g->story_idx = 0;
            g->story_next = GS_CREDITS;
            enter(g, GS_ENDING);
            break;
        default:
            break;
    }
}

static void update_play(Game *g, const GameInput *in) {
    world_update(&g->world, &in->p);
    camera_update(&g->cam, g->world.player.x, g->world.player.y, g->world.lv);
    g->play_frames++;
    consume_world_event(g);
    if (g->state != GS_PLAY) return;   /* el evento ya cambió de pantalla */

    if (g->world.player.dead) {
        g->deaths++;
        audio_play_song(SONG_NONE);
        enter(g, GS_DEAD);
        return;
    }
    if (in->start_pressed) { audio_play_sfx(SFX_CHECK); enter(g, GS_PAUSE); }
    else if (in->select_pressed) { audio_play_sfx(SFX_CHECK); enter(g, GS_INVENTORY); }
}

/* Reaparición: mismo criterio que respawn() en el prototipo — se vuelve
   con la vida llena y con un margen de invulnerabilidad, y si había un
   jefe a medias se reinicia el encuentro entero. Los puntos de control
   (`lamp`) se conectan en F7-13. */
static void respawn(Game *g) {
    /* Vuelve a la última lámpara encendida, aunque esté en otro nivel;
       si no se encendió ninguna, al principio del nivel actual. */
    uint8_t lv = g->has_checkpoint ? g->cp_level : g->level_index;
    game_load_level(g, lv);
    if (g->has_checkpoint) {
        world_place_player(&g->world, g->cp_tx, g->cp_ty);
        camera_update(&g->cam, g->world.player.x, g->world.player.y, g->world.lv);
    }
    g->world.player.inv = 120;
    enter(g, GS_PLAY);
}

/* Inventario: dos pestañas, como drawInventory() del prototipo. En
   "objetos" el botón A equipa y desequipa, con el tope de dos a la vez
   que es lo que hace que la elección importe. */
#define INV_ABILITY_ROWS 5   /* dos pasivas + tres de santuario */

static void update_inventory(Game *g, const GameInput *in) {
    if (in->select_pressed || in->cancel_pressed) { enter(g, GS_PLAY); return; }

    if (in->left_pressed || in->right_pressed) {
        g->inv_tab = (int8_t)(g->inv_tab ? 0 : 1);
        g->inv_sel = 0;
        audio_play_sfx(SFX_CHECK);
        return;
    }
    int rows = g->inv_tab == 0 ? INV_ABILITY_ROWS : RELIC_COUNT;
    if (in->down_pressed) g->inv_sel = (int8_t)((g->inv_sel + 1) % rows);
    if (in->up_pressed)   g->inv_sel = (int8_t)((g->inv_sel + rows - 1) % rows);

    if (!in->confirm_pressed || g->inv_tab != 1) return;

    /* Equipar / desequipar. */
    Player *p = &g->world.player;
    uint8_t bit = RELIC_BIT(g->inv_sel);
    if (!(p->relics_found & bit)) { audio_play_sfx(SFX_DENY); return; }
    if (p->relics_equipped & bit) {
        p->relics_equipped &= (uint8_t)~bit;
        audio_play_sfx(SFX_CHECK);
    } else if (relic_equipped_count(p->relics_equipped) < RELIC_MAX_EQUIPPED) {
        p->relics_equipped |= bit;
        audio_play_sfx(SFX_ABILITY);
    } else {
        audio_play_sfx(SFX_DENY);   /* ya lleva dos: hay que soltar una */
        return;
    }
    world_take_progress(&g->world, &g->progress);
}

static void update_dead(Game *g, const GameInput *in) {
    if (g->state_t < DEAD_FRAMES) return;
    if (in->confirm_pressed || in->start_pressed) respawn(g);
}

static void update_trans(Game *g) {
    if (g->trans_closing) {
        if (g->state_t < TRANS_FRAMES) return;
        game_load_level(g, g->trans_to);
        g->trans_closing = false;
        g->state_t = 0;
        return;
    }
    if (g->state_t >= TRANS_FRAMES) enter_level_or_story(g);
}

void game_update(Game *g, const GameInput *in) {
    g->frame++;
    g->state_t++;
    if (g->zone_t > 0) g->zone_t--;

    switch (g->state) {
        case GS_TITLE:
            /* El fondo del título es el nivel desplazándose solo. */
            g->cam.x = fx_from_int((int32_t)((g->frame / 4) % 600));
            update_title(g, in);
            break;

        case GS_STORY:
            update_story(g, in);
            break;

        case GS_BOSSDIALOG:
            update_boss_dialog(g, in);
            break;

        case GS_ENDING:
            update_story(g, in);
            break;

        case GS_CREDITS:
            update_credits(g, in);
            break;

        case GS_PLAY:
            update_play(g, in);
            break;

        case GS_BANNER:
            /* El mundo sigue vivo detrás del cartel, igual que en el
               prototipo: el cartel informa, no interrumpe. */
            world_update(&g->world, &in->p);
            camera_update(&g->cam, g->world.player.x, g->world.player.y, g->world.lv);
            g->world.event.kind = WEV_NONE;  /* nada nuevo mientras hay cartel */
            if (g->banner_t > 0) g->banner_t--;
            if (g->banner_t == 0 || in->confirm_pressed) enter(g, GS_PLAY);
            break;

        case GS_PAUSE:
            if (in->start_pressed || in->cancel_pressed) enter(g, GS_PLAY);
            break;

        case GS_INVENTORY:
            update_inventory(g, in);
            break;

        case GS_DEAD:
            update_dead(g, in);
            break;

        case GS_TRANS:
            update_trans(g);
            break;

        default:
            break;
    }
}
