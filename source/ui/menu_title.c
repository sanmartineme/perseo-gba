/* =====================================================================
   menu_title.c — pantalla de título (drawTitle, [index.html:2481])
   =====================================================================
   El prototipo escribe "PERSEO" con una fuente de 36 px y sombra dura.
   Acá la interfaz es una rejilla de tiles de 8x8: no hay fuente grande
   que valga. En vez de conformarse con el título en letra chica — que
   dejaría la pantalla más importante del juego como la más pobre — el
   logotipo se dibuja con bloques macizos, tres tiles de ancho por cinco
   de alto por letra. Es la misma solución que usaban los juegos de la
   época, y a 240x160 se lee enorme.

   El fondo es el nivel 1 desplazándose solo (lo mueve game_update), como
   en el original: el título se siente parte del mundo y no una pantalla
   pegada encima.
   ===================================================================== */
#include "menu_title.h"
#include "../platform/pal.h"   /* el nombre del boton asignado a cada accion */
#include "text.h"

/* Letras de 3x5 en bits, una fila por nibble bajo. Sólo hacen falta las
   cinco que forman PERSEO. */
typedef struct BlockGlyph { char ch; uint8_t rows[5]; } BlockGlyph;

static const BlockGlyph GLYPHS[] = {
    { 'P', { 0x7, 0x5, 0x7, 0x4, 0x4 } },
    { 'E', { 0x7, 0x4, 0x7, 0x4, 0x7 } },
    { 'R', { 0x7, 0x5, 0x7, 0x6, 0x5 } },
    { 'S', { 0x7, 0x4, 0x7, 0x1, 0x7 } },
    { 'O', { 0x7, 0x5, 0x5, 0x5, 0x7 } },
};

static const BlockGlyph *glyph_of(char c) {
    for (unsigned i = 0; i < sizeof(GLYPHS) / sizeof(GLYPHS[0]); i++) {
        if (GLYPHS[i].ch == c) return &GLYPHS[i];
    }
    return 0;
}

/* Dibuja una palabra en bloques. `shadow` la repite un tile abajo y a la
   derecha en gris antes de la tinta: es la sombra dura del original, que
   es lo que despega el logotipo del fondo del túnel. */
static void draw_block_word(const char *word, int col, int row, UiColor c) {
    for (int pass = 0; pass < 2; pass++) {
        int x = col + (pass == 0 ? 1 : 0);
        int y = row + (pass == 0 ? 1 : 0);
        UiColor ink = pass == 0 ? UI_GREY : c;
        for (const char *p = word; *p; p++, x += 4) {
            const BlockGlyph *g = glyph_of(*p);
            if (!g) continue;
            for (int ry = 0; ry < 5; ry++)
                for (int rx = 0; rx < 3; rx++)
                    if (g->rows[ry] & (1 << (2 - rx)))
                        ui_text_block(x + rx, y + ry, ink);
        }
    }
}

void ui_title_draw(const Game *g) {
    /* Velo translúcido: el túnel se sigue viendo moverse detrás. */
    ui_text_dim(11);

    /* "PERSEO": 6 letras de 3 tiles + 1 de separación = 23 tiles. */
    draw_block_word("PERSEO", (UI_COLS - 23) / 2, 2, UI_YELLOW);
    ui_text_center(9, UI_WHITE, "SOMBRAS DE SILENCIO");

    /* Cuatro opciones como mucho: CONTINUAR sólo aparece si hay partida
       guardada, y entonces es la primera. */
    const char *items[5];
    char sound[16];
    char diff[24];
    int n = 0;
    if (game_has_save()) items[n++] = "CONTINUAR";
    items[n++] = "PARTIDA NUEVA";

    /* Sin snprintf: concatenar dos cadenas cortas a mano cuesta menos que
       arrastrar <stdio.h> a la ROM. */
    {
        const char *v = g->muted ? "OFF" : "ON";
        char *d = sound;
        for (const char *s = "SONIDO: "; *s; s++) *d++ = *s;
        for (const char *s = v; *s; s++) *d++ = *s;
        *d = 0;
        items[n++] = sound;
    }
    {
        const char *v = DIFF_LABELS[g->difficulty];
        char *d = diff;
        for (const char *s = "DIFICULTAD: "; *s; s++) *d++ = *s;
        for (const char *s = v; *s; s++) *d++ = *s;
        *d = 0;
        items[n++] = diff;
    }
    items[n++] = "OPCIONES";

    /* Cinco filas no caben de dos en dos: con CONTINUAR el menú llega a
       la fila 20, que ya no existe. A partir de cinco se aprietan a una
       fila de separación, que sigue lejísimos de estar apretado. */
    int step = (n >= 5) ? 1 : 2;
    int top = (n >= 5) ? 11 : 12;
    for (int i = 0; i < n; i++) {
        int row = top + i * step;
        bool sel = (i == g->title_sel);
        int w = ui_text_width(items[i]);
        int col = (UI_COLS - w) / 2;
        ui_text_put(col, row, sel ? UI_YELLOW : UI_WHITE, items[i]);
        /* Las flechas parpadean alrededor de la opción elegida, como el
           "▶ ... ◀" del prototipo. No se resalta con una barra de fondo:
           los tiles de letra son opacos y dejarían la barra a huecos. */
        if (sel && ((g->frame >> 4) & 1)) {
            ui_text_put(col - 2, row, UI_YELLOW, ">");
            ui_text_put(col + w + 1, row, UI_YELLOW, "<");
        }
    }

    /* Las pistas caben justas en 30 tiles: partirlas en dos líneas
       cortas evita que ui_text_center las recorte por los dos lados. */
    /* Con cuatro opciones la última cae en la fila 18, así que la ayuda
       se reduce a una sola línea. */
    ui_text_center(n >= 4 ? 19 : 18, UI_GREY, "ARRIBA/ABAJO  IZQ/DER  A: OK");
}

/* ---------------------------------------------------------------------
   OPCIONES
   ---------------------------------------------------------------------
   Una sola pantalla en vez de submenús: son siete filas y caben todas,
   y así se ve de un vistazo cómo está configurado todo. Las cuatro
   primeras reasignan botones; las dos siguientes son interruptores.
   --------------------------------------------------------------------- */
static const char *const OPT_LABELS[OPT_ROW_COUNT] = {
    "SALTAR", "GANCHITO", "DASH", "TRAZA", "PODERES", "CLAVES", "VOLVER"
};

void ui_options_draw(const Game *g) {
    ui_text_dim(14);
    ui_text_panel(1, 1, UI_COLS - 2, UI_ROWS - 2, UI_FILL_NONE, true);
    ui_text_center(2, UI_YELLOW, "OPCIONES");

    for (int i = 0; i < OPT_ROW_COUNT; i++) {
        int row = 5 + i;
        bool sel = (i == g->opt_sel);
        UiColor c = sel ? UI_YELLOW : UI_WHITE;
        ui_text_put(4, row, c, OPT_LABELS[i]);

        const char *value = 0;
        if (i <= OPT_THROW) {
            value = pal_input_button_name(pal_input_binding((InputAction)i));
        } else if (i == OPT_POWERS) {
            value = g->opt_all_powers ? "TODOS" : "NORMAL";
        } else if (i == OPT_CHEATS) {
            value = g->opt_cheats ? "ON" : "OFF";
        }
        if (value) ui_text_put(18, row, sel ? UI_YELLOW : UI_CYAN, value);

        if (sel && ((g->frame >> 4) & 1)) ui_text_put(2, row, UI_YELLOW, ">");
    }

    /* Las claves sólo se explican cuando están encendidas: si no, es
       ruido — y contarlas sin querer le arruina el descubrimiento a
       quien no las buscaba. */
    if (g->opt_cheats) {
        ui_text_put(2, 13, UI_GREY, "JUGANDO:");
        ui_text_put(2, 14, UI_GREEN, "A DER IZQ A");
        ui_text_put(15, 14, UI_GREY, "INVENCIBLE");
        ui_text_put(2, 15, UI_GREEN, "IZQ IZQ IZQ B");
        ui_text_put(16, 15, UI_GREY, "SUPER+VOLAR");
        ui_text_put(2, 16, UI_GREEN, "ARR ABA ARR ABA");
        ui_text_put(18, 16, UI_GREY, "PAREDES");
    }

    ui_text_center(18, UI_GREY, "IZQ/DER CAMBIA   B: VOLVER");
}
