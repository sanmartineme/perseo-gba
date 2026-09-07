#---------------------------------------------------------------------------------
# Makefile de "Perseo: Sombras de Silencio" (GBA)
#
# Nota de arquitectura: a diferencia de la plantilla clásica de devkitARM (que
# recompila recursivamente entrando a build/), este Makefile es de una sola
# pasada: usa `vpath` para encontrar los .c en source/** y un patrón explícito
# `$(BUILD)/%.o : %.c` para dejar los objetos en build/. Es más simple de leer
# y evita un problema reproducido en este entorno de desarrollo concreto donde
# la auto-invocación recursiva de `make` (el truco `$(MAKE) -C build -f ...`)
# resuelve mal su propia ruta bajo el MSYS2 embebido de devkitPro cuando se
# invoca fuera de su terminal MSYS2 dedicada. Ver docs/PLAN_MIGRACION_GBA_C.md.
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM)
endif
ifeq ($(strip $(DEVKITPRO)),)
$(error Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro)
endif

include $(DEVKITARM)/base_tools

# base_tools fuerza SHELL := /usr/bin/env bash; ese bash intermedio resetea
# TMP/TEMP al iniciar (reproducido en este entorno de desarrollo), lo que hace
# que arm-none-eabi-gcc intente escribir en C:\WINDOWS y falle por permisos.
# Se revierte a /bin/sh, que no tiene ese problema.
SHELL := /bin/sh

# Las reglas del pipeline de assets se declaran antes que `all`, asi que hay
# que fijar el objetivo por defecto explicitamente: si no, make tomaria como
# goal el primer target del archivo (un .c generado) y no construiria la ROM.
.DEFAULT_GOAL := all

#---------------------------------------------------------------------------------
# TARGET   : nombre del binario final (sin extensión)
# BUILD    : carpeta de artefactos intermedios (objetos, .elf, .gba)
# SOURCES  : carpetas con código fuente C (una por módulo, ver source/)
# INCLUDES : carpetas con headers propios y de terceros
# GENDIR   : salida del pipeline de assets (grit). No se versiona: se
#            regenera sola desde assets/src/ con las reglas de más abajo.
# LIBS     : librerías externas a enlazar
# LIBDIRS  : dónde buscar esas librerías (headers y .a)
#---------------------------------------------------------------------------------
TARGET      := perseo
BUILD       := build
GENDIR      := assets/gen
SOURCES     := source \
               source/core \
               source/core/enemies \
               source/core/boss \
               source/core/level \
               source/platform/gba \
               source/ui
INCLUDES    := include source $(GENDIR) $(DEVKITPRO)/libtonc/include third_party/libtonc/include
LIBS        := -ltonc
LIBDIRS     := $(DEVKITPRO)/libtonc third_party/libtonc

GAME_TITLE  := PERSEO
GAME_CODE   := APSE
MAKER_CODE  := 00

#---------------------------------------------------------------------------------
# opciones de compilación
#---------------------------------------------------------------------------------
ARCH     := -mthumb -mthumb-interwork
CFLAGS   := -g -Wall -O2 -mcpu=arm7tdmi -mtune=arm7tdmi $(ARCH) \
            $(foreach dir,$(INCLUDES),-I$(dir))
LDFLAGS  := -g $(ARCH) -specs=gba.specs -Wl,-Map,$(BUILD)/$(TARGET).map
LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

#---------------------------------------------------------------------------------
# TMP/TEMP explícitos para el compilador. Sin esto, en algunos entornos Windows
# arm-none-eabi-gcc intenta escribir sus archivos temporales en C:\WINDOWS y
# falla por permisos en vez de usar una carpeta propia del proyecto.
#---------------------------------------------------------------------------------
export TMP  := $(CURDIR)/$(BUILD)/tmp
export TEMP := $(CURDIR)/$(BUILD)/tmp

#---------------------------------------------------------------------------------
# Pipeline de assets (Fase 3, ver docs/PLAN_MIGRACION_GBA_C.md sección 5)
#
# assets/src/**.png  --grit-->  assets/gen/*.c + *.h  --gcc-->  ROM
#
# Los .png son la fuente de verdad editable (los genera una vez
# tools/spritegen/ascii_to_png.py a partir del prototipo, y de ahí en más
# los edita quien haga el arte). Las opciones de conversión de cada imagen
# viven en el .grit que la acompaña, que grit lee solo.
#---------------------------------------------------------------------------------
# Cada nombre de aca produce assets/gen/<nombre>.c + .h.
#
# OJO: los objetos se nombran por basename (build/<nombre>.o), asi que
# ningun asset generado puede llamarse igual que un modulo de source/.
# Por eso la hoja de particulas es fx_particles y no particles: chocaba
# con source/core/particles.c y el enlazado fallaba por simbolo duplicado. Los .png y sus
# .grit se buscan con vpath, asi que no hace falta repetir rutas.
GEN_NAMES := perseo enemies_16x8 enemies_8x8 enemies_16x16 fx_8x8 fx_particles bosses              tileset_tuneles
GEN_C := $(addprefix $(GENDIR)/,$(addsuffix .c,$(GEN_NAMES)))
GEN_H := $(GEN_C:.c=.h)

ASSET_DIRS := assets/src/sprites/perseo assets/src/sprites/enemigos               assets/src/sprites/fx assets/src/sprites/jefes assets/src/tiles
vpath %.png $(ASSET_DIRS)
vpath %.grit $(ASSET_DIRS)

$(GENDIR)/%.c $(GENDIR)/%.h: %.png %.grit
	@mkdir -p $(GENDIR)
	$(SILENTMSG) grit $(notdir $<)
	$(SILENTCMD)grit $< -o $(GENDIR)/$*

vpath %.c $(SOURCES) $(GENDIR)

CFILES := $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.c)) $(GEN_C)
OFILES := $(addprefix $(BUILD)/,$(notdir $(CFILES:.c=.o)))

# Nadie compila hasta que existan los headers generados (en un build limpio
# todavía no hay .d de dependencias que lo garantice).
$(OFILES): | $(GEN_H)

.PHONY: all clean pregen run

all: $(BUILD)/$(TARGET).gba

$(BUILD):
	mkdir -p $(BUILD) $(BUILD)/tmp

$(BUILD)/%.o: %.c | $(BUILD)
	$(SILENTMSG) $(notdir $<)
	$(SILENTCMD)$(CC) $(CFLAGS) -MMD -MP -MF $(BUILD)/$*.d -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OFILES)
	$(SILENTMSG) linking $(notdir $@)
	$(SILENTCMD)$(CC) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@

$(BUILD)/$(TARGET).gba: $(BUILD)/$(TARGET).elf
	$(SILENTCMD)$(OBJCOPY) -O binary $< $@
	@echo built ... $(notdir $@)
	$(SILENTCMD)gbafix $@ -t$(GAME_TITLE) -c$(GAME_CODE) -m$(MAKER_CODE)

#---------------------------------------------------------------------------------
# pregen: re-siembra el arte desde el prototipo y regenera los datos de nivel.
# NO hace falta para compilar: los .png de assets/src/ y los niveles
# generados en source/core/level/ están versionados. Sólo se corre al
# tocar un .json de nivel o al querer volver a partir del prototipo.
PYTHON ?= python3

pregen:
	$(PYTHON) tools/spritegen/ascii_to_png.py
	$(PYTHON) tools/levelgen/levelgen.py assets/src/levels/ source/core/level/

clean:
	@echo clean ...
	@rm -rf $(BUILD) $(GENDIR)

-include $(OFILES:.o=.d)
