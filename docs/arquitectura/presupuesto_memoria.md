# Presupuesto de memoria y de frame

Medido sobre la ROM de la Fase 9 (`build/perseo.gba`, 119.252 bytes).
Los números de memoria salen del ELF enlazado (`arm-none-eabi-readelf -S`,
`nm -S`) y del reparto de VRAM que fija el código; los de tiempo, del
medidor de frame que lleva la propia ROM
([pal_gba_profile.c](../../source/platform/gba/pal_gba_profile.c)).

Cómo repetir las mediciones: al final, en "Cómo se reproduce esto".

---

## 1. Resumen

| Recurso | Capacidad | Usado | Libre |
|---|---:|---:|---:|
| ROM (cartucho) | 32 MB | 119.252 B (0,4 %) | prácticamente todo |
| EWRAM | 262.144 B | **0 B** | 100 % |
| IWRAM | 32.768 B | 18.496 B (56 %) | 14.272 B menos la pila |
| VRAM de fondos | 65.536 B | 12.640 B (19 %) | 52.896 B |
| VRAM de sprites | 32.768 B | 9.728 B (30 %) | 23.040 B |
| Peor frame medido | 100 % | **60,1 %** (pelea con Betty) | 39,9 % |

Nada está cerca de su techo. La consecuencia práctica está en la sección 6:
las optimizaciones que la Fase 9 tenía anotadas por si hacían falta **no
hacen falta**, y hacerlas igual sólo añadiría complejidad.

---

## 2. ROM

| Sección | Bytes | Qué es |
|---|---:|---|
| `.text` | 29.096 | código |
| `.rodata` | 89.252 | datos constantes |
| `.iwram` + `.data` (copias) | 328 | se copian a IWRAM al arrancar |
| cabecera + relleno de `gbafix` | 576 | |
| **total** | **119.252** | |

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

## 3. IWRAM — 18.496 B de 32.768

`.bss` + `.data` terminan en `0x03004840`. Por encima queda la pila (que crece hacia
abajo desde `0x03007F00`) y la tabla de interrupciones.

| Símbolo | Bytes | Qué es |
|---|---:|---|
| `s_tiles` | 8.800 | copia mutable del tilemap del nivel |
| `s_pool` (entidades) | 3.584 | pool de entidades vivas |
| `s_scroll_room_tiles` | 2.000 | **sala de prueba de la Fase 2, ya nadie la usa** |
| `s_shadow` | 1.280 | buffer sombra de la capa de interfaz (Fase 9) |
| `s_pool` (partículas) | 640 | |
| `s_lines` | 640 | ajuste de línea del texto |
| `s_test_room_tiles` | 600 | **sala de prueba de la Fase 1, ya nadie la usa** |
| `g_game` | 216 | el estado entero de la partida |
| resto | ~744 | |

**El uso de IWRAM no cambia de un nivel a otro.** `s_tiles` está
dimensionado a `LEVEL_MAX_TILES` (200 × 44 = 8.800), que es el mayor nivel
que se permite, y se reserva una sola vez; cargar un nivel copia dentro y
no reserva nada. Lo mismo el pool de entidades. Así que la tabla de arriba
vale para los siete niveles, y ningún nivel puede empeorarla salvo que
alguien suba `LEVEL_MAX_TILES`.

### Deuda anotada: 2.632 B de salas de prueba

`s_test_room_tiles` + `s_test_room` (Fase 1) y `s_scroll_room_tiles` +
`s_scroll_room` (Fase 2) suman 2.632 bytes de IWRAM — el 14 % de lo
usado — y sus constructores ocupan ROM. `level_get_test_room()` y
`level_get_scroll_test_room()` siguen declaradas en
[level.h](../../source/core/level/level.h) pero **no las llama nadie**, ni
en `source/` ni en `tests/`: son el andamio con el que se validaron la
física y el scroll antes de que existieran los niveles reales.

No se han borrado porque sobra IWRAM y porque son el banco de pruebas más
chico que hay para depurar la física sin cargar un nivel entero. Queda
anotado para que la decisión sea de alguien y no un olvido.

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
| CBB 0 | 352 | tileset del nivel: **11 tiles, los mismos para los siete** |
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

### Sprites — 9.728 B de 32.768 (304 tiles de 1.024)

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

Todos los bancos se cargan al arrancar y se quedan: no hay carga por nivel
que pueda fallar a mitad de partida. Los seis jefes son las tres cuartas
partes del gasto y aun así sobran 23 KB.

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

Perseo yendo y viniendo unos 15 s en cada nivel, con sus enemigos, sus
carteles y su rótulo de zona.

| Nivel | Peor frame | MUNDO | TILES | OAM | UI | Frames caídos |
|---|---:|---:|---:|---:|---:|---:|
| 1 Túneles | 41,8 % | 11,8 | 2,1 | 9,9 | 19,8 | 0 |
| 2 Vertedero | 51,4 % | 18,9 | 2,0 | 11,2 | 19,4 | 0 |
| 3 Estación | 56,2 % | 20,8 | 2,0 | 11,6 | 21,8 | 0 |
| 4 Residuos | 45,4 % | 11,5 | 0,3 | 10,2 | 23,6 | 0 |
| 5 Madriguera | 50,8 % | 21,7 | 2,0 | 11,5 | 15,7 | 0 |
| 6 Mercado | 34,8 % | 19,6 | 2,0 | 11,0 | 9,7 | 0 |
| 7 Trono | 45,5 % | 19,8 | 2,0 | 11,0 | 21,3 | 0 |

### Peleas de jefe

| Encuentro | Peor frame | MUNDO | TILES | OAM | UI | Frames caídos |
|---|---:|---:|---:|---:|---:|---:|
| El Capataz (nivel 2) | 40,0 % | 22,5 | 4,6 | 12,1 | 21,2 | 0 |
| Nivel 4 | 39,1 % | 21,8 | 2,9 | 11,8 | 19,1 | 0 |
| **BETTY (nivel 7)** | **60,1 %** | 40,0 | 4,6 | 14,5 | 24,4 | 0 |

Betty es el peor caso medido, que es justo lo que la Fase 9 sospechaba
(F9-07): con invocaciones y proyectiles en pantalla, MUNDO llega al 40 %
del frame. Aun así el frame más caro de toda la campaña se queda en el
60,1 %, y el contador de frames caídos no se movió del cero en ninguna de
las diez escenas.

### Lo que dicen estos números sobre F9-03 … F9-06

Con un 40 % de margen en el peor caso, cada una de las optimizaciones
anotadas costaría complejidad a cambio de nada medible:

- **F9-03 (mover física y colisión a IWRAM).** Ya están en IWRAM: `.text`
  vive en ROM pero `.bss` entero — el tilemap, el pool, el estado — está en
  IWRAM, que es donde importa. Mover además el código exigiría marcar
  funciones con `__attribute__((section(".iwram")))` y compilarlas en ARM
  en vez de Thumb, lo que las **agranda** un 30-40 %. A cambio de recortar
  un tramo MUNDO que en el peor caso ocupa el 40 % de un frame que sobra.
- **F9-04 (particionado espacial para la colisión).** El nivel con más
  enemigos ya está medido y MUNDO no pasa del 21,7 % fuera de las arenas.
  Un quadtree sobre 56 entidades es más código, más estado y más formas de
  equivocarse que un bucle doble que cabe de sobra.
- **F9-05 (doble buffer de OAM).** OAM nunca pasa del 14,5 %, y con el
  reparto actual el volcado cae dentro del VBlank. El problema real de esta
  clase **sí existía pero era de la capa de interfaz, no de OAM**, y está
  arreglado: ver abajo.
- **F9-06 (comprimir tilesets).** Sección 2: ahorra ~40 KB de una ROM que
  usa el 0,4 % del cartucho.

### El problema que sí apareció: la interfaz se veía a medio pintar

Buscando el coste del tramo UI salió un fallo real. El bucle arranca al
empezar el VBlank, y el VBlank son 1.309 pasos del medidor: el 29,8 % del
frame. Con el HUD suelto (UI ≈ 2,4 %) todo el trabajo cabía ahí dentro,
pero una pantalla con panel — inventario, cartel, diálogo de jefe — sube el
tramo UI a más del 20 %, el frame entero pasa del 45 % y el repintado se
derrama sobre el VDraw. Como la interfaz se borra entera y se vuelve a
pintar cada frame **escribiendo directamente en el screenblock**, el haz
llegaba a leerlo a medio escribir: se capturó el panel ya pintado pero sin
su texto, con la pantalla anterior asomando por las filas de arriba.

No era un fallo de rendimiento — no se caía ni un frame — sino de *cuándo*
se toca la VRAM. La capa de texto ahora pinta en un buffer sombra de IWRAM
y lo vuelca de una sola vez (320 palabras) al empezar el VBlank siguiente:
[pal_gba_text.c](../../source/platform/gba/pal_gba_text.c). Cuesta 1.280
bytes y un frame de retraso en la interfaz, imperceptible a 60 Hz.

---

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
