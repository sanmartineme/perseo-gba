# Mejoras de Efectos Visuales - Sistema de Partículas Mejorado
**Fecha:** 2026-09-15  
**Objetivo:** Explosiones y impactos visualmente ricos como en Castlevania/Metroid

---

## 📊 Cambios en Sistema de Partículas

### Capacidad
```
Antes: 32 partículas máximo
Ahora: 48 partículas máximo
Razón: Soportar explosiones más ricas sin sacrificar OAM
```

### Física Mejorada

| Aspecto | Antes | Después | Efecto |
|---------|-------|---------|--------|
| Gravedad | 0.10 | 0.08 | Caídas más suaves, menos "caída al suelo" |
| Fricción | N/A | 0.97/frame | Desaceleración realista (resistencia del aire) |
| Duración | 20-35 frames | 20-35 frames | Tiempo suficiente para ver el efecto |

---

## 🎨 Tipos de Efectos Visuales

### 1. Impacto de Garra (`particles_impact_claw`)

**Uso:** Cuando Perseo ataca con Ganchito Mortal y golpea

**Composición:**
- 3x Partículas blancas (FX_C(1.2)) → Chispas de metal
- 2x Partículas rojas (FX_C(1.0)) → Rasgadura/sangre

**Patrón visual:** Explosión compacta, impacto cráneo directo
**Inspiración:** Castlevania (impactos de látigo)

```
        * ✦ *
      *   ✦   *
        * * *
      ← IMPACTO →
```

---

### 2. Impacto de Traza (`particles_impact_traza`)

**Uso:** Cuando la Traza golpea a un enemigo

**Composición:**
- 4x Partículas marrones (FX_C(1.5)) → Polvo e energía
- 2x Partículas naranjas (FX_C(1.3)) → Chispa caliente

**Patrón visual:** Explosión mediana, dispersión más amplia
**Inspiración:** Metroid (impactos de proyectiles)

```
      ✦ o ✦
    o   ✦   o
      ✦ o ✦
      EXPANSIÓN
```

---

### 3. Muerte de Enemigo (`particles_death_enemy`)

**Uso:** Cuando un enemigo muere (vida → 0)

**Composición:**
- 6x Partículas grises (FX_C(2.0)) → Polvo masivo
- 3x Partículas marrones (FX_C(1.5)) → Restos/chatarra

**Patrón visual:** Explosión grande, desintegración
**Inspiración:** Castlevania (enemigos que se desintegran)

```
      * ○ *
    ○ ○ ○ ○ ○
      * ○ *
      DISPERSIÓN MASIVA
```

---

### 4. Chispa Eléctrica (`particles_electric`)

**Uso:** Trampas eléctricas, enemigos con energía, efectos de corriente

**Composición:**
- 5x Partículas cyan (FX_C(1.8)) → Electricidad
- 2x Partículas blancas (FX_C(1.5)) → Núcleo brillante

**Patrón visual:** Explosión radial rápida, muy visible
**Inspiración:** Metroid Fusion (enemigos eléctricos)

```
      ⚡ ✦ ⚡
    ✦ ⚡ ✦ ⚡ ✦
      ⚡ ✦ ⚡
      RADIACIÓN
```

---

### 5. Veneno/Gas (`particles_poison`)

**Uso:** Trampas de veneno, aliento tóxico, enemigos venenosos

**Composición:**
- 4x Partículas verdes (FX_C(0.8)) → Gas denso, movimiento lento
- 2x Partículas púrpuras (FX_C(0.6)) → Neblina oscura

**Patrón visual:** Explosión lenta y tóxica, menos energía
**Inspiración:** Castlevania (efectos de magia oscura)

```
      ✧ ≈ ✧
    ≈ ≈ ≈ ≈ ≈
      ✧ ≈ ✧
      DIFUSIÓN LENTA
```

---

## 🎨 Paleta de Colores Expandida

### Nuevos Colores (Sept 2026)

```
PCOL_GREEN   = 6   Veneno, ácido, efectos tóxicos
PCOL_PURPLE  = 7   Magia oscura, energía oscura
PCOL_ORANGE  = 8   Fuego, calor, explosión
```

### Paleta Completa

| Índice | Nombre | Uso | RGB555 |
|--------|--------|-----|--------|
| 0 | RED | Sangre, daño | (31, 0, 0) |
| 1 | GOLD | Monedas, recompensas | (31, 31, 0) |
| 2 | WHITE | Metal, impactos | (31, 31, 31) |
| 3 | BROWN | Tierra, polvo | (21, 10, 0) |
| 4 | GREY | Escombros, polvo gris | (15, 15, 15) |
| 5 | CYAN | Agua, electricidad | (0, 31, 31) |
| 6 | GREEN | Veneno, ácido | (0, 31, 0) |
| 7 | PURPLE | Magia oscura | (31, 0, 31) |
| 8 | ORANGE | Fuego, calor | (31, 15, 0) |

---

## ⚙️ Técnicas de Física Mejoradas

### Fricción del Aire

Cada partícula pierde 3% de velocidad por frame (multiplicada por 0.97):

```c
p->vx = fx_mul(p->vx, PARTICLE_FRICTION);  /* 0.97 = 3% pérdida */
p->vy = fx_mul(p->vy, PARTICLE_FRICTION);
```

**Efecto visual:** Las partículas "flotan" de forma realista, no salen disparadas indefinidamente

---

### Dispersión Realista

Las partículas se crean en ángulos aleatorios desde el punto de impacto:

```c
int32_t angle = rng_below(256);           /* 0-255 = 0-360 grados */
fx_t h_spread = fx_mul(power, (fx_t)...); /* Dispersión horizontal */
fx_t v_spread = fx_mul(power, (fx_t)...); /* Dispersión vertical (sesgo arriba) */
```

**Efecto visual:** Explosión natural, no simétrica ni artificial

---

## 📈 Impacto Visual

### Antes
- Explosiones pequeñas, 8 partículas máximo por evento
- Sin fricción: partículas salen disparadas indefinidamente
- Solo 6 colores disponibles
- Efectos genéricos, indistinguibles

### Después
- Explosiones ricas, hasta 12 partículas por tipo de evento
- Fricción realista: efecto "flotante" y natural
- 9 colores disponibles para variedad
- Cada evento tiene identidad visual propia

---

## 🔌 Integración con Código de Juego

### Uso en Player (Ganchito Mortal)
```c
// Antes:
particles_burst(hb.x, hb.y, PCOL_WHITE, 4, FX_C(1.5));

// Después:
particles_impact_claw(hb.x, hb.y);  /* Automáticamente 5 partículas en patrón */
```

### Uso en Enemigos (Muerte)
```c
// Antes:
particles_burst(e->x, e->y, PCOL_GREY, 4, FX_C(2.0));

// Después:
particles_death_enemy(e->x, e->y);  /* 9 partículas en patrón de desintegración */
```

### Uso en Trampas
```c
// Trampa eléctrica:
particles_electric(trap_x, trap_y);

// Trampa de veneno:
particles_poison(trap_x, trap_y);
```

---

## 🎬 Secuencia Visual Típica

### Ataque de Perseo → Golpe a Enemigo → Muerte

```
Frame 0:    Ganchito Mortal toca enemigo
           ↓ Llamar: particles_impact_claw(x, y)
           
Frame 1-5:  Explosión de 5 partículas (blanco+rojo)
           ↓ Física: gravedad + fricción
           
Frame 6+:   Partículas flotan y caen
           ↓ Llamar: particles_death_enemy(x, y)
           
Frame 7-20: Explosión de 9 partículas (gris masivo)
           ↓ Dispersión amplia
           
Frame 21+:  Polvo se dispersa y desaparece
           ↓ Sensación de IMPACTO y CIERRE visual
```

---

## 🔧 Próxima Fase

Para actualizar completamente:

1. **Paletas PNG:** Agregar verde, púrpura y naranja a `fx_8x8.png`
2. **Sonido:** Asociar SFX a cada tipo de impacto
3. **Timing:** Sincronizar partículas con animación de ataque
4. **Variación:** Usar diferentes poderes según dificultad/estado

---

**Estado:** ✅ Sistema de partículas mejorado - Listo para compilar
