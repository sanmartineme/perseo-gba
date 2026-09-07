/* =====================================================================
   projectiles.h — proyectiles (traza de Perseo, chatarra, onda de choque)
   =====================================================================
   Traducción de projs[] del prototipo. A diferencia del original, que
   tenía su propio array, acá los proyectiles son Entity del pool común
   (core/entity_pool.h): un solo pool, un solo camino de dibujado.

   Los tres tipos del prototipo:
     - traza  : la que lanza Perseo. Vuela recta, muere contra muros y
                revienta contra el primer enemigo. Nunca daña al jugador.
     - junk   : chatarra que arquean los gunners y los jefes (con gravedad).
     - shock  : onda que corre pegada al suelo tras el pisotón de un jefe.
   ===================================================================== */
#ifndef PERSEO_CORE_PROJECTILES_H
#define PERSEO_CORE_PROJECTILES_H

#include "entity.h"
#include "level/level.h"

struct World;

Entity *proj_spawn_traza(fx_t x, fx_t y, int8_t face);
Entity *proj_spawn_junk(fx_t x, fx_t y, fx_t vx, fx_t vy);
Entity *proj_spawn_shock(fx_t x, fx_t y, fx_t vx);

/* Avanza un proyectil un frame; lo mata si choca, caduca o impacta. */
void proj_update(Entity *e, struct World *w);

#endif /* PERSEO_CORE_PROJECTILES_H */
