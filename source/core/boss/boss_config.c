#include "boss_config.h"

/* Valores tomados uno a uno de BOSS_CONFIG{} del prototipo. La vida sube
   en cada encuentro (16 -> 20 -> 22 -> 25 -> 28) y Betty se despega con
   46: es el doble largo de la primera pelea. */
const BossConfig BOSS_CONFIGS[BOSS_COUNT] = {
    [BOSS_CAPATAZ] = { "EL CAPATAZ", "CAPATAZ DERROTADO",
        "La puerta al fondo tiembla... Betty sabe que vienes.",
        16, 29, 21, 0, false, 2 },
    [BOSS_REVISOR] = { "EL REVISOR", "REVISOR DERROTADO",
        "Los túneles se abren hacia una zona que apesta a químicos.",
        20, 29, 22, 1, false, 3 },
    [BOSS_TOXICO]  = { "EL TÓXICO", "CRIATURA VENCIDA",
        "Más allá, gruñidos hostiles anuncian la Madriguera de Betty.",
        22, 29, 22, 2, false, 4 },
    [BOSS_MATON]   = { "EL GUARDIÁN", "GUARDIÁN CAÍDO",
        "El Mercado Negro está cerca. Y con él, respuestas sobre Aurorita.",
        25, 29, 22, 3, false, 5 },
    [BOSS_GUARDIA] = { "EL GUARDIA", "GUARDIA ABATIDO",
        "Una jaula vacía. Un aroma familiar. Betty se la llevó a su Trono.",
        28, 29, 22, 4, false, 6 },
    [BOSS_BETTY]   = { "BETTY", "¡BETTY HA CAÍDO!",
        "El reinado de Silencio se termina. Al fondo, una jaula espera ser abierta.",
        46, 31, 22, 5, true, 0 },
};
