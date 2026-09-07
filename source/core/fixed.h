/* =====================================================================
   fixed.h — aritmética de punto fijo Q8.8
   =====================================================================
   La GBA (ARM7TDMI) no tiene FPU: toda la física del juego usa este tipo
   en vez de float/double (ver docs/PLAN_MIGRACION_GBA_C.md, sección 4.2,
   principio 3). Q8.8 = 8 bits de parte entera con signo + 8 bits de
   fracción, dentro de un int32_t: 1/256 de precisión, rango útil de
   posición/velocidad muy por encima de lo que necesita un mundo de unos
   pocos miles de píxeles.

   Header-only a propósito: son operaciones triviales que el compilador
   debe poder inlinear en el hot path de la física (Fase 9 puede revisar
   esto con perfilado real, pero no hay razón para no partir así).
   ===================================================================== */
#ifndef PERSEO_CORE_FIXED_H
#define PERSEO_CORE_FIXED_H

#include <stdint.h>

typedef int32_t fx_t;

#define FX_SHIFT 8
#define FX_ONE   (1 << FX_SHIFT)

/* Convierte un literal decimal legible a Q8.8 en tiempo de compilación
   (ej. FX_C(1.35)). El compilador pliega esto a una constante entera —
   no genera ninguna operación de punto flotante en tiempo de ejecución.
   Sólo para inicializar constantes; en runtime usar siempre las
   operaciones de abajo. */
#define FX_C(literal) ((fx_t)((literal) * FX_ONE + ((literal) >= 0 ? 0.5 : -0.5)))

static inline fx_t fx_from_int(int32_t i) { return (fx_t)(i << FX_SHIFT); }

/* Trunca hacia -infinito (desplazamiento aritmético), no hacia 0 — es
   lo que hace falta para indexar tiles correctamente con coordenadas
   negativas (borde izquierdo/superior del mundo). */
static inline int32_t fx_to_int(fx_t f) { return (int32_t)(f >> FX_SHIFT); }

static inline fx_t fx_add(fx_t a, fx_t b) { return a + b; }
static inline fx_t fx_sub(fx_t a, fx_t b) { return a - b; }
static inline fx_t fx_neg(fx_t a)         { return -a; }

static inline fx_t fx_mul(fx_t a, fx_t b) {
    return (fx_t)(((int64_t)a * (int64_t)b) >> FX_SHIFT);
}
static inline fx_t fx_div(fx_t a, fx_t b) {
    return (fx_t)(((int64_t)a << FX_SHIFT) / b);
}

static inline fx_t fx_abs(fx_t a)               { return a < 0 ? -a : a; }
static inline fx_t fx_min(fx_t a, fx_t b)       { return a < b ? a : b; }
static inline fx_t fx_max(fx_t a, fx_t b)       { return a > b ? a : b; }
static inline fx_t fx_clamp(fx_t v, fx_t lo, fx_t hi) { return fx_min(fx_max(v, lo), hi); }

#endif /* PERSEO_CORE_FIXED_H */
