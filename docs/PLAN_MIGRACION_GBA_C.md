# Plan de Migración a C / Game Boy Advance — "Perseo: Sombras de Silencio"

**Versión del documento:** 1.0
**Fecha:** 2026-09-04
**Autor:** Análisis técnico asistido (Claude Code)
**Insumos analizados:** [index.html](index.html) (prototipo jugable, ~3150 líneas JS/Canvas), [DOCUMENTACION_PERSEO.md](DOCUMENTACION_PERSEO.md), [PRD_Perseo_Metroidvania.md](PRD_Perseo_Metroidvania.md), [Estilo_Grafico_Perseo.md](Estilo_Grafico_Perseo.md), [Especificación de Diseño Técnico...md](<Especificación de Diseño Técnico_ Arquitectura y Progresión en Sistemas Metroidvania.md>)

---

## 0. Resumen ejecutivo

Hoy "Perseo: Sombras de Silencio" existe como un **simulador web** de una sola página ([index.html](index.html)): un motor 2D en JavaScript + Canvas que ya implementa, de forma jugable, casi todos los sistemas descritos en el PRD — física de plataformas, combate, 7 niveles con datos de tilemap y entidades, 6 jefes con IA basada en máquina de estados, dos perfiles de resolución (GBA 240×160 / GBC 160×144), música chiptune sintetizada con Web Audio y una capa de UI/narrativa completa (intro, diálogos de jefe, inventario, créditos).

Esto es una ventaja enorme para el port: **no hay que diseñar el juego, hay que traducir un diseño ya validado y jugable a C para hardware real de GBA.** El prototipo web funciona como *spec ejecutable* — cada sistema de este documento tiene su contraparte exacta y probada en `index.html`, referenciada por número de línea.

Este documento cubre:
1. Qué hay hoy y cómo se traduce cada pieza a C.
2. Las restricciones reales de la GBA que van a moldear las decisiones técnicas.
3. Una arquitectura **modular** en dos capas (lógica de juego portable + capa de plataforma) pensada explícitamente para que el trabajo de optimización futuro (perfilar, reemplazar un subsistema, ajustar el uso de VRAM/IWRAM) no obligue a tocar el resto del juego.
4. Un plan de implementación por fases, con entregables y criterios de aceptación.
5. Una plantilla de estructura de carpetas para el nuevo repositorio en C.

> **Nota de alcance:** el prototipo web es la referencia de diseño/comportamiento, no la fuente de los assets finales. Los sprites `SPR.*` (arte ASCII por letras de paleta) y las canciones `SONGS.*` (secuencias de notas por osciladores Web Audio) son **datos de diseño perfectamente reutilizables como especificación**, pero deben pasar por un pipeline de conversión a formatos nativos de GBA (tiles 4bpp/8bpp, canales de sonido PSG) — no se puede compilar Canvas/WebAudio a un cartucho.

---

## 1. Estado actual: inventario de sistemas (base de la migración)

Todo vive en un único archivo. La tabla siguiente es el mapa de "qué es cada cosa" que se usará como checklist de migración (ver también la sección 7, tabla de trazabilidad completa).

| Sistema | Ubicación en `index.html` | Descripción |
|---|---|---|
| Paleta y perfiles de resolución | [index.html:35-57](index.html#L35-L57) | `TILE=8`, modos `gba` (240×160) / `gbc` (160×144), paleta lógica de 28 colores con variante por modo |
| Sprites (arte procedural ASCII) | [index.html:60-940](index.html#L60-L940) | ~40 sprites definidos como matrices de caracteres → color |
| Habilidades y reliquias (datos) | [index.html:941-958](index.html#L941-L958) | `ABILITIES[]`, `RELICS[]`, data-driven |
| Configuración de jefes (datos) | [index.html:964-1013](index.html#L964-L1013) | `BOSS_CONFIG{}` (stats por jefe) + `BOSS_DIALOG{}` (guion) |
| Caché de sprites + blitting | [index.html:1015-1042](index.html#L1015-L1042) | Pre-renderiza cada sprite a un canvas por modo; `spr()` dibuja con flip |
| Audio: síntesis y secuenciador | [index.html:1044-1156](index.html#L1044-L1156) | Osciladores (`tone`, `noise`), `SFX{}`, `SONGS{}` (patrones de 8/16 pasos), `playSong()` |
| Tiles de mundo (dibujo procedural) | [index.html:1157-1213](index.html#L1157-L1213) | `SOLID[]`, `isSolid()`, `drawTile()` con 9 tipos de tile |
| Narrativa (intro/final/créditos) | [index.html:1215-1250](index.html#L1215-L1250) | `INTRO_STORY[]`, `ENDING_STORY[]`, `CREDITS[]` |
| Construcción de niveles (datos) | [index.html:1250-1540](index.html#L1250-L1540) | `newLevel/carve/E()` + `buildLevel1..7()`, `LEVEL_BUILDERS[]` |
| Estado global de partida | [index.html:1542-1567](index.html#L1542-L1567) | Máquina de estados (`state`), cámara, progreso persistente (`broken`, `collected`, `checkpoint`) |
| Jugador: datos y control | [index.html:1569-1584](index.html#L1569-L1584) | Struct `P{}`, mapa de teclas `IN{}` |
| Utilidades de colisión de mapa | [index.html:1586-1602](index.html#L1586-L1602) | `tileAt`, `rectSolid`, `overlap`, `burst` (partículas) |
| Carga de nivel / progreso | [index.html:1604-1652](index.html#L1604-L1652) | `loadLevel`, `newGame`, `damage`, `respawn` |
| Física y control del jugador | [index.html:1655-1789](index.html#L1655-L1789) | `updatePlayer()`: movimiento, salto (coyote time + buffer), doble salto, dash, escalada, ataque, lanzamiento |
| Entidades / enemigos | [index.html:1792-2032](index.html#L1792-L2032) | `entBox`, `hitEnemy`, `groundY`, `updateEnts()` (IA de rat/roach/mosq/bat/thug/brute/gunner) |
| Jefes: FSM de combate | [index.html:2033-2157](index.html#L2033-L2157) | `initBoss`, `updateBoss()` — estados `wait/intro/choose/tele/charge/leap/throw/trashblast/summon/stun/die` |
| Transición de nivel (mosaico) | [index.html:2159-2187](index.html#L2159-L2187) | `startTransition`, `updateTransition`, `pixelate` |
| Render: texto, tuberías, paralaje | [index.html:2188-2273](index.html#L2188-L2273) | `text`, `wrapText`, `pipeH/pipeV`, `drawParallax` |
| Render: entidades y jugador | [index.html:2274-2381](index.html#L2274-L2381) | `drawEnt`, `WALK_CYCLE`, `drawPlayer` |
| Render: mundo y HUD | [index.html:2348-2481](index.html#L2348-L2481) | `drawWorld`, `drawHUD`, `silhouette`, `drawShield` |
| UI: título, muerte, historia, inventario | [index.html:2481-2917](index.html#L2481-L2917) | `drawTitle/drawDead/drawStory/drawIntroCine/drawInventory/drawVictory/drawBossDialog/drawBanner/drawEnding/drawCredits` |
| Cámara y bucle principal | [index.html:2918-3020](index.html#L2918-L3020) | `updateCam`, `step()`, `render()`, `loop()`, `setMode`, `fit` |

**Catálogo de entidades de nivel** (valor de `type` pasado a `E(L, tipo, ...)`): `sign`, `lamp`, `door`, `vdoor`, `shrine`, `bossgate`, `boss`, `chapa`, `relic`, `hp`, `rat`, `roach`, `mosq`, `gunner`, `bat`, `thug`, `brute`. Estos 17 tipos son el catálogo de "actores de nivel" que el port debe soportar.

**Contenido cuantificado** (de [DOCUMENTACION_PERSEO.md](DOCUMENTACION_PERSEO.md)): 7 niveles, 6 jefes, 2 habilidades pasivas + 3 desbloqueables, 3 reliquias, 6+ tipos de enemigo, ~60-90 min de duración objetivo.

---

## 2. Restricciones de la plataforma objetivo (GBA)

Estas cifras son las que determinan casi todas las decisiones de arquitectura de las secciones siguientes:

| Recurso | Límite físico | Implicación para Perseo |
|---|---|---|
| CPU | ARM7TDMI a 16.78 MHz (ARM/Thumb), sin FPU | Nada de `float` en el loop caliente; usar aritmética de punto fijo |
| IWRAM (RAM interna, 0 wait states) | 32 KB | Reservar para el código y datos "calientes" del frame: física, colisión, cola de sprites |
| EWRAM (RAM externa) | 256 KB | Estado de nivel activo (tilemap 200×44 = ~8.8 KB por nivel, entidades, partículas) |
| VRAM | 96 KB | Tiles de fondo + tiles de sprites + tilemaps visibles a la vez; presupuesto que hay que planear por nivel |
| Paleta | 2×256 entradas de 15 bits (una para fondos, una para sprites) | Los ~28 colores lógicos de `COL{}` entran holgados; hay que agrupar en subpaletas de 16 si se usan sprites 4bpp |
| OAM (sprites) | 128 objetos en pantalla, tamaños hasta 64×64 | Jefes grandes (`boss_betty` es 32×24 px) se arman con varios objetos de 8×8/16×16, no un solo sprite gigante |
| Refresco | ~59.73 Hz, VBlank fijo | El loop de juego es de *timestep fijo* por diseño (no hace falta un timer de delta variable como en JS) |
| Guardado | SRAM 32 KB / Flash 64-128 KB / EEPROM (según cartucho) | El progreso actual (`broken`, `collected`, `collectedRelics`, `checkpoint`, `ab`, `relicsEq`, `stats`) cabe muchas veces en 32 KB de SRAM |
| ROM | Cartuchos homebrew típicos: 1-32 MB | Con arte pixel-art de 8×8/16×16 y música por PSG (no muestras), el presupuesto es cómodo incluso a 4-8 MB |
| Modos de vídeo | Modo 0 (4 capas tiled, sin rotación/escala) recomendado para plataformero 2D con paralaje | El paralaje de `drawParallax()` se traduce 1:1 a 2-3 capas de fondo con distinto factor de scroll |

**Decisión de modo de vídeo:** Modo 0, con BG0/BG1 como capas de paralaje lejanas, BG2 como el tilemap del nivel (colisionable) y los sprites por hardware (OBJ) para jugador, enemigos, jefes, proyectiles y partículas. Esto reemplaza directamente el modelo actual de `drawParallax()` + `drawWorld()` + `drawEnt()`.

**Decisión de audio:** los patrones de `SONGS{}` ya están escritos en términos de forma de onda (`square`/`triangle`/`sine`/`sawtooth`) + `noise`, que es exactamente el modelo de los 4 canales de sonido "PSG" heredados de Game Boy que la GBA sigue integrando (2 cuadrada, 1 onda programable, 1 ruido). Esto significa que **el motor de audio actual se puede portar casi literalmente** reprogramando los registros de sonido en lugar de reescribir toda la música como tracker. Maxmod (sample-based) queda como mejora opcional de "más adelante", no como requisito inicial.

---

## 3. Toolchain y dependencias

| Herramienta | Uso | Notas |
|---|---|---|
| **devkitARM** (devkitPro) | Compilador (`arm-none-eabi-gcc`), linker, `gbafix` | Estándar de facto para homebrew de GBA en C |
| **libtonc** | Capa de utilidades sobre los registros de hardware (vídeo, tiles, sprites, texto, punto fijo) | Se usa en vez de tocar registros a mano; es C puro, bien documentado, con ejemplos ("Tonc") que cubren exactamente los patrones que necesitamos (scroll de mapas grandes, animación de sprites, sonido PSG) |
| **grit** | Convierte PNG (tiles/sprites) a arrays C (`.c/.h`) con la paleta ya cuantizada | Entra en el pipeline de assets (sección 5) |
| **mmutil** (opcional, fase de mejora) | Genera el soundbank para Maxmod a partir de `.xm/.it/.wav` | Solo si se decide subir la música de PSG a tracker más adelante |
| **mGBA** | Emulador principal de desarrollo (debugger, inspector de memoria/VRAM, muy preciso) | Verificación diaria |
| **no$gba** | Segundo emulador para contrastar timing/audio | Verificación cruzada antes de cada hito |
| **Hardware real + flashcart** (ej. EZ-Flash) | Validación final | Al menos en los hitos de fin de fase, no en cada commit |
| **Python 3** | Scripts propios del pipeline de niveles/sprites (sección 5) | No forma parte del build de la ROM, es "tooling" |
| **CMocka o `assert.h` simple** | Pruebas unitarias del código de `core/` compilado nativamente (PC) | Ver sección 8, capa portable |

---

## 4. Arquitectura modular propuesta

El requisito explícito del proyecto es: **modularidad para poder mejorar el rendimiento más adelante sin reescribir el juego.** La forma más efectiva de lograr esto en C, y la que se recomienda aquí, es separar el código en dos capas con una interfaz fija entre ambas:

```
┌─────────────────────────────────────────────────────────┐
│  core/   — lógica de juego, 100% portable, sin registros │
│  de hardware ni includes de <gba.h>. Compila igual en    │
│  GBA que en PC (build nativo con SDL2 para iterar rápido)│
└───────────────────────┬───────────────────────────────────┘
                         │  usa solamente
                         ▼
┌─────────────────────────────────────────────────────────┐
│  platform/pal.h  — interfaz de la Capa de Abstracción de │
│  Plataforma (Platform Abstraction Layer): vídeo, input,  │
│  audio, temporización, guardado                          │
└───────────┬─────────────────────────────┬─────────────────┘
            ▼                             ▼
  platform/gba/...              platform/sdl/...  (opcional)
  implementación real           implementación de escritorio
  sobre libtonc/hardware        para debug/iteración rápida
```

### 4.1 Por qué esta separación (y no compilar todo junto para GBA desde el día uno)

- **Rendimiento futuro sin tocar gameplay:** si más adelante hay que optimizar colisiones (ej. de fuerza bruta a *broad phase* por celdas), cambiar cómo se gestionan los sprites en OAM (double buffering, pooling), o mover funciones calientes a IWRAM, esos cambios ocurren **dentro de `platform/gba/`** o en los módulos de `core/` marcados como "hot path" — nunca obligan a re-tocar la definición de un jefe, un nivel o una habilidad.
- **Iteración rápida:** compilar y correr en un binario nativo de PC (SDL2) tarda segundos, contra minutos si cada cambio de una condición de salto hay que probarlo en emulador. Los sistemas de física, IA de enemigos y FSM de jefes (que son el 70% del código y el que más se itera) se pueden depurar con herramientas de escritorio (breakpoints, sanitizers, Valgrind) antes de validarlos en GBA.
- **Pruebas automatizadas reales:** el código de `core/` no depende de hardware, así que se puede compilar con GCC nativo y correr aserciones sobre la física del jugador, la FSM de un jefe o el sistema de guardado — algo imposible de automatizar directamente sobre una ROM.
- **Aislar la parte irreductiblemente específica de GBA:** el acceso a VRAM/OAM, DMA, IRQs y registros de sonido queda contenido en `platform/gba/`, que es deliberadamente la parte más pequeña y la única que un futuro port a otro sistema (u otro emulador/hardware) necesitaría reescribir.

> Si el equipo prefiere simplicidad máxima y descarta el build de escritorio, la misma separación `core/` vs `platform/gba/` se mantiene igual de válida — el build SDL es un extra opcional de la fase 0, no una dependencia del resto del plan. Lo que **no** es opcional es mantener `core/` libre de includes de `<gba.h>`/`<tonc.h>`, porque ahí es donde vive el valor de la modularidad.

### 4.2 Principios de diseño de `core/` orientados a rendimiento

1. **Sin memoria dinámica.** Nada de `malloc`/`free` en el loop de juego. Todo actor (jugador, enemigo, proyectil, partícula) vive en un **pool estático** dimensionado al máximo real observado en los niveles actuales (ver `buildLevelN()`: el nivel con más entidades ronda ~25-30). Esto es directamente equivalente al array `LV.ents` de JS, pero de tamaño fijo y sin GC.
2. **Structure-of-Arrays donde importe.** Para las entidades activas de un nivel, separar los campos que se tocan cada frame (posición, velocidad, estado) de los que casi no cambian (sprite id, tipo). Facilita mover el bucle de física "caliente" a datos compactos que quepan en caché/IWRAM sin arrastrar campos fríos.
3. **Punto fijo, no flotante.** Toda la física actual (`SPD=1.35`, `GRV=0.22`, `JV=4.2`, etc., en [index.html:1656](index.html#L1656)) se re-expresa en Q8.8 o Q12.4 (tipo `fx_t` de 32 bits, ver `tonc_fixed.h` de libtonc). Es un cambio mecánico, valor por valor, no un rediseño.
4. **Tablas de datos const en ROM.** `ABILITIES[]`, `RELICS[]`, `BOSS_CONFIG{}`, `BOSS_DIALOG{}`, `SONGS{}` se traducen a `static const` en C — el compilador los coloca en ROM, no consumen RAM. Es la misma filosofía "data-driven" del JS, simplemente en un lenguaje sin recolector de basura.
5. **Mutación mínima del nivel.** El JS ya hace esto bien: `LV.t` es la base del tilemap y `broken[i]` es un `Set` aparte con las coordenadas rotas ([index.html:1607](index.html#L1607)). Se conserva el patrón: el tilemap de cada nivel es un array `const` en ROM, y solo un pequeño *overlay* de "tiles modificados" vive en RAM y se aplica al cargar el nivel. Esto reduce drásticamente el uso de EWRAM comparado con copiar 8.8 KB por nivel a RAM mutable sin necesidad.
6. **Interfaces desacopladas por sistema**, cada una con un `.h` que declara *qué hace* sin exponer *cómo*: `physics.h`, `boss_fsm.h`, `render_queue.h` (cola de comandos de dibujo que `platform/gba/` consume para escribir a OAM/VRAM), `audio.h` (reproduce un `Song` sin saber qué son los canales PSG por debajo). Esto es lo que permite reemplazar una implementación por otra más rápida sin tocar a quien la llama.

---

## 5. Pipeline de datos y assets

El prototipo web codifica todo el contenido como literales JS. Ninguno de esos formatos es directamente utilizable en una ROM, pero todos son **convertibles de forma casi mecánica**:

| Dato en JS | Formato actual | Conversión a GBA |
|---|---|---|
| `SPR.*` (sprites) | Matriz de strings, 1 carácter = 1 color lógico | Script propio (`tools/spritegen/ascii_to_png.py`) que renderiza cada `SPR.*` a un PNG indexado usando la misma paleta `COL{}` → luego `grit` genera tiles 4bpp + paleta `.c/.h`. Mantiene el arte actual como semilla real en vez de redibujar todo desde cero. |
| `COL{}` (paleta) | 28 colores lógicos, 2 variantes (gba/gbc) | Paleta maestra de referencia para `grit`; se agrupa en sub-paletas de 16 colores por sprite/tileset (límite real de GBA en modo 4bpp) |
| Tiles de mundo (`drawTile`, 9 tipos) | Dibujado procedural por `id` | Se pre-renderizan una vez como tileset 8×8 (PNG) y se convierten con `grit`; el `id` de tile pasa a ser un índice de tileset, igual que hoy, pero ya no se "dibuja" en runtime, se referencia |
| `buildLevel1..7()` (niveles) | Llamadas a `carve()`/`E()` en código | `tools/levelgen/levelgen.py`: se conserva el mismo lenguaje declarativo (rectángulos + lista de entidades) en un `.json` por nivel, y el script emite un header `.h` con el tilemap empaquetado (1 byte/tile) + un array de entidades iniciales. Alternativa de futuro: importar desde Tiled (`.tmx`) si el equipo de niveles lo prefiere — el formato intermedio (`level.h`) no cambia. |
| `SONGS{}` / `SFX{}` | Patrones de notas por paso + tipo de onda | `tools/audiogen/` traduce cada patrón a una secuencia de escrituras a los registros de sonido PSG (frecuencia, duty, envolvente) reproducida por un secuenciador propio en `platform/gba/pal_gba_audio.c`; no requiere convertir a audio muestreado |
| `INTRO_STORY` / `ENDING_STORY` / `CREDITS` / `BOSS_DIALOG` | Arrays de `{scene, who, text}` | Se copian a `static const` en `dialogue_data.c`; el único trabajo real es decidir el wrapping de texto para el visor de texto de libtonc (TTE) a 240×160 |

**Regla de oro del pipeline:** nada bajo `assets/gen/` se edita a mano — siempre se regenera desde `assets/src/` o `tools/`. Esto es lo que permite que, en la fase de optimización, se pueda cambiar de 4bpp a 8bpp en un tileset concreto (por ejemplo) regenerando un solo comando, sin tocar el código de juego.

---

## 6. Mapeo de controles

| Acción (JS, [index.html:1578](index.html#L1578)) | Tecla actual | Botón GBA propuesto |
|---|---|---|
| Mover | ←→ | D-Pad |
| Saltar / salto doble | Z, Espacio | **A** |
| Ganchito mortal (ataque) | X, J | **B** |
| Dash Sombrío | C, K | **R** |
| Lanzamiento de traza | V, L | **L** |
| Inventario | I | **Select** |
| Pausa | Enter | **Start** |
| Escalar muro (Garra Felina) | ← / → sobre pared + salto | D-Pad + A (sin botón nuevo) |

---

## 7. Tabla de trazabilidad JS → módulos C

| Módulo C (propuesto) | Origen en `index.html` | Responsabilidad |
|---|---|---|
| `core/fixed.{h,c}` | constantes numéricas de física dispersas | Tipo `fx_t` y operaciones de punto fijo |
| `core/entity.{h,c}`, `core/entity_pool.{h,c}` | `LV.ents`, `E()` [1259](index.html#L1259) | Struct de entidad genérica + pool estático |
| `core/player.{h,c}` | struct `P` [1569](index.html#L1569), `updatePlayer` [1655](index.html#L1655) | Física, animación y estado del jugador |
| `core/abilities.{h,c}` | `ABILITIES[]` [941](index.html#L941) | Tabla de habilidades + flags desbloqueadas |
| `core/relics.{h,c}` | `RELICS[]` [950](index.html#L950), `equippedCount` [958](index.html#L958) | Tabla de reliquias + efectos sobre daño |
| `core/enemies/*.c` | `updateEnts()` [1821](index.html#L1821) (una función por tipo: rat/roach/mosq/bat/thug/brute/gunner) | IA de cada tipo de enemigo, separada en su propio archivo |
| `core/boss/boss_fsm.{h,c}` | `updateBoss()` [2049](index.html#L2049) | Máquina de estados genérica de jefe (intro/choose/tele/charge/leap/throw/trashblast/summon/stun/die) |
| `core/boss/boss_config.h` | `BOSS_CONFIG{}` [964](index.html#L964) | Tabla `const` de stats por jefe |
| `core/level/level.{h,c}` | `newLevel/carve/tileAt/rectSolid` [1250](index.html#L1250), [1587](index.html#L1587) | Representación de tilemap + colisión + overlay de tiles rotos |
| `core/level/level01..07.h` | `buildLevel1..7()` [1261](index.html#L1261) | Datos generados de cada nivel (ver pipeline, sección 5) |
| `core/camera.{h,c}` | `cam{}`, `updateCam` [2918](index.html#L2918) | Seguimiento de cámara + límites de nivel |
| `core/particles.{h,c}` | `particles[]`, `burst()` [1599](index.html#L1599) | Pool de partículas |
| `core/projectiles.{h,c}` | `projs[]` (traza, junk, shock) | Pool de proyectiles |
| `core/dialogue.{h,c}` + `dialogue_data.c` | `BOSS_DIALOG`, `INTRO_STORY`, `ENDING_STORY`, `CREDITS` [982](index.html#L982), [1218](index.html#L1218) | Datos y avance de diálogo/cinemática |
| `core/save.{h,c}` | `broken`, `collected`, `collectedRelics`, `checkpoint`, `stats` | Struct de guardado serializable a SRAM |
| `core/game_state.{h,c}` | variable `state` + todo `step()` [2923](index.html#L2923) | Máquina de estados de alto nivel (title/story/play/dead/inventory/banner/trans/ending/credits) |
| `core/audio.h` + `audio_data.c` | `SONGS{}`, `SFX{}`, `playSong` [1085](index.html#L1085) | Secuenciador de música/SFX en términos de forma de onda (independiente del backend) |
| `platform/gba/pal_gba_video.c` | `drawWorld/drawParallax/drawEnt/drawPlayer/pipeH/pipeV` [2159-2381](index.html#L2159-L2381) | Escritura real a VRAM/OAM/registros de scroll |
| `platform/gba/pal_gba_input.c` | `keys{}`, `IN{}` [1576](index.html#L1576) | Lectura de `REG_KEYINPUT` |
| `platform/gba/pal_gba_audio.c` | `tone/noise` (Web Audio) [1046](index.html#L1046) | Programación de los 4 canales PSG reales |
| `platform/gba/pal_gba_save.c` | (no existe hoy; es `localStorage` implícito o nada) | Lectura/escritura de SRAM con checksum |
| `ui/hud.{h,c}` | `drawHUD` [2382](index.html#L2382) | Corazones, monedas, iconos de habilidad |
| `ui/inventory.{h,c}` | `drawInventory`, `invOpen/invTab/invSel` [2677](index.html#L2677) | Pantalla de inventario |
| `ui/menu_title.{h,c}` | `drawTitle`, `titleSel`, `DIFF_CFG` [1560](index.html#L1560), [2481](index.html#L2481) | Menú de título y selección de dificultad |
| `ui/transition.{h,c}` | `startTransition/updateTransition/pixelate` [2159](index.html#L2159) | Transición mosaico entre niveles |

---

## 8. Plan de implementación por fases

Cada fase tiene un criterio de aceptación verificable en emulador. El tamaño (S/M/L/XL) es relativo, no una estimación de calendario — depende del ritmo real del equipo.

### Fase 0 — Cimientos del proyecto (S)
- Instalar devkitARM + libtonc; scaffolding del repo según la sección 9.
- "Hola mundo": una pantalla en Modo 0 con un tile de color sólido y lectura de input, corriendo en mGBA.
- (Opcional) scaffolding del build SDL de escritorio para `core/`.
- **Aceptación:** ROM arranca en mGBA y no$gba, responde al D-Pad.

### Fase 1 — Núcleo del loop y física del jugador (M)
- Punto fijo (`core/fixed.h`), loop de VBlank, `core/player.c` con el movimiento/salto/coyote-time/dash/doble salto de [index.html:1655-1789](index.html#L1655-L1789) portado valor por valor.
- Un tilemap mínimo (una habitación de prueba) con colisión AABB contra tiles, equivalente a `rectSolid`.
- **Aceptación:** el jugador se mueve, salta y colisiona igual de "bien" que en el prototipo web (mismo *feel*, comparado lado a lado en emulador vs. navegador).

### Fase 2 — Renderizado de mundo y cámara (M)
- Fondo de nivel como BG regular con scroll por hardware (técnica de "big map" para tilemaps mayores a 32×32 tiles, ver Tonc), cámara siguiendo al jugador con límites de nivel.
- 1-2 capas de paralaje (BG0/BG1) equivalentes a `drawParallax()`.
- **Aceptación:** recorrer un nivel completo de ancho (200 tiles) con scroll fluido a 60 fps y sin *tearing*.

### Fase 3 — Pipeline de assets y primer nivel real (L)
- `tools/spritegen` y `tools/levelgen` funcionando end-to-end: de los `SPR.*`/`buildLevel1()` actuales a tiles y tilemap reales en la ROM.
- Nivel 1 completo (Túneles de Filtración) jugable sin enemigos ni jefe.
- **Aceptación:** Nivel 1 reconocible visualmente respecto al prototipo web, recorrible de punta a punta.

### Fase 4 — Entidades, combate y enemigos (L)
- Pool de entidades, sistema de partículas/proyectiles, Ganchito Mortal y Lanzamiento de Traza.
- IA de los 7 tipos de enemigo (`rat/roach/mosq/bat/thug/brute/gunner`).
- **Aceptación:** Nivel 1 jugable con todos sus enemigos y daño/vida funcionando.

### Fase 5 — Jefes (L)
- `boss_fsm.c` genérica + `BOSS_CONFIG`/`BOSS_DIALOG` como tablas de datos.
- Los 6 jefes, reutilizando la misma FSM (igual que hoy reutiliza `updateBoss()`).
- **Aceptación:** los 6 combates de jefe son superables y respetan sus patrones de ataque documentados en [DOCUMENTACION_PERSEO.md](DOCUMENTACION_PERSEO.md).

### Fase 6 — Audio (M)
- Secuenciador PSG (`pal_gba_audio.c`) reproduciendo los patrones de `SONGS{}`/`SFX{}` portados.
- **Aceptación:** todas las canciones y SFX suenan reconociblemente igual que en el prototipo web (mismo tempo/forma de onda relativa).

### Fase 7 — UI, narrativa y progresión completa (L)
- HUD, inventario, menú de título con dificultad, cinemáticas de intro/final, créditos, transición mosaico, diálogos de jefe.
- Sistema de habilidades/reliquias/santuarios y guardado en SRAM (checkpoints, tiles rotos, coleccionables, progreso).
- **Aceptación:** partida completa jugable de principio a fin, con guardado persistente entre resets del emulador/hardware.

### Fase 8 — Los 7 niveles completos (L)
- Repetir el pipeline de la fase 3 para los niveles 2-7.
- **Aceptación:** las 7 zonas + Mercado Negro (nivel de transición) están completas y enlazadas por las puertas/checkpoints.

### Fase 9 — Optimización y pulido (M, y la fase pensada para repetirse)
- Perfilado real en hardware/mGBA: frames que exceden el VBlank, uso de VRAM/IWRAM por nivel, colisiones de sprites en OAM.
- Candidatos típicos ya anticipados por la arquitectura modular: mover el bucle de física a IWRAM, cambiar el *broad phase* de colisión entidad-entidad de O(n²) a particionado espacial si el nivel con más enemigos lo requiere, *double buffering* de OAM, compresión LZ77/Huffman de tilesets si el presupuesto de ROM aprieta.
- **Aceptación:** 60 fps estables en el nivel/escena más exigente (candidato: Nivel 7, jefe final con invocaciones + proyectiles).

### Fase 10 — QA y empaquetado final
- Checklist de playtest (ver `tests/playtest_checklist.md` en la plantilla), pase en hardware real, `gbafix` final, build de release.

---

## 9. Plantilla de infraestructura de carpetas

Este es el esqueleto de repositorio recomendado para el nuevo proyecto en C. Sigue la separación `core/` (portable) vs `platform/` (específico de GBA) descrita en la sección 4.

```
perseo-gba/
├── Makefile                      # build principal (devkitARM)
├── README.md
├── LICENSE
│
├── docs/                         # documentación de diseño y técnica
│   ├── PLAN_MIGRACION_GBA_C.md   # este documento
│   ├── DOCUMENTACION_PERSEO.md
│   ├── PRD_Perseo_Metroidvania.md
│   ├── Estilo_Grafico_Perseo.md
│   └── arquitectura/
│       ├── formato_niveles.md    # spec del .json de nivel y del level.h generado
│       └── presupuesto_memoria.md# tracking de VRAM/IWRAM/EWRAM/ROM por hito
│
├── source/
│   ├── main.c                    # entry point GBA
│   │
│   ├── core/                     # lógica de juego portable (sin <gba.h>/<tonc.h>)
│   │   ├── fixed.h
│   │   ├── entity.h
│   │   ├── entity.c
│   │   ├── entity_pool.h
│   │   ├── entity_pool.c
│   │   ├── physics.h
│   │   ├── physics.c
│   │   ├── player.h
│   │   ├── player.c
│   │   ├── abilities.h
│   │   ├── abilities.c
│   │   ├── relics.h
│   │   ├── relics.c
│   │   ├── camera.h
│   │   ├── camera.c
│   │   ├── particles.h
│   │   ├── particles.c
│   │   ├── projectiles.h
│   │   ├── projectiles.c
│   │   ├── dialogue.h
│   │   ├── dialogue.c
│   │   ├── save.h
│   │   ├── save.c
│   │   ├── game_state.h
│   │   ├── game_state.c
│   │   ├── audio.h
│   │   ├── audio_data.c
│   │   ├── rng.h
│   │   ├── rng.c
│   │   │
│   │   ├── enemies/
│   │   │   ├── enemy_common.h
│   │   │   ├── enemy_common.c
│   │   │   ├── enemy_rat.c
│   │   │   ├── enemy_roach.c
│   │   │   ├── enemy_mosq.c
│   │   │   ├── enemy_bat.c
│   │   │   ├── enemy_thug.c
│   │   │   ├── enemy_brute.c
│   │   │   └── enemy_gunner.c
│   │   │
│   │   ├── boss/
│   │   │   ├── boss_fsm.h
│   │   │   ├── boss_fsm.c
│   │   │   ├── boss_config.h     # BOSS_CONFIG como tabla const
│   │   │   └── boss_dialog.h     # BOSS_DIALOG como tabla const
│   │   │
│   │   └── level/
│   │       ├── level.h
│   │       ├── level.c
│   │       ├── level01_tuneles.h
│   │       ├── level02_vertedero.h
│   │       ├── level03_estacion.h
│   │       ├── level04_residuos.h
│   │       ├── level05_madriguera.h
│   │       ├── level06_mercado.h
│   │       └── level07_trono.h   # generados por tools/levelgen (no editar a mano)
│   │
│   ├── platform/
│   │   ├── pal.h                 # interfaz: pal_video_*, pal_input_*, pal_audio_*, pal_save_*
│   │   │
│   │   ├── gba/                  # implementación real sobre libtonc
│   │   │   ├── pal_gba_video.c
│   │   │   ├── pal_gba_input.c
│   │   │   ├── pal_gba_audio.c
│   │   │   ├── pal_gba_save.c    # acceso a SRAM + checksum
│   │   │   ├── pal_gba_timer.c
│   │   │   ├── irq.c
│   │   │   └── gba_main.c
│   │   │
│   │   └── sdl/                  # (opcional) build de escritorio para iterar rápido
│   │       ├── pal_sdl_video.c
│   │       ├── pal_sdl_input.c
│   │       ├── pal_sdl_audio.c
│   │       └── sdl_main.c
│   │
│   └── ui/
│       ├── hud.h
│       ├── hud.c
│       ├── inventory.h
│       ├── inventory.c
│       ├── menu_title.h
│       ├── menu_title.c
│       ├── menu_pause.h
│       ├── menu_pause.c
│       ├── transition.h
│       ├── transition.c
│       └── text.h                # envoltorio fino sobre TTE (libtonc)
│
├── include/                      # (opcional) headers públicos si se separan de source/
│
├── assets/
│   ├── src/                      # fuentes editables — SIEMPRE la fuente de verdad
│   │   ├── sprites/
│   │   │   ├── perseo/
│   │   │   ├── aurorita/
│   │   │   ├── enemigos/
│   │   │   ├── jefes/
│   │   │   └── fx/                # slash, splat, shock, chapa, corazón...
│   │   ├── tiles/
│   │   │   ├── tileset_tuneles.png
│   │   │   ├── tileset_vertedero.png
│   │   │   └── ...
│   │   ├── backgrounds/           # capas de paralaje por nivel
│   │   ├── audio/
│   │   │   ├── sfx/
│   │   │   └── music/             # patrones fuente si se migra a Maxmod más adelante
│   │   └── levels/                # definiciones declarativas de nivel (.json), ver sección 5
│   │
│   └── gen/                       # SALIDA generada — nunca editar a mano, va en .gitignore
│       ├── sprites/
│       ├── tiles/
│       ├── soundbank/
│       └── levels/
│
├── tools/                         # scripts de pipeline (no compilan a la ROM)
│   ├── spritegen/
│   │   └── ascii_to_png.py        # reutiliza los SPR.* del prototipo como semilla
│   ├── levelgen/
│   │   ├── levelgen.py
│   │   └── schema_nivel.md
│   ├── audiogen/
│   │   └── songs_to_psg.py        # traduce SONGS.*/SFX.* a secuencias de registros PSG
│   └── grit_configs/              # un .grit por hoja de sprites/tileset
│
├── tests/
│   ├── unit/                      # pruebas de core/ compiladas nativas (gcc de escritorio)
│   │   ├── test_physics.c
│   │   ├── test_boss_fsm.c
│   │   ├── test_entity_pool.c
│   │   └── test_save.c
│   └── playtest_checklist.md
│
├── build/                         # artefactos de compilación (gitignore)
│
└── third_party/                   # o gestionado como submódulos git
    ├── libtonc/
    └── maxmod/                    # solo si se adopta en la fase de mejora de audio
```

### 9.1 Convenciones que sostienen la modularidad

- **Ningún archivo bajo `source/core/` incluye `<gba.h>`, `<tonc.h>` ni ningún header de `platform/`.** Es la regla que hace cumplible la separación de la sección 4; conviene reforzarla con un lint simple (`grep` en CI) más que confiar en disciplina manual.
- **`assets/gen/` y `build/` van en `.gitignore`**: son 100% reproducibles desde `assets/src/` + `tools/` + `Makefile`.
- **Un `.h` por sistema en `core/` declara solo la interfaz** (funciones + structs de datos que otros módulos necesitan); los detalles internos quedan `static` en el `.c`. Esto es lo que permite sustituir una implementación (p. ej. cambiar el algoritmo de colisión) sin romper a quien la usa.
- **Los niveles y las tablas de datos (`boss_config.h`, `abilities.h` con su array, `boss_dialog.h`, `dialogue_data.c`) son siempre `static const`**, para que el compilador los ubique en ROM y no compitan por RAM.

### 9.2 Esqueleto de `Makefile` (referencia)

```makefile
# Ver el Makefile de ejemplo de devkitARM/libtonc; puntos clave a mantener:
export DEVKITARM := $(DEVKITPRO)/devkitARM
include $(DEVKITARM)/gba_rules

TARGET   := perseo
BUILD    := build
SOURCES  := source source/core source/core/enemies source/core/boss \
            source/core/level source/platform/gba source/ui
INCLUDES := source third_party/libtonc/include
LIBS     := -ltonc
LIBDIRS  := third_party/libtonc

# Regla adicional: generar source/core/level/*.h y assets/gen/** ANTES de compilar
pregen:
	python3 tools/levelgen/levelgen.py assets/src/levels/ source/core/level/
	python3 tools/spritegen/ascii_to_png.py assets/src/sprites/
	# grit sobre assets/src/sprites/**/*.png y assets/src/tiles/**/*.png -> assets/gen/
```

---

## 10. Estrategia de guardado

Traducción directa del estado persistente actual (`broken`, `collected`, `collectedRelics`, `checkpoint`, `P.ab`, `P.relicsEq`, `stats`, dificultad) a un único `struct SaveData` empaquetado, escrito a SRAM:

```c
typedef struct {
    uint16_t magic;          // firma de versión de formato
    uint8_t  checksum;       // validación simple
    uint8_t  difficulty;
    uint8_t  abilities;      // bitmask: djump/dash/climb
    uint8_t  relics_found;   // bitmask
    uint8_t  relics_equipped;// bitmask
    uint8_t  cur_level;
    uint8_t  checkpoint_tx, checkpoint_ty;
    uint32_t collected_chapas[LEVEL_COUNT];  // bitmask por nivel (posiciones fijas conocidas en tiempo de build)
    uint32_t broken_tiles[LEVEL_COUNT][BROKEN_WORDS]; // bitmask de tiles rotos por nivel
    uint16_t deaths;
    uint32_t play_time_frames;
} SaveData;
```

Esto cabe muchas veces en los 32 KB típicos de SRAM homebrew, incluso con varios slots de guardado. La lectura/escritura vive en `platform/gba/pal_gba_save.c`; `core/save.c` solo conoce el struct y no sabe que "por debajo" es SRAM — otro ejemplo de la separación de la sección 4.

---

## 11. Riesgos y mitigaciones

| Riesgo | Mitigación |
|---|---|
| Jefes grandes (`boss_betty` ~31×22 tiles de sprite) exceden el tamaño de un objeto OAM único | Componer el jefe con una cuadrícula de sprites de 16×16/8×8 (patrón estándar en homebrew de GBA para "sprites grandes") |
| Presupuesto de VRAM ajustado si cada nivel usa tileset propio | Compartir un tileset base (ladrillo/tubería/pincho/plataforma, que ya son solo 9 `id`s hoy) y variar solo la paleta por nivel, en vez de tilesets 100% distintos por zona |
| Física "se siente distinta" al pasar de floats a punto fijo | Portar las constantes de [index.html:1656](index.html#L1656) a Q8.8 y validar lado a lado contra el prototipo web con las mismas secuencias de input (grabar inputs del prototipo y reproducirlos en el port es la forma más objetiva de comparar) |
| Scope grande (7 niveles, 6 jefes, narrativa completa) sin cortes de progreso intermedios | El plan de fases 3-8 entrega **un nivel jugable end-to-end antes de paralelizar el resto**, siguiendo el mismo consejo de "graybox primero, pulido al final" que ya recomienda el propio [Especificación de Diseño Técnico...md](<Especificación de Diseño Técnico_ Arquitectura y Progresión en Sistemas Metroidvania.md>) |
| Acoplar por error lógica de juego a hardware (rompe la modularidad) | Regla de la sección 9.1 (sin includes de plataforma en `core/`) + revisión de PR enfocada en ese único punto |

---

## 12. Próximos pasos inmediatos

1. Decidir si se adopta el build de escritorio (SDL2) para `core/` desde la Fase 0, o se pospone (recomendado: adoptarlo, el costo es bajo y el retorno en velocidad de iteración es alto).
2. Crear el repositorio con la estructura de la sección 9 (vacío, con los `.h` de interfaz de `core/` y `platform/pal.h` ya declarados aunque no implementados).
3. Ejecutar la Fase 0 y validar el toolchain end-to-end con un "hola mundo" en mGBA.
4. Empezar `tools/spritegen` sobre los `SPR.*` existentes — es el desbloqueador de todo el arte real y no depende de que el motor esté terminado.
