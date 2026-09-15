# ☁️ COMPILACIÓN EN LA NUBE - PERSEO GBA
**Alternativa a instalación local del DevKit**

---

## 🚀 OPCIÓN 1: GitHub Actions (RECOMENDADO)

### Paso 1: Configurar GitHub Actions

El workflow ya está creado en `.github/workflows/build.yml`

### Paso 2: Subir cambios a GitHub

```bash
cd /Users/carlossanmartin/Downloads/perseo-gba

# Agregar todos los cambios
git add -A

# Commit
git commit -m "Mejoras visuales: animaciones, efectos, UI, fondos, enemigos, transiciones, iluminación"

# Push (esto activará GitHub Actions automáticamente)
git push origin master
```

### Paso 3: Esperar compilación

1. Ir a: `https://github.com/[tu-usuario]/perseo-gba/actions`
2. Ver workflow "Compilar Perseo GBA" corriendo
3. Esperar ~5 minutos
4. Descargar `perseo-gba.zip` (ROM compilada)

### Ventajas
- ✅ Sin instalar nada localmente
- ✅ Compilación automática en cada push
- ✅ ROM descargable desde Actions
- ✅ 100% gratuito

### Desventajas
- ⏳ Toma ~5 minutos
- 🌐 Requiere GitHub conectado
- 📝 Requiere push a repositorio

---

## 🐳 OPCIÓN 2: Docker (LOCAL)

### Paso 1: Instalar Docker

```bash
# macOS
brew install docker

# O descargar desde: https://www.docker.com/products/docker-desktop
```

### Paso 2: Usar imagen preconfigurada de DevKit

```bash
# Descargar imagen con DevKit preinstalado
docker pull devkitpro/devkitarm:latest

# Compilar Perseo dentro de Docker
docker run --rm -v /Users/carlossanmartin/Downloads/perseo-gba:/perseo \
  devkitpro/devkitarm:latest \
  bash -c "cd /perseo && make clean && make"

# La ROM se genera en: build/perseo.gba
```

### Ventajas
- ✅ Instalación rápida
- ✅ Ambiente aislado
- ✅ Sin contaminar el sistema
- ✅ Reproducible en cualquier máquina

### Desventajas
- 📦 Requiere Docker instalado
- 🐢 Más lento que local (primera vez 2 GB descarga)

---

## 💻 OPCIÓN 3: Compilación Local (Sin DevKit)

### Usando Homebrew + compilador instalado

```bash
# 1. Crear stub de libtonc mínimo
mkdir -p ~/.devkit/include
cat > ~/.devkit/include/tonc.h << 'EOF'
// Stub mínimo para compilación
#ifndef TONC_H
#define TONC_H
#include <stdint.h>
#include <stddef.h>

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

#endif
EOF

# 2. Compilar con include mínimo
cd /Users/carlossanmartin/Downloads/perseo-gba
DEVKITPRO=~/.devkit \
DEVKITARM=$(which arm-none-eabi-gcc | xargs dirname | xargs dirname) \
make clean && make
```

### Ventajas
- ✅ Compilación local rápida
- ✅ Sin dependencias externas

### Desventajas
- ❌ Stub incompleto (puede fallar)
- ❌ No recomendado para producción

---

## 🎯 RECOMENDACIÓN

### Si tienes GitHub conectado:
→ **Usa GitHub Actions (Opción 1)**
- Sencillo, automático, gratuito
- Toma 5 minutos

### Si tienes Docker instalado:
→ **Usa Docker (Opción 2)**
- Rápido después de descarga inicial
- Perfectamente reproducible

### Si quieres algo ya:
→ **Prueba Opción 3**
- Pero espera posibles errores

---

## 📋 PASO A PASO: GitHub Actions

### 1. Verificar que git esté configurado
```bash
cd /Users/carlossanmartin/Downloads/perseo-gba
git config user.name
git config user.email

# Si no está configurado:
git config --global user.name "Tu Nombre"
git config --global user.email "tu@email.com"
```

### 2. Hacer commit y push
```bash
git add -A
git commit -m "✨ Mejoras visuales completas: 7 pasos implementados

- Animaciones: 23 frames (vs 12)
- Efectos: 5 tipos especializados
- UI: 6 elementos animados
- Fondos: 3 patrones temáticos
- Enemigos: animaciones fluidas
- Transiciones: 3 tipos
- Iluminación: sistema dinámico

Estado: Listo para compilar"

git push origin master
```

### 3. Monitorear compilación
```bash
# En terminal (si tienes GitHub CLI)
gh run list --repo [tu-usuario]/perseo-gba

# O en web: 
# https://github.com/[tu-usuario]/perseo-gba/actions
```

### 4. Descargar ROM compilada
```bash
# Una vez que la compilación termina:
# 1. Ir a Actions tab
# 2. Click en último run
# 3. Descargar "perseo-gba" artifact
# 4. Ejecutar: mgba perseo-gba/build/perseo.gba
```

---

## 📊 COMPARATIVA

| Método | Tiempo | Configuración | Dependencias |
|--------|--------|---------------|--------------|
| GitHub Actions | 5 min | Mínima | GitHub |
| Docker | 3-5 min | Media | Docker |
| Local Manual | 1-2 min | Alta | DevKit completo |
| Stub Local | 2-3 min | Baja | Parcial |

---

## 🔧 SI ALGO FALLA

### GitHub Actions no compila
```
1. Ir a Actions tab
2. Ver logs del run fallido
3. Leer el error en "Compilar Perseo GBA"
4. Reportar en Issues si es necesario
```

### Docker no descarga imagen
```bash
# Reintentar con proxy
docker pull devkitpro/devkitarm:latest --verbose

# O usar imagen alternativa
docker run --rm -v $(pwd):/perseo \
  devkitpro/devkitarm:multilib-latest \
  bash -c "cd /perseo && make"
```

### Compilación local lenta
```bash
# Usar más cores
make -j8  # 8 cores en lugar de 1

# Compilación paralela
make -j$(nproc)  # Detecta automáticamente
```

---

## 📚 REFERENCIAS

- GitHub Actions: https://github.com/features/actions
- Docker: https://www.docker.com/
- DevKitPro: https://devkitpro.org/

---

## 🎉 SIGUIENTES PASOS

### Inmediato (5 min)
```bash
cd /Users/carlossanmartin/Downloads/perseo-gba
git push origin master
# → GitHub Actions compila automáticamente
```

### Después de compilación (2 min)
```bash
# Descargar ROM desde Actions
mgba perseo-gba/build/perseo.gba
# → ¡Ver el juego mejorado!
```

---

**Método recomendado:** GitHub Actions (Opción 1)  
**ETA:** 5-10 minutos hasta tener ROM compilada
