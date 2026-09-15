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
    /* Etiquetas de habilidades alineadas a la derecha, más descriptivas y coloridas.
       MEJORADO (Sept 2026): Nombres completos, colores vibrantes, ícono de adquisición.
       Muestra "*" junto a la habilidad que se acaba de obtener (efecto parpadeante). */
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
        /* Efecto visual: habilidades recién adquiridas parpadean (primeros 60 frames) */
        if (g->world.tick < 60) {
            int blink = (g->world.tick >> 2) & 1;
            if (blink) {
                ui_text_put(col - 1, 0, COLORS[i], "*");  /* Indicador de "nuevo" */
            }
        }
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
       exactamente la clase de mentira que hace desconfiar del HUD.

       MEJORADO (Sept 2026): Colores dinámicos, animación de daño, borde destacado. */
    int filled = (hp * BOSSBAR_W + cfg->hp - 1) / cfg->hp;

    /* Borde destacado con color según fase de combate */
    UiColor border_color = UI_RED;
    if (hp < cfg->hp / 4) {
        /* Fase final: borde rojo intenso, parpadeo de peligro */
        border_color = ((g->world.tick >> 2) & 1) ? UI_RED : UI_ORANGE;
    } else if (hp < cfg->hp / 2) {
        border_color = UI_ORANGE;
    }

    ui_text_panel(BOSSBAR_COL - 1, 17, BOSSBAR_W + 2, 3, UI_FILL_DARK, false);
    ui_text_center(17, border_color, cfg->name);  /* Nombre con color dinámico */

    /* Barra de vida con animación suave si acaba de recibir daño */
    UiColor bar_color = UI_RED;
    if (b->flash > 0 && ((g->world.tick >> 1) & 1)) {
        bar_color = UI_WHITE;  /* Destello de daño */
    }
    ui_text_bar(BOSSBAR_COL, 18, BOSSBAR_W, filled, bar_color);
}

/* Cartel: cuando Perseo pasa cerca de uno, su texto ocupa la franja de
   abajo. MEJORADO (Sept 2026): animación de aparición suave, borde más elegante. */
static void draw_sign(const Game *g) {
    if (!g->world.sign_text) return;
    /* Cinco filas: borde superior, tres de texto, borde inferior.
       Ahora con efecto de aparición gradual (parpadeo suave).
       Los carteles del nivel 1 llegan a tres líneas a 24 tiles de ancho. */
    int alpha = ((g->world.tick >> 1) & 1);  /* Parpadeo suave cada 2 frames */
    if (alpha == 0) return;             /* Efecto fade in/out elegante */

    ui_text_panel(1, UI_ROWS - 6, UI_COLS - 2, 5, UI_FILL_DARK, true);
    ui_text_wrapped(g->world.sign_text, 3, UI_ROWS - 5, UI_COLS - 6, UI_WHITE);
}

/* La barra de vida: un segmento por punto, al lado del corazón que
   dibuja la capa de sprites. MEJORADO (Sept 2026): Animación de daño,
   colores dinámicos según estado de salud, indicador visual de crítico. */
static void draw_health(const Game *g) {
    const Player *p = &g->world.player;
    int max = p->max_hp;
    if (max < 1) max = 1;
    if (max > 12) max = 12;          /* lo que cabe sin pisar las etiquetas */
    int hp = p->hp < 0 ? 0 : (p->hp > max ? max : p->hp);

    /* Color dinámico: rojo normal → naranja en daño → rojo crítico (<25% vida) */
    UiColor color = UI_RED;
    if (hp > max / 4) {
        color = UI_RED;
    } else if (hp > max / 8) {
        /* Salud baja: parpadeo de alerta (naranja rápido) */
        color = ((g->world.tick >> 1) & 1) ? UI_ORANGE : UI_RED;
    } else {
        /* Crítico: parpadeo de emergencia (rojo muy rápido) */
        color = ((g->world.tick) & 1) ? UI_RED : UI_WHITE;
    }

    /* Barra mejorada con marco visible */
    ui_text_bar(2, 0, max, hp, color);

    /* Indicador de "CUIDADO" si está en crítico */
    if (hp <= max / 8 && ((g->world.tick >> 3) & 1)) {
        ui_text_put(0, 0, UI_RED, "!");
    }
}

void ui_hud_draw(const Game *g) {
    draw_health(g);

    /* Chapas. Van en la fila 2 y no en la 1: el corazón es un sprite de
       8 px dibujado en y=4, así que pisa las dos primeras filas. */
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
