# 📦 Instrucciones de Compilación - Perseo GBA
**Fecha:** 2026-09-15  
**Estado:** Preparado para compilación (necesita devkit)

---

## ⚠️ ESTADO ACTUAL

El compilador ARM está instalado, pero **falta el devkit GBA completo** (libtonc y herramientas).

```
✅ Compilador ARM: arm-none-eabi-gcc (16.1.0)
❌ DevKit GBA: /opt/devkitpro → no existe
❌ Tonc Library: necesaria
```

---

## 📋 REQUISITOS PARA COMPILAR

### 1. DevKit GBA (Opción A: Instalación Automática)

#### macOS (Homebrew)
```bash
# Instalar devkitPro
brew tap devkitpro/devkitpro
brew install devkitarm

# Instalar Tonc (librería GBA)
brew install libtonc

# Verificar instalación
ls -la /opt/devkitpro/devkitARM/
```

#### Linux (Ubuntu/Debian)
```bash
# Agregar repositorio
sudo apt-add-repository ppa:devkitpro/ppa
sudo apt-get update

# Instalar
sudo apt-get install devkitarm gba-tools libtonc

# Verificar
ls -la /opt/devkitpro/
```

#### Windows (MSYS2 o WSL)
```bash
# En MSYS2/MinGW64:
pacman -S devkitpro-devkitarm devkitpro-libtonc

# Verificar
ls /opt/devkitpro/
```

### 2. Configurar Variables de Entorno

Después de instalar, configura:

```bash
# En ~/.zshrc o ~/.bash_profile (macOS)
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=$DEVKITPRO/devkitARM
export PATH=$DEVKITARM/bin:$PATH

# Luego recargar:
source ~/.zshrc
```

### 3. Verificar Instalación

```bash
# Verificar devkit
ls -la $DEVKITARM/base_tools

# Verificar libtonc
ls -la $DEVKITPRO/libtonc/

# Verificar compilador
arm-none-eabi-gcc --version
arm-none-eabi-objcopy --version
```

---

## 🔧 COMPILACIÓN (Después de instalar DevKit)

### Paso 1: Limpiar

```bash
cd /Users/carlossanmartin/Downloads/perseo-gba
make clean
```

### Paso 2: Compilar

```bash
make -j4  # Usar 4 cores para más velocidad
```

### Paso 3: Verificar ROM

```bash
# Verificar que se generó
ls -la build/perseo.gba

# Verificar tamaño
file build/perseo.gba
```

---

## 🎮 EJECUTAR EN EMULADOR

```bash
# Usar mgba (instalado)
mgba build/perseo.gba &

# O en otro emulador
vba-m build/perseo.gba  # VisualBoyAdvance
```

---

## ✅ CHECKLIST PRE-COMPILACIÓN

- [ ] DevKit instalado: `ls $DEVKITPRO`
- [ ] Tonc library: `ls $DEVKITPRO/libtonc/include/tonc.h`
- [ ] ARM compiler: `arm-none-eabi-gcc --version`
- [ ] Variables de entorno configuradas
- [ ] Proyecto limpio: `make clean`

---

## 🐛 TROUBLESHOOTING

### Error: "base_tools: No such file or directory"
```bash
# Solución: instalar devkit
brew install devkitarm
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=$DEVKITPRO/devkitARM
```

### Error: "tonc.h: No such file"
```bash
# Solución: instalar tonc
brew install libtonc
# O descargar manualmente desde: http://www.coranac.com/tonc/
```

### Error: "multiple target patterns"
```bash
# Solución: limpiar build
make clean
rm -rf build/
mkdir build
```

### Compilación lenta
```bash
# Usar múltiples cores
make -j4   # 4 cores
make -j8   # 8 cores
```

---

## 📊 TIEMPO ESTIMADO

```
Instalar DevKit:    5-10 minutos
Compilar proyecto:  2-3 minutos
Testing emulador:   5 minutos
Total:              15-20 minutos
```

---

## 🎬 DESPUÉS DE COMPILAR

### 1. Probar en Emulador
```bash
mgba build/perseo.gba
```

### 2. Observar Cambios Visuales
```
✓ Animaciones fluidas
✓ Efectos de partículas
✓ UI responsiva
✓ Fondos atmosféricos
✓ Transiciones suaves
```

### 3. Editar Assets (Opcional)
```bash
# Si quieres más frames:
open assets/exports/sprites/perseo/perseo.png
# Editar con Aseprite/GIMP
cp assets/exports/sprites/perseo/perseo.png assets/src/sprites/perseo/
make clean && make
```

---

## 📞 SI ALGO FALLA

### Verificar Rutas
```bash
echo $DEVKITPRO
echo $DEVKITARM
ls -la $DEVKITARM/bin/arm-none-eabi-*
```

### Verificar Includes
```bash
arm-none-eabi-gcc -I$DEVKITPRO/libtonc/include -c /dev/null
```

### Forzar Recompilación
```bash
make clean
rm -rf build/
mkdir build
make -B
```

---

## 🎯 PRÓXIMOS PASOS

1. **Instalar DevKit** (5-10 min)
2. **Compilar** (2-3 min)
3. **Ejecutar en emulador** (5 min)
4. **Observar mejoras visuales** ✨
5. **Opcional: Editar assets y recompilar**

---

## 📚 REFERENCIAS

- **DevKit GBA:** https://devkitpro.org/
- **Tonc Tutorial:** http://www.coranac.com/tonc/
- **GBA Specs:** https://problemkaputt.de/gbatek.htm
- **mGBA Emulator:** https://mgba.io/

---

**Estado:** 🚀 Listo para instalar DevKit y compilar  
**Próximo paso:** Ejecutar instrucciones de instalación arriba
