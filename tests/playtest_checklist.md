# Checklist de playtest — Perseo: Sombras de Silencio (GBA)

Lista de comprobación para dar la ROM por buena. Está pensada para
recorrerse **jugando**, no leyendo código.

## Cómo se marca cada punto

| Marca | Significado |
|---|---|
| `[x]` | Verificado, y la columna *Cómo* dice de qué manera |
| `[~]` | Verificado a medias — el detalle explica qué falta |
| `[ ]` | Sin verificar |

Y en la columna *Cómo*:

- **auto** — comprobado guiando el emulador desde fuera y mirando la captura.
  Reproducible: ver la sección 7 de
  [presupuesto_memoria.md](../docs/arquitectura/presupuesto_memoria.md).
- **datos** — comprobado sobre los datos o el binario, no jugando.
- **mano** — hace falta una persona con el mando.

**Lo que "auto" no puede hacer.** Guiar el emulador sirve para llegar a una
pantalla, disparar una interacción y leer un contador. No sirve para
plataformear: un salto encadenado que hay que clavar no sale con un guion a
ciegas, y en varias pruebas Perseo terminó muerto antes de llegar. Todo lo
que dependa de recorrer un nivel de punta a punta está marcado **mano** a
propósito, no por pereza.

**Y lo que nadie puede hacer desde aquí.** No hay forma de capturar el audio
del emulador, así que **todo el sonido está sin verificar de oído**. La
afinación se comprobó numéricamente en la Fase 6 (error máximo 0,35 %), que
no es lo mismo que haberlo escuchado.

---

## 1. Arranque y título

| | Punto | Cómo |
|---|---|---|
| `[x]` | La ROM arranca en el título, con logotipo y el nivel 1 de fondo | auto |
| `[x]` | Cabecera correcta: título `PERSEO`, código `APSE`, checksum válido, logo de Nintendo presente | datos |
| `[x]` | La etiqueta `SRAM_V113` está en la ROM (sin ella los emuladores no emulan el guardado) | datos |
| `[x]` | "PARTIDA NUEVA" arranca la cinemática de introducción | auto |
| `[x]` | Con partida guardada aparece "CONTINUAR" | auto (Fase 7) |
| `[x]` | "CONTINUAR" carga y **se puede seguir jugando** | auto |
| `[x]` | El fondo se desplaza con la cámara (tuberías y ciudad) | auto |
| `[x]` | Caen gotas de humedad de las juntas | auto |
| `[x]` | La vida se lee como corazón + barra | auto |
| `[x]` | El menú de pausa: reanudar, inventario, salir al menú | auto |
| `[x]` | OPCIONES abre su pantalla | auto |
| `[x]` | Reasignar un botón **intercambia** con quien lo tenía | auto |
| `[x]` | Los controles reasignados sobreviven a apagar la consola | auto |
| `[x]` | PODERES: TODOS da las tres habilidades | auto |
| `[x]` | Las dos claves se activan y avisan con su cartel | auto |
| `[ ]` | Jugar con los controles cambiados se siente bien | mano |
| `[ ]` | El ajuste de SONIDO se oye | mano |
| `[ ]` | El ajuste de DIFICULTAD cambia el daño recibido de verdad | mano |

## 2. Narrativa

| | Punto | Cómo |
|---|---|---|
| `[x]` | Cinemática de introducción: 8 viñetas, avanzan con A | auto |
| `[x]` | Cada viñeta lleva su **ilustración**, no sólo texto | auto |
| `[x]` | El desenlace: sus 7 viñetas ilustradas | auto |
| `[x]` | La historia de cada nivel sale la primera vez que se entra | auto |
| `[x]` | El texto ajusta línea y no se sale del panel | auto |
| `[ ]` | La historia **no** se repite al morir y volver al mismo nivel | mano |
| `[ ]` | Cinemática final (7 viñetas) y créditos, alcanzados **jugando** | mano |
| `[x]` | Los créditos pasan solos cada 3 s y vuelven al título | auto (Fase 7) |

## 3. Movimiento y física

| | Punto | Cómo |
|---|---|---|
| `[x]` | Correr, saltar y caer | auto |
| `[x]` | Doble salto | auto |
| `[x]` | Dash Sombrío | auto |
| `[ ]` | Trepar por paredes | mano |
| `[ ]` | Las plataformas de un sentido se atraviesan desde abajo | mano |
| `[ ]` | El dash rompe rejillas oxidadas | mano |
| `[x]` | Un cambio de tiles (rejilla rota, arena sellada) **se ve** en el acto | auto |
| `[ ]` | No hay sitios donde uno se quede encajado en la geometría | mano |
| `[ ]` | Ningún salto obligatorio del recorrido es imposible | mano |

## 4. Combate

| | Punto | Cómo |
|---|---|---|
| `[x]` | Ataque cuerpo a cuerpo (Ganchito Mortal) | auto |
| `[x]` | Lanzamiento de Traza | auto |
| `[x]` | Los enemigos reciben daño y mueren, con sus partículas | auto |
| `[x]` | Perseo recibe daño por contacto y muere | auto |
| `[ ]` | Los siete tipos de enemigo se comportan como en el prototipo | mano |
| `[ ]` | El lodo tóxico y los pinchos hacen daño | mano |
| `[ ]` | La invulnerabilidad tras recibir un golpe dura lo que debe | mano |

## 5. Jefes

| | Punto | Cómo |
|---|---|---|
| `[x]` | Los seis encuentros arrancan: la arena se sella y el jefe entra | auto |
| `[x]` | El diálogo de tres réplicas precede a la pelea | auto |
| `[x]` | Los seis jefes reciben daño (barra de vida bajando) | auto |
| `[~]` | **Se les puede matar.** Confirmado de punta a punta en dos: El Capataz (barra a cero, agonía, paso abierto y corazón caído) y Betty. En los otros cuatro se vio la pelea y el daño, pero no se capturó el momento de la muerte | auto |
| `[x]` | Al caer el jefe se abre el paso y aparece la puerta al nivel siguiente | auto (Capataz) |
| `[x]` | La pared que sella la arena **se dibuja** al cerrarse | auto |
| `[ ]` | Cada jefe hace sus ataques propios y su patrón se lee bien | mano |
| `[ ]` | La pelea está equilibrada (ni trivial ni injusta) | mano |

## 6. Progresión

| | Punto | Cómo |
|---|---|---|
| `[x]` | Perseo empieza sin ninguna habilidad llave | auto |
| `[ ]` | Los tres santuarios entregan su habilidad, con cartel y fanfarria | mano |
| `[ ]` | Las tres reliquias se encuentran y su efecto se nota | mano |
| `[x]` | El inventario abre con SELECT y tiene sus dos pestañas | auto |
| `[ ]` | Equipar/desequipar reliquias respeta el tope de dos | mano |
| `[ ]` | Los carteles se leen al pasar por delante | auto (parcial) |

## 7. Guardado y muerte

| | Punto | Cómo |
|---|---|---|
| `[x]` | Las lámparas guardan solas al encenderse | auto (Fase 7) |
| `[x]` | El guardado sobrevive a cerrar el emulador | auto |
| `[x]` | Una lámpara guarda jugando de verdad, sin build de prueba | auto |
| `[x]` | Morir devuelve a la última lámpara | auto |
| `[ ]` | Morir devuelve a una lámpara **de otro nivel** correctamente | mano |
| `[x]` | Reaparecer no tira ningún frame | auto |
| `[ ]` | El contador de muertes y el tiempo de partida cuadran | mano |

| `[x]` | Una chapa recogida **no vuelve a aparecer** al recargar | auto |
| `[x]` | Una rejilla rota **sigue rota** al recargar | auto |

La deuda que la Fase 7 dejó anotada aquí (las chapas y las rejillas no se
guardaban) está saldada. Comprobado de punta a punta y con el juego apagado
en medio: una ROM recoge la chapa y rompe la rejilla por los caminos
normales y guarda; otra distinta arranca, lee la SRAM y las dos siguen
como se dejaron.

## 8. Recorrido completo

| | Punto | Cómo |
|---|---|---|
| `[x]` | El grafo 0→1→2→3→4→5→6 cierra, sin niveles inalcanzables | datos (`check_chain.py`) |
| `[x]` | Cada nivel arranca con su geometría, sus entidades y su paleta | auto |
| `[x]` | Al cambiar de nivel se ve el nivel **nuevo**, no el anterior | auto |
| `[x]` | La transición de mosaico se ve al cruzar una puerta | auto |
| `[ ]` | **Una partida entera, de principio a fin, sin cortes** | mano |

Ese último punto es el que cierra la fase, y es el único que no puede hacer
nadie más que una persona jugando.

## 9. Audio

| | Punto | Cómo |
|---|---|---|
| `[x]` | La ROM corre con el audio activo, sin cuelgues ni regresión visual | auto |
| `[x]` | Afinación correcta: error máximo 0,13 % (bajo) y 0,35 % (melodía) | datos |
| `[ ]` | Las 11 canciones suenan reconocibles respecto del prototipo | mano |
| `[ ]` | Los 15 efectos suenan y disparan cuando toca | mano |
| `[ ]` | El silencio del menú de pausa funciona | mano |

## 10. Rendimiento

| | Punto | Cómo |
|---|---|---|
| `[x]` | 0 frames caídos en los siete niveles y las seis peleas | auto |
| `[x]` | Peor frame medido: 87,4 % (nivel 4) | auto |
| `[ ]` | 0 frames caídos en una partida entera jugada por una persona | mano |

Se mira con **START+SELECT**, que abre el medidor. El número que decide es
`FRAMES CAIDOS`: si es 0, va a 60. Detalle en
[presupuesto_memoria.md](../docs/arquitectura/presupuesto_memoria.md).

## 11. Hardware real

| | Punto | Cómo |
|---|---|---|
| `[ ]` | Arranca en una GBA con flashcart | mano |
| `[ ]` | El guardado en SRAM persiste en el cartucho | mano |
| `[ ]` | Los colores se ven bien en la pantalla real (no retroiluminada) | mano |
| `[ ]` | El sonido por altavoz y por auriculares | mano |

Nada de esta sección puede comprobarse desde aquí: hace falta el cartucho y
la consola. Y la última fila importa más de lo que parece — la pantalla de
la GBA original es mucho más oscura que la de cualquier emulador, y este
juego transcurre entero en cloacas.

---

## Resumen honesto

Lo verificable sin manos está verificado: el juego arranca, los siete
niveles se cargan y se recorren, las seis peleas se pelean — dos de ellas
hasta matar al jefe y abrir el paso — el guardado persiste, y no se cae un
frame. **Lo que falta es de una clase que no se
puede automatizar**: si el juego se *siente* bien, si el equilibrio es justo,
si algún salto es imposible, si la música suena como debe, y si todo eso
sigue siendo cierto en una consola de verdad.
