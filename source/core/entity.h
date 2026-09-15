/* =====================================================================
   entity.h — entidad genérica de nivel
   =====================================================================
   Traducción del catálogo de `type` que el prototipo pasa a E(L, tipo,
   ...) (ver docs/PLAN_MIGRACION_GBA_C.md, sección 1, "Catálogo de
   entidades de nivel") a un enum — mucho más barato en tiempo y memoria
   que comparar strings en cada frame, algo que en JS no importaba pero
   en un ARM7TDMI a 16.78MHz sí.

   El jugador NO es una Entity: tiene su propio struct (core/player.h)
   porque es un singleton con muchos más campos, igual que el prototipo
   separa P{} de LV.ents[].

   Esta struct recién se usa de verdad a partir de la Fase 4 (enemigos,
   proyectiles) — se introduce ahora, junto con el pool (entity_pool.h),
   para fijar el patrón de memoria estática desde el principio.
   ===================================================================== */
#ifndef PERSEO_CORE_ENTITY_H
#define PERSEO_CORE_ENTITY_H

#include <stdint.h>
#include <stdbool.h>
#include "fixed.h"

struct LevelEntitySpawn;

typedef enum EntityType {
    ENT_NONE = 0,
    ENT_SIGN, ENT_LAMP, ENT_DOOR, ENT_VDOOR, ENT_SHRINE, ENT_BOSSGATE,
    ENT_BOSS, ENT_CHAPA, ENT_RELIC, ENT_HP,
    ENT_RAT, ENT_ROACH, ENT_MOSQ, ENT_GUNNER, ENT_BAT, ENT_THUG, ENT_BRUTE,
    ENT_PROJ_TRAZA, ENT_PROJ_JUNK, ENT_PROJ_SHOCK,
    ENT_PARTICLE,
    ENT_TYPE_COUNT
} EntityType;

typedef struct Entity {
    EntityType type;
    bool alive;
    bool started;    /* ya corrió su inicialización perezosa (el
                        "if (e.dir===undefined)" del prototipo, que hacía
                        falta porque las entidades nacen sólo con posición) */
    fx_t x, y;       /* posición en píxeles, Q8.8 */
    fx_t vx, vy;     /* velocidad en píxeles/frame, Q8.8 */
    fx_t home_x, home_y; /* posición de origen: la usan los enemigos
                            voladores para orbitar y volver a su puesto */
    int16_t w, h;    /* caja de colisión, en píxeles enteros */
    int8_t face;     /* -1 o 1: hacia dónde mira */
    int8_t dir;      /* -1 o 1: dirección de patrulla */
    int16_t hp;
    int16_t state;   /* estado de FSM propio de cada tipo (enemigo/jefe) */
    int16_t timer;   /* contador de frames genérico */
    int16_t cooldown;/* segundo contador: cadencia de tiro, respiro tras embestir */
    int16_t flash;   /* frames restantes de destello blanco al ser golpeado */
    int16_t param;   /* dato que viene de los datos del nivel (LevelEntitySpawn) */
    bool friendly;   /* proyectiles: true si lo lanzó Perseo */
    uint16_t hit_swing; /* id del último zarpazo que ya le pegó: impide que
                           un mismo golpe cuente varias veces (el
                           e.hitSwing del prototipo) */
    int8_t next_attack, last_attack; /* jefes: ataque elegido y el anterior,
                                        para no repetir dos veces seguidas */
    /* Datos del nivel de los que nació esta entidad. Guardar el puntero
       (4 bytes) sale mucho más barato que copiarle los rectángulos de la
       puerta de arena a cada una de las 64 ranuras del pool. */
    const struct LevelEntitySpawn *spawn;
} Entity;

#endif /* PERSEO_CORE_ENTITY_H */
