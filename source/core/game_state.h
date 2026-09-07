/* =====================================================================
   game_state.h — la partida entera y en qué pantalla está
   =====================================================================
   Hasta la Fase 6 el juego era un único bucle que siempre jugaba. El
   prototipo, en cambio, tiene una variable `state` ([index.html:1542])
   que decide qué se actualiza y qué se dibuja: título, viñetas de
   historia, juego, cartel de objeto, diálogo de jefe, inventario,
   muerte, transición, final y créditos.

   Acá esa variable se vuelve explícita, y con ella todo lo que el
   prototipo tenía suelto en globales (`lvIndex`, `stats`, `checkpoint`,
   `difficulty`, `banner`, `trans`...). Un solo struct `Game` que se pasa
   por parámetro: el mismo diseño que ya se usó para `World`, y por el
   mismo motivo — nada de estado oculto, y todo esto sigue siendo código
   portable que no sabe qué es un screenblock.

   Ver docs/TAREAS_MIGRACION_GBA.md, Fase 7.
   ===================================================================== */
#ifndef PERSEO_CORE_GAME_STATE_H
#define PERSEO_CORE_GAME_STATE_H

#include "world.h"
#include "camera.h"
#include "dialogue.h"

typedef enum GameState {
    GS_TITLE = 0,   /* menú principal */
    GS_STORY,       /* viñetas: intro del juego o presentación del nivel */
    GS_BOSSDIALOG,  /* las réplicas previas a una pelea de jefe */
    GS_PLAY,
    GS_BANNER,      /* cartel de habilidad/reliquia recién conseguida */
    GS_INVENTORY,
    GS_PAUSE,
    GS_DEAD,
    GS_TRANS,       /* transición mosaico entre niveles */
    GS_ENDING,
    GS_CREDITS,
    GS_STATE_COUNT
} GameState;

typedef enum Difficulty {
    DIFF_EASY = 0, DIFF_NORMAL, DIFF_HARD, DIFF_COUNT
} Difficulty;

/* DIFF_CFG del prototipo ([index.html:1561]). El daño se guarda como
   fracción entera (num/den) para no meter coma flotante: la GBA no tiene
   unidad de punto flotante y esto se evalúa en cada golpe recibido. */
typedef struct DiffCfg {
    int16_t start_hp;
    uint8_t dmg_num, dmg_den;
} DiffCfg;

extern const DiffCfg DIFF_CFGS[DIFF_COUNT];
extern const char *const DIFF_LABELS[DIFF_COUNT];

/* Input ya resuelto por la PAL. Lo del juego va en `p`; el resto son los
   flancos que sólo usan los menús. */
typedef struct GameInput {
    PlayerInput p;
    bool up_pressed, down_pressed, left_pressed, right_pressed;
    bool confirm_pressed;   /* A */
    bool cancel_pressed;    /* B */
    bool start_pressed;
    bool select_pressed;
} GameInput;

#define GAME_LEVEL_COUNT 7

typedef struct Game {
    GameState state;
    uint32_t  state_t;      /* frames dentro del estado actual */
    uint32_t  frame;        /* contador global, para parpadeos y animaciones */

    Difficulty difficulty;
    bool       muted;

    World  world;
    Camera cam;
    uint8_t level_index;

    /* --- menú de título --- */
    int8_t title_sel;       /* 0 jugar, 1 sonido, 2 dificultad */

    /* --- viñetas de historia --- */
    const char *const *story_pages;  /* texto plano: historia de nivel */
    const StoryPage   *story_scenes; /* con interlocutor: intro y final */
    uint8_t story_count, story_idx;
    /* Qué hacer cuando se acaban las páginas. */
    uint8_t story_next;              /* un GameState */
    /* Un bit por nivel: su historia se cuenta la primera vez y no
       vuelve a aparecer al morir y recargar. */
    uint8_t story_shown_mask;

    /* --- cartel de objeto conseguido --- */
    const char *banner_title;
    const char *banner_desc;
    int16_t     banner_t;

    /* --- transición entre niveles --- */
    uint8_t trans_to;       /* nivel destino */
    bool    trans_closing;  /* true mientras cierra, false mientras abre */

    /* Frames que le quedan al rótulo de zona. Va aparte de state_t
       porque cuenta desde que se ENTRA AL NIVEL: si dependiera del
       estado, cada cartel de objeto lo haría reaparecer. */
    int16_t zone_t;

    /* --- inventario --- */
    int8_t inv_tab;   /* 0 habilidades, 1 objetos */
    int8_t inv_sel;

    /* --- progreso que sobrevive a recargar un nivel --- */
    PlayerProgress progress;
    /* Último punto de control: nivel y tile donde reaparecer. `has_cp`
       distingue "sin lámpara encendida" de "lámpara en el tile 0,0". */
    bool    has_checkpoint;
    uint8_t cp_level;
    int16_t cp_tx, cp_ty;

    /* --- estadísticas de la partida --- */
    uint16_t deaths;
    uint32_t play_frames;
} Game;

/* Guardado. La partida no sabe que por debajo hay SRAM: llama a estas
   dos y la PAL resuelve el resto (ver source/core/save.h). */
bool game_save(const Game *g);
/* Carga y deja la partida lista para seguir. false si no hay guardado
   válido, y en ese caso `g` no se toca. */
bool game_load(Game *g);
/* ¿Hay una partida guardada? Lo usa el título para ofrecer CONTINUAR. */
bool game_has_save(void);

void game_init(Game *g);
void game_update(Game *g, const GameInput *in);

/* Daño ya escalado por dificultad. Lo llama world_damage_player(). */
int  game_scale_damage(const Game *g, int amount);

/* El nivel que toca, o 0 si `i` se sale de los que existen. */
const Level *game_level(uint8_t i);

/* Arranca una partida nueva en el nivel 0 con la dificultad elegida. */
void game_start_new(Game *g);
/* Carga un nivel y coloca a Perseo en su punto de aparición. */
void game_load_level(Game *g, uint8_t index);
/* Pide la transición mosaico hacia otro nivel. */
void game_begin_transition(Game *g, uint8_t to_level);

#endif /* PERSEO_CORE_GAME_STATE_H */
