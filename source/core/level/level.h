/* =====================================================================
   level.h — tilemap y colisión de nivel
   =====================================================================
   Traducción de newLevel/carve/tileAt/rectSolid/isSolid del prototipo
   (ver docs/PLAN_MIGRACION_GBA_C.md, tabla de trazabilidad, fila
   "Utilidades de colisión de mapa" / "Construcción de niveles").

   Por ahora sólo existe level_get_test_room(): una habitación fija de
   30x20 tiles (= 240x160 px, el tamaño exacto de la pantalla de GBA)
   para poder probar la física del jugador en la Fase 1 sin depender
   todavía del pipeline de assets (Fase 3) que genera los 7 niveles
   reales a partir de los .json de assets/src/levels/.
   ===================================================================== */
#ifndef PERSEO_CORE_LEVEL_H
#define PERSEO_CORE_LEVEL_H

#include <stdint.h>
#include <stdbool.h>
#include "../fixed.h"
#include "../entity.h"

#define TILE_SIZE 8

/* Catálogo de tiles: mismos IDs que drawTile()/SOLID[] del prototipo.
   Sólo se listan los que ya importan para colisión; el resto (tuberías
   de vapor con animación, etc.) se agrega cuando el renderer real
   (Fase 2/3) los necesite dibujar. */
typedef enum TileId {
    TILE_EMPTY     = 0,
    TILE_BRICK     = 1,  /* sólido */
    TILE_PLATFORM  = 2,  /* sólido sólo desde arriba (plataforma de un sentido) */
    TILE_SPIKE     = 3,  /* dañino, no sólido — se usa desde la Fase 4 */
    TILE_MUD       = 4,  /* dañino, no sólido — se usa desde la Fase 4 */
    TILE_RUST      = 5,  /* sólido, rompible con Dash Sombrío (Fase 4) */
    TILE_GRATE     = 6,  /* sólido */
    TILE_DOOR_FRAME= 8,  /* sólido */
    TILE_STEAM     = 9   /* dañino intermitente, no sólido — Fase 4 */
} TileId;

/* Entidad tal como viene colocada en los datos del nivel — equivale a una
   llamada E(L, tipo, tx, ty, props) del prototipo. Es sólo la "semilla":
   al cargar el nivel se instancian a partir de acá las Entity vivas del
   pool (core/entity_pool.h), desde la Fase 4. */
typedef struct LevelEntitySpawn {
    uint8_t type;     /* EntityType */
    int16_t tx, ty;   /* posición en tiles */
    int16_t param;    /* significado según el tipo: destino de una puerta,
                         dirección inicial de un enemigo, qué jefe es... */

    /* Campos que sólo usan las entidades de encuentro de jefe. Se dejan
       con nombre propio en vez de un array genérico porque son datos de
       diseño de nivel que alguien va a tener que leer y ajustar a mano. */
    int16_t gate[4];     /* bossgate: rectángulo que se rellena al sellar la arena */
    int16_t passage[4];  /* bossgate: rectángulo que se vacía al abrir la salida */
    int16_t yband[2];    /* bossgate: franja vertical donde el disparador es válido */
    int16_t exit_tx, exit_ty; /* boss: dónde aparece la puerta al vencerlo */

    /* Texto que acompaña a la entidad, o 0. El cartel usa `text` para lo
       que dice; el santuario, `text` como nombre de la habilidad y
       `desc` para explicarla. Apuntan a literales en ROM. */
    const char *text;
    const char *desc;
} LevelEntitySpawn;

typedef struct Level {
    int16_t w, h;                /* tamaño en tiles */
    const uint8_t *tiles;        /* w*h bytes, fila por fila (fila 0 = arriba) */
    int16_t spawn_tx, spawn_ty;  /* tile de aparición del jugador */
    const LevelEntitySpawn *entities;
    int16_t entity_count;
    /* Musica del nivel: un SongId de core/audio.h. Se guarda como
       uint8_t a proposito, para que el tilemap no arrastre una
       dependencia del subsistema de audio. */
    uint8_t song;

    /* Texto del nivel, tal cual venia en el .json: el rotulo de zona que
       aparece al entrar y las paginas de historia que lo presentan. Vive
       en ROM y nadie lo copia. */
    const char *name;
    const char *const *story;
    uint8_t story_count;
} Level;

bool tile_is_solid(uint8_t id);

/* Fuera del nivel: sólido arriba/a los lados (impide salir del mundo),
   vacío por debajo (permite caer al vacío) — mismo criterio que
   tileAt() del prototipo. */
uint8_t level_tile_at(const Level *lv, int16_t tx, int16_t ty);

/* Convierte una coordenada en píxeles (Q8.8) al índice de tile que la
   contiene, truncando hacia -infinito. No usar división entera común
   (/) acá: trunca hacia 0 y da el tile equivocado con coordenadas
   negativas (borde izquierdo/superior del mundo). Válido porque
   TILE_SIZE es potencia de 2. */
static inline int16_t fx_to_tile(fx_t v) {
    return (int16_t)(v >> (FX_SHIFT + 3)); /* 3 = log2(TILE_SIZE) */
}

/* Colisión de un rectángulo (en píxeles, Q8.8) contra los tiles sólidos
   del nivel. Traducción directa de rectSolid() del prototipo. */
bool level_rect_solid(const Level *lv, fx_t x, fx_t y, int16_t w, int16_t h);

/* Rellena un rectángulo de tiles (ambos extremos inclusive), igual que
   carve() en el prototipo. Sólo funciona sobre el tilemap mutable que
   arma world_load(): los tilemaps generados viven en ROM. */
void level_carve(Level *lv, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t id);

/* Tamaño máximo de un nivel, que es lo que se reserva en RAM para su
   copia mutable. Los 7 niveles del prototipo son todos de 200x44. */
#define LEVEL_MAX_TILES (200 * 44)

/* Habitación de prueba de la Fase 1 (ver docs/TAREAS_MIGRACION_GBA.md,
   F1-06). Se reemplaza por los niveles reales generados por
   tools/levelgen en la Fase 3 — el resto del motor no debería depender
   de que esta función exista después de esa fase. */
const Level *level_get_test_room(void);

/* Sala de prueba de la Fase 2 (más ancha que un screenblock de
   hardware, ver docs/TAREAS_MIGRACION_GBA.md, F2-09). */
const Level *level_get_scroll_test_room(void);

#endif /* PERSEO_CORE_LEVEL_H */
