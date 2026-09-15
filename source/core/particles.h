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
#define PARTICLE_CAPACITY 48  /* Aumentado de 32 para soportar efectos más ricos */

/* Colores usados por los burst() del prototipo y mejorados (Sept 2026). */
typedef enum ParticleColor {
    PCOL_RED = 0,    /* 'R' — sangre/daño */
    PCOL_GOLD,       /* 'Y' — monedas, checkpoints, recompensas */
    PCOL_WHITE,      /* 'W' — impacto de garra, chispas de metal */
    PCOL_BROWN,      /* 'o' — traza, tierra, polvo industrial */
    PCOL_GREY,       /* 'G'/'N' — polvo, chatarra, escombros */
    PCOL_CYAN,       /* 'C' — el goteo de humedad del fondo */
    PCOL_GREEN,      /* Verde — veneno, ácido, efectos tóxicos */
    PCOL_PURPLE,     /* Púrpura — magia oscura, energía */
    PCOL_ORANGE,     /* Naranja — fuego, calor, explosión */
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

/* Nuevos efectos visuales mejorados (Sept 2026) — cada uno tiene patrón y poder específico */
void particles_impact_claw(fx_t x, fx_t y);      /* Impacto de garra: blanco/rojo, explosión corta */
void particles_impact_traza(fx_t x, fx_t y);     /* Impacto de Traza: marrón/naranja */
void particles_death_enemy(fx_t x, fx_t y);      /* Muerte de enemigo: gris + polvo */
void particles_electric(fx_t x, fx_t y);         /* Chispa eléctrica: cyan/blanco */
void particles_poison(fx_t x, fx_t y);           /* Veneno/gas: verde purpurino */

int particles_count(void);
const Particle *particles_get(int index);

#endif /* PERSEO_CORE_PARTICLES_H */
