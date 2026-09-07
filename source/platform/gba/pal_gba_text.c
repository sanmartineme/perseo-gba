/* =====================================================================
   pal_gba_text.c — implementación GBA de ui/text.h
   =====================================================================
   La UI es una capa de fondo propia (BG0) con una rejilla de 30x20
   tiles. Escribir texto es escribir entradas de screenblock: una por
   carácter. Cuesta una escritura de 16 bits por letra, así que un menú
   entero se redibuja sin acercarse al presupuesto de VBlank.

   DESVIACIÓN respecto de F7-01, que pedía "un envoltorio fino sobre
   TTE": se usa la FUENTE de libtonc (sys8Glyphs) pero no su motor. TTE
   codifica el color y el tile base dentro de un mismo `cattr` de 16
   bits que hay que armar a mano, y su modo de superficie de bits
   (chr4c), que es el que permitiría texto no alineado, costaría ~19 KB
   de VRAM. Descomprimir los glifos y escribir las entradas nosotros son
   40 líneas, no depende de esa semántica y deja el color como un
   parámetro normal. La fuente sigue siendo la de tonc: no hay arte
   nuevo que mantener.

   VRAM (continúa el reparto de pal_gba_video.c):
     - Charblock 2 (CBB 2): tiles de la UI. 0 = transparente,
       1 = relleno oscuro, 2 = borde; los glifos van del 32 en adelante.
     - Screenblock 26 (SBB 26): el tilemap de BG0.
     - Bancos de paleta de fondo 8..15: uno por color de texto, más uno
       para los rellenos de caja. El banco 0 sigue siendo del tileset.
   ===================================================================== */
#include <tonc.h>
#include "../../ui/text.h"
#include "ui_tiles.h"

#define UI_SBB 26
#define DIM_SBB 27   /* BG3: el velo, una capa entera del tile oscuro */

/* Tiles de la UI dentro de CBB 2. */
#define TILE_FONT  32   /* el glifo del carácter 32 (' ') empieza acá, así
                           que el tile de un carácter es su propio código */

/* Un banco de paleta por color: el color del texto es, literalmente, el
   banco que lleva la entrada de screenblock. */
#define PB_FIRST_COLOR 8
#define PB_FILL        15

/* RGB15 de libtonc es una funcion inline, no una constante, y no sirve
   para inicializar una tabla const. El formato es el mismo: 5 bits por
   canal, rojo en los bits bajos. */
#define RGB(r, g, b) ((uint16_t)((r) | ((g) << 5) | ((b) << 10)))

/* Colores del prototipo (docs/prototipo_referencia.html, paleta PAL{}). */
static const uint16_t UI_RGB[UI_COLOR_COUNT] = {
    RGB(31, 31, 31),  /* UI_WHITE  */
    RGB(31, 27,  8),  /* UI_YELLOW */
    RGB(10, 27, 29),  /* UI_CYAN   */
    RGB(28,  8, 10),  /* UI_RED    */
    RGB(17, 18, 21),  /* UI_GREY   */
    RGB(12, 26, 12),  /* UI_GREEN  */
    RGB(31, 17,  6),  /* UI_ORANGE */
};

#define RGB_PANEL  RGB( 1,  2,  4)   /* casi negro, pero no negro puro */
#define RGB_EDGE   RGB( 8, 15, 17)   /* acero azulado */

static bool s_ready = false;

/* ---------------------------------------------------------------------
   Buffer sombra de la capa de interfaz
   ---------------------------------------------------------------------
   La UI se borra y se vuelve a pintar entera cada frame. Mientras se
   escribia directamente en el screenblock, ese repintado era visible: el
   bucle arranca al empezar el VBlank, y el VBlank son 1.309 pasos del
   medidor de la Fase 9 (el 29,8% del frame). Con el HUD suelto todo el
   trabajo cabia ahi, pero una pantalla con panel — inventario, cartel,
   dialogo de jefe — sube el tramo de UI del 2,4% a ~25%, el frame entero
   se va al 45% y el repintado se derrama sobre el VDraw. Entonces el haz
   lee el screenblock a medio escribir y sale lo que se capturo probando:
   el panel ya pintado pero sin su texto, con la pantalla anterior asomando
   por las filas de arriba.

   La solucion es no escribir en VRAM mientras se dibuja: put_tile() pinta
   en este buffer de IWRAM y ui_text_flush() lo vuelca de una sola vez al
   empezar el VBlank siguiente. Son 320 palabras — un pestaneo dentro del
   VBlank — y ademas sale mas barato, porque escribir en VRAM con el
   display activo hace esperar a la CPU y en IWRAM no.

   Cuesta 1.280 bytes y un frame de retraso en la interfaz, que a 60 Hz no
   se percibe. Solo las 20 filas visibles: BG0 no hace scroll, asi que las
   12 de abajo del screenblock no se ven nunca.
   --------------------------------------------------------------------- */
static uint16_t s_shadow[UI_ROWS * 32];

/* ---------------------------------------------------------------------
   Fuente: sys8Glyphs de libtonc son 96 glifos de 8x8 en 1 bpp, dos
   palabras por glifo (4 filas en cada una, un byte por fila). Acá se
   expanden a tiles de 4 bpp con el índice de color 1, que es el que
   cada banco de paleta define como "la tinta".
   --------------------------------------------------------------------- */
static void font_unpack(void) {
    for (int g = 0; g < 96; g++) {
        uint32_t src[2] = { sys8Glyphs[g * 2], sys8Glyphs[g * 2 + 1] };
        TILE *dst = &tile_mem[UI_CBB][TILE_FONT + g];
        for (int r = 0; r < 8; r++) {
            uint32_t bits = (src[r >> 2] >> ((r & 3) * 8)) & 0xFF;
            uint32_t row = 0;
            /* El bit 0 es el píxel de la izquierda. */
            for (int x = 0; x < 8; x++) {
                if (bits & (1u << x)) row |= 1u << (x * 4);
            }
            dst->data[r] = row;
        }
    }
}

static void solid_tile(int index, uint32_t color_index) {
    TILE *t = &tile_mem[UI_CBB][index];
    uint32_t row = color_index * 0x11111111u;
    for (int r = 0; r < 8; r++) t->data[r] = row;
}

void ui_text_init(void) {
    font_unpack();
    solid_tile(TILE_BLANK, 0);
    solid_tile(TILE_DARK, 1);
    solid_tile(TILE_EDGE, 2);

    for (int i = 0; i < UI_COLOR_COUNT; i++) {
        pal_bg_mem[(PB_FIRST_COLOR + i) * 16 + 1] = UI_RGB[i];
    }
    pal_bg_mem[PB_FILL * 16 + 1] = RGB_PANEL;
    pal_bg_mem[PB_FILL * 16 + 2] = RGB_EDGE;

    REG_BG0CNT = BG_CBB(UI_CBB) | BG_SBB(UI_SBB) | BG_4BPP | BG_REG_32x32 | BG_PRIO(0);
    REG_BG0HOFS = 0;
    REG_BG0VOFS = 0;

    /* El velo: BG3 relleno con el tile oscuro. Comparte prioridad 0 con
       BG0, y en los empates gana el BG de número más bajo, así que el
       texto queda por delante del velo sin tener que mezclarlo — que es
       justo lo que se quiere: velo translúcido, letras nítidas.
       El resto del reparto de prioridades está en pal_gba_video.c. */
    for (int i = 0; i < 32 * 32; i++) {
        se_mem[DIM_SBB][i] = SE_ID(TILE_DARK) | SE_PALBANK(PB_FILL);
    }
    REG_BG3CNT = BG_CBB(UI_CBB) | BG_SBB(DIM_SBB) | BG_4BPP | BG_REG_32x32 | BG_PRIO(0);
    REG_BG3HOFS = 0;
    REG_BG3VOFS = 0;
    ui_text_dim(0);
    s_ready = true;
    /* El screenblock entero una vez: las 12 filas de abajo no se ven,
       pero tampoco las repinta nadie, asi que si arrancaran con basura
       se quedarian con ella. De ahi en mas solo se vuelcan las 20 de
       arriba, que son las que cambian. */
    uint32_t blank = SE_ID(TILE_BLANK);
    memset32(se_mem[UI_SBB], blank | (blank << 16), (32 * 32) / 2);
    ui_text_clear();
    ui_text_flush();
    ui_text_visible(true);
}

/* Mezcla alfa del hardware. Los campos de REG_BLDCNT se arman a mano
   porque libtonc los nombra por capa y acá conviene ver la lista de
   abajo completa: todo lo que hay detrás del velo participa. */
#define BLD_1ST_BG3   (1 << 3)
#define BLD_MODE_ALPHA (1 << 6)
#define BLD_2ND_BG1   (1 << 9)
#define BLD_2ND_BG2   (1 << 10)
#define BLD_2ND_OBJ   (1 << 12)
#define BLD_2ND_BD    (1 << 13)

void ui_text_dim(int strength) {
    if (strength <= 0) {
        REG_DISPCNT &= ~DCNT_BG3;
        REG_BLDCNT = 0;
        return;
    }
    if (strength > 16) strength = 16;
    REG_DISPCNT |= DCNT_BG3;
    REG_BLDCNT = BLD_1ST_BG3 | BLD_MODE_ALPHA |
                 BLD_2ND_BG1 | BLD_2ND_BG2 | BLD_2ND_OBJ | BLD_2ND_BD;
    /* eva = peso del velo, evb = peso de lo que hay debajo. */
    REG_BLDALPHA = (uint16_t)(strength | ((16 - strength) << 8));
}

void ui_text_visible(bool on) {
    if (on) REG_DISPCNT |= DCNT_BG0;
    else    REG_DISPCNT &= ~DCNT_BG0;
}

void ui_text_clear(void) {
    if (!s_ready) return;
    /* La capa de UI se rehace entera cada frame, asi que este borrado
       estaba en el camino caliente: 1024 escrituras de 16 bits, y medido
       era el tramo mas caro del frame.
       Dos cambios, los dos por la misma razon (escribir menos veces):
       - Solo las 20 filas visibles. El screenblock tiene 32, pero las 12
         de abajo no se ven nunca (BG0 no hace scroll) y ui_text_init()
         ya las dejo en blanco de una vez. Las filas son contiguas, de
         modo que las 20 primeras son un unico bloque de 640 entradas.
       - De a palabra en vez de a media palabra: 320 escrituras de 32
         bits en lugar de 640 de 16. */
    uint32_t blank = SE_ID(TILE_BLANK);
    memset32(s_shadow, blank | (blank << 16), (UI_ROWS * 32) / 2);
}

void ui_text_flush(void) {
    if (!s_ready) return;
    memcpy32(se_mem[UI_SBB], s_shadow, (UI_ROWS * 32) / 2);
}

static inline void put_tile(int col, int row, uint16_t tile, int palbank) {
    if (col < 0 || col >= UI_COLS || row < 0 || row >= UI_ROWS) return;
    s_shadow[row * 32 + col] = SE_ID(tile) | SE_PALBANK(palbank);
}

/* ---------------------------------------------------------------------
   Texto
   ---------------------------------------------------------------------
   La fuente de tonc llega hasta el ASCII 127, y los textos del juego
   están en español con tildes y eñes (UTF-8, dos bytes). En vez de
   escribir esas cadenas sin acentos en el código fuente — que es donde
   se leen y se corrigen — se decodifican acá y se dibuja la letra sin
   tilde. Se pierde el acento en pantalla; no se pierde en el texto.
   --------------------------------------------------------------------- */
static int decode(const char **pp) {
    const unsigned char *p = (const unsigned char *)*pp;
    unsigned c = *p++;
    if (c < 0x80) { *pp = (const char *)p; return (int)c; }
    unsigned cp;
    if ((c & 0xE0) == 0xC0 && (*p & 0xC0) == 0x80) {
        cp = ((c & 0x1F) << 6) | (*p & 0x3F);
        p += 1;
    } else if ((c & 0xF0) == 0xE0 && (p[0] & 0xC0) == 0x80 && (p[1] & 0xC0) == 0x80) {
        cp = ((c & 0x0F) << 12) | ((p[0] & 0x3F) << 6) | (p[1] & 0x3F);
        p += 2;
    } else {
        *pp = (const char *)p;
        return '?';
    }
    *pp = (const char *)p;
    switch (cp) {
        case 0xE1: case 0xE0: case 0xE4: return 'a';
        case 0xE9: case 0xE8: case 0xEB: return 'e';
        case 0xED: case 0xEC: case 0xEF: return 'i';
        case 0xF3: case 0xF2: case 0xF6: return 'o';
        case 0xFA: case 0xF9: case 0xFC: return 'u';
        case 0xF1: return 'n';
        case 0xC1: case 0xC0: return 'A';
        case 0xC9: case 0xC8: return 'E';
        case 0xCD: case 0xCC: return 'I';
        case 0xD3: case 0xD2: return 'O';
        case 0xDA: case 0xD9: return 'U';
        case 0xD1: return 'N';
        case 0xA1: return '!';   /* ¡ */
        case 0xBF: return '?';   /* ¿ */
        case 0x2014: case 0x2013: return '-';   /* — y – */
        case 0x201C: case 0x201D: return '"';
        case 0x2018: case 0x2019: return '\'';
        case 0x2026: return '.';                /* … */
        default: return cp < 128 ? (int)cp : '?';
    }
}

int ui_text_width(const char *s) {
    int n = 0;
    while (*s) { decode(&s); n++; }
    return n;
}

void ui_text_put(int col, int row, UiColor c, const char *s) {
    if (!s_ready || c >= UI_COLOR_COUNT) return;
    int pb = PB_FIRST_COLOR + (int)c;
    while (*s && col < UI_COLS) {
        int ch = decode(&s);
        if (ch < 32 || ch > 127) ch = '?';
        if (ch != ' ') put_tile(col, row, (uint16_t)(TILE_FONT + ch - 32), pb);
        col++;
    }
}

void ui_text_center(int row, UiColor c, const char *s) {
    /* Si no entra, se ancla a la izquierda y se corta por la derecha:
       "Cámara de Comercio (Mercado Negro)" son 34 tiles en una pantalla
       de 30, y centrarla la recortaba por los DOS lados, que es la
       manera de que no se lea ni el principio. */
    int col = (UI_COLS - ui_text_width(s)) / 2;
    ui_text_put(col < 0 ? 0 : col, row, c, s);
}

void ui_text_panel(int col, int row, int w, int h, UiFill fill, bool border) {
    uint16_t tile = fill == UI_FILL_DARK ? TILE_DARK
                  : fill == UI_FILL_EDGE ? TILE_EDGE : TILE_BLANK;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            put_tile(col + x, row + y, tile, PB_FILL);

    if (!border) return;
    for (int x = 0; x < w; x++) {
        put_tile(col + x, row, TILE_EDGE, PB_FILL);
        put_tile(col + x, row + h - 1, TILE_EDGE, PB_FILL);
    }
    for (int y = 0; y < h; y++) {
        put_tile(col, row + y, TILE_EDGE, PB_FILL);
        put_tile(col + w - 1, row + y, TILE_EDGE, PB_FILL);
    }
}

void ui_text_block(int col, int row, UiColor c) {
    if (c >= UI_COLOR_COUNT) return;
    put_tile(col, row, TILE_DARK, PB_FIRST_COLOR + (int)c);
}

void ui_text_bar(int col, int row, int w, int filled, UiColor c) {
    if (c >= UI_COLOR_COUNT) return;
    if (filled < 0) filled = 0;
    if (filled > w) filled = w;
    for (int x = 0; x < w; x++) {
        /* El tile lleno se pinta con la tinta del color pedido (indice 1
           de su banco); el vacio, con el relleno oscuro del panel. */
        if (x < filled) put_tile(col + x, row, TILE_DARK, PB_FIRST_COLOR + (int)c);
        else            put_tile(col + x, row, TILE_DARK, PB_FILL);
    }
}

const char *ui_itoa(int32_t v) {
    static char buf[12];
    char *p = buf + sizeof(buf) - 1;
    bool neg = v < 0;
    uint32_t u = neg ? (uint32_t)(-v) : (uint32_t)v;
    *p = 0;
    do { *--p = (char)('0' + (u % 10)); u /= 10; } while (u);
    if (neg) *--p = '-';
    return p;
}
