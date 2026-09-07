/* =====================================================================
   screens.c — el dibujo de cada estado de la partida
   =====================================================================
   Un único switch por estado, gemelo del de game_update(). Todo lo que
   hay acá son cajas y texto sobre la capa de UI (ui/text.h); el mundo lo
   dibuja aparte la PAL de vídeo, y estas pantallas se superponen.

   Ver docs/TAREAS_MIGRACION_GBA.md, Fase 7.
   ===================================================================== */
#include "screens.h"
#include "text.h"
#include "hud.h"
#include "menu_title.h"
#include "../core/boss/boss_config.h"

/* ---------------------------------------------------------------------
   Ajuste de líneas
   ---------------------------------------------------------------------
   Equivalente a wrapText() del prototipo. Corta por palabras, midiendo
   en caracteres visibles (ui_text_width entiende UTF-8), y copia a un
   buffer estático: los textos son de ROM y no se pueden modificar in
   situ, y no hay malloc en todo el proyecto.
   --------------------------------------------------------------------- */
#define WRAP_MAX_LINES 10
#define WRAP_MAX_BYTES 64

static char s_lines[WRAP_MAX_LINES][WRAP_MAX_BYTES];

static int wrap_text(const char *src, int width) {
    if (width > WRAP_MAX_BYTES - 1) width = WRAP_MAX_BYTES - 1;
    int line = 0, len = 0, cols = 0;
    s_lines[0][0] = 0;

    while (*src && line < WRAP_MAX_LINES) {
        /* Toma la próxima palabra completa (bytes) y mide su ancho. */
        const char *word = src;
        while (*src && *src != ' ') src++;
        int bytes = (int)(src - word);
        while (*src == ' ') src++;

        char tmp[WRAP_MAX_BYTES];
        int n = bytes < WRAP_MAX_BYTES - 1 ? bytes : WRAP_MAX_BYTES - 1;
        for (int i = 0; i < n; i++) tmp[i] = word[i];
        tmp[n] = 0;
        int wcols = ui_text_width(tmp);

        int need = cols ? wcols + 1 : wcols;
        if (cols && cols + need > width) {
            /* No entra: cierra la línea y sigue en la de abajo. */
            s_lines[line][len] = 0;
            if (++line >= WRAP_MAX_LINES) break;
            len = 0; cols = 0;
            need = wcols;
        }
        if (cols && len < WRAP_MAX_BYTES - 1) { s_lines[line][len++] = ' '; }
        for (int i = 0; i < n && len < WRAP_MAX_BYTES - 1; i++) s_lines[line][len++] = tmp[i];
        s_lines[line][len] = 0;
        cols += need;
    }
    return line < WRAP_MAX_LINES ? line + 1 : WRAP_MAX_LINES;
}

/* Dibuja un bloque de texto ajustado dentro de una caja y devuelve
   cuántas filas ocupó. */
static int draw_wrapped(const char *text, int col, int row, int width, UiColor c) {
    int n = wrap_text(text, width);
    for (int i = 0; i < n && row + i < UI_ROWS; i++) {
        ui_text_put(col, row + i, c, s_lines[i]);
    }
    return n;
}

/* Parpadeo del "pulsa un botón", igual que (frame>>4)%2 del prototipo. */
static bool blink(const Game *g) { return ((g->frame >> 4) & 1) != 0; }

/* ---------------------------------------------------------------------
   Pantallas
   --------------------------------------------------------------------- */
static void draw_story(const Game *g) {
    ui_text_dim(15);
    ui_text_panel(0, 0, UI_COLS, UI_ROWS, UI_FILL_NONE, true);

    const char *text = 0;
    const char *who = 0;
    if (g->story_scenes && g->story_idx < g->story_count) {
        text = g->story_scenes[g->story_idx].text;
        who = g->story_scenes[g->story_idx].who;
    } else if (g->story_pages && g->story_idx < g->story_count) {
        text = g->story_pages[g->story_idx];
    }
    if (!text) return;

    int row = 4;
    if (who) {
        ui_text_put(3, 2, UI_YELLOW, who);
        row = 5;
    } else if (g->world.lv->name) {
        ui_text_center(2, UI_CYAN, g->world.lv->name);
    }
    draw_wrapped(text, 3, row, UI_COLS - 6, UI_WHITE);

    /* Contador de páginas, como "3 / 7" en el prototipo. */
    char counter[8];
    char *d = counter;
    for (const char *s = ui_itoa(g->story_idx + 1); *s; s++) *d++ = *s;
    *d++ = '/';
    for (const char *s = ui_itoa(g->story_count); *s; s++) *d++ = *s;
    *d = 0;
    ui_text_center(17, UI_GREY, counter);
    if (blink(g)) ui_text_center(18, UI_YELLOW, "A: CONTINUAR");
}

static void draw_banner(const Game *g) {
    if (!g->banner_title) return;
    ui_text_panel(1, 13, UI_COLS - 2, 6, UI_FILL_DARK, true);
    ui_text_center(14, UI_YELLOW, g->banner_title);
    if (g->banner_desc) draw_wrapped(g->banner_desc, 3, 15, UI_COLS - 6, UI_WHITE);
}

static void draw_pause(const Game *g) {
    ui_text_panel(7, 6, 16, 7, UI_FILL_DARK, true);
    ui_text_center(8, UI_YELLOW, "PAUSA");
    ui_text_center(10, UI_WHITE, "CHAPAS");
    ui_text_center(11, UI_YELLOW, ui_itoa(g->world.chapas));
}

static void draw_dead(const Game *g) {
    /* El velo se cierra en medio segundo, como el fundido rojo del
       prototipo: la muerte se siente, no corta de golpe. */
    int t = (int)g->state_t;
    ui_text_dim(t > 30 ? 13 : 1 + (t * 12) / 30);
    ui_text_center(8, UI_RED, "HAS CAIDO");
    ui_text_center(10, UI_GREY, "MUERTES");
    ui_text_center(11, UI_WHITE, ui_itoa(g->deaths));
    if (g->state_t >= 60 && blink(g)) ui_text_center(14, UI_YELLOW, "A: REINTENTAR");
}

/* El inventario completo (pestañas de habilidades y reliquias) es F7-04;
   lo que hay hoy es la lista de lo conseguido, que ya sirve para saber
   qué se tiene equipado. */
static void draw_inventory(const Game *g) {
    const Player *p = &g->world.player;
    ui_text_dim(15);
    ui_text_panel(1, 1, UI_COLS - 2, UI_ROWS - 2, UI_FILL_NONE, true);
    ui_text_center(2, UI_YELLOW, "INVENTARIO");

    ui_text_put(3, 4, UI_CYAN, "HABILIDADES");
    int row = 5;
    ui_text_put(4, row++, UI_WHITE, "GANCHITO MORTAL");
    ui_text_put(4, row++, UI_WHITE, "LANZAMIENTO DE TRAZA");
    if (p->ab.double_jump) ui_text_put(4, row++, UI_WHITE, "SALTO DOBLE");
    if (p->ab.dash)        ui_text_put(4, row++, UI_WHITE, "DASH SOMBRIO");
    if (p->ab.climb)       ui_text_put(4, row++, UI_WHITE, "GARRA FELINA");

    ui_text_put(3, ++row, UI_CYAN, "CHAPAS");
    ui_text_put(4, ++row, UI_YELLOW, ui_itoa(g->world.chapas));

    ui_text_center(UI_ROWS - 3, UI_GREY, "SELECT/B: VOLVER");
}

void ui_screens_draw(const Game *g) {
    ui_text_clear();
    /* El velo se apaga por defecto y lo vuelve a pedir la pantalla que
       lo necesite: así ninguna se olvida de quitarlo al salir. */
    ui_text_dim(0);

    switch (g->state) {
        case GS_TITLE:     ui_title_draw(g);   break;
        case GS_STORY:     draw_story(g);      break;
        case GS_PLAY:      ui_hud_draw(g);     break;
        case GS_BANNER:    ui_hud_draw(g); draw_banner(g); break;
        case GS_PAUSE:     ui_hud_draw(g); draw_pause(g);  break;
        case GS_INVENTORY: draw_inventory(g);  break;
        case GS_DEAD:      draw_dead(g);       break;
        case GS_TRANS:     break;  /* el mosaico lo hace el hardware */
        default: break;
    }
}
