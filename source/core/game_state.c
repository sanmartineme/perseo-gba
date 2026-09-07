#include "game_state.h"
#include "audio.h"
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
    g->world.dmg_num = DIFF_CFGS[g->difficulty].dmg_num;
    g->world.dmg_den = DIFF_CFGS[g->difficulty].dmg_den;
    g->world.player.max_hp = DIFF_CFGS[g->difficulty].start_hp;
    g->world.player.hp = g->world.player.max_hp;
    /* camera_update() es instantánea (no interpola), así que llamarla
       una vez ya deja la cámara encuadrada sin barrido inicial. */
    camera_init(&g->cam);
    camera_update(&g->cam, g->world.player.x, g->world.player.y, g->world.lv);
    audio_play_song((SongId)lv->song);
}

/* Presenta el nivel con sus páginas de historia la primera vez que se
   entra; las siguientes veces (morir y volver) entra directo al juego,
   igual que `storyShown` en el prototipo. */
static void enter_level_or_story(Game *g) {
    const Level *lv = g->world.lv;
    static uint8_t shown_mask = 0;   /* un bit por nivel */
    uint8_t bit = (uint8_t)(1u << (g->level_index & 7));
    if (lv->story_count && !(shown_mask & bit)) {
        shown_mask |= bit;
        g->story_pages = lv->story;
        g->story_scenes = 0;
        g->story_count = lv->story_count;
        g->story_idx = 0;
        g->story_is_intro = false;
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
    g->story_pages = 0;
    g->story_scenes = 0;
    g->story_count = g->story_idx = 0;
    g->story_is_intro = false;
    g->banner_title = g->banner_desc = 0;
    g->banner_t = 0;
    g->trans_to = 0;
    g->trans_closing = false;
    g->deaths = 0;
    g->play_frames = 0;

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
    if (in->down_pressed)  g->title_sel = (int8_t)((g->title_sel + 1) % 3);
    if (in->up_pressed)    g->title_sel = (int8_t)((g->title_sel + 2) % 3);

    int8_t delta = (in->right_pressed ? 1 : 0) - (in->left_pressed ? 1 : 0);
    if (g->title_sel == 1 && (delta || in->confirm_pressed)) {
        g->muted = !g->muted;
        audio_set_muted(g->muted);
        if (!g->muted) audio_play_sfx(SFX_CHECK);
        return;
    }
    if (g->title_sel == 2 && delta) {
        int d = (int)g->difficulty + delta;
        if (d < 0) d = DIFF_COUNT - 1;
        if (d >= DIFF_COUNT) d = 0;
        g->difficulty = (Difficulty)d;
        audio_play_sfx(SFX_CHECK);
        return;
    }
    if (g->title_sel == 0 && (in->confirm_pressed || in->start_pressed)) {
        audio_play_sfx(SFX_ABILITY);
        game_start_new(g);
    }
}

static void update_story(Game *g, const GameInput *in) {
    if (!(in->confirm_pressed || in->start_pressed)) return;
    audio_play_sfx(SFX_CHECK);
    g->story_idx++;
    if (g->story_idx < g->story_count) return;
    /* Se acabaron las páginas. */
    if (g->story_is_intro) game_start_new(g);
    else enter(g, GS_PLAY);
}

static void update_play(Game *g, const GameInput *in) {
    world_update(&g->world, &in->p);
    camera_update(&g->cam, g->world.player.x, g->world.player.y, g->world.lv);
    g->play_frames++;

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
    game_load_level(g, g->level_index);
    g->world.player.inv = 120;
    enter(g, GS_PLAY);
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

    switch (g->state) {
        case GS_TITLE:
            /* El fondo del título es el nivel desplazándose solo. */
            g->cam.x = fx_from_int((int32_t)((g->frame / 4) % 600));
            update_title(g, in);
            break;

        case GS_STORY:
            update_story(g, in);
            break;

        case GS_PLAY:
            update_play(g, in);
            break;

        case GS_BANNER:
            /* El mundo sigue vivo detrás del cartel, igual que en el
               prototipo: el cartel informa, no interrumpe. */
            world_update(&g->world, &in->p);
            camera_update(&g->cam, g->world.player.x, g->world.player.y, g->world.lv);
            if (g->banner_t > 0) g->banner_t--;
            if (g->banner_t == 0 || in->confirm_pressed) enter(g, GS_PLAY);
            break;

        case GS_PAUSE:
            if (in->start_pressed || in->cancel_pressed) enter(g, GS_PLAY);
            break;

        case GS_INVENTORY:
            if (in->select_pressed || in->cancel_pressed) enter(g, GS_PLAY);
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
