/* =====================================================================
   relics.h — las tres reliquias equipables
   =====================================================================
   Traducción de RELICS[] del prototipo ([index.html:950]). Se guardan
   como máscara de bits en el Player (encontradas y equipadas): son tres,
   caben en un byte cada una, y así el guardado en SRAM de F7-11 no tiene
   que serializar nada más que dos enteros.

   La regla que las hace interesantes ya estaba en el original y se
   conserva: **sólo se pueden llevar dos de las tres a la vez**, así que
   hay que elegir entre pegar más, aguantar más o regenerar.
   ===================================================================== */
#ifndef PERSEO_CORE_RELICS_H
#define PERSEO_CORE_RELICS_H

#include <stdint.h>
#include <stdbool.h>

typedef enum RelicId {
    RELIC_COLMILLO = 0,  /* +1 de daño en cada Ganchito Mortal */
    RELIC_PATA,          /* regenera 1 corazón tras 12 s sin recibir daño */
    RELIC_BIGOTES,       /* -1 de daño recibido (mínimo 1) */
    RELIC_COUNT
} RelicId;

#define RELIC_BIT(id)       ((uint8_t)(1u << (id)))
#define RELIC_MAX_EQUIPPED  2

typedef struct RelicDef {
    const char *name;
    const char *desc;
} RelicDef;

extern const RelicDef RELICS[RELIC_COUNT];

/* Cuántas hay equipadas ahora mismo (equippedCount() del prototipo). */
int relic_equipped_count(uint8_t equipped_mask);

#endif /* PERSEO_CORE_RELICS_H */
