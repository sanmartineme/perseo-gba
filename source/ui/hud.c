/* =====================================================================
   hud.c — el HUD de juego (drawHUD del prototipo, [index.html:2382])
   =====================================================================
   El original dibuja rectángulos sueltos sobre un canvas. Acá la
   interfaz es una rejilla de tiles de 8x8, así que hay que decidir qué
   sobrevive a esa rejilla y qué cambia de forma:

     - Vida: en el prototipo es una barra segmentada con un corazón de
       icono. Acá se mantiene como corazones sueltos de OAM, que es lo
       que ya dibujaba pal_video_draw_world(): con 4-6 puntos de vida se
       leen igual de rápido y no gastan un tile.
     - Chapas, habilidades y nombre de zona: texto, tal cual.
     - Vida del jefe: barra de tiles, que a 16 tiles de ancho tiene mejor
       resolución que los 4-6 segmentos de la vida del jugador.

   Ver docs/TAREAS_MIGRACION_GBA.md, F7-02.
   ===================================================================== */
#include "hud.h"
#include "text.h"
#include "../core/entity.h"
#include "../core/boss/boss_config.h"
#include "../core/boss/boss_fsm.h"

#define BOSSBAR_COL 7
#define BOSSBAR_W  16

static void draw_abilities(const Game *g) {
    /* Etiquetas de tres letras alineadas a la derecha, en el mismo orden
       en que se consiguen. */
    static const char *const LABELS[3] = { "S2", "DSH", "GRR" };
    static const UiColor COLORS[3] = { UI_CYAN, UI_ORANGE, UI_GREEN };
    const bool have[3] = {
        g->world.player.ab.double_jump,
        g->world.player.ab.dash,
        g->world.player.ab.climb,
    };
    int col = UI_COLS;
    for (int i = 2; i >= 0; i--) {
        if (!have[i]) continue;
        int w = ui_text_width(LABELS[i]);
        col -= w + 1;
        ui_text_put(col, 0, COLORS[i], LABELS[i]);
    }
}

static void draw_boss_bar(const Game *g) {
    const Entity *b = g->world.boss;
    if (!g->world.boss_active || !b || !b->alive || b->state == BOSS_WAIT) return;

    const BossConfig *cfg = boss_config_of(b);
    int hp = b->hp < 0 ? 0 : b->hp;
    /* Redondeo hacia arriba: mientras le quede un punto de vida, la barra
       muestra al menos un tile. Que parezca vacía y siga peleando es
       exactamente la clase de mentira que hace desconfiar del HUD. */
    int filled = (hp * BOSSBAR_W + cfg->hp - 1) / cfg->hp;

    ui_text_panel(BOSSBAR_COL - 1, 17, BOSSBAR_W + 2, 3, UI_FILL_DARK, false);
    ui_text_center(17, UI_WHITE, cfg->name);
    ui_text_bar(BOSSBAR_COL, 18, BOSSBAR_W, filled, UI_RED);
}

/* Cartel: cuando Perseo pasa cerca de uno, su texto ocupa la franja de
   abajo. Se dibuja acá y no como una pantalla aparte porque el juego no
   se detiene, igual que en el prototipo. */
static void draw_sign(const Game *g) {
    if (!g->world.sign_text) return;
    /* Cinco filas: borde, tres de texto y borde. Los carteles del nivel 1
       llegan a tres líneas a 24 tiles de ancho. */
    ui_text_panel(1, UI_ROWS - 6, UI_COLS - 2, 5, UI_FILL_DARK, true);
    ui_text_wrapped(g->world.sign_text, 3, UI_ROWS - 5, UI_COLS - 6, UI_WHITE);
}

void ui_hud_draw(const Game *g) {
    /* Chapas. Van en la fila 2 y no en la 1: los corazones son sprites
       de 8 px dibujados en y=4, así que pisan las dos primeras filas de
       tiles. */
    ui_text_put(1, 2, UI_YELLOW, "x");
    ui_text_put(2, 2, UI_YELLOW, ui_itoa(g->world.chapas));

    draw_abilities(g);
    draw_boss_bar(g);
    draw_sign(g);

    /* Rótulo de zona al entrar (zoneT del prototipo). Se muestra sólo
       durante el juego normal, no encima de un cartel o un diálogo. */
    if (g->state == GS_PLAY && g->zone_t > 0 && g->world.lv->name) {
        int w = ui_text_width(g->world.lv->name) + 4;
        if (w > UI_COLS) w = UI_COLS;
        int col = (UI_COLS - w) / 2;
        ui_text_panel(col, 3, w, 3, UI_FILL_DARK, false);
        ui_text_center(4, UI_CYAN, g->world.lv->name);
    }
}
