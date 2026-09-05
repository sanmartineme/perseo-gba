# Product Requirements Document (PRD): Proyecto "Perseo: Sombras de Silencio"

## 1. Resumen Ejecutivo
**"Perseo: Sombras de Silencio"** es un videojuego 2D de acción, aventura y plataformas con estructura *Metroidvania*, influenciado visual y mecánicamente por títulos como *Hollow Knight* y *Blasphemous*. El jugador controla a **Perseo**, un gato negro que debe atravesar **Silencio**, una extensa ciudad subterránea ambientada en las cloacas de Nueva York, para rescatar a su hermana **Aurorita** de las garras de **Betty**, una rata líder de una red de tráfico ilegal de animales.

El juego está diseñado para ser desplegado de forma nativa como un archivo ROM compatible con **Game Boy, Game Boy Color y Game Boy Advance**, con un estilo visual *pixel art* y música MIDI, apuntando a una duración máxima de 8 horas de juego.

---

## 2. Pilares de Diseño del Juego

El diseño de un Metroidvania exitoso no es solo un juego de plataformas con exploración; es la arquitectura de un ecosistema interconectado [cite: 1]. Nuestros pilares fundamentales son:

*   **Progresión Basada en Habilidades (Lock-and-Key):** El sistema *Lock-and-Key* regula el flujo de juego [cite: 1]. Las habilidades adquiridas por Perseo actuarán como "llaves" permanentes que permitirán desbloquear nuevas áreas [cite: 1]. La regla de oro será "mostrar primero la puerta, luego la llave" para generar marcadores mentales en el jugador [cite: 1].
*   **Backtracking con Propósito y Diseño de Herradura:** El retroceso no debe ser tedioso [cite: 1]. Implementaremos el *Diseño de Herradura* (Horseshoe Design), donde el jugador ve un objetivo bloqueado y debe dar un rodeo para llegar, desbloqueando un atajo de vuelta (como puertas unidireccionales o elevadores) que recompensa el dominio del mapa [cite: 1, 2].
*   **Combate Hack and Slash Desafiante:** Un sistema de combate fluido contra enemigos de las cloacas, culminando en intensas batallas contra jefes que actúan como evaluación final de las habilidades del bioma [cite: 1].
*   **Narrativa Lineal en un Mundo Abierto:** Aunque la historia avanza linealmente de nivel en nivel hasta llegar al rescate de Aurorita, la exploración intra-nivel e inter-nivel será abierta y guiada por las mejoras mecánicas.

---

## 3. Universo y Narrativa

*   **Protagonista:** Perseo (Gato negro). Ágil, silencioso, inicialmente equipado con habilidades pasivas básicas que evolucionan.
*   **Antagonista:** Betty (Rata gigante). Líder del sindicato criminal de las cloacas, dedicada al comercio ilegal de animales.
*   **Motivación:** Rescatar a Aurorita (hermanita menor).
*   **Entorno (Silencio):** Una ciudad ficticia y sombría que simula las vastas e interconectadas cloacas de New York City. Los biomas deben tener contrastes marcados pero coherentes lógicamente [cite: 1].
*   **Evolución del Mundo:** Conforme Perseo derrota a los lugartenientes de Betty, los entornos ya visitados pueden evolucionar (por ejemplo, cambios de iluminación o nuevos enemigos) para mantener la frescura durante el *backtracking* [cite: 1, 2].

---

## 4. Mecánicas de Juego

### 4.1. Habilidades y Progresión
*   **Habilidades Pasivas por Defecto:** Perseo comienza con movilidad ágil básica (salto estándar, ataque cuerpo a cuerpo).
*   **Árbol de Habilidades Incremental:** El jugador utilizará coleccionables para desbloquear mejoras pasivas (aumento de vida, reducción de daño) y especiales.
*   **Habilidades Clave (Llaves):** Habilidades especiales (doble salto, trepar paredes, *dash*) que permiten evitar la *ruptura de secuencia* (Sequence Breaking), asegurando que Perseo no pueda acceder a jefes avanzados sin antes obtener las herramientas necesarias [cite: 1].

### 4.2. Inventario y Objetos
*   Uso de múltiples objetos consumibles (curación) y reliquias que modifican temporal o permanentemente las estadísticas de Perseo a través de su inventario.

### 4.3. Ingeniería de Encuentros y Jefes
*   Cada uno de los 8 niveles culmina con un Jefe.
*   Los jefes se diseñarán para evitar patrones predecibles, utilizando un arsenal de ataques seleccionados dinámicamente y puntos débiles que requieren el uso de las mecánicas de plataformas recién adquiridas [cite: 1, 2].

---

## 5. Diseño de Niveles y Mundo

El mundo se divide en **8 niveles principales** (biomas). El mapeo se realizará con la metodología "Top-Down", creando diagramas de flujo bidireccionales antes de diseñar los *tiles* para asegurar múltiples puntos de conexión y atajos [cite: 1, 2].

1.  **Nivel Introductorio:** (A nivel de desarrollo, este debe ser el último en terminarse para integrar la experiencia pulida [cite: 2]).
2.  **Túneles de Filtración:** Acueductos primarios.
3.  **La Ciudad Vertedero:** Zonas de chatarra apilada.
4.  **Estación Abandonada:** Viejos túneles de metro estilo NYC con vagones oxidados.
5.  **Zona de Residuos Tóxicos:** Plataformas de precisión sobre químicos.
6.  **Madriguera Subterránea:** Territorio hostil poblado por matones de Betty.
7.  **Cámara de Comercio (El Mercado Negro):** Laberinto de jaulas y trampas de tráfico animal.
8.  **El Trono de Betty:** El núcleo de Silencio.

---

## 6. Especificaciones Técnicas y Target de Plataforma

El gran diferencial técnico de este proyecto es su despliegue nativo en consolas retro. El entregable final será un **ROM (.gb, .gbc, .gba)** ejecutable en hardware real y emuladores.

### 6.1. Resoluciones y Aspect Ratio
Se desarrollarán perfiles de compilación adaptados a cada plataforma:
*   **Game Boy / Game Boy Color:** Resolución nativa de **160 x 144 píxeles**. Paleta monocromática (GB) o indexadas (GBC).
*   **Game Boy Advance:** Resolución nativa de **240 x 160 píxeles**. Soporte para color de 16/32 bits, lo que permite efectos de transparencia y fondos con paralaje complejos.

### 6.2. Arte y Audio
*   **Estilo Visual:** *Pixel art* 2D altamente optimizado para el uso estricto de memoria de video (VRAM) de las consolas target.
*   **Transiciones:** Efectos visuales de transición de 16 y 32 bits.
*   **Audio:** Música de formato MIDI utilizando los canales integrados del hardware (ondas cuadradas, ruido, wavetable) para la ambientación.

### 6.3. Limitaciones de Memoria y Manejo de Escenas
*   Debido a las limitaciones de un ROM clásico, los mundos interconectados deben gestionarse eficientemente, fragmentando inteligentemente las pantallas de *scroll* [cite: 1, 2].

---

## 7. Planificación y Pipeline de Desarrollo

1.  **Prototipado de Mecánicas (Grayboxing):** No se comprometerán recursos artísticos finales (pixel art) hasta que el flujo de movimiento, el salto y el combate se sientan satisfactorios mediante el uso de cajas y bloques básicos [cite: 1]. Crear un entorno controlado interno para probar colisiones y saltos es fundamental antes de construir el mundo [cite: 2].
2.  **Arquitectura del Mapa (Diagrama de Flujo):** Definir el inicio, fin, y la ubicación de las llaves (habilidades) y candados (obstáculos) [cite: 2]. Para estructurar estos ciclos y subsistemas se puede referenciar un esquema de arquitectura mental (por ejemplo, el estilo visualizado en el archivo *"NotebookLM Mind Map.jpg"* provisto).
3.  **Implementación de Jefes:** Desarrollo iterativo de patrones de comportamiento (como los implementados mediante *Triggers* y animadores) [cite: 1, 2].
4.  **Pulido (El Primer Nivel al Final):** Construir la zona inicial del juego al final del ciclo de desarrollo para garantizar que sirva como una introducción perfecta a la experiencia global ya optimizada [cite: 2].
