/* =====================================================================
   world.h — el mundo en juego: nivel + jugador + entidades vivas
   =====================================================================
   El prototipo trabajaba con globales (LV, P, projs, particles, shake).
   Acá se agrupan en un struct que se pasa explícitamente: el mismo
   código queda testeable y sin estado oculto, sin cambiar la lógica.

   Es también el dueño del ciclo de actualización: mueve al jugador,
   despacha la IA de cada entidad viva (source/core/enemies/) y resuelve
   los choques entre unos y otros.
   ===================================================================== */
#ifndef PERSEO_CORE_WORLD_H
#define PERSEO_CORE_WORLD_H

#include "entity.h"
#include "player.h"
#include "level/level.h"

typedef struct Rect {
    fx_t x, y;
    int16_t w, h;
} Rect;

typedef struct World {
    /* Apunta a level_rt: todo el juego lee el tilemap por acá. */
    const Level *lv;
    /* Copia mutable del nivel. Los tilemaps generados viven en ROM, pero
       el juego necesita poder cambiarlos: romper una rejilla oxidada con
       el dash, sellar la arena de un jefe, abrir el paso al vencerlo. Se
       copia el nivel entero a RAM (8.8 KB de los 256 KB de EWRAM) en vez
       de llevar una lista de parches: sale más simple y no cuesta nada en
       el camino caliente de la colisión. */
    Level level_rt;
    Player player;
    uint32_t tick;
    int16_t shake;      /* sacudida de pantalla pendiente (la consume la cámara) */
    int16_t chapas;     /* monedas recogidas */

    /* Encuentro de jefe en curso. `boss_active` distingue al jefe
       dormido en su arena del que ya esta peleando: hasta que el
       jugador cruza el disparador, el jefe no se mueve ni hace dano. */
    Entity *boss;
    Entity *boss_gate;
    bool boss_active;

    /* Escala de daño de la dificultad elegida, como fracción entera
       (ver DIFF_CFGS en core/game_state.c). Vive acá y no en Game para
       que world_damage_player() no necesite conocer la partida entera;
       la fija game_load_level(). */
    uint8_t dmg_num, dmg_den;
} World;

void world_load(World *w, const Level *lv);

/* Cambia tiles del nivel en marcha (rejillas rotas, puertas de arena). */
void world_carve(World *w, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t id);
void world_update(World *w, const PlayerInput *in);

/* ---------- Helpers compartidos por la IA de enemigos ---------- */

bool rect_overlap(Rect a, Rect b);
Rect entity_box(const Entity *e);
Rect player_box(const Player *p);

/* Primer tile sólido bajo un punto; devuelve su borde superior, en
   píxeles. Equivale a groundY() del prototipo, y lo usan los enemigos
   terrestres para apoyarse en el suelo al aparecer. */
fx_t world_ground_y(const World *w, fx_t px, fx_t py);

/* Patrulla de ida y vuelta: avanza, y se da la vuelta si choca contra un
   muro o si se le acaba el piso. Es el comportamiento base de todos los
   enemigos terrestres del prototipo, que sólo se diferencian en la
   velocidad, el ancho y a qué altura miran. */
void world_patrol(const World *w, Entity *e, fx_t speed, int16_t width,
                  int16_t wall_probe_y, int16_t floor_probe_y);

/* Daño al jugador (invulnerabilidad, empujón, muerte) y al enemigo
   (destello, empujón, botín). */
void world_damage_player(World *w, int amount, int8_t dir);
void world_hit_enemy(World *w, Entity *e, int amount);

/* La llama la FSM del jefe al terminar su agonia: abre el paso que
   se habia sellado al empezar la pelea. */
void world_boss_defeated(World *w);

/* Contacto enemigo-jugador: si se tocan, hiere a Perseo desde el lado
   correcto. Lo llaman todos los enemigos al final de su update. */
void world_touch_player(World *w, Entity *e);

#endif /* PERSEO_CORE_WORLD_H */
