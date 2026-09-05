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

#---------------------------------------------------------------------------------
# TARGET   : nombre del binario final (sin extensión)
# BUILD    : carpeta de artefactos intermedios (objetos, .elf, .gba)
# SOURCES  : carpetas con código fuente C (una por módulo, ver source/)
# INCLUDES : carpetas con headers propios y de terceros
# DATA     : carpetas con binarios ya generados (assets/gen) — se suman cuando
#            el pipeline de assets (docs/PLAN_MIGRACION_GBA_C.md, sección 5)
#            empiece a producir archivos .bin reales.
# LIBS     : librerías externas a enlazar
# LIBDIRS  : dónde buscar esas librerías (headers y .a)
#---------------------------------------------------------------------------------
TARGET      := perseo
BUILD       := build
SOURCES     := source \
               source/core \
               source/core/enemies \
               source/core/boss \
               source/core/level \
               source/platform/gba \
               source/ui
INCLUDES    := include source $(DEVKITPRO)/libtonc/include third_party/libtonc/include
DATA        := assets/gen
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

vpath %.c $(SOURCES)

CFILES := $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.c))
OFILES := $(addprefix $(BUILD)/,$(notdir $(CFILES:.c=.o)))

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
# pregen: genera source/core/level/*.h y assets/gen/** a partir de assets/src/
# y de los scripts de tools/, ANTES de compilar. Ver docs/PLAN_MIGRACION_GBA_C.md
# sección 5 (pipeline de datos y assets). Vacío por ahora: se llena a medida que
# existan esos scripts (Fase 3 del checklist).
#---------------------------------------------------------------------------------
pregen:
	@echo "(pendiente: tools/spritegen y tools/levelgen, ver Fase 3 del checklist)"

clean:
	@echo clean ...
	@rm -rf $(BUILD)

-include $(OFILES:.o=.d)
