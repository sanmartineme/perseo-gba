/* =====================================================================
   entity_pool.h — pool estático de entidades
   =====================================================================
   Sin malloc/free en el loop de juego (docs/PLAN_MIGRACION_GBA_C.md,
   sección 4.2, principio 1): todo enemigo/proyectil vive en este array
   de tamaño fijo, igual de simple que LV.ents[] del prototipo pero sin
   asignación dinámica ni recolector de basura.
   ===================================================================== */
#ifndef PERSEO_CORE_ENTITY_POOL_H
#define PERSEO_CORE_ENTITY_POOL_H

#include "entity.h"

/* El nivel con más entidades del prototipo (buildLevel5, Madriguera
   Subterránea) tiene ~30 entre enemigos y decoración interactiva; se
   deja margen para los proyectiles y partículas que se suman en la
   Fase 4. Ajustar acá si algún nivel real (Fase 8) lo necesita. */
#define ENTITY_POOL_CAPACITY 64

void entity_pool_reset(void);

/* Devuelve NULL si el pool está lleno — el llamador decide qué hacer
   (en la práctica: no debería pasar nunca con el presupuesto de arriba,
   pero se revisa explícitamente en vez de desbordar silenciosamente). */
Entity *entity_pool_alloc(EntityType type);
void entity_pool_free(Entity *e);

/* Acceso directo por indice (0..ENTITY_POOL_CAPACITY-1), incluidas las
   ranuras libres: es lo que usan los bucles que recorren todo el pool
   cada frame, donde un callback por entidad saldria mas caro. */
Entity *entity_pool_at(int index);

#endif /* PERSEO_CORE_ENTITY_POOL_H */
