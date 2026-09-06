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
int entity_pool_count(void);

typedef bool (*EntityVisitor)(Entity *e, void *ctx);
/* Recorre las entidades vivas; visit puede devolver false para cortar
   la iteración antes de tiempo (ej. al encontrar el primer impacto). */
void entity_pool_for_each(EntityVisitor visit, void *ctx);

#endif /* PERSEO_CORE_ENTITY_POOL_H */
