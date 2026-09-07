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
/* ---------- Botones reasignables ----------
   Las cuatro acciones de juego se pueden mover entre los cuatro botones
   de acción. El D-Pad, START y SELECT no: mover la cruceta no tiene
   sentido, y dejar los menús sin botón de confirmar tampoco — por eso
   confirmar y cancelar siguen fijos en A y B pase lo que pase con el
   mapeo de juego. */
typedef enum PadButton {
    PAD_A = 0, PAD_B, PAD_L, PAD_R, PAD_COUNT
} PadButton;

typedef enum InputAction {
    ACT_JUMP = 0,   /* saltar */
    ACT_ATTACK,     /* Ganchito Mortal */
    ACT_DASH,       /* Dash Sombrío */
    ACT_THROW,      /* Lanzamiento de Traza */
    ACT_COUNT
} InputAction;

/* Nombre corto del botón, para pintarlo en el menú: "A", "B", "L", "R". */
const char *pal_input_button_name(PadButton b);
/* Asigna un botón a una acción. Si ya lo tenía otra, las dos se
   intercambian: así el mapeo sigue siendo una permutación y nunca queda
   una acción sin botón ni dos acciones en el mismo. */
void pal_input_bind(InputAction act, PadButton btn);
PadButton pal_input_binding(InputAction act);
/* Restaura las cuatro asignaciones de golpe (al cargar una partida). Si lo
   que llega no es una permutación de los cuatro botones — SRAM corrupta,
   un guardado de otra versión — se ignora y quedan las de fábrica: mejor
   unos controles inesperados que una acción sin botón. */
void pal_input_set_bindings(const uint8_t *four);

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

/* START+SELECT a la vez: combinacion de depuracion, imposible de
   pulsar sin querer, que muestra el medidor de frame de la Fase 9. */
bool pal_input_debug_held(void);

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

/* ---------- Viñetas de la cinemática ----------
   Las pantallas de historia no dibujan el mundo: componen una escena a
   mano, con los personajes puestos donde toca. El prototipo lo hace con
   `drawImage` sobre el canvas; acá hace falta un camino propio, porque el
   único que había — pal_video_draw_world() — sólo sabe pintar el pool de
   entidades, y en esas pantallas el pool está vacío o es de otro nivel.

   Los actores son los mismos personajes del juego pero al doble de
   tamaño (32x32), como en el original. Las coordenadas son de PANTALLA y
   apuntan a la esquina superior izquierda del sprite. */
typedef enum CineActor {
    CINE_AURORA = 0,   /* la hermanita; sólo aparece en las viñetas */
    CINE_THUG1,
    CINE_THUG2,
    CINE_PERSEO,
    CINE_BETTY,        /* reaprovecha el sprite del jefe final */
    CINE_CAGE,         /* la jaula, con los huecos transparentes */
    CINE_MOON,         /* 16x16, el único que no es de 32 */
    CINE_ACTOR_COUNT
} CineActor;

/* El fondo de una viñeta. No es decoración: los protagonistas son gatos
   negros, y sobre un cielo negro no se les ve más que el hocico. En el
   prototipo lo que les da contraste es la silueta de la ciudad detrás
   (en la calle) y las franjas del trónó (bajo tierra), así que acá hay
   que dibujarlas igual. */
typedef enum CineBackdrop {
    CINE_BG_NONE = 0,   /* ninguno: se ve el nivel, como siempre */
    CINE_BG_NIGHT,      /* la superficie de noche, con su horizonte */
    CINE_BG_THRONE      /* el tróno de Betty, bajo tierra */
} CineBackdrop;

/* Cambia el fondo de la cinemática. Esconde las capas del nivel mientras
   dura y las devuelve como estaban al volver a CINE_BG_NONE. */
void pal_video_cine_backdrop(CineBackdrop kind);

/* El paneo lento de la ciudad. Es scroll de hardware: no cuesta nada. */
void pal_video_cine_pan(uint32_t frame);

/* Empieza una escena: descarta lo que hubiera en OAM. */
void pal_video_cine_begin(void);
void pal_video_cine_actor(CineActor who, int x, int y, bool flip_h);
/* Un punto suelto de un color de partícula, que es lo que hace de
   estrella en el cielo y de chispa. */
void pal_video_cine_dot(int x, int y, uint8_t color);
/* Cierra la escena: esconde las ranuras de OAM que sobraron. */
void pal_video_cine_end(void);

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

/* Olvida lo que el buffer circular de BG2 tenía al día, de modo que el
   próximo pal_video_sync_level() lo llene entero. Hay que llamarla al
   CAMBIAR DE NIVEL: el rango sincronizado es de tiles de mundo y no sabe
   de qué nivel son, así que sin esto el nivel nuevo hereda el rango del
   viejo y sólo se redibuja lo que la cámara descubra al moverse. Los
   niveles 2 a 7 aparecen todos en el mismo tile, con lo que la ventana
   quedaba idéntica y no se redibujaba nada: se entraba al nivel nuevo
   viendo la geometría del anterior. */
void pal_video_reset_level_sync(void);

/* ---------- Audio (Fase 6) ----------
   La interfaz de audio quedó declarada en core/audio.h (es core quien
   define QUÉ suena), así que no hay nada de audio en la PAL. */

/* ---------- Perfilado (Fase 9) ----------
   Reparto del trabajo de un frame en cuatro tramos, para saber no sólo
   si cabe sino QUE parte no cabria si dejara de caber. Ver
   source/platform/gba/pal_gba_profile.c. */
typedef enum PalProfileSlot {
    PAL_PROF_UPDATE = 0,  /* mundo, física, IA, audio */
    PAL_PROF_TILES,       /* streaming del tilemap de fondo */
    PAL_PROF_SPRITES,     /* volcado de OAM */
    PAL_PROF_UI,          /* capa de texto */
    PAL_PROFILE_COUNT
} PalProfileSlot;

void pal_profile_frame_start(void);
void pal_profile_mark(PalProfileSlot slot);
/* `worst` = el peor valor visto desde el ultimo reset, que es el numero
   que importa: un promedio bonito con un pico que se pasa del frame se
   ve igual de mal en pantalla. */
uint16_t pal_profile_ticks(PalProfileSlot slot, bool worst);
/* Con worst=true devuelve el peor frame COMPLETO que se vio, no la suma
   de los peores de cada tramo: esos maximos pueden venir de frames
   distintos y sumarlos describe un frame que quiza nunca ocurrio. */
uint16_t pal_profile_total(bool worst);
int      pal_profile_percent(uint16_t ticks);
void     pal_profile_reset_worst(void);
/* Reinicia las marcas y concede unos frames de cortesia: el primero de
   un nivel llena el tilemap entero y no representa al juego en marcha. */
void     pal_profile_level_changed(void);
/* Frames cuyo trabajo NO cupo en su frame. Es la respuesta directa a
   "¿va a 60?": si es 0, va a 60. */
uint16_t pal_profile_overruns(void);

/* ---------- Guardado (Fase 7) ---------- */
bool pal_save_read(void *dst, uint32_t size);
bool pal_save_write(const void *src, uint32_t size);

#endif /* PERSEO_PAL_H */
