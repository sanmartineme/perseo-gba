#include "save.h"

/* El checksum cubre desde el byte siguiente a sí mismo hasta el final,
   así que se calcula igual antes y después de sellarlo. */
#define CHECKSUM_OFFSET (sizeof(uint16_t) + 2 * sizeof(uint8_t))

uint8_t save_checksum(const SaveData *s) {
    const uint8_t *p = (const uint8_t *)s;
    uint8_t sum = 0x5A;   /* semilla: un bloque de ceros no da checksum 0 */
    for (unsigned i = CHECKSUM_OFFSET; i < sizeof(SaveData); i++) {
        sum = (uint8_t)(sum + p[i]);
        sum = (uint8_t)((sum << 1) | (sum >> 7));  /* rota: detecta bytes cambiados de sitio */
    }
    return sum;
}

void save_seal(SaveData *s) {
    s->magic = SAVE_MAGIC;
    s->version = SAVE_VERSION;
    s->checksum = save_checksum(s);
}

bool save_is_valid(const SaveData *s) {
    return s->magic == SAVE_MAGIC &&
           s->version == SAVE_VERSION &&
           s->checksum == save_checksum(s);
}
