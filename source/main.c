/* =====================================================================
   main.c — punto de entrada de GBA
   =====================================================================
   Fase 0 (ver historial de git): color sólido + lectura de botón A.
   Fase 1: loop a 60Hz + física real del jugador, con un render de
   depuración en Modo 3 (rectángulos) que ya se descarta acá.

   Fase 2: renderer real en Modo 0 — BG2 es el tilemap colisionable del
   nivel (streaming de "mapa grande", ver
   source/platform/gba/pal_gba_video.c), BG1 una capa de paralaje, y el
   jugador un sprite OBJ de hardware.

   Fase 3: el Nivel 1 real (Túneles de Filtración, 200x44 tiles,
   generado desde assets/src/levels/level01.json) con el arte de verdad
   del prototipo.

   Fase 4 (docs/TAREAS_MIGRACION_GBA.md, F4-01..F4-15): el nivel ya se
   puebla con sus enemigos y objetos. Este archivo queda mínimo a
   propósito: todo el juego vive en core/world.c, y acá sólo se
   traducen botones a intenciones y se pide dibujar.

   NOTA de arquitectura: este archivo sigue siendo el único con permiso
   para incluir <tonc.h>. Toda la física (source/core/player.c) y la
   cámara (source/core/camera.c) no saben que existe Modo 0, BG2HOFS ni
   ningún registro de GBA — sólo hablan con la PAL (source/platform/pal.h).
   ===================================================================== */
#include <tonc.h>
#include "core/fixed.h"
#include "core/level/level.h"
#include "core/level/level01_tuneles.h"
#include "core/camera.h"
#include "core/player.h"
#include "core/world.h"
#include "platform/pal.h"

int main(void) {
    irq_init(NULL);
    irq_enable(II_VBLANK);

    pal_video_init();

    World world;
    world_load(&world, &level01_tuneles);

    /* Debug: habilidades forzadas hasta que existan los santuarios
       (Fase 7) — ver la misma nota desde la Fase 1. */
    world.player.ab.double_jump = true;
    world.player.ab.dash = true;
    world.player.ab.climb = true;

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
            .attack_pressed = pal_input_attack_pressed(),
            .throw_pressed = pal_input_throw_pressed(),
        };
        world_update(&world, &in);
        camera_update(&cam, world.player.x, world.player.y, world.lv);

        int cam_x = fx_to_int(cam.x), cam_y = fx_to_int(cam.y);
        /* La sacudida de pantalla mueve la cámara, no el mundo: se aplica
           al dibujar y no altera ninguna posición real. */
        if (world.shake > 0) {
            cam_x += (int)(world.tick & 1) ? world.shake / 2 : -world.shake / 2;
        }
        pal_video_sync_level(world.lv, cam_x, cam_y);
        pal_video_set_parallax_scroll(cam_x, cam_y);
        pal_video_draw_world(&world, cam_x, cam_y);
    }

    return 0;
}
