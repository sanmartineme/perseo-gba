#include "particles.h"
#include "rng.h"
#include <string.h>

#define PARTICLE_GRAVITY FX_C(0.1)

static Particle s_pool[PARTICLE_CAPACITY];

void particles_reset(void) {
    memset(s_pool, 0, sizeof(s_pool));
}

void particles_update(void) {
    for (int i = 0; i < PARTICLE_CAPACITY; i++) {
        Particle *p = &s_pool[i];
        if (!p->alive) continue;
        p->x = fx_add(p->x, p->vx);
        p->y = fx_add(p->y, p->vy);
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
        if (!p) return; /* pool lleno: se pierden las que sobran, igual que
                           el prototipo perdía las que salían de pantalla */
        p->alive = true;
        p->x = x;
        p->y = y;
        /* (random-0.5)*pow en horizontal y (random-0.9)*pow en vertical:
           el sesgo hacia arriba del original hace que la chispa "salte". */
        p->vx = fx_mul(power, (fx_t)(rng_below(256) - 128));
        p->vy = fx_mul(power, (fx_t)(rng_below(256) - 230));
        p->life = 20 + (int16_t)rng_below(15);
        p->color = (uint8_t)color;
    }
}

int particles_count(void) { return PARTICLE_CAPACITY; }
const Particle *particles_get(int index) { return &s_pool[index]; }
