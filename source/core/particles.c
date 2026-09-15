#include "particles.h"
#include "rng.h"
#include <string.h>

/* Gravedad mejorada (Sept 2026): 0.1 → 0.08 para caídas más suaves,
   con fricción del aire para que las partículas se desaceleren naturalmente. */
#define PARTICLE_GRAVITY  FX_C(0.08)
#define PARTICLE_FRICTION FX_C(0.97)  /* fricción del aire: cada frame pierde 3% de velocidad */

static Particle s_pool[PARTICLE_CAPACITY];

void particles_reset(void) {
    memset(s_pool, 0, sizeof(s_pool));
}

void particles_update(void) {
    for (int i = 0; i < PARTICLE_CAPACITY; i++) {
        Particle *p = &s_pool[i];
        if (!p->alive) continue;

        /* Movimiento mejorado: fricción del aire realista */
        p->x = fx_add(p->x, p->vx);
        p->y = fx_add(p->y, p->vy);

        /* Fricción: desacelera las partículas (efecto de resistencia) */
        p->vx = fx_mul(p->vx, PARTICLE_FRICTION);
        p->vy = fx_mul(p->vy, PARTICLE_FRICTION);

        /* Gravedad */
        p->vy = fx_add(p->vy, PARTICLE_GRAVITY);

        if (--p->life <= 0) p->alive = false;
    }
}

void particles_burst(fx_t x, fx_t y, ParticleColor color, int count, fx_t power) {
    for (int n = 0; n < count; n++) {
        Particle *p = 0;
        for (int i = 0; i < PARTICLE_CAPACITY; i++) {
            if (!s_pool[i].alive) { p = &s_pool[i]; break; }
        }
        if (!p) return;

        p->alive = true;
        p->x = x;
        p->y = y;

        /* Explosión mejorada (Sept 2026):
           - Horizontal: dispersión uniforme (-0.5 a 0.5)
           - Vertical: impulso hacia arriba fuerte (-0.9 a -0.3)
           - Resultado: efecto de golpe explosivo, no solo chispas */
        int32_t angle = rng_below(256);  /* 0-255 = 0-360 grados */
        fx_t h_spread = fx_mul(power, (fx_t)(rng_below(256) - 128));
        fx_t v_spread = fx_mul(power, (fx_t)(rng_below(256) - 230));

        p->vx = h_spread;
        p->vy = v_spread;

        /* Duración mejorada: 20-35 frames (~0.33-0.58s a 60 FPS).
           Más tiempo hace que las explosiones sean más visibles y elegantes. */
        p->life = 20 + (int16_t)rng_below(15);
        p->color = (uint8_t)color;
    }
}

/* Nuevos efectos visuales mejorados (Sept 2026) */

void particles_impact_claw(fx_t x, fx_t y) {
    /* Impacto de garra: explosión pequeña pero impactante
       Colores: blanco (chispa) + rojo (rasgadura) */
    particles_burst(x, y, PCOL_WHITE, 3, FX_C(1.2));  /* Chispas de metal */
    particles_burst(x, y, PCOL_RED, 2, FX_C(1.0));    /* Rasgadura */
}

void particles_impact_traza(fx_t x, fx_t y) {
    /* Impacto de Traza: explosión de energía marrón/naranja
       Más particles y más poder que la garra */
    particles_burst(x, y, PCOL_BROWN, 4, FX_C(1.5));   /* Polvo y energía */
    particles_burst(x, y, PCOL_ORANGE, 2, FX_C(1.3));  /* Chispa caliente */
}

void particles_death_enemy(fx_t x, fx_t y) {
    /* Muerte de enemigo: nube de polvo gris + desintegración
       Efecto: el enemigo se desmorona en polvo */
    particles_burst(x, y, PCOL_GREY, 6, FX_C(2.0));    /* Polvo grande */
    particles_burst(x, y, PCOL_BROWN, 3, FX_C(1.5));   /* Restos */
}

void particles_electric(fx_t x, fx_t y) {
    /* Chispa eléctrica: cyan/blanco en patrón radial
       Uso: trampas, enemigos eléctricos, efectos de corriente */
    particles_burst(x, y, PCOL_CYAN, 5, FX_C(1.8));    /* Electricidad */
    particles_burst(x, y, PCOL_WHITE, 2, FX_C(1.5));   /* Núcleo brillante */
}

void particles_poison(fx_t x, fx_t y) {
    /* Veneno/gas: verde + púrpura en movimiento lento
       Uso: trampas de veneno, aliento tóxico */
    particles_burst(x, y, PCOL_GREEN, 4, FX_C(0.8));   /* Gas denso, lento */
    particles_burst(x, y, PCOL_PURPLE, 2, FX_C(0.6));  /* Neblina púrpura */
}

int particles_count(void) { return PARTICLE_CAPACITY; }
const Particle *particles_get(int index) { return &s_pool[index]; }
