#---------------------------------------------------------------------------------
# Makefile de "Perseo: Sombras de Silencio" (GBA)
# Basado en la plantilla estándar de devkitARM para GBA.
# Ver docs/PLAN_MIGRACION_GBA_C.md, sección 9.2.
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/gba_rules

#---------------------------------------------------------------------------------
# TARGET   : nombre del binario final (sin extensión)
# BUILD    : carpeta de artefactos intermedios (objetos, .elf)
# SOURCES  : carpetas con código fuente C/C++/ASM (una por módulo, ver source/)
# INCLUDES : carpetas con headers propios
# DATA     : carpetas con binarios ya generados (assets/gen) que se incluyen como .o
# LIBS     : librerías externas a enlazar
# LIBDIRS  : dónde buscar esas librerías (headers y .a)
#---------------------------------------------------------------------------------
TARGET      :=  perseo
BUILD       :=  build
SOURCES     :=  source \
                source/core \
                source/core/enemies \
                source/core/boss \
                source/core/level \
                source/platform/gba \
                source/ui
INCLUDES    :=  include source
DATA        :=  assets/gen
LIBS        :=  -ltonc
LIBDIRS     :=  $(DEVKITPRO)/libtonc third_party/libtonc

#---------------------------------------------------------------------------------
# opciones de compilación para ambos ARM y THUMB
#---------------------------------------------------------------------------------
ARCH    :=  -mthumb -mthumb-interwork

CFLAGS  :=  -g -Wall -O2\
            -mcpu=arm7tdmi -mtune=arm7tdmi\
            $(ARCH)

CFLAGS  +=  $(INCLUDE)
CXXFLAGS    := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS :=  -g $(ARCH)
LDFLAGS =   -g $(ARCH) -Wl,-Map,$(notdir $*.map)

#---------------------------------------------------------------------------------
# lista de directorios de librerías propias del proyecto
#---------------------------------------------------------------------------------
LIBDIRS := $(LIBDIRS)

#---------------------------------------------------------------------------------
# no tocar nada debajo de esta línea, es genérico para todos los proyectos devkitARM
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT   :=  $(CURDIR)/$(BUILD)/$(TARGET)

export VPATH    :=  $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
                    $(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR  :=  $(CURDIR)/$(BUILD)

CFILES      :=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES    :=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES      :=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES    :=  $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.bin)))

#---------------------------------------------------------------------------------
export LD   :=  $(CC)
#---------------------------------------------------------------------------------

export OFILES   :=  $(BINFILES:.bin=.o) $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE  :=  $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                    $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                    -I$(CURDIR)/$(BUILD)

export LIBPATHS :=  $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean pregen

#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
# pregen: genera source/core/level/*.h y assets/gen/** a partir de assets/src/
# y de los scripts de tools/, ANTES de compilar. Ver docs/PLAN_MIGRACION_GBA_C.md
# sección 5 (pipeline de datos y assets).
#---------------------------------------------------------------------------------
pregen:
	python3 tools/spritegen/ascii_to_png.py assets/src/sprites/
	python3 tools/levelgen/levelgen.py assets/src/levels/ source/core/level/
	# grit sobre assets/src/sprites/**/*.png y assets/src/tiles/**/*.png -> assets/gen/
	# (se añaden aquí las invocaciones concretas de grit a medida que existan los .grit)

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).gba

#---------------------------------------------------------------------------------
else

DEPENDS :=  $(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).gba   :   $(OUTPUT).elf
$(OUTPUT).elf   :   $(OFILES)

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
