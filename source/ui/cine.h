/* =====================================================================
   cine.h — las ilustraciones de las viñetas
   =====================================================================
   La introducción y el desenlace no son bloques de texto: cada viñeta
   lleva una escena dibujada, igual que en el prototipo. Acá se componen
   con los personajes del juego puestos a mano, y el cielo nocturno que
   los acompaña.

   Vive en ui/ y no en platform/ porque decide QUÉ se ve; el CÓMO —
   sprites, OAM, bancos de tiles — lo resuelve pal_video_cine_*().
   ===================================================================== */
#ifndef PERSEO_UI_CINE_H
#define PERSEO_UI_CINE_H

#include "../core/game_state.h"
#include "../platform/pal.h"

/* La línea del suelo de las viñetas: los personajes se apoyan acá y el
   texto empieza justo debajo. La comparten cine.c y screens.c. */
#define CINE_GROUND_Y 88

/* ¿Esta pantalla lleva ilustración? Las historias de nivel son texto
   solo (el prototipo tampoco las ilustra); la intro y el final, no. */
bool ui_cine_has_scene(const Game *g);

/* Qué fondo pide la viñeta actual: la calle de noche, el trono, o
   ninguno (las historias de nivel, que no llevan ilustración). */
CineBackdrop ui_cine_backdrop(const Game *g);

/* Dibuja la escena de la viñeta actual: cielo, decorado y personajes.
   Sustituye al dibujo del mundo mientras dura la cinemática. */
void ui_cine_draw(const Game *g);

#endif /* PERSEO_UI_CINE_H */
