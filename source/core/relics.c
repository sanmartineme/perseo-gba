#include "relics.h"

const RelicDef RELICS[RELIC_COUNT] = {
    [RELIC_COLMILLO] = { "COLMILLO AFILADO", "+1 de daño en cada Ganchito Mortal." },
    [RELIC_PATA]     = { "PATA DE LA SUERTE", "Regenera 1 corazón tras 12 s sin recibir daño." },
    [RELIC_BIGOTES]  = { "BIGOTES DE ACERO", "Reduce en 1 el daño de cualquier golpe (mín. 1)." },
};

int relic_equipped_count(uint8_t equipped_mask) {
    int n = 0;
    for (int i = 0; i < RELIC_COUNT; i++) {
        if (equipped_mask & RELIC_BIT(i)) n++;
    }
    return n;
}
