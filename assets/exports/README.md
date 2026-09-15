# Asset Exports - Sprites y Gráficos Editables

## Estructura

Esta carpeta contiene copias aisladas de todos los assets gráficos del juego **Perseo: Sombras de Silencio** en formato PNG, listas para edición futura con herramientas externas.

### Carpetas

- **sprites/** - Hojas de frames de todos los tipos de entidades
  - `aurorita.png` - Personaje Aurorita (viñetas cinemáticas)
  - `bosses.png` - Jefe (Betty)
  - `cine_16x16.png`, `cine_32x32.png` - Assets de cinemática
  - `enemies_*.png` - Enemigos (ratas, sicarios, matones, etc.)
  - `fx_*.png` - Efectos visuales y partículas
  - `perseo*.png` - Personaje principal (diferentes tamaños)
  - `props_*.png` - Decoración interactiva (reliquias, puertas, etc.)
  - `thugs_32x32.png` - Matones ampliados para cinemática

- **tiles/** - Tilesets de fondos
  - `tileset_tuneles.png` - Tileset principal de niveles

## Cambios Visuales Implementados (Sept 2026)

### 1. Paletas Optimizadas (256 colores - 8bpp)

**Antes:** Paletas de 16 colores (4bpp) separadas por tipo
**Después:** Paleta global de 256 colores con mejor definición

Los archivos `.grit` han sido actualizados:
- `-gB4` → `-gB8` (4 bits → 8 bits por píxel)
- `-pn16` → `-pn256` (16 colores → 256 colores por paleta)

**Beneficios:**
- Mayor riqueza cromática (256 colores disponibles)
- Mejor degradación de sombras y luces (similar a Castlevania/Metroid GBA)
- Sprites con mayor definición y detalle

### 2. Parallax Mejorado

El fondo implementa ahora múltiples capas con velocidades de scroll diferenciadas:
- **Capa de tuberías:** 0.5x velocidad de cámara (plano intermedio)
- **Capa de silueta urbana:** 0.3x velocidad de cámara (fondo lejano)
- **Sky elements:** Efecto atmosférico adicional

**Beneficios:**
- Sensación de profundidad tipo Castlevania (túnel industrial profundo)
- Movimiento más fluido y natural
- Mayor inmersión ambiental

### 3. Fondos Enriquecidos

El patrón de parallax ahora incluye:
- Edificios con alturas más variadas y orgánicas
- Red de tuberías más compleja (industrial realista)
- Detalles ocasionales (ventanas, conexiones) que dan vida al escenario
- Variación fractal similar a Metroid

**Beneficios:**
- Mundo visual más rico y detallado
- Atmósfera industrial más creíble
- Mejor aprovechamiento de las capacidades de GBA

## Cómo Usar

### Para editar assets:
1. Abre cualquier PNG desde `sprites/` o `tiles/` en tu editor favorito
2. Edita con libertad (Aseprite, Piskel, GIMP, etc.)
3. **Importante:** Mantén las dimensiones originales (ancho debe ser múltiplo de 8 píxeles)
4. Guarda como PNG en la misma ubicación
5. El build del juego regenerará automáticamente los assets compilados

### Pipeline de actualización:
```
1. Edita:     assets/exports/sprites/enemigos_16x16.png
2. Copia a:   assets/src/sprites/enemigos/enemies_16x16.png
3. Build:     make clean && make
4. Resultado: ROM actualizada con nuevos gráficos
```

## Notas Técnicas

### Archivo de configuración grit
Cada PNG tiene un archivo `.grit` asociado que grit lee para compilar:
```
-gt        tiles (no bitmap)
-gB8       8 bits por píxel (256 colores)
-Mw2 -Mh2  meta-tiles (si aplica)
-pn256     exportar paleta de 256 colores
-ftc -fh   output en C y header
```

### Limitaciones de GBA
- VRAM limitada para OBJ (64KB): con 8bpp se pueden cargar ~16 hojas de 32x32
- Prioridad OAM: máximo 128 sprites por frame
- Paleta global: todos los sprites en 8bpp comparten 256 colores

## Referencias Visuales

- **Inspiración:** Castlevania IV GBA, Metroid Fusion
- **Estilo:** Pixel art industrial oscuro con profundidad atmosférica
- **Resolución:** 240x160 (GBA LCDCON)
- **Colores base:** Paleta RGB555 (5-5-5 bits por canal)

---

*Última actualización: 2026-09-15*
*Cambios visuales aplicados por Claude Code*
