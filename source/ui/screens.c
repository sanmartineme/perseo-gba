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
#include "../core/relics.h"
#include "../core/dialogue.h"

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
    } else if (g->story_pages && g->world.lv->name) {
        /* Sólo la historia de nivel se encabeza con el nombre de la zona;
           la intro y el final no ocurren en ningún nivel. */
        ui_text_center(2, UI_CYAN, g->world.lv->name);
    }
    ui_text_wrapped(text, 3, row, UI_COLS - 6, UI_WHITE);

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

/* Diálogo de jefe: globo abajo con el nombre de quien habla arriba, que
   es la forma de cómic que usaba el prototipo. El mundo se ve entero
   detrás — la arena ya está sellada y el jefe esperando, y eso es parte
   de lo que el diálogo tiene que transmitir. */
static void draw_boss_dialog(const Game *g) {
    if (!g->story_scenes || g->story_idx >= g->story_count) return;
    const StoryPage *pg = &g->story_scenes[g->story_idx];

    ui_text_panel(1, UI_ROWS - 7, UI_COLS - 2, 6, UI_FILL_DARK, true);
    if (pg->who) ui_text_put(3, UI_ROWS - 7, UI_YELLOW, pg->who);
    ui_text_wrapped(pg->text, 3, UI_ROWS - 6, UI_COLS - 6, UI_WHITE);
    if (blink(g)) ui_text_put(UI_COLS - 4, UI_ROWS - 2, UI_YELLOW, "A>");
}

/* Créditos: una pantalla por bloque, centrada. */
static void draw_credits(const Game *g) {
    uint8_t n;
    const CreditPage *pages = story_credits(&n);
    ui_text_dim(16);
    if (g->story_idx >= n) return;
    const CreditPage *p = &pages[g->story_idx];
    if (p->title && p->title[0]) ui_text_center(7, UI_YELLOW, p->title);
    if (p->line1) ui_text_center(10, UI_CYAN, p->line1);
    if (p->line2) ui_text_center(12, UI_WHITE, p->line2);
}

static void draw_banner(const Game *g) {
    if (!g->banner_title) return;
    ui_text_panel(1, 13, UI_COLS - 2, 6, UI_FILL_DARK, true);
    ui_text_center(14, UI_YELLOW, g->banner_title);
    if (g->banner_desc) ui_text_wrapped(g->banner_desc, 3, 15, UI_COLS - 6, UI_WHITE);
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

/* Inventario, con las dos pestañas del prototipo (drawInventory,
   [index.html:2677]). Las dos primeras habilidades son PASIVAS: Perseo
   nace con ellas. Las tres siguientes salen de santuarios, y se listan
   aunque no se tengan — saber que existen es parte de querer buscarlas.
   La descripción de la seleccionada va abajo, como en el original. */
static const char *const AB_ROWS[5] = {
    "GANCHITO MORTAL", "LANZAMIENTO DE TRAZA",
    "SALTO DOBLE", "DASH SOMBRIO", "GARRA FELINA"
};
static const char *const AB_DESCS[5] = {
    "Zarpazo veloz con la garra delantera (B). Su golpe base.",
    "Arroja su propia traza a distancia (L). Revienta al impactar.",
    "Voltereta felina: pulsa salto en el aire para un segundo impulso.",
    "Embiste veloz; rompe rejillas oxidadas y esquiva peligros.",
    "Agarrate a los muros y salta entre ellos para escalar.",
};
#define AB_NOT_FOUND "Aun no encontrada. Buscala en un santuario de Silencio."

static bool has_ability_row(const Player *p, int i) {
    switch (i) {
        case 0: case 1: return true;              /* pasivas */
        case 2: return p->ab.double_jump;
        case 3: return p->ab.dash;
        default: return p->ab.climb;
    }
}

static void draw_inventory(const Game *g) {
    const Player *p = &g->world.player;
    ui_text_dim(15);
    ui_text_panel(1, 1, UI_COLS - 2, UI_ROWS - 2, UI_FILL_NONE, true);
    ui_text_center(2, UI_YELLOW, "INVENTARIO DE PERSEO");

    /* Pestañas. La activa va en amarillo con corchetes; sin barra de
       fondo, que los tiles de letra son opacos y la dejarían a huecos. */
    ui_text_put(3, 4, g->inv_tab == 0 ? UI_YELLOW : UI_GREY,
                g->inv_tab == 0 ? "[HABILIDADES]" : " HABILIDADES ");
    ui_text_put(17, 4, g->inv_tab == 1 ? UI_YELLOW : UI_GREY,
                g->inv_tab == 1 ? "[OBJETOS]" : " OBJETOS ");

    const char *desc = 0;
    if (g->inv_tab == 0) {
        for (int i = 0; i < 5; i++) {
            bool has = has_ability_row(p, i);
            bool sel = (i == g->inv_sel);
            if (sel) ui_text_put(3, 6 + i, UI_YELLOW, ">");
            ui_text_put(5, 6 + i, has ? (sel ? UI_WHITE : UI_CYAN) : UI_GREY, AB_ROWS[i]);
            if (i < 2) ui_text_put(UI_COLS - 9, 6 + i, UI_YELLOW, "PASIVA");
            else       ui_text_put(UI_COLS - 9, 6 + i, has ? UI_GREEN : UI_GREY,
                                   has ? "SI" : "NO");
        }
        desc = has_ability_row(p, g->inv_sel) ? AB_DESCS[g->inv_sel] : AB_NOT_FOUND;
    } else {
        for (int i = 0; i < RELIC_COUNT; i++) {
            bool found = (p->relics_found & RELIC_BIT(i)) != 0;
            bool eq = (p->relics_equipped & RELIC_BIT(i)) != 0;
            bool sel = (i == g->inv_sel);
            if (sel) ui_text_put(3, 6 + i, UI_YELLOW, ">");
            ui_text_put(5, 6 + i, found ? (sel ? UI_WHITE : UI_CYAN) : UI_GREY,
                        found ? RELICS[i].name : "- - - - -");
            /* "EQ" y no "PUESTA": los nombres largos ("PATA DE LA
               SUERTE") llegan hasta la columna 22 y se pisaban. */
            if (eq) ui_text_put(UI_COLS - 3, 6 + i, UI_GREEN, "EQ");
        }
        desc = (p->relics_found & RELIC_BIT(g->inv_sel)) ? RELICS[g->inv_sel].desc
                                                        : "Todavia no la has encontrado.";
        ui_text_put(3, 10, UI_GREY, "A: EQUIPAR (MAX 2)   EQ=PUESTA");
    }

    if (desc) ui_text_wrapped(desc, 3, 12, UI_COLS - 6, UI_WHITE);

    ui_text_put(3, UI_ROWS - 3, UI_GREY, "CHAPAS");
    ui_text_put(10, UI_ROWS - 3, UI_YELLOW, ui_itoa(g->world.chapas));
    ui_text_put(UI_COLS - 16, UI_ROWS - 3, UI_GREY, "IZQ/DER PESTANA");
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
        case GS_BOSSDIALOG: ui_hud_draw(g); draw_boss_dialog(g); break;
        case GS_ENDING:    draw_story(g);      break;
        case GS_CREDITS:   draw_credits(g);    break;
        case GS_TRANS:     break;  /* el mosaico lo hace el hardware */
        default: break;
    }
}
