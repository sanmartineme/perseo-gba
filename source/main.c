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
#include "ui/cine.h"
#include "ui/profile.h"

/* Estados en los que el mundo no se ve y los sprites sólo estorbarían
   por encima del texto. */
static bool state_hides_sprites(GameState st) {
    /* El título usa el nivel como fondo en movimiento, pero no su
       contenido: sin esto se verían los corazones del HUD y las ratas
       del nivel 1 paseando por debajo del logotipo.
       GS_STORY y GS_ENDING NO están: ahí los sprites son la viñeta. */
    return st == GS_TITLE || st == GS_INVENTORY || st == GS_CREDITS;
}

/* Pantallas cuyos sprites los pone la cinemática y no el mundo. */
static bool state_is_cine(GameState st) {
    return st == GS_STORY || st == GS_ENDING;
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

/* --- Build de medicion (F9-01) -------------------------------------
   `make BUILD=build_prof EXTRA_CFLAGS=-DPERSEO_PROFILE_BOOT=4` arranca
   directo en ese nivel, con las tres habilidades y el medidor de frame
   siempre a la vista. Existe porque medir un nivel de los siete exigia
   llegar jugando hasta el, y una partida entera no sale igual dos veces;
   asi la medicion se repite con un comando en vez de con un parche a mano
   que despues hay que acordarse de revertir.
   No entra en la ROM normal: sin la bandera, esto no existe. */
#ifdef PERSEO_PROFILE_BOOT
/* Con -DPERSEO_PROFILE_ARENA ademas deja a Perseo delante del disparador
   del jefe, de modo que la pelea arranca sola a los pocos frames. Es lo
   que hace medible F9-07 (el peor caso candidato es un combate de jefe):
   llegar a una arena jugando lleva minutos y no sale igual dos veces. */
#ifdef PERSEO_PROFILE_ARENA
static void profile_goto_arena(Game *g) {
    const Level *lv = g->world.lv;
    for (int i = 0; i < lv->entity_count; i++) {
        const LevelEntitySpawn *sp = &lv->entities[i];
        if (sp->type != ENT_BOSSGATE) continue;
        /* update_bossgate() exige estar a la izquierda de la puerta y
           dentro de su franja vertical; se apunta al centro de la franja
           y a cuatro tiles de la puerta, con margen de sobra. */
        int16_t ty = (int16_t)(((sp->yband[0] + sp->yband[1]) / 2) / TILE_SIZE);
        world_place_player(&g->world, (int16_t)(sp->tx - 4), ty);
        camera_update(&g->cam, g->world.player.x, g->world.player.y, lv);
        return;
    }
}
#endif

static void profile_boot(Game *g) {
    g->progress.ab.double_jump = true;
    g->progress.ab.dash = true;
    g->progress.ab.climb = true;
    game_load_level(g, (uint8_t)PERSEO_PROFILE_BOOT);
    g->state = GS_PLAY;
    g->state_t = 0;
#ifdef PERSEO_PROFILE_ARENA
    profile_goto_arena(g);
#endif
}
#endif

/* El medidor se ve manteniendo START+SELECT, salvo en una build de
   medicion, donde esta siempre puesto. */
static bool profile_visible(void) {
#ifdef PERSEO_PROFILE_BOOT
    return true;
#else
    return pal_input_debug_held();
#endif
}

int main(void) {
    irq_init(NULL);
    irq_enable(II_VBLANK);

    pal_video_init();
    ui_text_init();
    audio_init();
    game_init(&g_game);
#ifdef PERSEO_PROFILE_BOOT
    profile_boot(&g_game);
#endif

    while (1) {
        VBlankIntrWait();
        pal_profile_frame_start();
        /* Lo primero del VBlank: subir al hardware la interfaz que se
           dibujo en el frame anterior. Tiene que ser aca y no al pintarla
           — ver el buffer sombra en pal_gba_text.c. Se contabiliza en el
           tramo de interfaz, que es de donde sale el trabajo. */
        ui_text_flush();
        pal_profile_mark(PAL_PROF_UI);

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
            .debug_held = pal_input_debug_held(),
        };

        game_update(&g_game, &in);
        audio_update();
        pal_profile_mark(PAL_PROF_UPDATE);

        const World *w = &g_game.world;
        int cam_x = fx_to_int(g_game.cam.x), cam_y = fx_to_int(g_game.cam.y);
        /* La sacudida de pantalla mueve la cámara, no el mundo: se aplica
           al dibujar y no altera ninguna posición real. */
        if (w->shake > 0) {
            cam_x += (int)(w->tick & 1) ? w->shake / 2 : -w->shake / 2;
        }

        /* La paleta sólo cambia al cambiar de zona; compararla sale más
           barato que copiar 32 bytes cada frame. */
        static int last_level = -1;
        if ((int)g_game.level_index != last_level) {
            last_level = (int)g_game.level_index;
            pal_video_set_level_palette(last_level);
            /* El tilemap de la zona anterior ya no vale: que se rellene
               entero. Va junto a la paleta porque es el mismo motivo — lo
               que cambia al cambiar de nivel. */
            pal_video_reset_level_sync();
            pal_profile_level_changed();
        }
        /* Tiles cambiados dentro de lo que ya se ve: hay que rehacer el
           tilemap entero, porque el camino incremental sólo cubre lo que
           entra por los bordes. */
        if (g_game.world.tiles_dirty) {
            pal_video_reset_level_sync();
            g_game.world.tiles_dirty = false;
        }
        pal_video_set_mosaic(transition_mosaic(&g_game));
        pal_video_show_sprites(!state_hides_sprites(g_game.state));
        pal_video_sync_level(w->lv, cam_x, cam_y);
        pal_video_set_parallax_scroll(cam_x, cam_y);
        pal_profile_mark(PAL_PROF_TILES);

        /* En las viñetas el pool de entidades no pinta nada — es de otro
           nivel, o está vacío — y quien manda es la escena. */
        pal_video_cine_backdrop(state_is_cine(g_game.state) ? ui_cine_backdrop(&g_game)
                                                            : CINE_BG_NONE);
        pal_video_cine_pan(g_game.frame);
        if (state_is_cine(g_game.state)) ui_cine_draw(&g_game);
        else                             pal_video_draw_world(w, cam_x, cam_y);
        pal_profile_mark(PAL_PROF_SPRITES);

        ui_screens_draw(&g_game);
        pal_profile_mark(PAL_PROF_UI);
        /* El medidor se dibuja DESPUES de cerrar el tramo de interfaz, a
           propósito: así los números que muestra describen la ROM tal
           como se juega, sin contar el coste de estarla mirando. */
        if (profile_visible()) ui_profile_draw();
    }

    return 0;
}
