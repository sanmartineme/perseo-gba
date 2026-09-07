/* =====================================================================
   rng.h — generador de números pseudoaleatorios
   =====================================================================
   El prototipo usaba Math.random() en varios lugares (cadencia de tiro
   del gunner, elección de ataque de los jefes, si un enemigo suelta un
   corazón al morir). Acá va un xorshift de 32 bits: determinista,
   barato y sin dependencias — importa que sea barato porque se llama
   dentro del bucle de entidades.
   ===================================================================== */
#ifndef PERSEO_CORE_RNG_H
#define PERSEO_CORE_RNG_H

#include <stdint.h>

void rng_seed(uint32_t seed);
uint32_t rng_next(void);

/* Entero en [0, n). */
uint32_t rng_below(uint32_t n);

/* true con probabilidad percent/100 — el equivalente legible de los
   "Math.random() < 0.25" del prototipo. */
int rng_chance(uint32_t percent);

#endif /* PERSEO_CORE_RNG_H */
