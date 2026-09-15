/* =====================================================================
   pal_gba_save.c — la SRAM del cartucho
   =====================================================================
   Dos reglas del hardware que no se pueden saltar y explican todo este
   archivo:

   1. La SRAM (0x0E000000, 32 KB en un cartucho homebrew típico) **sólo
      admite accesos de 8 bits**. Un memcpy normal usaría palabras de 16
      o 32 y devolvería basura, así que se copia byte a byte a mano.

   2. Los emuladores y las tarjetas flash deciden qué tipo de memoria de
      guardado emular **buscando una cadena dentro de la ROM**. Sin ella,
      la SRAM sencillamente no existe y el guardado se pierde en
      silencio, que es la peor forma de fallar. De ahí la etiqueta
      SRAM_Vnnn de abajo, marcada como `used` para que el enlazador no la
      descarte por no estar referenciada.

   Ver docs/TAREAS_MIGRACION_GBA.md, F7-12, y la sección 10 del plan.
   ===================================================================== */
#include <tonc.h>
#include "../pal.h"

#define SRAM_BASE ((volatile uint8_t *)0x0E000000)
/* Lo que de verdad trae un cartucho homebrew. libtonc define SRAM_SIZE
   como 0x10000, que es el espacio de direcciones reservado, no el chip. */
#define SAVE_SRAM_BYTES 32768

/* La firma que buscan los emuladores. Su único trabajo es estar en la
   ROM... y ahí está el detalle: `used` sólo impide que la descarte el
   COMPILADOR. devkitARM enlaza con -fdata-sections --gc-sections, así
   que el enlazador la tiraba igual por no estar referenciada y el
   emulador dejaba de ver la SRAM. Por eso se lee de verdad, a través de
   un puntero volatile que el optimizador no puede resolver. */
__attribute__((used, section(".rodata")))
static const char SAVE_TYPE_TAG[] = "SRAM_V113";
static const char *volatile s_save_tag = SAVE_TYPE_TAG;

static bool s_ready = false;

/* La SRAM es lenta: necesita 8 ciclos de espera. Con el valor por
   defecto las lecturas salen corruptas en hardware real. */
static void sram_init(void) {
    if (s_ready) return;
    REG_WAITCNT = (REG_WAITCNT & ~0x0003) | 0x0003;
    (void)*s_save_tag;   /* ancla la etiqueta: ver el comentario de arriba */
    s_ready = true;
}

bool pal_save_read(void *dst, uint32_t size) {
    if (!dst || size == 0 || size > SAVE_SRAM_BYTES) return false;
    sram_init();
    uint8_t *out = (uint8_t *)dst;
    for (uint32_t i = 0; i < size; i++) out[i] = SRAM_BASE[i];
    return true;
}

bool pal_save_write(const void *src, uint32_t size) {
    if (!src || size == 0 || size > SAVE_SRAM_BYTES) return false;
    sram_init();
    const uint8_t *in = (const uint8_t *)src;
    for (uint32_t i = 0; i < size; i++) SRAM_BASE[i] = in[i];
    /* Relectura de comprobación: si el cartucho no tiene SRAM (o el
       emulador no la emula), escribir "funciona" pero no queda nada.
       Mejor enterarse acá que al cargar. */
    for (uint32_t i = 0; i < size; i++) {
        if (SRAM_BASE[i] != in[i]) return false;
    }
    return true;
}
