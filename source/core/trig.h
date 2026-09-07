/* =====================================================================
   trig.h — seno y raíz cuadrada en punto fijo
   =====================================================================
   La GBA no tiene FPU y core/ no puede usar las tablas de libtonc (eso
   sería una dependencia de plataforma), así que acá va lo mínimo que
   necesita la IA de enemigos: el vaivén senoidal de mosquitos y
   murciélagos, y la normalización del vector de embestida del mosquito.

   El ángulo se mide en 256 unidades por vuelta completa (no en
   radianes): así el índice de tabla sale con un simple AND y nunca hace
   falta dividir.

   Equivalencia con el prototipo: donde el original hacía sin(t/N), acá
   se hace fx_sin((t * FX_SIN_RATE(N)) >> 8).
   ===================================================================== */
#ifndef PERSEO_CORE_TRIG_H
#define PERSEO_CORE_TRIG_H

#include <stdint.h>
#include "fixed.h"

/* Convierte un divisor del prototipo (sin(t/N)) al multiplicador que
   lleva `t` a unidades de ángulo: 65536 / (N * 2π). */
#define FX_SIN_RATE(n) ((uint32_t)(65536.0 / ((n) * 6.283185307)))

/* Seno de un ángulo de 8 bits (0..255 = una vuelta), en Q8.8: -256..256. */
fx_t fx_sin(uint32_t angle8);

/* Raíz cuadrada entera. Se usa para normalizar el vector de embestida
   del mosquito, que en el prototipo era Math.hypot(). */
uint32_t isqrt32(uint32_t v);

#endif /* PERSEO_CORE_TRIG_H */
