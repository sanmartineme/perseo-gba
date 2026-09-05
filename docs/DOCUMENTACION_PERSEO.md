# Perseo: Sombras de Silencio

**Videojuego Metroidvania 2D estilo Game Boy**

---

## 📋 Contenidos

1. [Personajes y Jefes](#personajes-y-jefes)
2. [Habilidades y Objetos](#habilidades-y-objetos)
3. [Niveles](#niveles)
4. [Créditos](#créditos)

---

## 🎮 Personajes y Jefes

### Perseo (Protagonista)

**Descripción:** Un gato valiente decidido a rescatar a Aurorita de las garras de Betty y su banda de criminales en las cloacas subterráneas.

**Habilidades iniciales:**
- Ganchito mortal (golpe cuerpo a cuerpo)
- Lanzamiento de traza (proyectil de excremento)

**Habilidades desbloqueables:**
- Salto Doble (voltereta felina en el aire)
- Dash Sombrío (embite veloz que rompe obstáculos)
- Garra Felina (escalada en muros)

---

### Aurorita (Personaje Rescatable)

**Descripción:** Gatita café atigrada, amiga de Perseo. Fue raptada por la banda de Betty y encerrada en una jaula en las cloacas. Su muletilla es "¡Mi papito es el mejor!"

**Rol:** Rescatable en la cinemática final. Su rescate desencadena la secuencia de créditos.

---

### Jefes Principales

#### 1. El Capataz
- **Nivel:** Túneles de Filtración (Nivel 1)
- **HP:** 16
- **Victoria:** "CAPATAZ DERROTADO"
- **Diálogo post-victoria:** "La puerta al fondo tiembla... Betty sabe que vienes."

#### 2. El Revisor
- **Nivel:** La Ciudad Vertedero (Nivel 2)
- **HP:** 20
- **Victoria:** "REVISOR DERROTADO"
- **Diálogo post-victoria:** "Los túneles se abren hacia una zona que apesta a químicos."

#### 3. El Tóxico
- **Nivel:** Estación Abandonada (Nivel 3)
- **HP:** 22
- **Victoria:** "CRIATURA VENCIDA"
- **Diálogo post-victoria:** "Más allá, gruñidos hostiles anuncian la Madriguera de Betty."

#### 4. El Guardián
- **Nivel:** Zona de Residuos Tóxicos (Nivel 4)
- **HP:** 25
- **Victoria:** "GUARDIÁN CAÍDO"
- **Diálogo post-victoria:** "El Mercado Negro está cerca. Y con él, respuestas sobre Aurorita."

#### 5. El Guardia
- **Nivel:** Madriguera Subterránea (Nivel 5)
- **HP:** 28
- **Victoria:** "GUARDIA ABATIDO"
- **Diálogo post-victoria:** "Una jaula vacía. Un aroma familiar. Betty se la llevó a su Trono."

#### 6. Betty (Jefe Final)
- **Nivel:** El Trono de Betty (Nivel 7)
- **HP:** 46
- **Victoria:** "¡BETTY HA CAÍDO!"
- **Diálogo post-victoria:** "El reinado de Silencio se termina. Al fondo, una jaula espera ser abierta."
- **Ataques especiales:**
  - Carga de ataque
  - Salto con impacto
  - Lanzamiento de proyectiles
  - Invocación de aliados
  - Explosión de basura (ataque exclusivo)
- **Música:** Tema tenebroso y épico, más grave y amenazante que los demás jefes

---

## ⚔️ Habilidades y Objetos

### Habilidades Pasivas

#### Ganchito Mortal
- **Tipo:** Pasiva (disponible desde inicio)
- **Control:** X / J
- **Descripción:** Zarpazo veloz con la garra delantera. Es el golpe base contra todo lo que se acerque. Animación de 3 fases: preparación → impacto con garras fuera → recobro.
- **Efectos visuales:** Rastro de garras (slashes de luz) en impacto
- **Alcance:** Cuerpo a cuerpo

#### Lanzamiento de Traza
- **Tipo:** Pasiva (disponible desde inicio)
- **Control:** V / L
- **Descripción:** Arroja excremento (traza) a distancia. La traza gira en vuelo y revienta al impactar con salpicadura marrón y partículas. Tiene enfriamiento de 34 frames.
- **Proyectil:** Sprite café rotatorio
- **Explosión:** Sprite splat marrón con partículas y sacudida de pantalla
- **Sonido:** Efecto de explosión (SFX.splat)
- **Daño:** Afecta a enemigos y jefes, nunca al jugador
- **Alcance:** Distancia media

---

### Habilidades de Santuario

#### Salto Doble
- **Tipo:** Desbloqueables en Santuarios
- **Control:** Salto en aire
- **Descripción:** Voltereta felina. Pulsa salto en el aire para un segundo impulso de salto.
- **Uso estratégico:** Alcanzar plataformas altas, evitar hazards

#### Dash Sombrío
- **Tipo:** Desbloqueables en Santuarios
- **Control:** Salto + dirección horizontal
- **Descripción:** Embite veloz que rompe rejillas oxidadas y esquiva peligros.
- **Uso estratégico:** Atravesar obstáculos frágiles, evadir ataques de enemigos

#### Garra Felina
- **Tipo:** Desbloqueables en Santuarios
- **Control:** Hacia muro + salto
- **Descripción:** Agárrate a los muros y salta entre ellos para escalar verticalmente.
- **Uso estratégico:** Escalada de pozos y estructuras verticales

---

### Reliquias Equipables

#### Colmillo Afilado
- **Efecto:** +1 de daño en cada Ganchito mortal
- **Ubicación:** Encontrable en los niveles
- **Uso:** Incrementa el daño de ataque cuerpo a cuerpo

#### Pata de la Suerte
- **Efecto:** Regenera 1 corazón tras 12 segundos sin recibir daño
- **Ubicación:** Encontrable en los niveles
- **Uso:** Recuperación pasiva de salud en combate

#### Bigotes de Acero
- **Efecto:** Reduce en 1 el daño de cualquier golpe (mínimo 1)
- **Ubicación:** Encontrable en los niveles
- **Uso:** Defensa contra daño entrante

---

## 🗺️ Niveles

### Nivel 1: Túneles de Filtración
- **Descripción:** Los primeros túneles de las cloacas. Lugar de entrenamiento donde Perseo aprende los básicos de combate y movimiento.
- **Enemigos progresivos:** Ratas pequeñas, cucarachas
- **Jefe:** El Capataz
- **Habilidades a desbloquear:** Salto Doble
- **Hazards:** Picos, plataformas móviles

### Nivel 2: La Ciudad Vertedero
- **Descripción:** Una vasta ciudad subterránea construida enteramente de desechos y basura apilada. Estructuras inestables y peligros ambientales.
- **Enemigos progresivos:** Ratas grandes, mosquitos
- **Jefe:** El Revisor
- **Habilidades a desbloquear:** Dash Sombrío
- **Hazards:** Lodo pegajoso, estructuras frágiles

### Nivel 3: Estación Abandonada
- **Descripción:** Infraestructura de tratamiento de cloacas desvencijada. Tuberías oxidadas y maquinaria herrumbrosa.
- **Enemigos:** Murciélagos agresivos, thugs (matones)
- **Jefe:** El Tóxico
- **Hazards:** Tuberías de vapor, electrocución

### Nivel 4: Zona de Residuos Tóxicos
- **Descripción:** Área contaminada con químicos peligrosos. Líquidos fluorescentes y ambiente hostil.
- **Enemigos:** Combinación de todos los anteriores + thugs armados
- **Jefe:** El Guardián
- **Hazards:** Químicos que queman, radiación

### Nivel 5: Madriguera Subterránea
- **Descripción:** Guarida profunda de la rata Betty. Cavernas naturales con arquitectura primitiva.
- **Enemigos:** Thugs, enemigo único "El Sicario" (brute), aparición única en el juego
- **Jefe:** El Guardia
- **Hazards:** Grietas profundas, zonas de caída

### Nivel 6: Cámara de Comercio (Mercado Negro)
- **Descripción:** Centro de operaciones criminal de Betty. Estructura subterránea con mercado clandestino.
- **Enemigos:** Todos los tipos previos en combinación estratégica
- **Jefe:** Ninguno (nivel de transición)
- **Hazards:** Trampas mecánicas, enemigos en emboscada

### Nivel 7: El Trono de Betty
- **Descripción:** El lair final de Betty. Sala del trono decorada con despojos y con una jaula donde Aurorita es mantenida prisionera.
- **Enemigos:** Aliados de Betty en forma de invocaciones durante el combate
- **Jefe:** BETTY (Jefe Final)
- **Hazards:** Ataque directo de Betty + hazards ambientales
- **Cinemática post-combate:** Perseo abre la jaula y rescata a Aurorita

---

## 🎬 Cinemática Introductoria

**Rapto de Aurorita**

La historia comienza con una cinemática por viñetas que muestra:

1. Aurorita caminando sola por las cloacas
2. Dos matones de Betty la emboscan
3. Diálogos siniestros sobre sus intenciones criminales
4. Aurorita grita asustada pero promete: "¡Se las verán conmigo si logro escapar!"
5. La enjaulan y la arrastran hacia una alcantarilla
6. Cierre con el juramento silencioso de Perseo de rescatarla

**Estilo visual:** Marcos de cómic con efectos de shake/flash en momentos dramáticos.

---

## 🎬 Cinemática Final

**Rescate de Aurorita**

Tras derrotar a Betty, se reproduce una cinemática por escenas que muestra:

1. **Caída de Betty:** Betty cae derrotada de su trono
2. **Acercamiento:** Perseo se acerca a la jaula donde Aurorita está encerrada
3. **Apertura:** Perseo usa su Ganchito Mortal para romper el candado
4. **Reencuentro:** Aurorita es liberada y reaparece
5. **Epilogo:** Perseo y Aurorita abandonan juntos las cloacas

**Transición a créditos:** La cinemática finaliza y se despliegan los créditos de producción.

---

## 📜 Créditos Finales

```
PERSEO: SOMBRAS DE SILENCIO

Diseño de Personajes: Carlos San Martín
Diseño de Escenarios: Carlos San Martín
Diseño de Niveles: Carlos San Martín
Historia: Perseo Andrés, Aurora Andrea, Mango Miguel

¡Feliz cumpleaños amor mío!
```

---

## 🎨 Estilo Visual

### Paleta de Colores
- **Primarios:** Azules pizarra y navy (frío, industrial)
- **Acentos:** Dorado (HUD, interfaz)
- **Rojo:** Segmentación de vida
- **Marrones:** Tuberías, ladrillos, excremento (proyectiles)

### Elementos de Diseño
- **Arquitectura:** Tuberías industriales, ladrillos con mortero, sistemas de alcantarillas
- **Atmósfera:** Oscura, subterránea, contaminada
- **Detalles:** Bridas de tuberías, válvulas, goteo de agua cian, oxidación
- **HUD:** Barra de vida roja segmentada, contador de monedas dorado, inventario de habilidades

### Animaciones
- Ciclos de caminata para enemigos terrestres (2 fases)
- Ataques de jefes con múltiples fases de animación
- Transición de pantalla tipo mosaico (estilo Super Metroid)
- Efectos de shake de cámara en impactos y explosiones

---

## 🎵 Audio

### Bandas Sonoras por Nivel
- Nivel 1-5: Temas chiptune únicos según ubicación
- Nivel 6: Transición hacia tema final
- Jefe (Niveles 1-5): Tema de combate estándar
- Jefe Final (Betty): Tema tenebroso y épico, más grave que los demás

### Efectos de Sonido
- Golpe de Ganchito: Sonido de impacto de garra
- Lanzamiento de traza: Sonido de lanzamiento/deflexión
- Explosión de traza: Efecto de explosión marrón (SFX.splat)
- Daño: Sonido de dolor/impacto
- Victoria: Fanfarria de victoria

---

## 📊 Estadísticas de Juego

| Aspecto | Valor |
|---------|-------|
| Cantidad de Niveles | 7 |
| Cantidad de Jefes | 6 |
| Habilidades Pasivas | 2 |
| Habilidades Desbloqueables | 3 |
| Reliquias Equipables | 3 |
| Enemigos Únicos | 6+ variaciones |
| Duración Aproximada | 60-90 minutos |

---

## 🎮 Mecánicas de Juego

### Sistema de Progresión Lock-and-Key
- **Salto Doble:** Desbloqueado en Nivel 1, permite alcanzar plataformas altas
- **Dash Sombrío:** Desbloqueado en Nivel 2, rompe obstáculos frágiles
- **Garra Felina:** Desbloqueado en Nivel 2, permite escalada vertical

### Sistema de Checkpoints
- Checkpoints en lámparas a lo largo de cada nivel
- Respawn en el último checkpoint al morir
- Pérdida de progreso mínima por defunción

### Sistema de Combate
- Ataque cuerpo a cuerpo: Ganchito Mortal (X/J)
- Ataque a distancia: Lanzamiento de Traza (V/L) con enfriamiento
- Evasión: Dash, Salto, escalada
- Reliquias modifican daño recibido y infligido

### Inventario
- Habilidades activas (mostrables en pantalla)
- Reliquias equipables (máximo seleccionable)
- Monedas y coleccionables

---

**Versión del documento:** 1.0  
**Última actualización:** 2026-07-20  
**Plataforma:** Web (HTML5 Canvas)  
**Archivo:** `simulador/index.html`
