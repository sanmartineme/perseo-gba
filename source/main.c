/* =====================================================================
   main.c — punto de entrada de GBA
   =====================================================================
   Fase 0 (ver historial de git): sólo un color sólido + lectura de
   botón A, para confirmar que el toolchain/Makefile/emulador funcionan.

   Fase 1 (docs/TAREAS_MIGRACION_GBA.md, F1-16/F1-17): agrega el loop
   real a 60Hz y conecta el jugador de verdad (source/core/player.c)
   sobre una habitación de prueba (source/core/level/level.c). El
   render de acá abajo es Modo 3 con rectángulos rellenos a propósito
   —no el Modo 0 tileado con scroll del juego real— sólo para poder VER
   en el emulador que la física funciona. Se descarta por completo en
   la Fase 2, que trae el renderer real.

   NOTA de rendimiento (importante incluso para código descartable):
   Modo 3 no tiene doble buffer, así que escribir a VRAM fuera del
   período de VBlank puede "tearear" contra el haz de refresco de la
   LCD si el dibujado tarda más que ese período. La primera versión de
   este archivo redibujaba TODA la habitación (~136 tiles) en cada
   vuelta del loop y se notaba como tearing estable (algunas filas
   nunca terminaban de dibujarse antes de que el haz las escaneara). La
   geometría del nivel es estática, así que se dibuja UNA sola vez antes
   del loop; en cada frame sólo se borra/redibuja el pequeño rectángulo
   del jugador — unos pocos tiles en el peor caso, muy por debajo del
   presupuesto de VBlank.

   NOTA de arquitectura: este archivo vive fuera de source/core/ a
   propósito — es el punto de entrada específico de GBA, el único lugar
   con permiso para incluir <tonc.h>. source/core/player.c y
   source/core/level/level.c no saben que existe.
   ===================================================================== */
#include <tonc.h>
#include "core/fixed.h"
#include "core/level/level.h"
#include "core/player.h"
#include "platform/pal.h"

static void draw_level_debug(const Level *lv, u16 tile_clr) {
    for (int16_t ty = 0; ty < lv->h; ty++) {
        for (int16_t tx = 0; tx < lv->w; tx++) {
            if (tile_is_solid(level_tile_at(lv, tx, ty))) {
                int x0 = tx * TILE_SIZE, y0 = ty * TILE_SIZE;
                m3_rect(x0, y0, x0 + TILE_SIZE, y0 + TILE_SIZE, tile_clr);
            }
        }
    }
}

/* Redibuja sólo la región [x0,y0)-(x1,y1) (en píxeles de pantalla) con
   el fondo correcto: primero el color de fondo, y encima cualquier
   tile sólido que se superponga a esa región. Se usa para "borrar" el
   rectángulo del jugador en su posición anterior sin tener que
   repintar la habitación entera. */
static void redraw_region(const Level *lv, int x0, int y0, int x1, int y1, u16 bg, u16 tile_clr) {
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > lv->w * TILE_SIZE) x1 = lv->w * TILE_SIZE;
    if (y1 > lv->h * TILE_SIZE) y1 = lv->h * TILE_SIZE;
    if (x0 >= x1 || y0 >= y1) return;

    m3_rect(x0, y0, x1, y1, bg);

    int16_t tx0 = x0 / TILE_SIZE, tx1 = (x1 - 1) / TILE_SIZE;
    int16_t ty0 = y0 / TILE_SIZE, ty1 = (y1 - 1) / TILE_SIZE;
    for (int16_t ty = ty0; ty <= ty1; ty++) {
        for (int16_t tx = tx0; tx <= tx1; tx++) {
            if (tile_is_solid(level_tile_at(lv, tx, ty))) {
                int tpx = tx * TILE_SIZE, tpy = ty * TILE_SIZE;
                m3_rect(tpx, tpy, tpx + TILE_SIZE, tpy + TILE_SIZE, tile_clr);
            }
        }
    }
}

int main(void) {
    irq_init(NULL);
    irq_enable(II_VBLANK);
    REG_DISPCNT = DCNT_MODE3 | DCNT_BG2;

    /* Colores de la paleta de referencia del prototipo (COL{} en
       docs/prototipo_referencia.html). */
    const u16 col_bg   = RGB15(4, 6, 10);   /* K/fondo: azul pizarra oscuro */
    const u16 col_tile = RGB15(14, 17, 22); /* T: gris azulado de tubería/muro */
    const u16 col_pl   = RGB15(31, 20, 2);  /* Y: dorado, silueta de Perseo */

    const Level *lv = level_get_test_room();

    Player player;
    player_init(&player,
                fx_from_int((int32_t)lv->spawn_tx * TILE_SIZE),
                fx_from_int((int32_t)lv->spawn_ty * TILE_SIZE));

    /* Debug: las 3 habilidades desbloqueables se fuerzan a "true" acá
       porque el sistema de santuarios (shrine) todavía no existe —
       llega en la Fase 7. Sin esto no habría forma de probar Salto
       Doble/Dash/Garra Felina en este smoke test. */
    player.ab.double_jump = true;
    player.ab.dash = true;
    player.ab.climb = true;

    /* Dibujo único de la geometría estática (ver nota de rendimiento
       arriba). */
    m3_fill(col_bg);
    draw_level_debug(lv, col_tile);

    int prev_x = fx_to_int(player.x), prev_y = fx_to_int(player.y);

    while (1) {
        VBlankIntrWait();

        pal_input_poll();
        PlayerInput in = {
            .left = pal_input_left(), .right = pal_input_right(),
            .up = pal_input_up(), .down = pal_input_down(),
            .jump_held = pal_input_jump_held(),
            .jump_pressed = pal_input_jump_pressed(),
            .dash_pressed = pal_input_dash_pressed(),
        };

        /* Borra al jugador en su posición anterior antes de mover: así
           nunca queda un frame con dos rectángulos superpuestos. */
        redraw_region(lv, prev_x, prev_y, prev_x + player.w, prev_y + player.h, col_bg, col_tile);

        player_update(&player, lv, &in);

        int x0 = fx_to_int(player.x), y0 = fx_to_int(player.y);
        m3_rect(x0, y0, x0 + player.w, y0 + player.h, col_pl);
        prev_x = x0;
        prev_y = y0;
    }

    return 0;
}
