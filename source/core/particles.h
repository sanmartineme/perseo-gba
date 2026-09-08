/* =====================================================================
   particles.h — pool de partículas
   =====================================================================
   Traducción de particles[] / burst() del prototipo: chispas que salen
   al golpear, al recoger algo o al reventar una traza. Física trivial
   (velocidad + gravedad + vida) y pool estático, sin asignación.

   El color se guarda como una clave de la paleta lógica del prototipo
   ('R' rojo, 'Y' dorado, 'W' blanco...) en vez de un color concreto:
   así core/ no necesita saber nada de RGB15 ni de bancos de paleta —
   eso lo resuelve la capa de vídeo.
   ===================================================================== */
#ifndef PERSEO_CORE_PARTICLES_H
#define PERSEO_CORE_PARTICLES_H

#include <stdint.h>
#include <stdbool.h>
#include "fixed.h"

/* Cuántas caben a la vez. El prototipo llegaba a ~40 en un golpe de
   jefe; acá el techo real lo pone el hardware: cada partícula visible
   gasta un objeto de OAM, y sólo hay 128 para todo. */
#define PARTICLE_CAPACITY 32

/* Colores usados por los burst() del prototipo. */
typedef enum ParticleColor {
    PCOL_RED = 0,    /* 'R' — sangre/daño */
    PCOL_GOLD,       /* 'Y' — monedas, checkpoints */
    PCOL_WHITE,      /* 'W' — impacto de garra */
    PCOL_BROWN,      /* 'o' — traza */
    PCOL_GREY,       /* 'G'/'N' — polvo, chatarra */
    PCOL_CYAN,       /* 'C' — el goteo de humedad del fondo */
    PCOL_COUNT
} ParticleColor;

typedef struct Particle {
    fx_t x, y, vx, vy;
    int16_t life;
    uint8_t color;
    bool alive;
} Particle;

void particles_reset(void);
void particles_update(void);

/* Equivalente de burst(x, y, col, n, pow) del prototipo. */
void particles_burst(fx_t x, fx_t y, ParticleColor color, int count, fx_t power);

int particles_count(void);
const Particle *particles_get(int index);

#endif /* PERSEO_CORE_PARTICLES_H */
