#!/bin/bash

# Script de compilación para GitHub Actions (Docker)
# Configurar DevKit correctamente y compilar

set -e

echo "🚀 COMPILACION EN DOCKER - PERSEO GBA"
echo "====================================="
echo ""

# Verificar que estamos en la raíz del proyecto
if [ ! -f "Makefile" ]; then
    echo "❌ Makefile no encontrado - no estamos en el directorio correcto"
    exit 1
fi

# Verificar variables de entorno en Docker
echo "Variables de entorno:"
echo "  DEVKITPRO=$DEVKITPRO"
echo "  DEVKITARM=$DEVKITARM"
echo ""

# Verificar herramientas
echo "Verificando herramientas:"
echo "  grit: $(which grit 2>&1 || echo 'ERROR: NO ENCONTRADO')"
echo "  arm-none-eabi-gcc: $(which arm-none-eabi-gcc 2>&1 || echo 'NO ENCONTRADO')"
echo ""

# Limpiar
echo "Limpiando build anterior..."
make clean || true

# Compilar
echo ""
echo "Compilando Perseo GBA..."
make -j4

# Verificar ROM generada
echo ""
echo "Verificando ROM generada..."
if [ -f "build/perseo.gba" ]; then
    echo "✅ ROM compilada exitosamente"
    ls -lh build/perseo.gba
    file build/perseo.gba
else
    echo "❌ ROM no se generó"
    exit 1
fi
