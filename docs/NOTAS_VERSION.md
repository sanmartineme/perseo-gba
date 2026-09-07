# Notas de versión

## v1.0-rc1 — candidata a versión

**Es una candidata, no la final.** El juego está completo y se sostiene solo:
arranca, se recorre, se guarda y va a 60 fps. Lo que falta para llamarla 1.0
no es código sino **jugarla**: una partida entera de principio a fin, y una
prueba en una GBA de verdad con flashcart. Ninguna de las dos se puede hacer
desde el entorno donde se desarrolló. El detalle de qué está verificado y
cómo está en [tests/playtest_checklist.md](../tests/playtest_checklist.md).

### Qué es

Port a Game Boy Advance en C de *Perseo: Sombras de Silencio*, un
metroidvania que existía como prototipo web de un solo archivo. Perseo, un
gato negro, baja a las cloacas de la ciudad de Silencio a buscar a Aurorita.

- 7 niveles, 6 jefes, 3 habilidades llave y 3 reliquias
- Guardado en SRAM por puntos de control
- Música y efectos PSG por los cuatro canales del hardware
- Cinemáticas de introducción y final, y créditos

### La ROM

| | |
|---|---|
| Archivo | `build/perseo.gba` |
| Tamaño | 119.500 bytes |
| MD5 | `ba919048dc348e5826042147860064af` |
| Título interno | `PERSEO` |
| Código de juego | `APSE` |
| Guardado | SRAM 32 KB (`SRAM_V113`) |

Se construye con `make` y una instalación de devkitARM. La ROM de arriba
salió de un `build/` borrado del todo, así que no arrastra nada de las
pruebas.

### Controles

| Botón | Acción |
|---|---|
| D-Pad | Mover, trepar |
| A | Saltar (dos veces con Salto Doble) |
| B | Ganchito Mortal (ataque) |
| R | Dash Sombrío |
| L | Lanzamiento de Traza |
| START | Pausa |
| SELECT | Inventario |
| START+SELECT | Medidor de coste de frame |

Ese último no es un huevo de pascua: es la herramienta con la que se validó
el rendimiento, y va en la ROM de release a propósito. Si alguna vez el juego
se siente pesado, ahí se ve en qué tramo se va el tiempo, y `FRAMES CAIDOS`
responde la pregunta sin interpretación.

### Rendimiento

Trece escenas medidas — los siete niveles recorridos y las seis peleas de
jefe — con **0 frames caídos** y un pico del **87,4 %** de un frame, en el
nivel 4. Detalle y método en
[docs/arquitectura/presupuesto_memoria.md](arquitectura/presupuesto_memoria.md).

Memoria: ROM 119 KB de 32 MB, IWRAM 18.504 de 32.768, **EWRAM sin usar**,
VRAM de fondos al 19 % y de sprites al 30 %.

### Lo que queda pendiente, y es sabido

- **Una partida completa jugada por una persona.** Es lo que cierra la Fase
  10 y lo único que puede encontrar los problemas que quedan: un salto
  imposible, un sitio donde uno se encaja, un jefe mal equilibrado.
- **Hardware real.** Sin cartucho no hay forma de comprobar el arranque, la
  persistencia de la SRAM en el cartucho, ni cómo se ven estos colores en la
  pantalla original — que es mucho más oscura que cualquier emulador, y este
  juego transcurre entero en cloacas.
- **El sonido, de oído.** La afinación se verificó numéricamente (error
  máximo 0,35 %), pero nadie lo ha escuchado: en el entorno de desarrollo no
  se puede capturar el audio del emulador.
- **Al recargar una partida, las chapas ya recogidas vuelven a estar en el
  mapa** (el contador sí se conserva) y las rejillas rotas vuelven a estar
  enteras. Es una deuda asumida a propósito en la Fase 7: guardar eso exige
  fijar un formato de máscaras por nivel, y entonces sólo existían dos de los
  siete niveles.
- **2.632 bytes de IWRAM** ocupados por las salas de prueba de las Fases 1 y
  2, que ya no llama nadie. Sobra memoria, así que se dejaron por si sirven
  para depurar; borrarlas es una decisión pendiente, no un olvido.

### Cambios notables de la última fase

La Fase 9 no sólo midió: encontró cuatro fallos reales que nadie había visto,
tres de ellos del tilemap y ninguno visible sin medir.

- Se caía **un frame** al reaparecer tras morir: un salto de cámara hacía que
  el streaming del tilemap recorriera cientos de columnas de las que sólo
  sobreviven 32. 153,5 % → 19,3 % de un frame, y el frame caído desapareció.
- **Al cambiar de nivel se veía la geometría del nivel anterior.** Los
  niveles 2 a 7 aparecen todos en el mismo tile, así que la ventana
  sincronizada quedaba idéntica y no se redibujaba nada.
- **Los tiles que cambian dentro de lo que ya se ve no se veían**: la pared
  que sella una arena, el paso que se abre al caer el jefe, una rejilla rota
  de un dash.
- **La interfaz se veía a medio pintar** en las pantallas con panel, porque
  se escribía directamente en la VRAM mientras el haz la estaba leyendo.
