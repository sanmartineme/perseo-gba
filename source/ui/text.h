/* =====================================================================
   text.h — capa de texto y paneles de la interfaz
   =====================================================================
   Todo lo que la UI necesita dibujar en el prototipo (menú de título,
   inventario, diálogos, viñetas de historia, HUD) es texto sobre cajas
   de color. En un canvas eso es trivial; en Modo 0 no hay dónde pintar
   un rectángulo arbitrario, así que la interfaz vive en su propia capa
   de fondo (BG0) como una rejilla de 30x20 tiles.

   Esa rejilla es la restricción real de este módulo: **el texto se
   alinea a 8 px**. No es una limitación técnica que convenga esquivar
   con una superficie de bits (costaría 19 KB de VRAM y redibujar por
   pixel); es el mismo compromiso que tomaban los juegos de la época, y
   la fuente de 8x8 se lee perfectamente a 240x160.

   Este header es la frontera, igual que core/audio.h: describe QUÉ se
   escribe, y source/platform/gba/pal_gba_text.c resuelve CÓMO (tiles,
   screenblocks y bancos de paleta). Así source/ui/ queda portable.
   ===================================================================== */
#ifndef PERSEO_UI_TEXT_H
#define PERSEO_UI_TEXT_H

#include <stdint.h>
#include <stdbool.h>

/* La pantalla de GBA en tiles de 8x8. */
#define UI_COLS 30
#define UI_ROWS 20

/* Los colores del prototipo que la UI usa de verdad (las letras de su
   paleta: 'W', 'Y', 'C', 'R', 'G', 'M', 'B'). Cada uno es un banco de
   paleta de fondo distinto, así que cambiar de color no cuesta nada. */
typedef enum UiColor {
    UI_WHITE = 0,
    UI_YELLOW,
    UI_CYAN,
    UI_RED,
    UI_GREY,
    UI_GREEN,
    UI_ORANGE,
    UI_COLOR_COUNT
} UiColor;

/* Relleno de las cajas. UI_FILL_NONE borra (deja ver el juego detrás). */
typedef enum UiFill {
    UI_FILL_NONE = 0,
    UI_FILL_DARK,    /* panel casi negro, el fondo de los menús */
    UI_FILL_EDGE     /* borde acerado, para marcar el contorno */
} UiFill;

void ui_text_init(void);
void ui_text_visible(bool on);

/* Velo oscuro semitransparente sobre el juego, detrás del texto: es el
   `rgba(8,12,20,0.55)` que el prototipo pinta bajo el título, los
   diálogos y las viñetas. `strength` va de 0 (apagado) a 16 (opaco).
   Se usa en vez de una caja opaca porque el fondo — el túnel
   desplazándose — es parte de la escena, no un estorbo. */
void ui_text_dim(int strength);

/* Borra la capa entera (deja ver el juego). */
void ui_text_clear(void);
/* Borra sólo un rectángulo, en tiles. */
void ui_text_clear_rect(int col, int row, int w, int h);

/* Escribe una cadena. Corta en el borde derecho de la pantalla.
   La fuente es ASCII: las vocales acentuadas y la eñe se escriben sin
   tilde en vez de salir como basura (ver la nota en pal_gba_text.c). */
void ui_text_put(int col, int row, UiColor c, const char *s);
void ui_text_center(int row, UiColor c, const char *s);
/* Largo en tiles que ocupará la cadena (cuenta UTF-8 correctamente). */
int  ui_text_width(const char *s);

/* Caja rellena; `border` dibuja el contorno con UI_FILL_EDGE. */
void ui_text_panel(int col, int row, int w, int h, UiFill fill, bool border);

/* Un tile macizo del color pedido. Es la pieza con la que el menú de
   título arma su logotipo: letras de bloques, que es lo más parecido al
   título de 36 px del prototipo que cabe en una rejilla de 8x8. */
void ui_text_block(int col, int row, UiColor c);

/* Barra de progreso horizontal de una fila de alto (vida de jefe, etc.):
   `filled` de `w` tiles se pintan con `c`, el resto queda oscuro. */
void ui_text_bar(int col, int row, int w, int filled, UiColor c);

/* Entero a cadena, sin traer <stdio.h> a la ROM por un printf. */
const char *ui_itoa(int32_t v);

#endif /* PERSEO_UI_TEXT_H */
