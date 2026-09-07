#ifndef PERSEO_UI_HUD_H
#define PERSEO_UI_HUD_H

#include "../core/game_state.h"

/* Dibuja el HUD sobre la capa de texto. La vida sigue siendo de sprites
   (la pinta pal_video_draw_world): un corazón por punto se lee mejor que
   una barra de tiles, y OAM ya los tenía. Acá va todo lo demás. */
void ui_hud_draw(const Game *g);

#endif /* PERSEO_UI_HUD_H */
