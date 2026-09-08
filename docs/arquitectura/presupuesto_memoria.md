# Presupuesto de memoria y de frame

Medido sobre `build/perseo.gba` (127.732 bytes).
Los números de memoria salen del ELF enlazado (`arm-none-eabi-readelf -S`,
`nm -S`) y del reparto de VRAM que fija el código; los de tiempo, del
medidor de frame que lleva la propia ROM
([pal_gba_profile.c](../../source/platform/gba/pal_gba_profile.c)).

Cómo repetir las mediciones: al final, en "Cómo se reproduce esto".

---

## 1. Resumen

| Recurso | Capacidad | Usado | Libre |
|---|---:|---:|---:|
| ROM (cartucho) | 32 MB | 127.732 B (0,4 %) | prácticamente todo |
| EWRAM | 262.144 B | **0 B** | 100 % |
| IWRAM | 32.768 B | 15.956 B (49 %) | 16.812 B menos la pila |
| VRAM de fondos | 65.536 B | 12.640 B (19 %) | 52.896 B |
| VRAM de sprites | 32.768 B | 12.416 B (38 %) | 20.352 B |
| Peor frame medido | 100 % | **87,4 %** (nivel 4) | 12,6 % |

La memoria no está cerca de su techo por ningún lado. El tiempo de frame sí
tiene un margen ajustado — 12,6 % en el peor caso — pero **no se cae ni un
frame en ninguna de las trece escenas medidas**. La sección 6 explica qué se
optimizó (y por qué), y qué se dejó sin optimizar con el número delante.

---

## 2. ROM

| Sección | Bytes | Qué es |
|---|---:|---|
| `.text` | 33.848 | código |
| `.rodata` | 92.976 | datos constantes |
| `.iwram` + `.data` (copias) | 328 | se copian a IWRAM al arrancar |
| cabecera + relleno de `gbafix` | 580 | |
| **total** | **127.732** | |

Los datos pesan tres veces más que el código, y dentro de los datos mandan
los siete tilemaps:

| Símbolo | Bytes |
|---|---:|
| `level01_tuneles_tiles` | 8.800 |
| `level02_vertedero_tiles` | 8.800 |
| `level07_trono_tiles` | 8.360 |
| `level05_madriguera_tiles` | 7.920 |
| `level06_mercado_tiles` | 7.920 |
| `level04_residuos_tiles` | 7.700 |
| `level03_estacion_tiles` | 7.480 |
| **subtotal tilemaps** | **56.980** (64 % de `.rodata`) |
| `sys8Glyphs` (fuente de libtonc) | 8.732 |
| `bossesTiles` | 6.144 |

Un tile es un byte sin comprimir. Comprimirlos con LZ77 de BIOS (F9-06)
ahorraría del orden de 40 KB de una ROM que usa el 0,4 % del cartucho: es
trabajo, riesgo y un descompresor en el arranque de cada nivel a cambio de
nada. Ver sección 6.

---

## 3. IWRAM — 15.956 B de 32.768

`.bss` + `.data` terminan en `0x03003E54`. Por encima queda la pila (que crece hacia
abajo desde `0x03007F00`) y la tabla de interrupciones.

| Símbolo | Bytes | Qué es |
|---|---:|---|
| `s_tiles` | 8.800 | copia mutable del tilemap del nivel |
| `s_pool` (entidades) | 3.584 | pool de entidades vivas |
| `s_shadow` | 1.280 | buffer sombra de la capa de interfaz (Fase 9) |
| `s_pool` (partículas) | 640 | |
| `s_lines` | 640 | ajuste de línea del texto |
| `g_game` | 336 | el estado entero de la partida |
| resto | ~664 | |

**El uso de IWRAM no cambia de un nivel a otro.** `s_tiles` está
dimensionado a `LEVEL_MAX_TILES` (200 × 44 = 8.800), que es el mayor nivel
que se permite, y se reserva una sola vez; cargar un nivel copia dentro y
no reserva nada. Lo mismo el pool de entidades. Así que la tabla de arriba
vale para los siete niveles, y ningún nivel puede empeorarla salvo que
alguien suba `LEVEL_MAX_TILES`.

### Borrado: 2.680 B que ocupaba el andamio

Las salas de prueba de las Fases 1 y 2 (`level_get_test_room()` y
`level_get_scroll_test_room()`) estuvieron un tiempo anotadas ací como
deuda: sus dos tilemaps estáticos ocupaban 2.600 bytes de IWRAM y **no las
llamaba nadie** desde que existen los niveles reales. Eran el andamio con
el que se validaron la física y el scroll antes de que hubiera nivel que
cargar, y el andamio se quita cuando el edificio se sostiene.

Con ellas se fueron otras seis funciones públicas que tampoco llamaba
nadie — `entity_pool_count()`, `entity_pool_for_each()` (y su
`EntityVisitor`), `audio_is_muted()`, `fx_round()`, `game_scale_damage()` y
`ui_text_clear_rect()`. La de `game_scale_damage()` además venía con un
comentario que afirmaba que la llamaba `world_damage_player()`, cosa que
hacía años que no era cierta: la dificultad se aplica con `dmg_num`/
`dmg_den` en el propio mundo. Un comentario que miente es peor que no
tener comentario.

Total recuperado: **2.680 bytes de IWRAM** (del 57 % al 49 % de uso) y 648
de ROM.

---

## 4. EWRAM — 0 B de 262.144

Ni un byte. Nada se declara `EWRAM_DATA`/`EWRAM_BSS` y no hay reserva
dinámica en ningún sitio: el juego entero cabe en IWRAM, que es la memoria
rápida (bus de 32 bits, sin esperas, contra 16 bits y 2 esperas de EWRAM).

Es el resultado que se quería, y además es el margen que hace que las
optimizaciones de la sección 6 no hagan falta: si algún día apretara la
IWRAM, mover lo frío a EWRAM son 256 KB intactos esperando.

---

## 5. VRAM

### Fondos — 12.640 B de 65.536

| Zona | Bytes | Contenido |
|---|---:|---|
| CBB 0 | 512 | tileset del nivel: **16 tiles, los mismos para los siete** (11 del mundo + los 5 del fondo de paralaje) |
| CBB 2 | 4.096 | capa de interfaz: 3 tiles macizos + 96 glifos de 8×8 |
| SBB 8 | 2.048 | tilemap de BG2 (el nivel colisionable) |
| SBB 9 | 2.048 | tilemap de BG1 (paralaje) |
| SBB 26 | 2.048 | tilemap de BG0 (interfaz) |
| SBB 27 | 2.048 | tilemap de BG3 (el velo) |

**La VRAM de fondos tampoco cambia por nivel**, y esa es la consecuencia
directa de la decisión de la Fase 8: los siete niveles comparten el mismo
dibujo de once tiles y se distinguen por su paleta. Cambiar de zona cuesta
`memcpy16(pal_bg_mem, ..., 16)` — 32 bytes, una vez por nivel — en vez de
recargar un tileset. Los cuatro screenblocks son de 32×32 y el scroll de
mapa grande reescribe la columna o fila que entra, así que su tamaño no
depende de lo grande que sea el nivel.

### Sprites — 12.416 B de 32.768 (388 tiles de 1.024)

| Banco | Bytes |
|---|---:|
| `bosses` | 6.144 |
| `perseo` | 1.536 |
| `enemies_16x16` | 512 |
| `enemies_16x8` | 256 |
| `fx_8x8` | 256 |
| `props_16x32` | 256 |
| `enemies_8x8` | 192 |
| `props_8x16` | 192 |
| `fx_particles` | 160 |
| `props_16x16` | 128 |
| `props_8x8` | 96 |
| `thugs_32x32` (viñetas) | 1.024 |
| `aurorita` (viñetas) | 512 |
| `perseo_32x32` (viñetas) | 512 |
| `cine_32x32`, la jaula | 512 |
| `cine_16x16`, la luna | 128 |

Todos los bancos se cargan al arrancar y se quedan: no hay carga por nivel
que pueda fallar a mitad de partida. Los seis jefes son más de la mitad del
gasto y aun así sobran 20 KB.

Los cinco últimos son de las viñetas de la cinemática. Cuatro de ellos son
personajes que ya estaban, **repetidos al doble de tamaño**: en las viñetas
el prototipo los dibuja escalados sobre un canvas mucho más ancho que la
pantalla de GBA, y a 1x se verían diminutos. Escalarlos al generar el arte
cuesta 2 KB de una VRAM que sobra; hacerlo en marcha habría pedido sprites
afínes, con su matriz y su bandera de doble tamaño, para el mismo
resultado.

### Paletas

- Fondo: banco 0 = tileset del nivel (lo intercambia
  `pal_video_set_level_palette`); bancos 8-14 = los siete colores de la
  interfaz; banco 15 = relleno y borde de los paneles.
- Sprites: banco 0 Perseo, 1 enemigos, 2 efectos, 3 jefes, 4 props.

---

## 6. Frame: qué cuesta y qué NO hay que optimizar

Un frame de GBA son 280.896 ciclos. El medidor los cuenta con TIMER3 a
preescala 64 (4.389 pasos por frame) y parte el bucle en cuatro tramos:
**MUNDO** (física, IA, audio), **TILES** (streaming del tilemap), **OAM**
(volcado de sprites) y **UI** (capa de texto, volcado incluido).

`PEOR` es el peor **frame completo** que se vio, no la suma de los peores
de cada tramo — esos máximos pueden venir de frames distintos y sumarlos
describe un frame que quizá nunca ocurrió.

### Recorrido de los siete niveles

Perseo recorriendo cada nivel unos 20 s de verdad: corriendo, saltando,
atacando, dasheando y lanzando la traza, con sus enemigos y sus carteles.

| Nivel | Peor frame | MUNDO | TILES | OAM | UI | Frames caídos |
|---|---:|---:|---:|---:|---:|---:|
| 1 Túneles | 77,1 % | 49,7 | 19,3 | 13,1 | 24,8 | 0 |
| 2 Vertedero | 66,9 % | 35,0 | 2,1 | 12,1 | 19,4 | 0 |
| 3 Estación | 70,7 % | 47,4 | 2,1 | 14,0 | 21,8 | 0 |
| **4 Residuos** | **87,4 %** | 55,4 | 18,8 | 13,7 | 22,5 | 0 |
| 5 Madriguera | 71,7 % | 48,4 | 2,1 | 14,2 | 15,7 | 0 |
| 6 Mercado | 83,5 % | 39,7 | 23,6 | 12,6 | 27,1 | 0 |
| 7 Trono | 57,3 % | 25,6 | 2,1 | 11,4 | 21,3 | 0 |

### Las seis peleas de jefe

Peleadas de verdad: en todas se le pega al jefe, y varias terminan con el
jefe muerto y el paso abierto.

| Encuentro | Peor frame | MUNDO | TILES | OAM | UI | Frames caídos |
|---|---:|---:|---:|---:|---:|---:|
| El Capataz (nivel 2) | 85,4 % | 43,4 | 24,3 | 12,3 | 23,7 | 0 |
| Nivel 3 | 83,0 % | 40,5 | 24,3 | 12,9 | 20,7 | 0 |
| Nivel 4 | 81,4 % | 38,6 | 24,3 | 11,8 | 19,1 | 0 |
| Nivel 5 | 85,0 % | 42,0 | 24,3 | 12,4 | 18,8 | 0 |
| Nivel 6 | 86,5 % | 39,7 | 26,7 | 11,9 | 18,6 | 0 |
| BETTY (nivel 7) | 82,6 % | 46,5 | 24,3 | 13,8 | 24,4 | 0 |

**El peor frame de toda la campaña es el 87,4 %, en el nivel 4, y no se cae
ni un frame en ninguna de las trece escenas.** El jefe final resultó no ser
el peor caso: las peleas se parecen mucho entre sí (todas rondan el 85 %) y
un nivel abierto con enemigos y el jugador pegando llega más arriba.

MUNDO es el tramo que manda — hasta el 55,4 % — y sube justamente cuando el
jugador ataca, dashea y lanza, porque cada golpe siembra partículas. TILES es
barato mientras uno se limita a caminar (2,1 %) y se dispara a ~20-27 % en el
frame concreto en que hay que rehacer el tilemap entero: al reaparecer tras
morir y al sellarse o abrirse una arena. Ese pico está medido y acotado.

### Lo que se optimizó, porque la medición lo pidió

**El tilemap se rehacía a lo bruto ante un salto de cámara.** El streaming de
BG2 sólo sube al hardware las columnas y filas que ENTRAN en la ventana al
desplazarse, lo cual es correcto y barato mientras la cámara se mueva poco.
Pero ante un salto — morir y reaparecer en la lámpara — el bucle incremental
recorría el hueco columna por columna: cientos de columnas de las que este
screenblock, que es de 32×32, sólo conserva las últimas 32. Todo lo demás se
escribía para ser pisado. Medido en el nivel 1: el tramo de tiles costó el
**153,5 %** de un frame y tiró **el único frame caído** de toda la campaña.

Dos cambios lo arreglaron:

1. Si la ventana nueva no toca a la vieja, no hay nada que reaprovechar y se
   rellena la ventana directamente. Cuando sí se tocan, el trabajo queda
   acotado solo: el desplazamiento no puede pasar del ancho de la ventana.
   El pico bajó de 153,5 % a 53,7 % — y seguía cayéndose un frame.
2. El relleno completo se hace ahora fila por fila resolviendo el puntero de
   la fila una sola vez, en lugar de llamar a `level_tile_at()` en cada uno
   de los ~760 tiles: esa llamada cruza unidad de traducción, no se puede
   inlinear, y repite por tile las comprobaciones de borde que son iguales
   para toda la fila. **53,7 % → 19,3 %**, y el frame caído desapareció.

### El fallo que sí encontró la medición: cambios de tiles que no se veían

Persiguiendo el coste de TILES salieron dos fallos de la misma familia, los
dos reales y los dos invisibles hasta que uno los busca.

**Al cambiar de nivel se veía la geometría del nivel anterior.** El rango
sincronizado son coordenadas de tile de mundo y no sabe de qué nivel son, y
sólo se reiniciaba en `pal_video_init()`, una vez al arrancar. Como los
niveles 2 a 7 aparecen todos en el mismo tile `[4,37]`, la ventana quedaba
**idéntica** al entrar al nivel siguiente y no se redibujaba absolutamente
nada: se entraba a la zona nueva, con su paleta nueva, viendo los ladrillos
de la vieja hasta que uno caminaba lo suficiente. No se había notado antes
porque cada nivel se probó arrancando directamente en él, que es el camino
del primer llenado.

**Y los tiles que cambian dentro de lo que ya se está viendo tampoco se
veían.** `world_carve()` edita el tilemap en marcha — la pared que sella una
arena, el paso que se abre al caer el jefe, una rejilla rota de un dash —
pero BG2 sólo recibe las columnas que entran por los bordes. Comprobado
tallando a propósito un bloque junto al jugador: sin el arreglo no aparece
nunca; con él, aparece.

Los dos se arreglan invalidando la ventana: `pal_video_reset_level_sync()` al
cambiar de nivel, y una bandera `World.tiles_dirty` que `world_carve()` alza
y la capa de plataforma consume. El mundo sigue sin saber cómo se dibuja,
igual que con `WorldEvent`.

### El otro fallo: la interfaz se veía a medio pintar

Midiendo el tramo UI salió un tercer problema, y tampoco era de rendimiento
sino de *cuándo* se toca la VRAM. El bucle arranca al empezar el VBlank, y el
VBlank son 1.309 pasos del medidor: el 29,8 % del frame. Con el HUD suelto
(UI ≈ 2,4 %) todo el trabajo cabía ahí dentro, pero una pantalla con panel —
inventario, cartel, diálogo de jefe — sube el tramo UI a más del 20 %, el
frame entero pasa del 45 % y el repintado se derrama sobre el VDraw. Como la
interfaz se borra entera y se vuelve a pintar cada frame **escribiendo
directamente en el screenblock**, el haz llegaba a leerlo a medio escribir:
se capturó el panel ya pintado pero sin su texto, con la pantalla anterior
asomando por las filas de arriba.

La capa de texto ahora pinta en un buffer sombra de IWRAM y lo vuelca de una
sola vez (320 palabras) al empezar el VBlank siguiente:
[pal_gba_text.c](../../source/platform/gba/pal_gba_text.c). Cuesta 1.280
bytes y un frame de retraso en la interfaz, imperceptible a 60 Hz.

### Lo que se dejó SIN optimizar (F9-03 … F9-06), y por qué

Con el peor frame en 87,4 % y cero frames caídos, el juego va a 60 y estas
cuatro no compran nada hoy. El margen es de 12,6 %, no enorme, así que la
regla práctica queda escrita abajo.

- **F9-03 (mover funciones calientes a IWRAM).** El estado ya está en IWRAM
  — el tilemap, el pool de entidades, la partida entera están en `.bss`, que
  es lo que importa. Mover el *código* exige marcar funciones con
  `__attribute__((section(".iwram")))` y compilarlas en ARM en vez de Thumb,
  lo que las **agranda** un 30-40 %. Es la primera candidata si algún día
  hace falta, porque MUNDO es el tramo que manda.
- **F9-04 (particionado espacial de la colisión).** MUNDO llega al 55,4 %
  con el jugador pegando, no por la colisión entidad-entidad. Un quadtree
  sobre las entidades de un nivel es más estado y más formas de equivocarse
  que el bucle doble actual, y atacaría la parte que no es el problema.
- **F9-05 (doble buffer de OAM).** OAM nunca pasa del 14,2 % y no se vio
  parpadeo. El problema real de esta familia estaba en la capa de interfaz,
  no en OAM, y está arreglado arriba.
- **F9-06 (comprimir tilesets).** Sección 2: ahorra ~40 KB de una ROM que
  usa el 0,4 % del cartucho.

**Cuándo volver a mirar esto.** El margen que queda es de un octavo de frame.
Si se añade contenido — un jefe con más invocaciones, un nivel con más
enemigos, más partículas por golpe — hay que volver a pasar el medidor antes
de darlo por bueno. Está en la ROM de release justamente para eso: se abre
con START+SELECT y el número que hay que mirar es "FRAMES CAIDOS".

## 7. Cómo se reproduce esto

**Memoria**, sobre `build/perseo.elf`:

```sh
arm-none-eabi-readelf -S build/perseo.elf     # secciones y dónde terminan
arm-none-eabi-nm -S --size-sort -r build/perseo.elf   # símbolos por tamaño
```

**Frame, jugando.** El medidor está en la ROM normal: se muestra
manteniendo **START+SELECT** a la vez (una combinación que no se pulsa sin
querer, y que mientras dure anula la pausa y el inventario para no
estorbar). Está siempre compilado, en release también: un contador que sólo
existe en las builds de depuración es un contador en el que no se puede
confiar.

**Frame, nivel por nivel.** Llegar jugando a cada nivel y a cada arena
lleva una partida entera y no sale igual dos veces, así que hay una build
de medición que arranca donde haga falta, con el medidor siempre a la
vista:

```sh
# arranca en el nivel N (0..6), con las tres habilidades
make BUILD=build_prof EXTRA_CFLAGS=-DPERSEO_PROFILE_BOOT=4

# además, delante del disparador del jefe: la pelea empieza sola
make BUILD=build_prof "EXTRA_CFLAGS=-DPERSEO_PROFILE_BOOT=6 -DPERSEO_PROFILE_ARENA"
```

Conviene el `BUILD` aparte: make sólo mira fechas, no banderas, así que
reutilizar `build/` mezclaría objetos compilados con y sin la bandera.

**Que el medidor sabe fallar.** El número que importa es "frames caídos =
0", y un contador que nunca se dispara no prueba nada. Se comprobó
compilando un derroche de ciclos dentro del bucle: con él el medidor marcó
**TOTAL 406,2 % y 116 frames caídos**, los dos en rojo, y el juego se
arrastra de forma visible. Así que el cero de las tablas de arriba es un
cero medido, no un contador dormido. El derroche era temporal y no está en
el fuente.

De paso sirvió de segunda prueba del arreglo de la sección 6: con el frame
desbordado más de cuatro veces, que es el caso más hostil posible para
escribir la pantalla a destiempo, el medidor se lee perfecto. Antes del
buffer sombra, esa misma escena salía con el panel pintado y el texto a
medias.

**Manejar el emulador desde fuera.** Las trece escenas se jugaron guiando a
VBA-M con `SendInput` desde PowerShell y capturando la ventana con
`PrintWindow`. Dos cosas hacen falta y ninguna es evidente:

- **Las teclas no son las de fabrica.** El mapeo real vive en
  `%LOCALAPPDATA%\visualboyadvance-m\vbam.ini` y en esta maquina es
  **WASD + I/K/L/O**, no las flechas ni el Z/X clasico: `Up=W Down=S Left=A
  Right=D`, `A=L B=K L=I R=O`, `Select=BACK Start=ENTER`. Vale la pena leer
  ese archivo antes de dar por hecho nada: las flechas ni siquiera estan
  asignadas.
- **Hay que hacer clic en la ventana.** `SetForegroundWindow` enfoca el
  marco, pero las teclas de letra las atiende el panel de juego de dentro,
  que hasta que no se lo pincha no tiene el foco de teclado. Sin el clic
  responden ENTER y BACKSPACE y nada mas — lo cual es peor que no responder
  nada, porque parece que funciona.

Ese segundo punto tuvo consecuencias reales: una primera tanda de medidas se
tomo creyendo mover a Perseo cuando en realidad estaba quieto, y daba un peor
caso del 60 % que no era cierto. Los numeros buenos son los de arriba, con el
jugador corriendo y pegando de verdad — y son los que destaparon el frame
caido y los dos fallos de tiles. **Antes de creerle a una medida, conviene
mirar la captura y comprobar que en la pantalla esta pasando lo que uno cree
que esta pasando.**

**Y para la captura**: `SetProcessDpiAwareness(2)` antes de `PrintWindow`, o
la imagen sale recortada a la esquina superior izquierda en cuanto Windows
tiene escalado de pantalla distinto del 100 %. Con `powershell.exe` (5.1), no
con `pwsh`: PowerShell 7 no trae `System.Drawing` para `Add-Type`.
