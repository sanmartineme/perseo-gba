/* =====================================================================
   main.c — punto de entrada de GBA
   =====================================================================
   Histórico de lo que fue apareciendo acá (ver docs/TAREAS_MIGRACION_GBA.md):
     Fase 0: color sólido + lectura de un botón.
     Fase 1: bucle a 60 Hz + física real, con un render de depuración en
             Modo 3 que ya se descartó.
     Fase 2: renderer real en Modo 0 (BG2 el nivel, BG1 paralaje, Perseo
             como sprite de hardware) con streaming de "mapa grande".
     Fase 3: el Nivel 1 de verdad, con el arte del prototipo.
     Fase 4: el nivel poblado con sus enemigos y objetos.
     Fase 5: los jefes.
     Fase 6: audio PSG.
     Fase 7: la partida deja de ser un único bucle que siempre juega y
             pasa a tener estados (título, historia, juego, pausa,
             inventario, muerte, transición) — pero esa máquina vive en
             core/game_state.c, no acá.

   Este archivo se mantiene mínimo a propósito, y sigue siendo el único
   con permiso para incluir <tonc.h>: traduce botones a intenciones, pide
   una actualización, pide un dibujo. Nada más.
   ===================================================================== */
#include <tonc.h>
#include "core/fixed.h"
#include "core/game_state.h"
#include "core/audio.h"
#include "platform/pal.h"
#include "ui/text.h"
#include "ui/screens.h"

/* Estados en los que el mundo no se ve y los sprites sólo estorbarían
   por encima del texto. */
static bool state_hides_sprites(GameState st) {
    /* El título usa el nivel como fondo en movimiento, pero no su
       contenido: sin esto se verían los corazones del HUD y las ratas
       del nivel 1 paseando por debajo del logotipo. */
    return st == GS_TITLE || st == GS_STORY || st == GS_INVENTORY ||
           st == GS_ENDING || st == GS_CREDITS;
}

/* Mosaico de la transición: sube mientras cierra y baja mientras abre,
   que es lo que hacía pixelate() en el prototipo. */
static int transition_mosaic(const Game *g) {
    if (g->state != GS_TRANS) return 0;
    int t = (int)g->state_t;
    if (t > 26) t = 26;
    int m = (t * 15) / 26;
    return g->trans_closing ? m : 15 - m;
}

static Game g_game;

int main(void) {
    irq_init(NULL);
    irq_enable(II_VBLANK);

    pal_video_init();
    ui_text_init();
    audio_init();
    game_init(&g_game);

    while (1) {
        VBlankIntrWait();

        pal_input_poll();
        GameInput in = {
            .p = {
                .left = pal_input_left(), .right = pal_input_right(),
                .up = pal_input_up(), .down = pal_input_down(),
                .jump_held = pal_input_jump_held(),
                .jump_pressed = pal_input_jump_pressed(),
                .dash_pressed = pal_input_dash_pressed(),
                .attack_pressed = pal_input_attack_pressed(),
                .throw_pressed = pal_input_throw_pressed(),
            },
            .up_pressed = pal_input_up_pressed(),
            .down_pressed = pal_input_down_pressed(),
            .left_pressed = pal_input_left_pressed(),
            .right_pressed = pal_input_right_pressed(),
            .confirm_pressed = pal_input_confirm_pressed(),
            .cancel_pressed = pal_input_cancel_pressed(),
            .start_pressed = pal_input_start_pressed(),
            .select_pressed = pal_input_select_pressed(),
        };

        game_update(&g_game, &in);
        audio_update();

        const World *w = &g_game.world;
        int cam_x = fx_to_int(g_game.cam.x), cam_y = fx_to_int(g_game.cam.y);
        /* La sacudida de pantalla mueve la cámara, no el mundo: se aplica
           al dibujar y no altera ninguna posición real. */
        if (w->shake > 0) {
            cam_x += (int)(w->tick & 1) ? w->shake / 2 : -w->shake / 2;
        }

        pal_video_set_mosaic(transition_mosaic(&g_game));
        pal_video_show_sprites(!state_hides_sprites(g_game.state));
        pal_video_sync_level(w->lv, cam_x, cam_y);
        pal_video_set_parallax_scroll(cam_x, cam_y);
        pal_video_draw_world(w, cam_x, cam_y);
        ui_screens_draw(&g_game);
    }

    return 0;
}
