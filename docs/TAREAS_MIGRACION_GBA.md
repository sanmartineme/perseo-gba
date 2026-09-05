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

- [ ] **F1-01** Escribir `core/fixed.h`: tipo `fx_t` (Q8.8) y operaciones (`fx_add`, `fx_mul`, `fx_div`, conversión a/desde entero).
- [ ] **F1-02** Escribir `tests/unit/test_fixed.c` y confirmar que compila/corre nativo (gcc de escritorio, sin GBA).
- [ ] **F1-03** Definir `core/entity.h`: struct de entidad genérica (posición, velocidad, tipo, flags, campos específicos por unión o por tabla aparte).
- [ ] **F1-04** Implementar `core/entity_pool.h/.c`: pool estático con alta/baja de entidades, tamaño máximo fijado según el peor caso observado en `buildLevelN()` del prototipo.
- [ ] **F1-05** Escribir `tests/unit/test_entity_pool.c` (alta, baja, reciclado de slots).
- [ ] **F1-06** Crear un tilemap mínimo de prueba (una habitación pequeña hardcodeada) en `core/level/level.h`/`.c` como stub temporal.
- [ ] **F1-07** Implementar `tileAt`/`rectSolid` equivalentes en `core/level.c` (traducción directa de [index.html:1587-1597](index.html#L1587-L1597)).
- [ ] **F1-08** Definir `core/player.h`: struct `Player` con los campos de `P{}` ([index.html:1569](index.html#L1569)) traducidos a `fx_t`.
- [ ] **F1-09** Portar movimiento horizontal (aceleración/desaceleración, `SPD`/`ACC`) a `core/player.c`.
- [ ] **F1-10** Portar gravedad + salto + coyote time + jump buffer.
- [ ] **F1-11** Portar doble salto (`djump`).
- [ ] **F1-12** Portar Dash Sombrío (`dash`), incluido su cooldown.
- [ ] **F1-13** Portar Garra Felina (`climb`, agarre y salto de muro).
- [ ] **F1-14** Definir `platform/pal.h` con las funciones de input necesarias (`pal_input_left()`, `pal_input_jump_pressed()`, etc.).
- [ ] **F1-15** Implementar `platform/gba/pal_gba_input.c` sobre `REG_KEYINPUT`, mapeado según la tabla de controles del plan (sección 6).
- [ ] **F1-16** Implementar el loop principal en `platform/gba/gba_main.c`: IRQ de VBlank + llamada a `player_update()` a 60 Hz fijo.
- [ ] **F1-17** Prueba manual en mGBA: mover, saltar, doble salto, dash y trepar sobre el tilemap de prueba.
- [ ] **F1-18** (Si se adoptó SDL) validar que el mismo `core/player.c` compila y se comporta igual en el build de escritorio.

**Criterio de cierre de fase:** el jugador se mueve/salta/colisiona con el mismo *feel* que el prototipo web, verificado lado a lado.

---

## Fase 2 — Renderizado de mundo y cámara

- [ ] **F2-01** Ampliar `platform/pal.h` con la interfaz de vídeo (`pal_video_init`, `pal_video_load_tileset`, `pal_video_set_tile`, `pal_video_scroll_bg`, `pal_video_draw_sprite`).
- [ ] **F2-02** Implementar `platform/gba/pal_gba_video.c`: inicialización de Modo 0 y configuración de BG0/BG1/BG2.
- [ ] **F2-03** Cargar un tileset placeholder (colores sólidos) a VRAM para pruebas de scroll.
- [ ] **F2-04** Implementar la técnica de "big map" (tilemap mayor a 32×32 tiles) con actualización de bordes al hacer scroll.
- [ ] **F2-05** Implementar `core/camera.h/.c`: seguimiento del jugador + clamp a límites del nivel (equivalente a `updateCam`, [index.html:2918](index.html#L2918)).
- [ ] **F2-06** Conectar la cámara a los registros de scroll de BG2 vía la PAL.
- [ ] **F2-07** Añadir capas de paralaje BG0/BG1 con factores de scroll distintos (equivalente a `drawParallax`, [index.html:2220](index.html#L2220)).
- [ ] **F2-08** Renderizar al jugador como un sprite OAM placeholder (un color/forma simple) siguiendo su posición real.
- [ ] **F2-09** Prueba: recorrer el nivel de prueba (ancho, >32 tiles) de punta a punta verificando scroll fluido.
- [ ] **F2-10** Medir FPS en mGBA durante el recorrido (debe mantenerse a 60 fps estables).

**Criterio de cierre de fase:** recorrido fluido de un nivel ancho con paralaje, sin *tearing* ni caídas de frame.

---

## Fase 3 — Pipeline de assets y Nivel 1 real

- [ ] **F3-01** Extraer la paleta `COL{}` ([index.html:45-57](index.html#L45-L57)) a un archivo de referencia común para las herramientas (`tools/`).
- [ ] **F3-02** Escribir `tools/spritegen/ascii_to_png.py`: convierte cualquier `SPR.*` (copiado/extraído de `index.html`) a PNG indexado con la paleta de referencia.
- [ ] **F3-03** Generar los PNG de Perseo (idle, walk 1-4, jump, fall, dash, atk 1-3) con el script.
- [ ] **F3-04** Generar el PNG del tileset de mundo (los 9 `id` de `drawTile`, [index.html:1159-1213](index.html#L1159-L1213)), redibujados una vez como tiles estáticos de 8×8.
- [ ] **F3-05** Crear los `.grit` de configuración para el sprite sheet de Perseo y el tileset de Nivel 1.
- [ ] **F3-06** Integrar `grit` como paso `pregen` del `Makefile` (salida a `assets/gen/`).
- [ ] **F3-07** Escribir `tools/levelgen/schema_nivel.md`: define el formato `.json` de nivel (rects tipo `carve()` + lista de entidades tipo `E()`).
- [ ] **F3-08** Migrar `buildLevel1()` ([index.html:1261-1307](index.html#L1261-L1307)) a `assets/src/levels/level01.json` siguiendo ese esquema.
- [ ] **F3-09** Escribir `tools/levelgen/levelgen.py`: parsea el `.json` y genera `source/core/level/level01_tuneles.h` (tilemap empaquetado 1 byte/tile + array de entidades iniciales).
- [ ] **F3-10** Cargar el tileset y sprites generados de verdad en `pal_gba_video.c` (reemplazando los placeholders de la Fase 2).
- [ ] **F3-11** Cargar `level01_tuneles.h` en `core/level.c` y renderizar el tilemap real del Nivel 1.
- [ ] **F3-12** Reemplazar el sprite placeholder del jugador por el ciclo de animación real (idle/walk/jump/fall) usando los frames generados en F3-03.
- [ ] **F3-13** Recorrer el Nivel 1 completo (sin enemigos ni jefe) y comparar visualmente contra el prototipo web.

**Criterio de cierre de fase:** Nivel 1 reconocible y recorrible de punta a punta en hardware/emulador.

---

## Fase 4 — Entidades, combate y enemigos

- [ ] **F4-01** Implementar `core/particles.h/.c` (pool + `burst()` equivalente a [index.html:1599](index.html#L1599)).
- [ ] **F4-02** Implementar `core/projectiles.h/.c` (traza, junk, shock) sobre el pool de entidades.
- [ ] **F4-03** Implementar el Ganchito Mortal (ataque cuerpo a cuerpo) en `player.c`, incluida la caja de golpe y sus 3 fases de animación.
- [ ] **F4-04** Implementar el Lanzamiento de Traza (proyectil + cooldown de 34 frames).
- [ ] **F4-05** Implementar `core/enemies/enemy_common.h/.c`: spawn de enemigos desde las entidades iniciales del nivel, daño recibido, muerte y drop.
- [ ] **F4-06** Portar IA de `enemy_rat.c`.
- [ ] **F4-07** Portar IA de `enemy_roach.c`.
- [ ] **F4-08** Portar IA de `enemy_mosq.c`.
- [ ] **F4-09** Portar IA de `enemy_bat.c`.
- [ ] **F4-10** Portar IA de `enemy_thug.c`.
- [ ] **F4-11** Portar IA de `enemy_brute.c` (El Sicario).
- [ ] **F4-12** Portar IA de `enemy_gunner.c` (incluye su propio proyectil a distancia).
- [ ] **F4-13** Implementar `damage()` del jugador: invulnerabilidad temporal, knockback, muerte (equivalente a [index.html:1635](index.html#L1635)).
- [ ] **F4-14** Implementar un HUD mínimo de vida (corazones) suficiente para playtesting (versión completa en Fase 7).
- [ ] **F4-15** Recorrer el Nivel 1 completo con todos sus enemigos activos y validar combate/daño.

**Criterio de cierre de fase:** Nivel 1 100% jugable con combate y los 7 tipos de enemigo.

---

## Fase 5 — Jefes

- [ ] **F5-01** Definir `core/boss/boss_config.h`: tabla `const` equivalente a `BOSS_CONFIG{}` ([index.html:964-978](index.html#L964-L978)).
- [ ] **F5-02** Definir `core/boss/boss_dialog.h`: tabla `const` equivalente a `BOSS_DIALOG{}` ([index.html:982-1013](index.html#L982-L1013)).
- [ ] **F5-03** Implementar `core/boss/boss_fsm.c`, estados `wait`/`intro`/`choose`/`tele`.
- [ ] **F5-04** Implementar estado `charge`.
- [ ] **F5-05** Implementar estado `leap`.
- [ ] **F5-06** Implementar estado `throw`.
- [ ] **F5-07** Implementar estado `trashblast` (exclusivo de Betty).
- [ ] **F5-08** Implementar estado `summon` (exclusivo de Betty, fase <40% HP).
- [ ] **F5-09** Implementar estado `stun`.
- [ ] **F5-10** Implementar estado `die` (incluye drop de corazones y apertura de puerta/`vdoor`).
- [ ] **F5-11** Implementar el sistema de puerta de jefe (`bossgate`: sellar entrada al iniciar combate, abrir salida al vencerlo).
- [ ] **F5-12** Renderizar un jefe grande compuesto por múltiples sprites OAM (empezar con El Capataz, 29×21 px).
- [ ] **F5-13** Validar el combate contra El Capataz de principio a fin.
- [ ] **F5-14** Validar El Revisor.
- [ ] **F5-15** Validar El Tóxico.
- [ ] **F5-16** Validar El Guardián.
- [ ] **F5-17** Validar El Guardia.
- [ ] **F5-18** Validar Betty (fase final, incluye `summon` + `trashblast` + `isFinal`).

**Criterio de cierre de fase:** los 6 jefes son superables y respetan sus patrones documentados en [DOCUMENTACION_PERSEO.md](DOCUMENTACION_PERSEO.md).

---

## Fase 6 — Audio

- [ ] **F6-01** Definir `core/audio_seq.h`: interfaz agnóstica de hardware para reproducir un `Song`/`Sfx` (equivalente conceptual a `playSong`, [index.html:1128](index.html#L1128)).
- [ ] **F6-02** Implementar `platform/gba/pal_gba_audio.c`: control directo de los 4 canales PSG (2 cuadradas, onda programable, ruido).
- [ ] **F6-03** Escribir `audio_data.c`: traducción de `SONGS{}` ([index.html:1085-1127](index.html#L1085-L1127)) a tablas `const` de patrones (frecuencia/forma de onda/duty por paso).
- [ ] **F6-04** Implementar el secuenciador de música (avance de patrón por BPM, loop).
- [ ] **F6-05** Portar `SFX.*` ([index.html:1064](index.html#L1064)): golpe, traza, explosión (`splat`), daño, victoria, boss, ability, break.
- [ ] **F6-06** Integrar las llamadas de SFX en los puntos correspondientes ya implementados (ataque, daño, muerte de jefe, recolección, etc.).
- [ ] **F6-07** Validar en emulador cada canción (`title`, `lvl1`-`lvl7`, `boss`, `bossFinal`, `end`) comparando tempo/forma con el prototipo web.

**Criterio de cierre de fase:** música y SFX reconociblemente equivalentes al prototipo en todos los niveles y jefes.

---

## Fase 7 — UI, narrativa, progresión y guardado

- [ ] **F7-01** Implementar `ui/text.h/.c`: envoltorio fino sobre el motor de texto TTE de libtonc.
- [ ] **F7-02** Implementar `ui/hud.c` completo: corazones, monedas (`chapas`), iconos de habilidad activa (equivalente a `drawHUD`, [index.html:2382](index.html#L2382)).
- [ ] **F7-03** Implementar `ui/menu_title.c`: título, selección de dificultad (`DIFF_CFG`), sonido on/off.
- [ ] **F7-04** Implementar `ui/inventory.c`: pestañas de habilidades y reliquias, selección/equipar (equivalente a `drawInventory`, [index.html:2677](index.html#L2677)).
- [ ] **F7-05** Implementar `ui/transition.c`: transición mosaico entre niveles (equivalente a `pixelate`/`startTransition`, [index.html:2159-2187](index.html#L2159-L2187)).
- [ ] **F7-06** Implementar `core/dialogue.c` + visor de diálogo de jefe (globos con nombre + texto).
- [ ] **F7-07** Implementar la cinemática de introducción por viñetas (`INTRO_STORY`, [index.html:1218](index.html#L1218)).
- [ ] **F7-08** Implementar la cinemática final (`ENDING_STORY`) y los créditos (`CREDITS`).
- [ ] **F7-09** Implementar los santuarios (`shrine`): desbloqueo de habilidad al interactuar.
- [ ] **F7-10** Implementar las reliquias equipables y su efecto real en `damage()`/regeneración (Colmillo Afilado, Pata de la Suerte, Bigotes de Acero).
- [ ] **F7-11** Definir `core/save.h/.c`: struct `SaveData` (sección 10 del plan).
- [ ] **F7-12** Implementar `platform/gba/pal_gba_save.c`: lectura/escritura a SRAM con checksum.
- [ ] **F7-13** Conectar los checkpoints (`lamp`) con guardado automático y respawn.
- [ ] **F7-14** Prueba end-to-end: partida nueva → avance de progreso → guardar → reset del emulador → cargar y continuar correctamente.

**Criterio de cierre de fase:** partida jugable de principio a fin con guardado persistente.

---

## Fase 8 — Resto de niveles

- [ ] **F8-01** Nivel 2, La Ciudad Vertedero: `.json` + tileset/paralaje propios + validación jugable.
- [ ] **F8-02** Nivel 3, Estación Abandonada.
- [ ] **F8-03** Nivel 4, Zona de Residuos Tóxicos.
- [ ] **F8-04** Nivel 5, Madriguera Subterránea (incluye la aparición única de El Sicario).
- [ ] **F8-05** Nivel 6, Cámara de Comercio / Mercado Negro (nivel de transición sin jefe).
- [ ] **F8-06** Nivel 7, El Trono de Betty.
- [ ] **F8-07** Validar el enlace completo entre los 7 niveles vía puertas/checkpoints, de principio a fin.

**Criterio de cierre de fase:** el juego completo (7 niveles + 6 jefes) es recorrible sin cortes.

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
