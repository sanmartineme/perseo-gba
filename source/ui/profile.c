/* =====================================================================
   profile.c — el medidor de frame en pantalla
   =====================================================================
   Muestra, en porcentaje del frame, lo que cuesta cada tramo del bucle:
   actualizar el mundo, sincronizar el tilemap, volcar OAM y dibujar la
   interfaz. Dos columnas: lo del frame actual y el PEOR valor visto
   desde que se abrió el medidor — que es el número que de verdad
   importa, porque un promedio bonito con un pico que se pasa del frame
   se ve igual de mal en pantalla.

   El medidor se dibuja al final del frame, así que su propio coste no
   entra en lo que mide del tramo de interfaz (queda contado, pero sólo
   mientras se está mirando). Es la única distorsión y es conocida.
   ===================================================================== */
#include "profile.h"
#include "text.h"
#include "../platform/pal.h"

static const char *const SLOT_NAMES[PAL_PROFILE_COUNT] = {
    "MUNDO", "TILES", "OAM", "UI"
};

/* Porcentaje con un decimal, sin printf ni coma flotante. */
static void put_pct(int col, int row, UiColor c, uint16_t ticks) {
    /* 4389 pasos = un frame entero. Se calcula en decimas para no perder
       la resolucion en los tramos baratos, que son casi todos. */
    uint32_t tenths = ((uint32_t)ticks * 1000u) / 4389u;
    char buf[8];
    char *d = buf;
    const char *n = ui_itoa((int32_t)(tenths / 10));
    while (*n) *d++ = *n++;
    *d++ = '.';
    *d++ = (char)('0' + (tenths % 10));
    *d = 0;
    ui_text_put(col, row, c, buf);
}

void ui_profile_draw(void) {
    ui_text_panel(1, 1, 24, 10, UI_FILL_DARK, true);
    ui_text_put(3, 2, UI_YELLOW, "COSTE DE FRAME (%)");
    ui_text_put(13, 3, UI_GREY, "AHORA");
    ui_text_put(19, 3, UI_GREY, "PEOR");

    for (int i = 0; i < PAL_PROFILE_COUNT; i++) {
        ui_text_put(3, 4 + i, UI_WHITE, SLOT_NAMES[i]);
        put_pct(13, 4 + i, UI_CYAN, pal_profile_ticks((PalProfileSlot)i, false));
        put_pct(19, 4 + i, UI_ORANGE, pal_profile_ticks((PalProfileSlot)i, true));
    }

    uint16_t worst = pal_profile_total(true);
    ui_text_put(3, 8, UI_WHITE, "TOTAL");
    put_pct(13, 8, UI_CYAN, pal_profile_total(false));
    /* Rojo en cuanto el peor caso pasa del 100%: ahí es donde el juego
       deja de ir a 60 y empieza a saltar frames. */
    put_pct(19, 8, pal_profile_percent(worst) >= 100 ? UI_RED : UI_GREEN, worst);

    /* El unico numero que responde la pregunta de la fase sin
       interpretacion: frames que no cupieron en su frame. */
    uint16_t over = pal_profile_overruns();
    ui_text_put(3, 9, over ? UI_RED : UI_GREEN, "FRAMES CAIDOS");
    ui_text_put(19, 9, over ? UI_RED : UI_GREEN, ui_itoa(over));
}
