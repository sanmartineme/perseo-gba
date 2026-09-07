/* =====================================================================
   pal.h — Platform Abstraction Layer (PAL)
   =====================================================================
   Interfaz que separa la lógica de juego portable (source/core/, source/ui/)
   de la implementación real de hardware (source/platform/gba/) o de un
   backend de escritorio para desarrollo (source/platform/sdl/).

   Regla de la arquitectura (ver docs/PLAN_MIGRACION_GBA_C.md, sección 4 y 9.1):
   NINGÚN archivo bajo source/core/ puede incluir <gba.h>, <tonc.h> ni ningún
   header de source/platform/ (ni sus subcarpetas). Todo lo que core/ necesita del hardware pasa
   por las funciones declaradas aquí.

   Este archivo se rellena de forma incremental a medida que avanza el
   checklist (docs/TAREAS_MIGRACION_GBA.md):
     - Fase 1 (F1-14): funciones de input.
     - Fase 2 (F2-01): funciones de vídeo.
     - Fase 6 (F6-01): funciones de audio.
     - Fase 7 (F7-12): funciones de guardado.
   ===================================================================== */
#ifndef PERSEO_PAL_H
#define PERSEO_PAL_H

#include <stdint.h>
#include <stdbool.h>
#include "../core/level/level.h"
#include "../core/player.h"

/* ---------- Input (Fase 1) ----------
   Mapeo de botones según docs/PLAN_MIGRACION_GBA_C.md, sección 6:
   D-Pad=mover, A=saltar, B=Ganchito Mortal, R=Dash Sombrío,
   L=Lanzamiento de Traza, Select=inventario, Start=pausa. */
void pal_input_poll(void);
bool pal_input_left(void);
bool pal_input_right(void);
bool pal_input_up(void);
bool pal_input_down(void);
bool pal_input_jump_held(void);
bool pal_input_jump_pressed(void);
bool pal_input_attack_pressed(void);
bool pal_input_dash_pressed(void);
bool pal_input_throw_pressed(void);
bool pal_input_start_pressed(void);
bool pal_input_select_pressed(void);

/* ---------- Vídeo (Fase 2) -----------------------------------------
   Modo 0 (4 capas tiled, sin rotación/escala — ver docs/PLAN_MIGRACION_GBA_C.md,
   sección 2): BG2 es el tilemap colisionable del nivel, BG1 una capa de
   paralaje simple. Los tiles de fondo son placeholders de color sólido
   (grit + arte real llegan en la Fase 3); lo que se prueba acá es el
   *mecanismo* de scroll con mapas más grandes que un screenblock de
   hardware (32x32 tiles), no el arte final. */
void pal_video_init(void);

/* Sincroniza la ventana visible del tilemap de BG2 con el nivel real y
   aplica el scroll de cámara. Usa la técnica de "mapa grande" (Tonc):
   un screenblock de 32x32 tiles se reutiliza como buffer circular,
   reescribiendo sólo las columnas/filas que quedan recién expuestas a
   medida que la cámara se mueve, en vez de redibujar todo el nivel
   cada frame. cam_x/cam_y en píxeles enteros de mundo. */
void pal_video_sync_level(const Level *lv, int cam_x, int cam_y);

/* Capa de paralaje (BG1): un único tile de relleno que se desplaza a
   una fracción de la velocidad de la cámara, dando sensación de
   profundidad — antesala de las capas de paralaje reales de la Fase 3
   (drawParallax() del prototipo). */
void pal_video_set_parallax_scroll(int cam_x, int cam_y);

/* Dibuja a Perseo como OBJ de hardware con el frame que le corresponde.
   `anim` lo decide core/player.c (player_get_anim); acá sólo se traduce a
   atributos de OAM. scr_x/scr_y son la esquina de su caja de colisión ya
   en coordenadas de pantalla (posición de mundo menos cámara): el
   desplazamiento del sprite respecto de la caja lo aplica esta función. */
void pal_video_draw_player(const PlayerAnim *anim, int scr_x, int scr_y);

/* ---------- Audio (Fase 6) ---------- */
void pal_audio_init(void);
/* void pal_audio_play_song(const Song *song); */
/* void pal_audio_play_sfx(SfxId id); */

/* ---------- Guardado (Fase 7) ---------- */
bool pal_save_read(void *dst, uint32_t size);
bool pal_save_write(const void *src, uint32_t size);

#endif /* PERSEO_PAL_H */
