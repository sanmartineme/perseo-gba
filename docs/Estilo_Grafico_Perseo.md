# Diseño Visual y Estilo Gráfico: "Perseo: Sombras de Silencio"

Este documento detalla la dirección de arte, referencias visuales, animaciones y transiciones para el juego, adaptado a las capacidades de Game Boy Color (GBC) y Game Boy Advance (GBA).

## 1. Diseño de Personajes (Estilo Game Boy Color)
Esta primera sección establece el aspecto base para la plataforma más limitada (GBC). Muestra a Perseo en su forma de sprite jugable. El diseño debe ser claro, con una paleta de colores muy restringida y píxeles gruesos.

![Hoja de Sprites de Perseo (GBC)](assets/img/perseo_sprite_sheet_gbc.png)
*> Nota para el equipo de arte: Insertar aquí la hoja de sprites con la silueta gordita y el pañuelo rojo de Perseo adaptados a los 4 colores por sprite permitidos en GBC.*

## 2. Diseño de Niveles y Atmósfera del Mundo (Estilo Game Boy Advance)
Para la versión de GBA, el salto gráfico es notable. Aquí podemos usar paletas de colores ricas, fondos con paralaje (múltiples capas que se mueven a diferentes velocidades) y efectos de iluminación sutiles para crear el ambiente lúgubre de "Silencio".

![Exploración en las Cloacas de Silencio (GBA)](assets/img/nivel_cloacas_gba.png)
*> Nota para el equipo de arte: Insertar aquí la captura que simula la resolución de GBA (240x160). Se deben notar los detalles en ladrillos húmedos, lodo verde brillante y el fondo borroso que sugiere las vías del metro de NYC.*

## 3. Escena de Lucha tipo *Hack and Slash* con Jefe Final (GBA)
Esta imagen ilustra el pilar de combate contra los jefes. Debemos mostrar dinamismo y acción, con sprites más grandes para los jefes y efectos visuales claros al atacar.

![Perseo vs. Rata Betty (GBA Boss Fight)](assets/img/boss_fight_betty_gba.png)
*> Nota para el equipo de arte: Insertar aquí el *mockup* del combate en GBA. Betty debe verse como un sprite grande y amenazante, mientras Perseo lanza un ataque rápido con un rastro azul pixelado indicando el 'Ganchito mortal'.*

## 4. Cinemáticas y Estilo Narrativo (Estilo 16/32-bit Transition)
Las cinemáticas utilizarán arte de pixel art de alta resolución (estilo SNES/Genesis avanzado o primeros juegos de 32-bit), con composiciones dramáticas y paletas de colores ricas para contar la historia sin usar texto excesivo.

![Cinemática de Introducción (GBA/High-Res Pixel Art)](assets/img/cinematica_intro_highres.png)
*> Nota para el equipo de arte: Insertar aquí el pixel art detallado y dramático (con sombreado dithered avanzado) que muestra a Perseo mirando la inmensidad de Silencio.*

## 5. Descripción de Animaciones y Transiciones

El estilo gráfico también se define fuertemente por el movimiento y la fluidez:

### 5.1. Animaciones de Perseo (Estilo Gordito y Ágil)
*   **Caminar:** Perseo debe tener un contoneo sutil debido a su peso, pero sus patas deben moverse rápido.
*   **Salto Doble:** El primer salto es estándar; el segundo es una voltereta rápida y apretada (como una bola negra) que muestra su agilidad oculta, seguida de una pose de "aterrizaje suave".
*   **Ataque 'Ganchito mortal':** La animación debe ser casi instantánea (1-2 frames de preparación, 1 frame de impacto con rastro de luz), enfatizando la velocidad Hack and Slash.

### 5.2. Transiciones de Nivel (Estilo 16/32-bit)
*   Al cambiar de una sección de la cloaca a otra (por ejemplo, de los *Túneles de Filtración* a la *Ciudad Vertedero*), la pantalla no debe simplemente cortarse.
*   **Efecto Mosaico/Fade:** La pantalla actual se "disuelve" en un patrón de mosaico de píxeles que se agranda, revelando la nueva área detrás, o hace un *fade* suave a negro seguido de una apertura rápida, similar a las transiciones dramáticas de *Super Metroid* o *Castlevania: Symphony of the Night*.
