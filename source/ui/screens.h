/* =====================================================================
   screens.h — qué se dibuja en cada estado de la partida
   =====================================================================
   El reparto es el mismo que ya usa el resto del port: core/game_state.c
   decide QUÉ pasa y este módulo decide CÓMO se ve. Nadie de acá toca
   registros: todo pasa por ui/text.h y por la PAL de vídeo.

   Una sola función pública a propósito. El bucle de main.c no debería
   tener un switch por estado repetido: ya hay uno en game_update() y dos
   copias de la misma lista se desincronizan a la primera.
   ===================================================================== */
#ifndef PERSEO_UI_SCREENS_H
#define PERSEO_UI_SCREENS_H

#include "../core/game_state.h"

void ui_screens_draw(const Game *g);

#endif /* PERSEO_UI_SCREENS_H */
