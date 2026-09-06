/* =====================================================================
   player.h — estado y física de Perseo
   =====================================================================
   Traducción de P{} y updatePlayer() del prototipo (ver
   docs/prototipo_referencia.html, líneas ~1569 y ~1655, y
   docs/PLAN_MIGRACION_GBA_C.md, tabla de trazabilidad).

   Acotado a lo que la Fase 1 necesita: movimiento, salto (con coyote
   time y jump buffer), Salto Doble, Dash Sombrío y Garra Felina. Los
   campos de combate (atkT/atkCd/swing/throwT/throwCd), vida/daño
   (hp/inv/regenT) y reliquias se agregan a este mismo struct en las
   Fases 4 y 7 respectivamente — no antes, para no cargar campos que
   todavía no tienen ningún sistema que los use (ver
   docs/TAREAS_MIGRACION_GBA.md, F4-03/F4-13, F7-10).
   ===================================================================== */
#ifndef PERSEO_CORE_PLAYER_H
#define PERSEO_CORE_PLAYER_H

#include <stdint.h>
#include <stdbool.h>
#include "fixed.h"
#include "level/level.h"

typedef struct PlayerAbilities {
    bool double_jump;
    bool dash;
    bool climb;
} PlayerAbilities;

typedef struct Player {
    fx_t x, y;             /* posición en píxeles, Q8.8 */
    fx_t vx, vy;           /* velocidad en píxeles/frame, Q8.8 */
    int16_t w, h;          /* caja de colisión: 10x13 px, igual que el prototipo */
    int8_t face;           /* -1 izquierda, 1 derecha */

    bool on_ground;
    int8_t coyote;         /* frames de gracia tras dejar el suelo */
    int8_t jump_buffer;    /* frames de gracia de un salto pulsado antes de aterrizar */
    bool can_double_jump;
    bool spun;             /* voltereta del Salto Doble en curso (dato para animación) */
    int8_t wall;           /* -1/0/1: lado del muro agarrado (Garra Felina) */

    int8_t dash_t;         /* frames restantes del dash actual (0 = no dasheando) */
    int8_t dash_cd;        /* frames de cooldown restantes */

    bool walking;           /* bandera para el ciclo de caminata (arte real en Fase 3) */
    uint16_t walk_anim;

    PlayerAbilities ab;
} Player;

/* Intenciones de input ya resueltas por la capa de plataforma (PAL) —
   ver docs/PLAN_MIGRACION_GBA_C.md, sección 4.2, principio 6:
   player.c no sabe nada de hardware ni de qué tecla/botón es cuál. */
typedef struct PlayerInput {
    bool left, right, up, down;   /* estado sostenido (held) */
    bool jump_held;                /* estado sostenido del botón de salto */
    bool jump_pressed;             /* flanco de subida (para el jump buffer) */
    bool dash_pressed;             /* flanco de subida */
} PlayerInput;

void player_init(Player *p, fx_t spawn_x, fx_t spawn_y);
void player_update(Player *p, const Level *lv, const PlayerInput *in);

#endif /* PERSEO_CORE_PLAYER_H */
