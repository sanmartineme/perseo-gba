#include "rng.h"

static uint32_t s_state = 0x2545F491u;

void rng_seed(uint32_t seed) {
    s_state = seed ? seed : 0x2545F491u; /* 0 se atasca en xorshift */
}

uint32_t rng_next(void) {
    uint32_t x = s_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    s_state = x;
    return x;
}

uint32_t rng_below(uint32_t n) {
    return n ? (rng_next() % n) : 0;
}

int rng_chance(uint32_t percent) {
    return rng_below(100) < percent;
}
