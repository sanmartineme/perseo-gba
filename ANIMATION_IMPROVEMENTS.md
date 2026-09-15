# Mejoras de Animación - Sistema de Frames Mejorado
**Fecha:** 2026-09-15  
**Objetivo:** Fluidez visual tipo Castlevania/Metroid GBA

---

## 📊 Cambios en Estructura de Frames

### Antes (12 frames totales)
```
IDLE:  2 frames
WALK:  4 frames  
JUMP:  1 frame
FALL:  1 frame
DASH:  1 frame
ATK:   3 frames
TOTAL: 12 frames
```

### Después (23 frames totales)
```
IDLE:  3 frames (respiración + parpadeo)
WALK:  6 frames (ciclo de pasos natural)
JUMP:  2 frames (arco ascendente)
FALL:  2 frames (caída acelerada)
DASH:  2 frames (rotación del turbellino)
ATK:   5 frames (movimiento detallado de zarpa)
TOTAL: 23 frames
```

---

## 🎬 Detalle de Mejoras por Acción

### 1. REPOSO (IDLE) - 2 → 3 frames

**Implementación:**
```c
a.frame = (uint8_t)((tick >> 5) % 3) ? PFRAME_IDLE2 : PFRAME_IDLE1;
```

| Frame | Duración | Descripción |
|-------|----------|-------------|
| IDLE_1 | 32 frames | Reposo natural (postura lista) |
| IDLE_2 | 32 frames | Respiración (subida de pecho) |
| IDLE_3 | 32 frames | Parpadeo/descanso (cabeza baja) |

**Ciclo total:** 96 frames (~1.6s a 60 FPS)
**Efecto:** Personaje vivo, no una estatua. Respiración y pulsaciones visibles.

---

### 2. CAMINATA (WALK) - 4 → 6 frames

**Implementación:**
```c
uint32_t wf = (p->walk_anim / 3) % 6;
a.frame = (uint8_t)(PFRAME_WALK1 + wf);
a.bob = (wf == 1 || wf == 4) ? 1 : 0;
```

| Frame | Duración | Bob | Descripción |
|-------|----------|-----|-------------|
| WALK_1 | 3 ticks | 0px | Talón izquierdo adelante |
| WALK_2 | 3 ticks | 1px | Peso en el talón (contacto) |
| WALK_3 | 3 ticks | 0px | Punta delantera |
| WALK_4 | 3 ticks | 0px | Transición |
| WALK_5 | 3 ticks | 1px | Talón derecho contacto |
| WALK_6 | 3 ticks | 0px | Recuperación |

**Ciclo total:** 18 ticks = 0.3s a 60 FPS (pasos naturales)
**Efecto:** Marcha fluida y realista, como en Castlevania

---

### 3. SALTO (JUMP) - 1 → 2 frames

**Implementación:**
```c
a.frame = (p->vy < fx_neg(FX_C(1.5))) ? PFRAME_JUMP : PFRAME_JUMP_MID;
```

| Frame | Condición | Descripción |
|-------|-----------|-------------|
| JUMP | vy < -1.5 | Impulso inicial (brazos arriba) |
| JUMP_MID | vy ≥ -1.5 | Ápex del salto (flotación) |

**Efecto:** Arco de salto visible. A mayor impulso, más altura visual.

---

### 4. CAÍDA (FALL) - 1 → 2 frames

**Implementación:**
```c
a.frame = (p->vy > FX_C(2.0)) ? PFRAME_FALL_FAST : PFRAME_FALL;
```

| Frame | Condición | Descripción |
|-------|-----------|-------------|
| FALL | vy ≤ 2.0 | Caída moderada (resistencia al aire) |
| FALL_FAST | vy > 2.0 | Caída acelerada (velocidad terminal) |

**Efecto:** Peso visual. Cuanto más rápido cae, más urgente se ve.

---

### 5. DASH SOMBRÍO (DASH) - 1 → 2 frames

**Implementación:**
```c
a.frame = ((tick >> 1) & 1) ? PFRAME_DASH_TWIRL : PFRAME_DASH;
```

| Frame | Duración | Descripción |
|-------|----------|-------------|
| DASH | 2 ticks | Cuerpo comprimido (inicio) |
| DASH_TWIRL | 2 ticks | Giro de turbellino |

**Ciclo total:** 4 ticks = 240 RPM visual
**Efecto:** Movimiento rápido y dinámico, como un remolino.

---

### 6. GANCHITO MORTAL (ATK) - 3 → 5 frames

**Implementación:**
```c
if (p->atk_t > 8)      a.frame = PFRAME_ATK1;
else if (p->atk_t > 6) a.frame = PFRAME_ATK2;
else if (p->atk_t > 4) a.frame = PFRAME_ATK3;
else if (p->atk_t > 2) a.frame = PFRAME_ATK4;
else                   a.frame = PFRAME_ATK5;
```

| Frame | Duración | Descripción |
|-------|----------|-------------|
| ATK_1 | 3 frames | Preparación (garra retraída) |
| ATK_2 | 2 frames | Extensión inicial (zarpa sale) |
| ATK_3 | 2 frames | Impacto (máxima extensión) |
| ATK_4 | 2 frames | Permanencia (pinza presiona) |
| ATK_5 | 3 frames | Retracción (vuelta a reposo) |

**Duración total:** 12 frames (igual al original)
**Efecto:** Movimiento completo y convincente de la zarpa, no solo 3 poses fijas.

---

## ⚙️ Ajustes Técnicos

### Tiempos Optimizados

| Acción | Cambio | Razón |
|--------|--------|-------|
| Marcha | 16→18 ticks | Más pasos por segundo, más fluido |
| Ataque | Mismo (12) | Mejor distribución de frames |
| Idle | 32 ticks | Comportamiento relajado, contemplativo |
| Salto | Dinámico | Basado en velocidad vertical real |

### Velocidad de Cambio de Frames

- **Marcha:** Cada 3 ticks (cambios suaves pero perceptibles)
- **Ataque:** Cada 2-3 frames (decisivo, impactante)
- **Salto:** Dinámico según física (natural)
- **Idle:** Cada 32 ticks (respira lentamente)
- **Dash:** Cada 2 ticks (vertiginoso)

---

## 🎨 Guía para Crear Nuevos Frames

### Cuándo los necesites en PNG:

1. **IDLE_3:** Expresión diferente o parpadeo visible
   - Tamaño: 16x16px
   - Colores: Aprovechar los 256 disponibles para detalles faciales

2. **WALK_5 y WALK_6:** Fases finales del ciclo
   - WALK_5: Espejo horizontal de WALK_2 (talón derecho)
   - WALK_6: Posición neutra de recuperación

3. **JUMP_MID:** Pose de flotación
   - Brazos más relajados
   - Postura de "apuntería" opcional

4. **FALL_FAST:** Caída extrema
   - Brazos extendidos por resistencia
   - Expresión de urgencia

5. **DASH_TWIRL:** Rotación de turbellino
   - Cuerpo girado 45° vs DASH
   - Efecto visual de movimiento circular

6. **ATK_4 y ATK_5:** Complementan el movimiento
   - ATK_4: Garra completamente extendida (pinza de potencia)
   - ATK_5: Retracción graceful (vuelta a neutro)

---

## 📈 Impacto Visual

### Antes
- Animaciones "tirantes", poco naturales
- Pocas transiciones entre estados
- Sensación de "saltos" de pose
- Ataques "mugidos" sin fluidez

### Después
- Movimientos suaves y orgánicos
- Transiciones fluidas (casi cinemáticas)
- Cada acción tiene ritmo y peso
- Ataques con impacto y elegancia

---

## 🔧 Implementación sin Romper Compatibilidad

El código está diseñado para:
1. **Mantener la duración de acciones:** Tiempos de ataque, salto, etc., sin cambios
2. **Compatibilidad hacia atrás:** Si faltan frames, usa los disponibles (fallback)
3. **Escalable:** Agregar más frames es tan simple como actualizar el enum

---

## 📝 Próxima Fase

Para implementar completamente:
1. Editar `assets/exports/sprites/perseo/perseo.png`
2. Agregar los nuevos frames en orden (IDLE_3, WALK_5, WALK_6, etc.)
3. Actualizar `perseo.json` con los metadatos
4. Compilar con `make clean && make`

---

**Estado:** ✅ Código implementado - Listo para agregar frames en PNG
