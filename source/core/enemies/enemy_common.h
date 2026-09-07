/* =====================================================================
   enemy_common.h — IA de enemigos
   =====================================================================
   Un archivo por tipo, como pide docs/PLAN_MIGRACION_GBA_C.md: cada uno
   es la traducción del `case` correspondiente de updateEnts() en el
   prototipo. Lo que comparten (patrulla, contacto, daño) vive en
   core/world.c para no repetirlo siete veces.
   ===================================================================== */
#ifndef PERSEO_CORE_ENEMY_COMMON_H
#define PERSEO_CORE_ENEMY_COMMON_H

#include "../entity.h"
#include "../world.h"

void enemy_rat_update(Entity *e, World *w);
void enemy_roach_update(Entity *e, World *w);
void enemy_mosq_update(Entity *e, World *w);
void enemy_bat_update(Entity *e, World *w);
void enemy_thug_update(Entity *e, World *w);
void enemy_gunner_update(Entity *e, World *w);
void enemy_brute_update(Entity *e, World *w);

/* Vida inicial de cada tipo, tal como la fijaba el prototipo en su
   inicialización perezosa. */
#define HP_RAT    2
#define HP_ROACH  1
#define HP_MOSQ   1
#define HP_BAT    2
#define HP_THUG   4
#define HP_GUNNER 2
#define HP_BRUTE  5

#endif /* PERSEO_CORE_ENEMY_COMMON_H */
