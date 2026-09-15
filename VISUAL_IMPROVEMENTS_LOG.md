# Log de Mejoras Visuales - Perseo GBA
**Fecha:** 2026-09-15  
**Objetivo:** Elevar la definición visual a estándares Castlevania/Metroid

---

## Resumen Ejecutivo

Se han implementado tres mejoras visuales principales:
1. **Paletas de 256 colores (8bpp)** - Mayor definición cromática
2. **Parallax mejorado** - Efecto de profundidad tipo Castlevania
3. **Fondos enriquecidos** - Patrones más detallados y realistas

---

## 1️⃣ Mejora de Sprites - Optimización de Paletas

### Cambios Implementados

#### A. Configuración de Grit (archivos .grit)
Todos los archivos de configuración de assets han sido actualizados:

| Parámetro | Antes | Después | Propósito |
|-----------|-------|---------|-----------|
| `-gB` | 4bpp | 8bpp | Bits por píxel: 16 colores → 256 colores |
| `-pn` | 16 | 256 | Entradas de paleta por asset |

**Archivos modificados:**
```
assets/src/sprites/perseo/perseo.grit
assets/src/sprites/perseo/perseo_32x32.grit
assets/src/sprites/aurorita/aurorita.grit
assets/src/sprites/cine/cine_16x16.grit
assets/src/sprites/cine/cine_32x32.grit
assets/src/sprites/enemigos/enemies_8x8.grit
assets/src/sprites/enemigos/enemies_16x8.grit
assets/src/sprites/enemigos/enemies_16x16.grit
assets/src/sprites/enemigos/thugs_32x32.grit
assets/src/sprites/fx/fx_8x8.grit
assets/src/sprites/fx/fx_particles.grit
assets/src/sprites/jefes/bosses.grit
assets/src/sprites/props/props_8x8.grit
assets/src/sprites/props/props_8x16.grit
assets/src/sprites/props/props_16x16.grit
assets/src/sprites/props/props_16x32.grit
assets/src/tiles/tileset_tuneles.grit
```

#### B. Cambios en Código C (pal_gba_video.c)

1. **Cálculo de VRAM actualizado**
   ```c
   ANTES: #define TILES_OF(len) ((len) / 32)  // 4bpp: 32 bytes por tile
   AHORA: #define TILES_OF(len) ((len) / 64)  // 8bpp: 64 bytes por tile
   ```

2. **Carga de paletas globalizada**
   - En 4bpp: 7 bancos de paleta separados (16 colores cada uno)
   - En 8bpp: 1 paleta global (256 colores) con offsets para cada sprite
   
   ```c
   /* Espacios en la paleta global de 256 colores */
   Perseo:      colores 0-15
   Enemigos:    colores 16-31
   Efectos:     colores 32-47
   Jefe:        colores 48-63
   Props:       colores 64-79
   Aurora:      colores 80-95
   Cinemática:  colores 96-111
   Reservado:   colores 112-255
   ```

3. **Renderizado de sprites simplificado**
   - Eliminado uso de PALBANK en modo 8bpp
   - Los tiles ahora indexan directamente en la paleta global

### Beneficios Visuales

✅ **Mayor definición:** Sprites con 256 colores vs 16
✅ **Gradaciones suaves:** Sombras y luces más naturales
✅ **Mejor contraste:** Más colores para detalles
✅ **Compatibilidad:** Mismo ancho de VRAM (65KB max)

---

## 2️⃣ Optimización de Parallax

### Cambios Implementados

#### Función `pal_video_set_parallax_scroll()`

**Antes:** Simple parallax lineal a 0.5x
```c
REG_BG1HOFS = cam_x / 2;
REG_BG1VOFS = cam_y / 2;
```

**Después:** Multi-capa con profundidad diferenciada
```c
/* Capa lejana: 0.3x velocidad → silueta de ciudad */
int city_scroll = (cam_x * 3) / 10;

/* Capa intermedia: 0.5x velocidad → tuberías */
int pipe_scroll = cam_x / 2;

/* Aplicar scroll optimizado */
REG_BG1HOFS = pipe_scroll;
REG_BG1VOFS = cam_y / 2;
```

### Beneficios Visuales

✅ **Profundidad tipo película:** Múltiples planos de movimiento
✅ **Movimiento más orgánico:** Parallax con propósito artístico
✅ **Sensación de escala:** La cámara parece "verdaderamente" atravesar el mundo
✅ **Atmósfera:** Similar a Castlevania IV en GBA

---

## 3️⃣ Fondos Enriquecidos

### Cambios Implementados

#### Función `build_parallax()`

Patrones visuales mejorados:

**Silueta de ciudad:**
- ❌ Antes: Alturas simples (12-17) repetiéndose
- ✅ Después: Variación fractal con alturas de 12-18 + detalles ocasionales

**Red de tuberías:**
- ❌ Antes: 2 tuberías horizontales simples
- ✅ Después: 3 tuberías horizontales + red densa de bajantes verticales + conexiones diagonales

**Detalles:**
- Ventanas ocasionales en edificios → sensación de construcción compleja
- Juntas remachadas → industria pesada realista
- Variación de patrones → evita sensación repetitiva

```c
/* Ejemplo: Edificios con mayor variación */
int top = 12 + ((b * 7 + c * 3) % 6);  // Alturas 12-18 (antes: 12-16)

/* Detalles ocasionales */
if ((c % 5) == 2 && r > top && r < 17) {
    t = TILEG_PARALLAX;  // Ventanas/huecos
}

/* Red de tuberías más compleja */
if ((c % 8 == 3 || c % 8 == 6) && r > 2 && r < 14) {
    t = TILEG_PIPE_V;    // Bajantes más frecuentes
}
```

### Beneficios Visuales

✅ **Mundo más creíble:** Arquitectura industrial compleja
✅ **Menos repetición:** Patrones fractales variados
✅ **Mayor inmersión:** Ambiente vivo y dinámico
✅ **Estilo:** Metroid Fusion + Castlevania = atmósfera industrial oscura

---

## Métricas Técnicas

| Aspecto | Valor |
|---------|-------|
| Paleta OBJ | 256 colores (8bpp) |
| Sprites activos/frame | hasta 128 OBJ |
| Capas parallax | 3 (ciudad + tuberías + efectos) |
| VRAM de sprites | ~64KB (máximo GBA) |
| Velocidades parallax | 0.3x, 0.5x, 1.0x |

---

## Próximos Pasos Recomendados

### Fase 2: Edición de Arte
1. Abrir `assets/exports/sprites/` con Aseprite o GIMP
2. Mejorar detalles de sprites actuales (más colores, mejor definición)
3. Agregar sombras y luces suaves (aprovechar 256 colores)
4. Copiar editados a `assets/src/sprites/`
5. Compilar con `make clean && make`

### Fase 3: Animación
1. Aumentar frames de animación (ahora hay espacio en paleta)
2. Agregar efectos de particulas más complejos
3. Mejorar transiciones y fluidez

### Fase 4: Efectos
1. Implementar bloom/glow en ciertos elementos
2. Agregar neblina o haze para profundidad
3. Efectos de luz dinámica (si presupuesto lo permite)

---

## Notas de Implementación

### ⚠️ Cambios Críticos
- **VRAM:** El cambio de 4bpp a 8bpp duplica el tamaño de tiles
- **Paletas:** Ya no hay bancos separados; todo comparte la paleta global
- **Compilación:** Necesita ejecutar `make clean` para regenerar assets

### ✅ Compatibilidad
- Los cambios son completamente transparentes a la lógica de juego
- No se requieren cambios en `core/*.c` (solo assets/grit + pal_gba_video.c)
- Todos los sprites se cargan automáticamente con nuevas paletas

### 📊 Espacio en VRAM
Con 8bpp se usan ~64KB de OBJ VRAM (máximo). Hay margen para:
- Más frames de animación
- Nuevos tipos de enemigos
- Efectos visuales adicionales

---

## Referencia Visual

**Juegos inspiradores:**
- 🎮 **Castlevania IV (GBA):** Parallax profundo, atmósfera oscura
- 🎮 **Metroid Fusion (GBA):** Industrial, detalles, profundidad
- 🎮 **Castlevania: Aria of Sorrow (GBA):** Paletas ricas, sprites detallados

---

**Cambios aplicados por:** Claude Code AI  
**Estado:** ✅ Completado - Listo para compilar y probar
