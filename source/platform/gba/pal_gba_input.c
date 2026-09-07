/* =====================================================================
   pal_gba_input.c — implementación de la PAL de input sobre libtonc
   =====================================================================
   Traducción de keys{}/IN{} del prototipo (docs/prototipo_referencia.html,
   línea ~1576) al hardware real: key_is_down() para estados sostenidos
   (mover, mantener salto) y key_hit() para flancos de subida (saltar,
   dashear — igual que jPress/dPress en el prototipo).

   Mapeo de botones POR DEFECTO (docs/PLAN_MIGRACION_GBA_C.md, sección 6):
   D-Pad=mover, A=saltar, B=Ganchito Mortal, R=Dash Sombrío,
   L=Lanzamiento de Traza, Select=inventario, Start=pausa. Las cuatro
   acciones de juego se pueden reasignar desde OPCIONES; el resto no.
   ===================================================================== */
#include <tonc.h>
#include "../pal.h"

/* ---------- Mapeo de botones ----------
   El reparto por defecto es el de la seccion 6 del plan: A saltar,
   B Ganchito, R Dash, L Traza. Se puede cambiar desde OPCIONES, y por eso
   vive en una tabla y no en cada funcion. */
static const uint16_t PAD_KEY[PAD_COUNT] = { KEY_A, KEY_B, KEY_L, KEY_R };
static const char *const PAD_NAME[PAD_COUNT] = { "A", "B", "L", "R" };

static uint8_t s_bind[ACT_COUNT] = { PAD_A, PAD_B, PAD_R, PAD_L };

static uint16_t key_of(InputAction act) { return PAD_KEY[s_bind[act]]; }

const char *pal_input_button_name(PadButton b) {
    return b < PAD_COUNT ? PAD_NAME[b] : "?";
}

PadButton pal_input_binding(InputAction act) {
    return act < ACT_COUNT ? (PadButton)s_bind[act] : PAD_A;
}

void pal_input_set_bindings(const uint8_t *four) {
    uint8_t seen = 0;
    for (int i = 0; i < ACT_COUNT; i++) {
        if (four[i] >= PAD_COUNT) return;
        uint8_t bit = (uint8_t)(1u << four[i]);
        if (seen & bit) return;      /* repetido: no es permutacion */
        seen |= bit;
    }
    for (int i = 0; i < ACT_COUNT; i++) s_bind[i] = four[i];
}

void pal_input_bind(InputAction act, PadButton btn) {
    if (act >= ACT_COUNT || btn >= PAD_COUNT) return;
    /* Intercambio en vez de asignacion: si el boton ya era de otra accion,
       esa se queda con el que suelta esta. El mapeo no puede degenerar. */
    for (int i = 0; i < ACT_COUNT; i++) {
        if (s_bind[i] == btn) { s_bind[i] = s_bind[act]; break; }
    }
    s_bind[act] = (uint8_t)btn;
}

void pal_input_poll(void) { key_poll(); }

bool pal_input_left(void)  { return key_is_down(KEY_LEFT)  != 0; }
bool pal_input_right(void) { return key_is_down(KEY_RIGHT) != 0; }
bool pal_input_up(void)    { return key_is_down(KEY_UP)    != 0; }
bool pal_input_down(void)  { return key_is_down(KEY_DOWN)  != 0; }

bool pal_input_jump_held(void)    { return key_is_down(key_of(ACT_JUMP)) != 0; }
bool pal_input_jump_pressed(void) { return key_hit(key_of(ACT_JUMP)) != 0; }

bool pal_input_attack_pressed(void) { return key_hit(key_of(ACT_ATTACK)) != 0; }
bool pal_input_dash_pressed(void)   { return key_hit(key_of(ACT_DASH)) != 0; }
bool pal_input_throw_pressed(void)  { return key_hit(key_of(ACT_THROW)) != 0; }

bool pal_input_start_pressed(void)  { return key_hit(KEY_START)  != 0; }
bool pal_input_select_pressed(void) { return key_hit(KEY_SELECT) != 0; }

bool pal_input_up_pressed(void)    { return key_hit(KEY_UP)    != 0; }
bool pal_input_down_pressed(void)  { return key_hit(KEY_DOWN)  != 0; }
bool pal_input_left_pressed(void)  { return key_hit(KEY_LEFT)  != 0; }
bool pal_input_right_pressed(void) { return key_hit(KEY_RIGHT) != 0; }
bool pal_input_confirm_pressed(void) { return key_hit(KEY_A) != 0; }
bool pal_input_cancel_pressed(void)  { return key_hit(KEY_B) != 0; }

bool pal_input_debug_held(void) {
    return key_is_down(KEY_START) && key_is_down(KEY_SELECT);
}
