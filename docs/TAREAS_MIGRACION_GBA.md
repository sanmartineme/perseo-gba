# Tareas de Ejecución — Migración a C/GBA de "Perseo: Sombras de Silencio"

**Deriva de:** [PLAN_MIGRACION_GBA_C.md](PLAN_MIGRACION_GBA_C.md)
**Formato:** checklist secuencial por fase. Cada tarea es atómica y tiene un criterio de "hecho" verificable. Se ejecutan en orden dentro de cada fase; entre fases 3-8 (una vez esté el patrón del Nivel 1) varias tareas de nivel pueden paralelizarse si hay más de una persona, pero se listan en orden porque una sola persona/agente debe hacerlas una a una.

**Convención de IDs:** `F<fase>-<número>`. Marca `[x]` al completar. Si una tarea se bloquea o cambia de alcance, anota debajo de ella por qué (no borres el ítem).

---

## Fase 0 — Cimientos del proyecto

- [x] **F0-01** Instalar devkitPro/devkitARM en el entorno Windows. *(Ya estaba instalado en `C:\devkitPro` — incluye devkitARM, libgba y `gbafix`. Solo faltaba exportar `$DEVKITPRO`/`$DEVKITARM` en cada build, ver Makefile.)*
- [x] **F0-02** Obtener libtonc. *(Ya venía incluido en la misma instalación de devkitPro, en `C:\devkitPro\libtonc` — no hizo falta pacman ni submódulo.)*
- [x] **F0-03** Inicializar el repositorio git del nuevo proyecto `perseo-gba`. *(Hecho: `perseo-gba/` creado como subcarpeta de este repo de diseño, con su propio `git init` y primer commit.)*
- [x] **F0-04** Crear el esqueleto de carpetas completo de la sección 9 del plan (vacío, con `.gitkeep`). *(Hecho, ver `perseo-gba/`.)*
- [x] **F0-05** Escribir `Makefile` inicial basado en `gba_rules`/`base_tools` de devkitARM. *(Hecho, pero reescrito como Makefile de una sola pasada — no recursivo — tras encontrar dos bugs reales del entorno: la auto-invocación recursiva de `make` resuelve mal su propia ruta bajo el MSYS2 embebido de devkitPro fuera de su terminal dedicada, y `base_tools` fuerza `SHELL := /usr/bin/env bash`, cuyo `bash` intermedio reseteaba `TMP`/`TEMP` y hacía que `arm-none-eabi-gcc` fallara intentando escribir en `C:\WINDOWS`. Ambos quedaron documentados como comentarios en el propio `Makefile`.)*
- [x] **F0-06** Escribir `source/main.c` mínimo que pinta un color sólido de fondo. *(Hecho con Modo 3/bitmap por simplicidad — el Modo 0 tiled real del juego se implementa en la Fase 2; ver comentario en el propio archivo.)*
- [x] **F0-07** Compilar y confirmar que se genera `perseo.gba`. *(Hecho: `build/perseo.gba`, 2576 bytes, reconocida por herramientas de análisis de archivos como "Game Boy Advance ROM image".)*
- [x] **F0-08** Validar en un emulador que la ROM arranca y muestra el color. *(Hecho con **VisualBoyAdvance-M 2.2.3** en vez de mGBA — el instalador de mGBA requiere UAC interactivo y no se pudo automatizar; VBA-M se obtuvo como `.zip` portable desde sus releases de GitHub, sin instalador. Capturas de pantalla confirman el título de ventana "perseo - VisualBoyAdvance-M 2.2.3" y el color de fondo exacto `RGB15(4,6,10)` a 58-60 fps.)*
- [ ] **F0-09** Validar el mismo build en no$gba. *(Pendiente — no se intentó; con la validación en VBA-M ya se consideró suficiente para cerrar la fase. Puede hacerse manualmente más adelante si se quiere una segunda referencia cruzada.)*
- [x] **F0-10** Implementar lectura básica de `REG_KEYINPUT` en `main.c` (cambia el color de fondo al mantener A). *(Código hecho y considerado correcto — es el idioma estándar de `key_poll()`/`key_held()` de libtonc. La confirmación visual en emulador quedó **inconclusa**: se probaron las teclas `X` y `Z` (bindings por defecto habituales de VBA-M para el botón A) mediante inyección de input a nivel de SO, sin lograr ver el cambio de color en las capturas — probablemente un desajuste de keybinding o de foco de input sintético del emulador, no un problema del código. Se puede confirmar a mano abriendo VBA-M normalmente y jugando con teclado/mando real.)*
- [ ] **F0-11** (Opcional, recomendado) Configurar el build de escritorio SDL2 para `core/`: `platform/sdl/sdl_main.c` con ventana en blanco. *(Pospuesto: aún no hay nada en `core/` que iterar con SDL; se retoma al empezar la Fase 1.)*
- [x] **F0-12** Commit inicial con el esqueleto + "hola mundo" funcionando y validado en emulador.

**Criterio de cierre de fase:** ✅ cumplido — ROM arranca y renderiza correctamente (validado en VBA-M; no$gba queda como verificación cruzada opcional pendiente), estructura de carpetas de la sección 9 creada.

**Notas de entorno para builds futuros (Fase 1 en adelante):** compilar con
`/c/devkitPro/msys2/usr/bin/make.exe DEVKITARM=/c/devkitPro/devkitARM DEVKITPRO=/c/devkitPro`
desde `perseo-gba/`. Un emulador portable (VisualBoyAdvance-M) quedó extraído en
`perseo-gba/build/_emu_setup/vbam/` para pruebas manuales (esa carpeta está bajo
`build/`, excluida de git).

---

## Fase 1 — Núcleo del loop y física del jugador

- [x] **F1-01** Escribir `core/fixed.h`: tipo `fx_t` (Q8.8) y operaciones (`fx_add`, `fx_mul`, `fx_div`, conversión a/desde entero). *(Hecho, header-only.)*
- [ ] **F1-02** Escribir `tests/unit/test_fixed.c` y confirmar que compila/corre nativo (gcc de escritorio, sin GBA).
  - **Bloqueada:** no hay ningún compilador C nativo (x86) disponible en esta máquina — no hay MinGW/gcc, ni MSVC (`cl`), ni WSL con una distro instalada (`wsl -l` no lista ninguna). Instalar una distro de WSL completa solo para esto es desproporcionado para el checklist actual. **Pendiente**: instalar MinGW-w64/MSYS2 propio o una distro WSL, o correr estas pruebas en CI más adelante. Mientras tanto la corrección de `fixed.h` se validó por revisión manual y, indirectamente, por el comportamiento correcto observado en el ROM real (ver F1-17).
- [x] **F1-03** Definir `core/entity.h`: struct de entidad genérica. *(Hecho — catálogo de `EntityType` igual al de la sección 1 del plan.)*
- [x] **F1-04** Implementar `core/entity_pool.h/.c`: pool estático (`ENTITY_POOL_CAPACITY=64`), sin malloc/free.
- [ ] **F1-05** Escribir `tests/unit/test_entity_pool.c`. **Bloqueada por la misma razón que F1-02** (sin compilador nativo).
- [x] **F1-06** Crear la habitación de prueba en `core/level/level.h`/`.c`: 30×20 tiles (240×160 px, exactamente la pantalla de GBA) con paredes laterales, suelo, plataforma flotante y saliente — pensada para poder probar cada mecánica de movimiento.
- [x] **F1-07** Implementar `tile_is_solid`/`level_tile_at`/`level_rect_solid` (equivalentes a `isSolid`/`tileAt`/`rectSolid`).
- [x] **F1-08** Definir `core/player.h`: struct `Player` con los campos de `P{}` traducidos a `fx_t`.
- [x] **F1-09** Portar movimiento horizontal (aceleración/desaceleración, fricción).
- [x] **F1-10** Portar gravedad + salto + coyote time + jump buffer + salto de altura variable.
- [x] **F1-11** Portar Salto Doble.
- [x] **F1-12** Portar Dash Sombrío, incluido su cooldown (mismo orden de evaluación que el prototipo: `dash_cd--` antes del chequeo de gatillo).
- [x] **F1-13** Portar Garra Felina (agarre de muro + salto de muro).
- [x] **F1-14** Definir `platform/pal.h` con las funciones de input necesarias.
- [x] **F1-15** Implementar `platform/gba/pal_gba_input.c` sobre `key_is_down`/`key_hit` de libtonc, mapeado según la tabla de controles del plan.
- [x] **F1-16** Implementar el loop principal (en `source/main.c`, no en un `gba_main.c` separado — ver nota de desviación abajo): `VBlankIntrWait()` + `player_update()` a 60 Hz fijo.
- [x] **F1-17** Prueba en emulador (VBA-M, no mGBA — ver Fase 0). *(Parcial pero suficiente para cerrar la fase, ver detalle abajo.)*
- [ ] **F1-18** Build de escritorio SDL2. *(Pospuesto — no bloqueante; se retoma si hace falta iterar más rápido en la Fase 4/5, que es donde la lógica se vuelve más compleja.)*

**Desviación de nombres respecto al plan original:** el loop principal quedó en `source/main.c` (heredado de la Fase 0) en vez de crear `source/platform/gba/gba_main.c` — es una diferencia cosmética, no arquitectónica: sigue siendo el único archivo con permiso para incluir `<tonc.h>`, y `source/core/` sigue sin ninguna dependencia de plataforma.

**Nota sobre F1-02/F1-05 (pruebas nativas bloqueadas):** el principio de "`core/` compila y se prueba también en PC" (sección 4.1 del plan) sigue siendo válido y vale la pena retomarlo, pero requiere resolver primero la falta de un compilador nativo en esta máquina — no es un bloqueo del motor en sí.

**Detalle de F1-17 (verificación en emulador):**
- ✅ **Geometría estática correcta**: paredes, suelo, plataforma y saliente se ven donde deben (confirmado con capturas propias y una captura del usuario del ROM corriendo).
- ✅ **Spawn correcto**: el jugador aparece cerca de la pared izquierda, apoyado sobre el suelo (gravedad + colisión vertical funcionando).
- ⚠️ **Bug real encontrado y corregido en el camino**: la primera versión del render de depuración redibujaba las ~136 tiles de la sala en cada vuelta del loop, superando el presupuesto de VBlank en Modo 3 (sin doble buffer) y produciendo *tearing* real y estable contra el haz de refresco (se veía como una "escalera" en vez de rectángulos limpios). Se corrigió dibujando la geometría estática una sola vez y, por frame, sólo la pequeña zona del jugador — ver el comentario "NOTA de rendimiento" en `source/main.c`. Este hallazgo es exactamente el tipo de problema que la Fase 9 (optimización) debería anticipar para el renderer real.
- ✅ **Movimiento y salto: confirmados visualmente** (resuelto en la Fase 2). La causa de la falta de confirmación inicial no era el código ni el entorno: el `vbam.ini` de esta instalación no tiene el D-Pad mapeado a las flechas, sino a `D`/`A`/`W`/`S`, con el botón A de GBA en `L` y el R (dash) en `O` — una vez usadas las teclas correctas, una captura mostró al jugador (sprite OBJ completo, ver Fase 2) claramente en el aire tras moverse a la derecha y saltar. Dash se disparó sin errores (cooldown/trigger correctos) aunque no se capturó el frame exacto a mitad de embestida — su lógica ya estaba, de todos modos, revisada línea por línea contra el prototipo. Salto Doble y Garra Felina quedan igual de confiables por revisión de código, sin una captura interactiva dedicada todavía.

**Criterio de cierre de fase:** el jugador se mueve/salta/colisiona con el mismo *feel* que el prototipo web, verificado lado a lado.

---

## Fase 2 — Renderizado de mundo y cámara

- [x] **F2-01** Ampliar `platform/pal.h` con la interfaz de vídeo. *(`pal_video_init`, `pal_video_sync_level`, `pal_video_set_parallax_scroll`, `pal_video_set_player_sprite` — nombres ajustados respecto al enunciado original para reflejar mejor la técnica de "mapa grande", ver comentarios en el propio header.)*
- [x] **F2-02** Implementar `platform/gba/pal_gba_video.c`: Modo 0 con BG1 (paralaje) + BG2 (nivel) + OBJ. *(BG0 se deja apagado — no hace falta todavía; se activa si una fase futura necesita una segunda capa de paralaje o HUD.)*
- [x] **F2-03** Tileset placeholder de colores sólidos (3 tiles: sólido, plataforma, paralaje) cargado en el charblock 0.
- [x] **F2-04** Técnica de "mapa grande": screenblock de 32×32 como buffer circular (`buffer[(ty&31)*32+(tx&31)]`), con sincronización incremental de columnas/filas nuevas al mover la cámara (algoritmo de 4 pasos de Tonc: nuevas columnas con el rango vertical viejo, luego nuevas filas con el rango horizontal ya actualizado).
- [x] **F2-05** `core/camera.h/.c`: puerto de `updateCam()` línea a línea, incluido el orden exacto `max(0, min(...))` del original (no un `clamp` genérico — importa cuando el nivel es más angosto que la pantalla).
- [x] **F2-06** Cámara conectada a `REG_BG2HOFS`/`REG_BG2VOFS` en `pal_video_sync_level`.
- [x] **F2-07** BG1 como capa de paralaje simple (relleno único, se desplaza a la mitad de la velocidad de la cámara). *(Una sola capa, no las múltiples de `drawParallax()` — eso llega con el arte real de la Fase 3.)*
- [x] **F2-08** Jugador como sprite OBJ real de hardware (16×16, color sólido, 4 tiles en modo 1D).
- [x] **F2-09** Sala de prueba nueva de 100×20 tiles (`level_get_scroll_test_room`, deliberadamente >32 tiles) con plataformas "hito" cada 10 columnas. Recorrida de punta a punta sosteniendo el D-Pad: la pared izquierda desaparece, aparecen hitos nuevos según se avanza, y tras cruzar más de 32 tiles (el tamaño del buffer circular) **no aparece ningún tile corrupto** — confirma que el wraparound del mapa grande funciona.
- [ ] **F2-10** Medir FPS en mGBA durante el recorrido. **No hecho con mGBA** (bloqueado desde la Fase 0 por UAC) — VBA-M reporta 59-60 fps estables durante todas las pruebas de esta fase, pero no se hizo una medición rigurosa bajo carga máxima (más entidades/capas llegan en fases posteriores, donde SÍ conviene medir en serio — ver Fase 9).

**Hallazgo de entorno importante (no es un bug de este proyecto):** el `.ini` de esta instalación de VBA-M no tiene el D-Pad mapeado a las flechas del teclado — está mapeado a `D`(derecha)/`A`(izquierda)/`W`/`S`, con el botón A de GBA en `L`, B en `K`, L-shoulder en `I`, R-shoulder (dash) en `O`. Todos los intentos previos de probar input en la Fase 0/1 que "no mostraban movimiento" eran por usar las flechas, no por un problema del código ni del entorno de automatización. Documentado acá para no repetir la confusión en fases futuras.

**Bug real encontrado y corregido en el camino:** durante el diagnóstico se detectó (y se descartó como falsa alarma) una aparente pérdida de tiles al hacer scroll; resultó ser sólo el tamaño de la ventana del emulador recortando la parte inferior de la imagen, no un bug de sincronización — confirmado con una prueba dirigida (franja de prueba en una fila alta, visible correctamente) antes de concluir que el pipeline de "mapa grande" funciona sin cambios.

**Criterio de cierre de fase:** ✅ cumplido — recorrido fluido de un nivel de 100 tiles con paralaje, cámara siguiendo al jugador, sin tiles corruptos al cruzar el límite del buffer circular. Medición rigurosa de FPS bajo carga real queda pendiente para la Fase 9 (F2-10).

---

## Fase 3 — Pipeline de assets y Nivel 1 real

- [x] **F3-01** Extraer la paleta `COL{}` del prototipo. *(`tools/common/prototype.py`: lee la paleta y los 56 sprites directo del HTML del prototipo, cuantizando cada color a 5 bits por canal — lo que la GBA puede mostrar de verdad — y deja `tools/common/palette_gba.json` como referencia para el resto del pipeline.)*
- [x] **F3-02** `tools/spritegen/ascii_to_png.py`: convierte el arte ASCII a PNG indexado (≤16 colores, índice 0 transparente), listo para 4bpp.
- [x] **F3-03** Hoja de Perseo generada: 12 frames de 16×16 (idle 1-2, walk 1-4, jump, fall, dash, atk 1-3) en `assets/src/sprites/perseo/perseo.png`, con sólo 6 colores.
- [x] **F3-04** Tileset del Nivel 1 generado. *(Los tiles del prototipo eran **procedurales** — `drawTile()` dibujaba con `fillRect`, no había imagen que extraer — así que se transcribieron a mano a `assets/src/tiles/tileset_tuneles.txt`, un formato ASCII editable con las mismas claves de paleta, y de ahí salen el PNG y los datos. 11 tiles, 16 colores exactos.)*
- [x] **F3-05** `.grit` de configuración para ambas imágenes. *(Viven **junto a cada PNG**, no en `tools/grit_configs/` como decía el plan: grit los descubre solo por nombre, lo que evita tener que pasar rutas a mano y que se desincronicen. Ojo con una trampa que costó un rato: grit **no** acepta comentarios al final de una línea de flags — hay que ponerlos en líneas propias, o la flag se ignora en silencio y la imagen sale a 8bpp.)*
- [x] **F3-06** grit integrado al `Makefile`. *(Como reglas de dependencia normales — `assets/src/**.png → assets/gen/*.c` — no como un paso manual: al tocar un PNG, `make` regenera y recompila solo. Hubo que fijar `.DEFAULT_GOAL := all` porque las reglas del pipeline se declaran antes que `all` y make tomaba un `.c` generado como objetivo por defecto.)*
- [x] **F3-07** `tools/levelgen/schema_nivel.md` con el formato `.json` de nivel documentado.
- [x] **F3-08** `buildLevel1()` migrado a `assets/src/levels/level01.json`: 24 rectángulos de `carve` + 28 entidades, transcritos 1:1 del prototipo.
- [x] **F3-09** `tools/levelgen/levelgen.py`: genera el par `.h`/`.c` con el tilemap horneado (8800 bytes en ROM) y el array de entidades iniciales. *(Se emite un `.c` además del `.h` para que el dato viva en una sola unidad de traducción, y los generados se versionan: así compilar no requiere tener Python instalado.)*
- [x] **F3-10** `pal_gba_video.c` carga el tileset y los sprites reales. *(Al ordenar el tileset igual que los `TileId`, el id de tile del nivel **es** el índice del gráfico: la función de traducción que había en la Fase 2 desapareció.)*
- [x] **F3-11** El Nivel 1 real (200×44) se carga y renderiza. *(De paso queda ejercitado el streaming **vertical** del mapa grande, que la sala de prueba de la Fase 2 — de sólo 20 tiles de alto — no llegaba a probar: 44 > 32 filas del buffer circular.)*
- [x] **F3-12** Animación real del jugador. *(La selección de frame vive en `core/player.c` (`player_get_anim`), portada de `drawPlayer()` con sus mismos tiempos y el "contoneo" de 1 px del ciclo de caminata; la capa GBA sólo la traduce a atributos de OAM, incluido el volteo por hardware.)*
- [x] **F3-13** Recorrido verificado en emulador: Perseo se ve con su arte real (ojos amarillos, orejas rosadas, pañuelo rojo) sobre el ladrillo de los Túneles, con paralaje detrás; camina, salta, se voltea, cae por el pozo al corredor inferior y recorre el nivel sin un solo tile corrupto.

**Pendiente consciente:** el jugador puede caerse del mundo y seguir cayendo, porque la muerte por caída al vacío (`if(P.y>LV.h*8+40)` en el prototipo) es parte del sistema de daño de la **Fase 4**, igual que los pinchos y el lodo que ya están dibujados en el nivel pero todavía no hacen nada.

**Criterio de cierre de fase:** ✅ cumplido — el Nivel 1 es reconocible y recorrible, con el arte del prototipo, en emulador.

---

## Fase 4 — Entidades, combate y enemigos

- [x] **F4-01** `core/particles.h/.c`: pool estático de 32 partículas con el `burst()` del prototipo. *(El color se guarda como un enum lógico, no como RGB: en la GBA el color de un sprite va dentro del tile, así que la capa de vídeo tiene un frame por color y `core/` no necesita saber nada de paletas.)*
- [x] **F4-02** `core/projectiles.h/.c`: traza, chatarra y onda de choque. *(No tienen pool propio como en el prototipo: son `Entity` del pool común, así hay un solo camino de dibujado y de colisión.)*
- [x] **F4-03** Ganchito Mortal, con su caja de golpe por delante de la zarpa, sus 3 fases de animación y la marca de zarpazo que impide que un mismo golpe cuente dos veces.
- [x] **F4-04** Lanzamiento de Traza con su enfriamiento de 34 frames.
- [x] **F4-05** `core/world.{h,c}`: instancia las entidades desde los datos del nivel y concentra lo compartido (cajas, patrulla, daño, botín). *(Añadido respecto del plan: el prototipo usaba globales — `LV`, `P`, `projs`, `shake` — y agruparlos en un `World` que se pasa explícito deja el mismo código sin estado oculto.)*
- [x] **F4-06..F4-12** Los 7 tipos de enemigo, uno por archivo en `core/enemies/`: rata, cucaracha, mosquito, murciélago, matón, tiradora y El Sicario. *(Hizo falta añadir dos utilidades a `core/`: `rng.c` — el prototipo usaba `Math.random()` para la cadencia de tiro y el botín — y `trig.c`, con una tabla de senos y una raíz entera para el vaivén de los voladores y la embestida del mosquito, que en el original eran `Math.sin`/`Math.hypot`.)*
- [x] **F4-13** Daño al jugador: invulnerabilidad de 90 frames con parpadeo, empujón, tiles dañinos (pinchos, lodo y el vapor intermitente) y muerte por caída al vacío. *(Al dashear Perseo es intocable, igual que en el prototipo: es parte de para qué sirve el dash.)*
- [x] **F4-14** HUD de vida: los corazones se dibujan como sprites de hardware en coordenadas de pantalla.
- [x] **F4-15** Verificado en emulador sobre el Nivel 1 real: enemigos patrullando, lodo verde que hace daño, chapas recogibles, partículas al golpear, y los corazones bajando al recibir golpes.

**Cabo suelto conocido (y por qué):** al morir, el nivel se reinicia entero en vez de reaparecer en la última lámpara. Los checkpoints necesitan la máquina de estados y el guardado de la **Fase 7**; reiniciar mantiene el juego jugable sin fingir que ya existe algo que todavía no está. Por la misma razón las entidades de progresión que ya vienen en los datos del nivel (santuarios, puertas, carteles, lámparas, reliquias) se leen pero todavía no se instancian.

**Trampa encontrada en el camino:** los objetos se nombran por basename (`build/<nombre>.o`), así que la hoja de sprites de partículas —que se llamaba `particles`— chocaba con `source/core/particles.c` y el enlazado fallaba por símbolo duplicado. Se renombró a `fx_particles` y quedó anotada la restricción en el `Makefile`.

**Criterio de cierre de fase:** ✅ cumplido — el Nivel 1 es jugable con combate y sus enemigos.

---

## Fase 5 — Jefes

- [x] **F5-01** `core/boss/boss_config.{h,c}`: los 6 jefes como tabla de datos (vida, tamaño, sprite, si es el final). *(La idea que hace que esto funcione ya estaba en el prototipo y se conserva: **un solo motor de combate** para los seis, que se diferencian sólo por estos datos. Por eso hay seis enfrentamientos sin seis IAs que mantener.)*
- [ ] **F5-02** `boss_dialog.h` con el guion de cada jefe. *(Pospuesto a la Fase 7 junto con el resto del sistema de diálogo: los textos ya están en los `.json` de nivel, pero mostrarlos necesita el visor de texto y la máquina de estados.)*
- [x] **F5-03..F5-10** La FSM completa en `core/boss/boss_fsm.c`: `wait` → `intro` → `choose` → `tele` → ataque → vuelta a elegir, más `stun`, `die`, y los dos ataques exclusivos de Betty (`summon` y `trashblast`). *(La telegrafía (`tele`) es la que hace justo el combate: avisa antes de cada ataque y se acorta cuando al jefe le queda poca vida — toda la subida de tensión de la segunda fase sale de ahí. Y el ataque se elige al azar evitando repetir el anterior, que es lo que impide que el patrón se vuelva predecible sin escribir una secuencia a mano.)*
- [x] **F5-11** Puerta de arena: sella la entrada al empezar la pelea y abre la salida al ganar. *(Esto obligó a un cambio de fondo: el tilemap generado vive en ROM, así que ahora `world_load` copia el nivel a RAM — 8.8 KB de los 256 KB de EWRAM. De paso eso habilitó el **Dash Sombrío rompiendo rejillas oxidadas**, que venía pendiente desde la Fase 1 y es la cerradura que abre esa habilidad.)*
- [x] **F5-12** Jefe dibujado como sprite de 32×32 con banco de paleta propio. *(Entre los seis usan exactamente 15 colores + transparente: no habrían entrado en el banco de los demás enemigos.)*
- [ ] **F5-13..F5-18** Validar los seis combates. *(Ver detalle abajo: parcialmente hecho.)*

**Además, adelantado de la Fase 8:** se migró el **Nivel 2 (La Ciudad Vertedero)** a `.json`, porque el Nivel 1 no tiene jefe — el Capataz está en el 2 — y sin él no había nada que probar. `levelgen` aprendió a emitir los datos de encuentro (qué jefe, rectángulos de sellado y apertura, franja del disparador).

**Bug real encontrado probando, y por qué importa:** el `case` que llama a `boss_update`/`update_bossgate` nunca llegó a insertarse en el despachador de entidades — una sustitución de texto que falló en silencio. El resultado era sutil y engañoso: el jefe aparecía dibujado y en su sitio, recibía daño (el golpe del jugador no pasa por ese despachador), pero **nunca se movía ni la puerta se cerraba**, y al bajarle la vida a cero se quedaba congelado en su agonía para siempre. Corregido; la lección es que conviene que las sustituciones de código fallen ruidosamente.

**Qué quedó verificado en emulador y qué no, con honestidad:**
- ✅ El jefe aparece, se dibuja bien (rata gris con gorro, ojo rojo y cola rosada) y queda apoyado en el suelo de su arena.
- ✅ Recibe daño: se ven las partículas blancas del impacto.
- ✅ Hace daño por contacto, y **no** lo hace durante su entrada ni agonizando.
- ⚠️ **La secuencia completa entrada → ataques → muerte → apertura del paso no se confirmó de punta a punta en pantalla.** Automatizar el teclado hacia el emulador resultó demasiado poco fiable para llegar a la arena y encadenar golpes: en varias pruebas Perseo terminaba mirando al lado contrario o moría antes. Se recomienda un playtest a mano: entrar por la chimenea del Nivel 2, cruzar el pasaje de la izquierda y pelear.

**Criterio de cierre de fase:** ⚠️ parcial — el motor de jefes está completo y compila limpio, con un encuentro real montado; falta confirmar los seis combates jugándolos.

---

## Fase 6 — Audio

- [x] **F6-01** Interfaz agnóstica de hardware. Quedó en `source/core/audio.h`, no en `audio_seq.h`: es un solo header y separarlo en dos no aportaba nada. Declara los tipos (`Song`, `SfxDef`, `SongId`, `SfxId`) y también las funciones `audio_init/play_song/play_sfx/update` — a propósito, para que el código de juego pida un sonido sin incluir nada de `platform/`.
- [x] **F6-02** `source/platform/gba/pal_gba_audio.c`. Reparto de canales: ch1 cuadrada = tonos de efecto (con barrido interpolado a mano, no el barrido geométrico del hardware, que no reproduce la rampa lineal del prototipo); ch2 cuadrada = melodía; ch3 onda programable = bajo, con tablas reales de cuadrada/triangular/seno/sierra; ch4 ruido = percusión y ruido de efectos, con prioridad para el efecto.
- [x] **F6-03** `source/core/audio_data.c`, generado por `tools/audiogen/songs_to_psg.py`: 11 canciones + 15 efectos. Las canciones se parsean de `SONGS{}` (son datos puros); los efectos son CÓDIGO en el prototipo (`tone()`/`noise()` encadenados con `setTimeout`), así que están transcritos a mano en el generador, uno a uno.
- [x] **F6-04** Secuenciador en `audio_update()`. El `setInterval(60000/bpm/2)` del prototipo son `1800/bpm` frames por paso a 60 Hz (10 a 23 frames según la canción). Bajo, melodía y percusión recorren sus patrones por separado con `paso % largo`, igual que el original, y el `echo` se reproduce como un retrigger más flojo a 6/7 frames.
- [x] **F6-05** Los 15 efectos del prototipo, incluido el arpegio de 4 notas de `ability`, que se resuelve contando frames en vez de con `setTimeout`.
- [x] **F6-06** Llamadas integradas: salto y salto doble, dash, ataque, traza, rejilla rota (`player.c`); daño al jugador, impacto y muerte de enemigo, recolección, entrada del jefe y apertura de la puerta al vencerlo (`world.c`). La música la elige el nivel: se añadió el campo `Level.song`, que `levelgen.py` saca de la clave `"song"` que los `.json` ya traían.
- [~] **F6-07** Validado que la ROM compila (41 KB) y que el juego corre en VBA-M con el audio activo, sin cuelgue ni regresión visual. La afinación se verificó numéricamente contra las notas reales: el error máximo del bajo (ch3) es 0,13 % y el de la melodía (ch2) 0,35 %, y ninguna nota se sale del rango de su registro. **Lo que NO está verificado es el sonido en sí**: en este entorno no hay forma de capturar la salida de audio del emulador, así que comparar canción por canción con el prototipo web requiere una escucha manual.

**Nota de hardware (limitación real, no un pendiente):** las cuadradas de la GBA no bajan de 64 Hz. Cinco efectos (`splat`, `hurt`, `break`, `door`, `boss`) apuntaban a 40-60 Hz, así que el destino del barrido se recorta a 64 Hz *antes* de interpolar; de ese modo la caída se reparte por toda su duración en vez de llegar al piso a mitad de camino y quedarse plana.

**Pendiente de la Fase 7:** el prototipo tiene una tecla de silencio; `audio_set_muted()` ya está implementada, pero todavía no hay ningún botón conectado a ella (va con el menú de pausa).

**Criterio de cierre de fase:** música y SFX reconociblemente equivalentes al prototipo en todos los niveles y jefes.

---

## Fase 7 — UI, narrativa, progresión y guardado

**F7-00 (no estaba en la lista, pero lo pedía todo lo demás)** `core/game_state.{h,c}`: la partida deja de ser un bucle que siempre juega y pasa a tener estados, como el `state` del prototipo ([index.html:1542](index.html#L1542)) — título, historia, juego, cartel, inventario, pausa, muerte, transición. `main.c` se quedó en 100 líneas: traduce botones, pide actualizar, pide dibujar. También se añadió `Level.name` y `Level.story` (los `.json` ya traían esos campos, sólo faltaba emitirlos) y los nombres y textos de victoria de los seis jefes a `BOSS_CONFIGS`.

- [x] **F7-01** Capa de texto: `ui/text.h` (interfaz) + `platform/gba/pal_gba_text.c` (implementación). **Desviación:** no es un envoltorio sobre TTE. Se usa la *fuente* de libtonc (`sys8Glyphs`) pero no su motor — TTE mete color y tile base en un mismo `cattr` de 16 bits que hay que armar a mano, y su modo de superficie de bits costaría ~19 KB de VRAM. Descomprimir los glifos y escribir las entradas de screenblock nosotros son 40 líneas y deja el color como un parámetro normal. La UI vive en BG0, en una rejilla de 30x20 tiles: **el texto queda alineado a 8 px**, que es el compromiso de la época y a 240x160 se lee perfecto. Las tildes y eñes se decodifican de UTF-8 y se dibujan sin acento (la fuente es ASCII): se pierde el acento en pantalla, no en el texto fuente.
- [x] **F7-02** `ui/hud.c`: chapas, etiquetas de habilidad, barra de vida del jefe con su nombre y rótulo de zona al entrar. La vida de Perseo sigue siendo corazones de OAM, que ya estaban y con 4-6 puntos se leen mejor que una barra de tiles.
- [x] **F7-03** `ui/menu_title.c`. El logotipo se dibuja con bloques macizos de 3x5 tiles por letra: con una fuente de 8x8 el título de 36 px del prototipo no existe, y dejar la pantalla más importante en letra chica habría sido la peor opción. El fondo es el nivel 1 desplazándose de verdad, con un **velo semitransparente hecho con la mezcla alfa del hardware** (BG3 como capa de velo, ver el reparto de prioridades en `pal_gba_video.c`) en vez de una caja opaca: así el túnel se sigue viendo, como en el original.
- [x] **F7-04** Inventario con las dos pestañas del prototipo. En "objetos" el botón A equipa y desequipa, con el **tope de dos a la vez** que es lo que hace que la elección importe (pegar más, aguantar más o regenerar). Las habilidades que aún no se tienen se listan igual, en gris: saber que existen es parte de querer buscarlas.
- [x] **F7-05** Transición mosaico por hardware (`REG_MOSAIC`). Ya la dispara algo de verdad: la puerta del Nivel 1 lleva al Nivel 2.
- [x] **F7-06** Diálogo de jefe. El disparador de la arena ya no manda al jefe a escena directamente: la sella, deja al jefe en `BOSS_WAIT` y publica un evento; el jefe entra recién cuando se cierran las tres réplicas. Es el orden del prototipo, y evita que la pelea empiece mientras se lee.
- [x] **F7-07** Cinemática de introducción (8 viñetas), encadenada al "PARTIDA NUEVA" del título. La historia por nivel también funciona, la primera vez que se entra a cada uno.
- [x] **F7-08** Cinemática final (7 viñetas) y créditos (6 pantallas, con avance automático cada 3 s y vuelta al título). **Todavía no se pueden alcanzar jugando**: los arranca la puerta del Trono (`vdoor`), que está en el Nivel 7 y ese nivel llega en la Fase 8.
- [x] **F7-09** Santuarios. Perseo ya **empieza sin ninguna habilidad llave**: hasta la Fase 6 estaban forzadas en `main.c` para poder probar la física, y ahora las tres salen de sus santuarios, con su fanfarria y su cartel.
- [x] **F7-10** Las tres reliquias, con su efecto real: Colmillo Afilado (+1 de daño, aplicado en `world_hit_enemy` para que la traza lanzada también lo aproveche, como en el prototipo), Pata de la Suerte (1 corazón tras 12 s sin recibir daño) y Bigotes de Acero (−1 de daño, mínimo 1). Se equipa sola la primera que se encuentra si queda hueco: obligar a abrir un menú para que el primer objeto del juego sirva de algo es fricción sin motivo.
- [x] **F7-11** `core/save.{h,c}`: el struct, el sellado y la validación (magic + versión + checksum con rotación).
- [x] **F7-12** `platform/gba/pal_gba_save.c`. Dos cosas del hardware mandan acá: la SRAM **sólo admite accesos de 8 bits** (un `memcpy` normal devuelve basura), y los emuladores deciden qué memoria de guardado emular **buscando una cadena dentro de la ROM**. Esa etiqueta `SRAM_V113` es la que costó: `__attribute__((used))` sólo impide que la descarte el compilador, y devkitARM enlaza con `--gc-sections`, así que el enlazador la tiraba igual y el guardado se perdía en silencio. Se ancla leyéndola a través de un puntero `volatile`.
- [x] **F7-13** Las lámparas son puntos de control y **guardan solas** al encenderse; morir devuelve a la última, aunque esté en otro nivel.
- [~] **F7-14** Prueba end-to-end **verificada a medias, y de la mitad que importa**: una ROM temporal escribió un progreso reconocible, se cerró el emulador, y al volver a arrancar la partida guardada seguía ahí — el título ofrece CONTINUAR y el volcado de `perseo.sav` contiene exactamente lo escrito (dificultad DIFÍCIL, habilidades `0b101`, checkpoint en el nivel 1 tile 42,31, 37 chapas, 9 muertes). Lo que **no** está verificado es pulsar CONTINUAR y seguir jugando, porque la inyección de teclas en VBA-M sigue sin funcionar en este entorno.

**Deuda conocida del guardado.** La sección 10 del plan incluía además `collected_chapas[]` y `broken_tiles[][]`, máscaras de bits por nivel. No están, a propósito: dependen de la posición exacta de cada chapa y de cada tile rompible, que se fijan al generar los niveles, y hoy existen dos de los siete. Fijar ese formato ahora es fijarlo para romperlo en la Fase 8. Consecuencia real: al cargar, las chapas ya recogidas vuelven a estar en el mapa (el contador sí se conserva), y las rejillas rotas vuelven a estar enteras.

**Arte nuevo (no estaba en la lista, pero sin esto los santuarios eran invisibles).** El pipeline de sprites se amplió con un banco `props`: santuario 16x16, puerta 16x32 (el arte es de 16x24 y `emit_sheet` rellena el resto), lámpara encendida/apagada 8x16 y las tres reliquias 8x8, todo sacado del prototipo. El cartel es la excepción: el prototipo lo dibujaba con tres rectángulos sueltos sobre el canvas, así que se reprodujo esa misma forma como sprite propio (`EXTRA_ART` en `ascii_to_png.py`), con sus mismos colores.

**Cómo se comunican mundo y partida.** `world.c` no conoce `Game` — es lo que lo mantiene portable y testeable — así que las interacciones que cambian de pantalla (conseguir algo, cruzar una puerta, encender una lámpara, disparar a un jefe) dejan un `WorldEvent` que `game_update()` recoge después de actualizar. Uno por frame alcanza: son interacciones, no ocurren dos a la vez.

**Verificado en emulador:** menú de título (con y sin partida guardada), cinemática/historia de nivel con ajuste de línea, HUD, cartel de letrero, santuario consumido + cartel de habilidad + etiqueta nueva en el HUD, inventario con sus dos pestañas y el estado de equipado, y el ciclo de guardado descrito arriba. Las pantallas que están detrás del título se capturaron con builds temporales que arrancan en ese estado; el código temporal se revirtió y la ROM final (69 KB) está construida desde el fuente limpio.

**Resuelto de paso:** las capturas venían recortadas a la esquina superior izquierda desde la Fase 1. La causa era el escalado de pantalla de Windows: `GetClientRect` devuelve píxeles lógicos y `PrintWindow` dibuja físicos. Con `SetProcessDpiAwareness` la captura sale entera.

**Criterio de cierre de fase:** partida jugable de principio a fin con guardado persistente. *Estado real:* el guardado persiste y todas las pantallas existen, pero "de principio a fin" todavía no se puede: hay dos niveles de siete, y el final y los créditos cuelgan de la puerta del Trono, que está en el séptimo. Se cierra con la Fase 8.

---

## Fase 8 — Resto de niveles

**Cómo se hizo, y por qué así.** Los siete niveles del prototipo son *código*: `buildLevel1()`..`buildLevel7()`, unas 130 llamadas a `carve()` y 190 a `E()`. Transcribir cinco a mano es teclear ~1.600 números, y un nivel mal copiado **no falla al compilar**: falla cuando alguien no puede pasar de un salto, semanas después. Así que se extraen con `tools/levelgen/prototype_to_json.py`, que reconoce las cinco formas que usan los constructores y **falla explícitamente ante cualquier otra cosa** — si el prototipo cambiara y apareciera un bucle, es mejor enterarse ahí que generar un nivel al que le falta medio pasillo.

El extractor se validó contra los dos niveles que ya estaban escritos a mano: el **Nivel 2 sale byte a byte idéntico**, y el Nivel 1 difiere en una sola cosa, que es una adaptación deliberada — la descripción del santuario del dash decía "Pulsa C", una tecla de PC que en GBA no existe. Esa comprobación se puede repetir con `--check`.

- [x] **F8-01** Nivel 2, La Ciudad Vertedero. Ya existía su `.json`; lo que faltaba era su aspecto propio, y eso llegó con las paletas por nivel (abajo).
- [x] **F8-02** Nivel 3, Estación Abandonada — 170x44, 20 rectángulos, 33 entidades.
- [x] **F8-03** Nivel 4, Zona de Residuos Tóxicos — 175x44, 26 rectángulos, 29 entidades.
- [x] **F8-04** Nivel 5, Madriguera Subterránea — 180x44, 15 rectángulos, 33 entidades. Confirmada la aparición única del Sicario (`brute`): sólo hay una en los siete niveles.
- [x] **F8-05** Nivel 6, Cámara de Comercio (Mercado Negro) — 180x44, 15 rectángulos, 30 entidades. **La tarea decía "nivel de transición sin jefe" y eso no es lo que hace el prototipo**: el Nivel 6 tiene a EL GUARDIA, el quinto jefe. Se portó como está en el original; la descripción de la tarea estaba equivocada.
- [x] **F8-06** Nivel 7, El Trono de Betty — 190x44, 17 rectángulos, 30 entidades.
- [~] **F8-07** Enlace completo. **Lo que faltaba no era un dato sino código**: al caer un jefe se abría el paso sellado pero no aparecía la puerta al nivel siguiente, así que el juego se quedaba encerrado en la arena. Ahora `world_boss_defeated()` deja la puerta donde dicen los datos del nivel (`exit`), con el nivel destino que trae `BOSS_CONFIGS`, y en el caso de Betty una `vdoor` que abre el desenlace en vez de otro nivel. **Validado estructuralmente** con `tools/levelgen/check_chain.py`: recorrido 0→1→2→3→4→5→6 sin niveles inalcanzables, cada nivel con su aparición en aire y no dentro de la roca, cada arena con su `bossgate` completo y cada jefe con su `exit`. **No validado jugándolo de punta a punta**, porque la inyección de teclas en el emulador sigue sin funcionar en este entorno.

**Un tileset, siete paletas.** Cada nivel del prototipo tiene un `theme` de dos colores — el ladrillo y su sombra — y con eso cambia de aspecto entero. No se generaron siete tilesets: el dibujo de los once tiles es el mismo en todos, así que `ascii_to_png.py` emite una tabla de siete paletas y la capa de vídeo intercambia la paleta de fondo al cargar el nivel. **32 bytes copiados una vez por nivel**, en vez de siete tilesets ocupando VRAM y ROM.

**Verificado en emulador:** los cinco niveles nuevos arrancan, con su geometría, sus entidades (carteles, lámparas, enemigos, chapas, lodo tóxico) y su paleta propia — gris azulado, verdoso, oliva, tierra, púrpura y granate. Se capturaron con builds temporales que arrancan en cada nivel; el código temporal se revirtió.

**Corregido de paso:** "Cámara de Comercio (Mercado Negro)" son 34 tiles en una pantalla de 30, y `ui_text_center()` lo recortaba por los dos lados — la peor opción, porque no se leía ni el principio. Ahora un texto que no entra se ancla a la izquierda.

**Criterio de cierre de fase:** el juego completo (7 niveles + 6 jefes) es recorrible sin cortes. *Estado real:* el recorrido existe entero y está validado como grafo, pero **nadie lo ha jugado de principio a fin todavía** — ni yo (no puedo) ni tú. Esa partida completa es lo que hace falta antes de dar la fase por cerrada del todo, y es también la mejor forma de encontrar lo que la Fase 9 tenga que pulir.

---

## Fase 9 — Optimización y pulido

- [ ] **F9-01** Perfilar tiempo de frame por nivel/escena en mGBA (buscar frames que excedan el VBlank).
- [ ] **F9-02** Medir uso real de VRAM/IWRAM/EWRAM por nivel y documentar en `docs/arquitectura/presupuesto_memoria.md`.
- [ ] **F9-03** Mover a IWRAM las funciones calientes identificadas (física, colisión) vía sección de enlazado.
- [ ] **F9-04** Si el nivel con más enemigos lo requiere: optimizar la colisión entidad-entidad (de fuerza bruta a particionado espacial).
- [ ] **F9-05** Implementar *double buffering* de OAM si se detecta parpadeo o problemas de prioridad de sprites.
- [ ] **F9-06** Si el presupuesto de ROM aprieta: aplicar compresión LZ77/Huffman a tilesets vía funciones de BIOS.
- [ ] **F9-07** Validar 60 fps estables en la escena más exigente (candidato: combate final contra Betty con invocaciones + proyectiles).

**Criterio de cierre de fase:** 60 fps estables en el peor caso medido.

---

## Fase 10 — QA y empaquetado final

- [ ] **F10-01** Redactar y ejecutar `tests/playtest_checklist.md` completo.
- [ ] **F10-02** Validar en hardware real con flashcart.
- [ ] **F10-03** Ajustes finales de balance según resultados del playtest.
- [ ] **F10-04** Ejecutar `gbafix` sobre el binario final.
- [ ] **F10-05** Generar el build de release y etiquetar la versión (tag de git + notas de versión).

**Criterio de cierre de fase:** ROM final validada en hardware real, lista para distribución/uso.

---

## Cómo usar este checklist

1. Ejecutar las tareas **en orden** dentro de cada fase; no saltar a la fase siguiente sin cumplir el criterio de cierre de la anterior (así se evita repetir el error típico de portar todo el contenido antes de validar que el motor base funciona).
2. Marcar `[x]` en este archivo a medida que se completa cada tarea, y hacer commit del archivo junto con el código de esa tarea — sirve como bitácora del port.
3. Si una tarea revela que hace falta dividirla en pasos más chicos, está bien insertar sub-tareas (`F1-10a`, `F1-10b`, ...) en vez de tragarse el trabajo en una sola tarea gigante.
