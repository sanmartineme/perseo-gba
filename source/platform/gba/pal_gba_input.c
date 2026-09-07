/* =====================================================================
   pal_gba_input.c — implementación de la PAL de input sobre libtonc
   =====================================================================
   Traducción de keys{}/IN{} del prototipo (docs/prototipo_referencia.html,
   línea ~1576) al hardware real: key_is_down() para estados sostenidos
   (mover, mantener salto) y key_hit() para flancos de subida (saltar,
   dashear — igual que jPress/dPress en el prototipo).

   Mapeo de botones (docs/PLAN_MIGRACION_GBA_C.md, sección 6):
   D-Pad=mover, A=saltar, B=Ganchito Mortal, R=Dash Sombrío,
   L=Lanzamiento de Traza, Select=inventario, Start=pausa.
   ===================================================================== */
#include <tonc.h>
#include "../pal.h"

void pal_input_poll(void) { key_poll(); }

bool pal_input_left(void)  { return key_is_down(KEY_LEFT)  != 0; }
bool pal_input_right(void) { return key_is_down(KEY_RIGHT) != 0; }
bool pal_input_up(void)    { return key_is_down(KEY_UP)    != 0; }
bool pal_input_down(void)  { return key_is_down(KEY_DOWN)  != 0; }

bool pal_input_jump_held(void)    { return key_is_down(KEY_A) != 0; }
bool pal_input_jump_pressed(void) { return key_hit(KEY_A) != 0; }

bool pal_input_attack_pressed(void) { return key_hit(KEY_B) != 0; }
bool pal_input_dash_pressed(void)   { return key_hit(KEY_R) != 0; }
bool pal_input_throw_pressed(void)  { return key_hit(KEY_L) != 0; }

bool pal_input_start_pressed(void)  { return key_hit(KEY_START)  != 0; }
bool pal_input_select_pressed(void) { return key_hit(KEY_SELECT) != 0; }

bool pal_input_up_pressed(void)    { return key_hit(KEY_UP)    != 0; }
bool pal_input_down_pressed(void)  { return key_hit(KEY_DOWN)  != 0; }
bool pal_input_left_pressed(void)  { return key_hit(KEY_LEFT)  != 0; }
bool pal_input_right_pressed(void) { return key_hit(KEY_RIGHT) != 0; }
bool pal_input_confirm_pressed(void) { return key_hit(KEY_A) != 0; }
bool pal_input_cancel_pressed(void)  { return key_hit(KEY_B) != 0; }
