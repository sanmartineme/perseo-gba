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
#include "../core/world.h"

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

/* Flancos que sólo usan los menús (Fase 7): moverse por una lista y
   confirmar/cancelar. El juego no los necesita — ahí la cruceta se lee
   sostenida — pero un menú que avanza mientras mantienes abajo es
   inusable. */
bool pal_input_up_pressed(void);
bool pal_input_down_pressed(void);
bool pal_input_left_pressed(void);
bool pal_input_right_pressed(void);
bool pal_input_confirm_pressed(void);
bool pal_input_cancel_pressed(void);

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

/* Dibuja todo lo que va en sprites de hardware, en un solo lugar: Perseo,
   las entidades vivas, las partículas y el HUD de vida. Reparte los 128
   objetos de OAM por orden de prioridad y esconde los que sobran, así que
   nadie más tiene que preocuparse por qué ranura le toca a quién.

   Qué pose le corresponde a Perseo lo decide core/ (player_get_anim);
   acá sólo se traduce a atributos de OAM. */
void pal_video_draw_world(const World *w, int cam_x, int cam_y);

/* Mosaico de hardware, 0 (nítido) a 15 (bloques de 16 px). Es la
   traducción de pixelate()/startTransition() del prototipo: allá se
   redibujaba el frame a baja resolución y se escalaba; acá el efecto ya
   existe en el hardware y no cuesta nada. */
void pal_video_set_mosaic(int amount);

/* Cambia la paleta de fondo a la de un nivel. Los siete comparten el
   mismo tileset y se diferencian sólo por el color del ladrillo y su
   sombra (ver source/platform/gba/level_palettes.c), así que cambiar de
   zona cuesta 32 bytes copiados, no un tileset nuevo. */
void pal_video_set_level_palette(int level_index);

/* Apaga la capa de objetos: las pantallas de menú y de historia tapan el
   juego, y los sprites tienen prioridad sobre los fondos, así que se
   colarían por encima del texto. */
void pal_video_show_sprites(bool on);

/* ---------- Audio (Fase 6) ----------
   La interfaz de audio quedó declarada en core/audio.h (es core quien
   define QUÉ suena), así que no hay nada de audio en la PAL. */

/* ---------- Guardado (Fase 7) ---------- */
bool pal_save_read(void *dst, uint32_t size);
bool pal_save_write(const void *src, uint32_t size);

#endif /* PERSEO_PAL_H */
