#include "boss_config.h"

/* Valores tomados uno a uno de BOSS_CONFIG{} del prototipo. La vida sube
   en cada encuentro (16 -> 20 -> 22 -> 25 -> 28) y Betty se despega con
   46: es el doble largo de la primera pelea. */
const BossConfig BOSS_CONFIGS[BOSS_COUNT] = {
    /*                 hp   w   h  sprite  final  siguiente nivel */
    [BOSS_CAPATAZ] = { 16, 29, 21,      0, false, 2 },
    [BOSS_REVISOR] = { 20, 29, 22,      1, false, 3 },
    [BOSS_TOXICO]  = { 22, 29, 22,      2, false, 4 },
    [BOSS_MATON]   = { 25, 29, 22,      3, false, 5 },
    [BOSS_GUARDIA] = { 28, 29, 22,      4, false, 6 },
    [BOSS_BETTY]   = { 46, 31, 22,      5, true,  0 },
};
