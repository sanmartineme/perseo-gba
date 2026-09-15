/* =====================================================================
   wrap.c — ajuste de líneas de la interfaz
   =====================================================================
   Equivalente a wrapText() del prototipo. Vive aparte porque lo usan
   tanto las pantallas completas (historia, inventario) como el HUD (el
   texto de los carteles), y es código portable: no toca hardware, sólo
   mide con ui_text_width() y dibuja con ui_text_put().
   ===================================================================== */
#include "text.h"

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

int ui_text_wrapped(const char *text, int col, int row, int width, UiColor c) {
    int n = wrap_text(text, width);
    for (int i = 0; i < n && row + i < UI_ROWS; i++) {
        ui_text_put(col, row + i, c, s_lines[i]);
    }
    return n;
}
