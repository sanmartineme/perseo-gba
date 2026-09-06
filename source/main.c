/* =====================================================================
   main.c — punto de entrada de GBA
   =====================================================================
   Fase 0 (ver historial de git): color sólido + lectura de botón A.
   Fase 1: loop a 60Hz + física real del jugador, con un render de
   depuración en Modo 3 (rectángulos) que ya se descarta acá.

   Fase 2 (docs/TAREAS_MIGRACION_GBA.md, F2-01..F2-10): renderer real en
   Modo 0 — BG2 es el tilemap colisionable del nivel (streaming de
   "mapa grande", ver source/platform/gba/pal_gba_video.c), BG1 una
   capa de paralaje simple, y el jugador es un sprite OBJ de hardware
   en vez de un rectángulo dibujado a mano. La sala de prueba de esta
   fase (level_get_scroll_test_room) es deliberadamente más ancha que
   un screenblock de hardware para ejercitar el mecanismo de scroll.

   NOTA de arquitectura: este archivo sigue siendo el único con permiso
   para incluir <tonc.h>. Toda la física (source/core/player.c) y la
   cámara (source/core/camera.c) no saben que existe Modo 0, BG2HOFS ni
   ningún registro de GBA — sólo hablan con la PAL (source/platform/pal.h).
   ===================================================================== */
#include <tonc.h>
#include "core/fixed.h"
#include "core/level/level.h"
#include "core/camera.h"
#include "core/player.h"
#include "platform/pal.h"

int main(void) {
    irq_init(NULL);
    irq_enable(II_VBLANK);

    pal_video_init();

    const Level *lv = level_get_scroll_test_room();

    Player player;
    player_init(&player,
                fx_from_int((int32_t)lv->spawn_tx * TILE_SIZE),
                fx_from_int((int32_t)lv->spawn_ty * TILE_SIZE));
    /* Debug: habilidades forzadas hasta que exista el sistema de
       santuarios (Fase 7) — ver la misma nota en la Fase 1. */
    player.ab.double_jump = true;
    player.ab.dash = true;
    player.ab.climb = true;

    Camera cam;
    camera_init(&cam);

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
        player_update(&player, lv, &in);
        camera_update(&cam, player.x, player.y, lv);

        int cam_x = fx_to_int(cam.x), cam_y = fx_to_int(cam.y);
        pal_video_sync_level(lv, cam_x, cam_y);
        pal_video_set_parallax_scroll(cam_x, cam_y);
        pal_video_set_player_sprite(fx_to_int(player.x) - cam_x, fx_to_int(player.y) - cam_y);
    }

    return 0;
}
