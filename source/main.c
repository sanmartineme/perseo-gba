/* =====================================================================
   main.c — "Hola mundo" de la Fase 0
   =====================================================================
   Objetivo único de este archivo (docs/TAREAS_MIGRACION_GBA.md, F0-06..F0-10):
   confirmar que el toolchain (devkitARM + libtonc), el Makefile y el
   emulador funcionan de punta a punta, antes de escribir una sola línea
   de lógica de juego real.

   Usa Modo 3 (bitmap de 16 bits) porque es la forma más simple de pintar
   algo en pantalla sin configurar tiles/paleta todavía. La Fase 2
   (docs/PLAN_MIGRACION_GBA_C.md, sección "Fase 2") reemplaza esto por el
   Modo 0 (tiled, 4 capas) que usará el juego real.

   NOTA de arquitectura: este archivo vive fuera de source/core/ a propósito
   — es el punto de entrada específico de GBA. La lógica de juego portable
   nunca debe parecerse a esto (nada de <tonc.h> en source/core/).
   ===================================================================== */
#include <tonc.h>

int main(void) {
    irq_init(NULL);
    irq_enable(II_VBLANK);

    REG_DISPCNT = DCNT_MODE3 | DCNT_BG2;

    /* Colores de la paleta de referencia del prototipo (ver COL{} en
       docs/prototipo_referencia.html): azul pizarra oscuro de fondo,
       dorado como respuesta visible al input. */
    const u16 col_bg = RGB15(4, 6, 10);
    const u16 col_hi = RGB15(31, 20, 2);

    m3_fill(col_bg);

    while (1) {
        key_poll();

        /* F0-10: confirma que REG_KEYINPUT llega correctamente. */
        m3_fill(key_held(KEY_A) ? col_hi : col_bg);

        VBlankIntrWait();
    }

    return 0;
}
