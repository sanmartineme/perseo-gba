/* =====================================================================
   pal.h — Platform Abstraction Layer (PAL)
   =====================================================================
   Interfaz que separa la lógica de juego portable (source/core/, source/ui/)
   de la implementación real de hardware (source/platform/gba/) o de un
   backend de escritorio para desarrollo (source/platform/sdl/).

   Regla de la arquitectura (ver docs/PLAN_MIGRACION_GBA_C.md, sección 4 y 9.1):
   NINGÚN archivo bajo source/core/ puede incluir <gba.h>, <tonc.h> ni ningún
   header de source/platform/*. Todo lo que core/ necesita del hardware pasa
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

/* ---------- Vídeo (Fase 2) ---------- */
void pal_video_init(void);
/* void pal_video_load_tileset(...); */
/* void pal_video_scroll_bg(int layer, int x, int y); */
/* void pal_video_draw_sprite(...); */

/* ---------- Audio (Fase 6) ---------- */
void pal_audio_init(void);
/* void pal_audio_play_song(const Song *song); */
/* void pal_audio_play_sfx(SfxId id); */

/* ---------- Guardado (Fase 7) ---------- */
bool pal_save_read(void *dst, uint32_t size);
bool pal_save_write(const void *src, uint32_t size);

#endif /* PERSEO_PAL_H */
