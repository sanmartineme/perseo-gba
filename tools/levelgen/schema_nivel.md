# Formato de nivel (`assets/src/levels/*.json`)

Los 7 niveles del prototipo estaban escritos como codigo (`buildLevel1()`..`buildLevel7()`
en `docs/prototipo_referencia.html`): una grilla inicial rellena de roca solida y una serie
de llamadas a `carve()` que van excavando corredores, mas una lista de entidades colocadas
con `E()`.

Ese modelo declarativo funciona bien y se conserva tal cual, pero movido a datos: cada nivel
es un `.json` que `tools/levelgen/levelgen.py` convierte en un header de C con el tilemap ya
"horneado" (1 byte por tile, en ROM) y la lista de entidades iniciales.

## Estructura

```jsonc
{
  "name": "Túneles de Filtración",   // nombre mostrable de la zona
  "size": [200, 44],                  // ancho x alto, en tiles de 8px
  "fill": 1,                          // tile inicial de toda la grilla (1 = roca solida)
  "spawn": [4, 26],                   // tile donde aparece Perseo
  "theme": { "brick": "T", "brickD": "t" },  // claves de la paleta COL{} del prototipo
  "song": "lvl1",                     // tema musical (se usa desde la Fase 6)
  "story": ["...", "..."],            // texto de entrada de zona (Fase 7)

  // Rectangulos excavados, en orden: [x0, y0, x1, y1, tileId].
  // Equivalen 1:1 a las llamadas carve() del prototipo. Ambos extremos
  // son INCLUSIVOS, igual que el original.
  "carve": [
    [2, 6, 197, 27, 0]
  ],

  // Entidades iniciales. "type" usa los mismos nombres que E() en el
  // prototipo; levelgen los traduce al enum EntityType de core/entity.h.
  // "param" es un entero cuyo significado depende del tipo (destino de
  // una puerta, habilidad de un santuario, etc.). Los textos se guardan
  // aca pero todavia no se emiten a C: el sistema de dialogo es de la
  // Fase 7.
  "entities": [
    { "type": "door", "at": [65, 25], "param": 1 },
    { "type": "sign", "at": [10, 26], "text": "..." }
  ]
}
```

## Ids de tile

Los mismos que `core/level/level.h` (y que `drawTile()` del prototipo):

| id | nombre | solido | notas |
|----|--------|--------|-------|
| 0 | EMPTY | no | aire |
| 1 | BRICK | si | roca/ladrillo, el relleno por defecto |
| 2 | PLATFORM | solo desde arriba | rejilla metalica de un sentido |
| 3 | SPIKE | no | daña al tocar (Fase 4) |
| 4 | MUD | no | lodo, daña y frena (Fase 4) |
| 5 | RUST | si | rejilla oxidada, rompible con Dash Sombrio (Fase 4) |
| 6 | GRATE | si | chatarra compactada |
| 8 | DOOR_FRAME | si | marco/pedestal |
| 9 | STEAM | no | vapor intermitente, daña (Fase 4) |

## Como regenerar

```sh
python3 tools/levelgen/levelgen.py assets/src/levels/ source/core/level/
```

Los headers generados (`level01_tuneles.h`, ...) **no se editan a mano**: se regeneran desde
el `.json`. Ver `docs/PLAN_MIGRACION_GBA_C.md`, seccion 5.
