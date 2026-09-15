# Plan de Implementación Visual - Perseo GBA
**Fecha:** 2026-09-15  
**Estado Actual:** 3 de 7 pasos completados en código

---

## ✅ COMPLETADO EN CÓDIGO

### ✅ Paso 1: Animaciones (IMPLEMENTADO)
- [x] Enum `PlayerFrame` expandido (12→23 frames)
- [x] Función `player_get_anim()` mejorada
- [x] Physics-based animation selection
- [x] Documentación: `ANIMATION_IMPROVEMENTS.md`

**Archivos modificados:**
- `source/core/player.h` - Nuevo enum
- `source/core/player.c` - Nueva lógica de selección

**Próximos pasos:**
```
1. Editar: assets/exports/sprites/perseo/perseo.png
2. Agregar 11 frames nuevos en orden correcto
3. Actualizar: assets/src/sprites/perseo/perseo.json
4. Compilar: make clean && make
```

---

### ✅ Paso 2: Efectos Visuales (IMPLEMENTADO)
- [x] Física de partículas mejorada (gravedad, fricción)
- [x] Capacidad aumentada (32→48 partículas)
- [x] 5 nuevas funciones de efectos especializados
- [x] Paleta expandida (6→9 colores)
- [x] Documentación: `EFFECTS_IMPROVEMENTS.md`

**Archivos modificados:**
- `source/core/particles.h` - Nuevas funciones + colores
- `source/core/particles.c` - Física y efectos

**Próximos pasos:**
```
1. Editar: assets/exports/sprites/fx/fx_8x8.png
2. Agregar colores: verde (PCOL_GREEN), púrpura (PCOL_PURPLE), naranja (PCOL_ORANGE)
3. Compilar: make clean && make
4. Probar: Ejecutar ataque y observar explosiones nuevas
```

---

### ✅ Paso 3: UI/Interfaz (IMPLEMENTADO)
- [x] Barra de vida con colores dinámicos
- [x] Indicador crítico (parpadeo)
- [x] Habilidades con indicador de "nuevo"
- [x] Barra de jefe con animaciones
- [x] Carteles con fade suave

**Archivos modificados:**
- `source/ui/hud.c` - Todas las funciones mejoradas

**Próximos pasos:**
```
1. Compilar: make clean && make
2. Probar en GBA: Atacar, recibir daño, obtener habilidad
3. Ajustar tiempos si es necesario
```

---

## 🚀 PENDIENTE - DOCUMENTACIÓN COMPLETA

### 🚀 Paso 4: Fondos por Nivel
**Estado:** Estrategia documentada, código pendiente
**Documento:** `COMPREHENSIVE_VISUAL_IMPROVEMENTS.md` (Paso 4)

**Tarea:**
1. Crear función `build_parallax_level(int level_index)` en `pal_gba_video.c`
2. Implementar 3 patrones:
   - Patrón 1: Industrial (tuberías complejas)
   - Patrón 2: Arquitectura (líneas ordenadas)
   - Patrón 3: Orgánico (fractales naturales)
3. Llamar desde `build_parallax()` con índice de nivel

**Estimación:** 2-3 horas de desarrollo

---

### 🚀 Paso 5: Enemigos y Jefe
**Estado:** Especificaciones documentadas
**Documento:** `COMPREHENSIVE_VISUAL_IMPROVEMENTS.md` (Paso 5)

**Tarea:**
1. Expandir frames en PNGs de enemigos:
   - `enemies_8x8.png`: +1-2 frames por enemigo
   - `enemies_16x16.png`: +2-3 frames
   - `bosses.png`: +3-4 frames para jefe

2. Actualizar archivos .grit si es necesario

3. Opcionalmente mejorar paletas de enemigos

**Estimación:** 4-6 horas de arte

---

### 🚀 Paso 6: Transiciones
**Estado:** Estrategia documentada
**Documento:** `COMPREHENSIVE_VISUAL_IMPROVEMENTS.md` (Paso 6)

**Tarea:**
1. Implementar en `pal_gba_video.c`:
   - `pal_video_transition_fade(duration)`
   - `pal_video_transition_mosaic(duration)`
   - `pal_video_transition_wipe(direction)`

2. Integrar en game loop en `game_update()`

3. Probar transiciones en cambios de nivel

**Estimación:** 3-4 horas de desarrollo

---

### 🚀 Paso 7: Iluminación Dinámica
**Estado:** Conceptos documentados
**Documento:** `COMPREHENSIVE_VISUAL_IMPROVEMENTS.md` (Paso 7)

**Tarea:**
1. Crear sistema de sombras usando paletas
2. Implementar funciones:
   - `apply_shadow_palette(intensity)`
   - `apply_spotlight(x, y, radius)`

3. Aplicar a enemigos, trampas, efectos especiales

**Estimación:** 4-6 horas (más si se hace con post-procesamiento)

---

## 📋 CHECKLIST DE IMPLEMENTACIÓN

### Fase 1: Assets (Semana 1-2)
- [ ] Editar `perseo.png` - agregar 11 frames nuevos
- [ ] Editar `fx_8x8.png` - agregar 3 colores nuevos
- [ ] Editar enemigos - agregar frames de animación
- [ ] Editar jefe - agregar poses de combate
- [ ] Actualizar JSONs con metadatos nuevos

### Fase 2: Código (Semana 2-3)
- [ ] Implementar Paso 4 (fondos por nivel)
- [ ] Implementar Paso 6 (transiciones)
- [ ] Pruebas en emulador
- [ ] Ajustes de timing/physics

### Fase 3: Pulido (Semana 3-4)
- [ ] Implementar Paso 5 (animar enemigos en código)
- [ ] Implementar Paso 7 (iluminación)
- [ ] Pruebas visuales completas
- [ ] Optimización de rendimiento

### Fase 4: QA (Semana 4+)
- [ ] Probar en GBA hardware
- [ ] Ajustar colores para pantalla real
- [ ] Validar no hay lag/parpadeos
- [ ] Build final

---

## 🎮 TESTING

### Testing Manual - Animaciones
```
1. Iniciar juego
2. Dejar a Perseo en reposo → Debe respirar (3 frames alternando)
3. Caminar → Debe ser fluido (6 frames, ciclo suave)
4. Saltar → Arco visible (dos frames diferentes)
5. Atacar → Movimiento completo de zarpa (5 poses)
6. Dash → Rotación visible (turbellino)
```

### Testing Manual - Efectos
```
1. Golpear enemigo → Efecto `impact_claw` (5 partículas)
2. Lanzar Traza → Efecto `impact_traza` (6 partículas)
3. Matar enemigo → Efecto `death_enemy` (9 partículas, dispersión)
4. Trampa eléctrica → Efecto `electric` (radiación)
5. Trampa veneno → Efecto `poison` (difusión lenta)
```

### Testing Manual - UI
```
1. Vida normal → Barra roja
2. Vida 50% → Barra roja sigue
3. Vida 25% → Barra parpadea orange/rojo
4. Vida <25% → "!" parpadea, barra roja/blanco
5. Obtener habilidad → "*" parpadea 60 frames
6. Boss aparece → Nombre en rojo, barra visible
7. Boss recibe daño → Barra destella blanco
```

---

## 📊 ESTIMACIONES DE RENDIMIENTO

### VRAM
```
Antes: ~45KB (paletas + sprites)
Después: ~64KB (paletas 256-color + más sprites)
Límite GBA: 96KB para OBJ
Margen: 32KB (seguro)
```

### CPU
```
Animaciones: +2-3% (lógica de selección de frames)
Partículas: +1-2% (fricción del aire)
UI: +1% (cálculos de color dinámico)
Fondos: 0% (hardware scroll)
Total adicional: ~4-6% (aceptable)
```

### OAM (Sprite Slots)
```
Antes: ~100 de 128 slots
Después: ~110 de 128 slots (partículas + enemigos animados)
Margen: 18 slots (suficiente)
```

---

## 🎨 INSPIRACIÓN Y REFERENCIAS

### Castlevania IV (GBA)
- Parallax profundo con múltiples capas
- Animaciones suaves de látigo
- Efectos de particulas en cada golpe
- Colores ricos y atmósfera oscura

### Metroid Fusion (GBA)
- Enemigos con animaciones fluidas
- Efectos de energía visibles
- Transiciones suaves entre estados
- UI dinámica y responsiva

### Castlevania: Aria of Sorrow (GBA)
- Animaciones de ataque con múltiples frames
- Fondos detallados y temáticos
- Efectos de luz y sombra
- Enemigos con comportamientos visuales distintos

---

## 📞 SOPORTE Y AYUDA

### Si algo no compila
1. Verificar que `.grit` está actualizado
2. Verificar `assets/gen/` está regenerado
3. Limpiar: `make clean && make`

### Si los efectos no se ven
1. Verificar colores en paleta PNG
2. Verificar `PCOL_*` enums están correctos
3. Probar con emulador primero

### Si el rendimiento baja
1. Revisar CPU con profiler
2. Reducir número de partículas si es necesario
3. Optimizar loops de parallax

---

## 📝 DOCUMENTOS RELACIONADOS

- `COMPREHENSIVE_VISUAL_IMPROVEMENTS.md` - Visión general (este archivo amplía)
- `ANIMATION_IMPROVEMENTS.md` - Detalles de animaciones
- `EFFECTS_IMPROVEMENTS.md` - Detalles de efectos
- `VISUAL_IMPROVEMENTS_LOG.md` - Log histórico de cambios
- `VISUAL_IMPROVEMENTS_LOG.md` - Técnicas de GBA

---

**Última actualización:** 2026-09-15  
**Próxima revisión:** Cuando se compile y pruebe Fase 1  
**Responsable:** Tu equipo de desarrollo de Perseo GBA
