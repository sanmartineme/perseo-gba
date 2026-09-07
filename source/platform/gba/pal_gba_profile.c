/* =====================================================================
   pal_gba_profile.c — cuánto tarma cada frame, medido en la consola
   =====================================================================
   F9-01 pedía perfilar en mGBA. mGBA no se pudo instalar en esta máquina
   (su instalador pide elevación), pero para lo que hace falta saber —
   ¿cabe el trabajo de un frame en los 280.896 ciclos que dura? — no hace
   falta un emulador con perfilador: la propia GBA tiene cuatro
   temporizadores, y medir dentro de la ROM tiene la ventaja de que el
   número sale del mismo código que se va a ejecutar en el cartucho.

   Se usa TIMER3 con preescala de 64 ciclos: un frame entero son
   280896/64 = 4389 pasos, que entran de sobra en el contador de 16 bits
   y dan una resolución del 0,02% del frame. TIMER0 y TIMER1 se dejan
   libres a propósito, porque son los que usaría el sonido por DMA si
   alguna vez se cambia el PSG por Maxmod.

   Coste cuando no se mide: cinco escrituras de 16 bits por frame. Se
   deja siempre activo — un contador que sólo existe en las builds de
   depuración es un contador en el que no se puede confiar.
   ===================================================================== */
#include <tonc.h>
#include "../pal.h"

/* Ciclos de un frame completo (VDraw + VBlank) a 16,78 MHz. */
#define FRAME_CYCLES 280896
#define TICKS_PER_FRAME (FRAME_CYCLES / 64)   /* 4389 */

static uint16_t s_marks[PAL_PROFILE_COUNT];
/* Foto del frame anterior. El medidor se dibuja DENTRO del tramo de
   interfaz, asi que si leyera s_marks veria su propia fila todavia sin
   marcar y siempre mostraria cero justo en la que mas interesa. */
static uint16_t s_prev[PAL_PROFILE_COUNT];
static uint16_t s_worst[PAL_PROFILE_COUNT];
/* El peor TOTAL se guarda aparte, y no se suman los peores de cada tramo:
   cada maximo puede venir de un frame distinto, asi que esa suma describe
   un frame que quiza no ocurrio nunca y siempre exagera. Este es el peor
   frame de verdad, que es el que decide si el juego va a 60. */
static uint16_t s_worst_total;
static uint16_t s_last_start;
static uint16_t s_overruns;      /* frames que no cupieron en su frame */
static uint16_t s_grace;         /* frames a ignorar tras cargar un nivel */

/* Frames de cortesía tras cargar un nivel. El primer frame de una zona
   hace el llenado completo del tilemap (32x32 entradas) y pinta la
   interfaz entera: es caro y es real, pero ocurre detrás de la
   transición y no dice nada sobre si el juego va a 60. Contarlo como
   "peor caso" sólo servía para esconder el número que importa. */
#define PROFILE_GRACE_FRAMES 20

void pal_profile_frame_start(void) {
    /* Cuenta del frame ANTERIOR: si su trabajo no cupo, la consola se
       comió un VBlank y el juego bajó de 60. */
    if (s_grace == 0) {
        uint32_t total = 0;
        for (int i = 0; i < PAL_PROFILE_COUNT; i++) {
            total += s_marks[i];
            /* El peor caso se cierra aca y no en pal_profile_mark(): un
               tramo puede marcarse mas de una vez por frame (la interfaz
               lo hace: el volcado al principio y el dibujo al final), y
               lo que interesa comparar es lo que costo el tramo en el
               frame entero, no cada mitad por separado. */
            if (s_marks[i] > s_worst[i]) s_worst[i] = s_marks[i];
        }
        if (total > s_worst_total) s_worst_total = (uint16_t)total;
        if (total > TICKS_PER_FRAME) s_overruns++;
    } else {
        s_grace--;
    }

    REG_TM3CNT_H = 0;                 /* parar para poder recargar */
    REG_TM3CNT_L = 0;
    REG_TM3CNT_H = TM_FREQ_64 | TM_ENABLE;
    s_last_start = 0;
    for (int i = 0; i < PAL_PROFILE_COUNT; i++) {
        s_prev[i] = s_marks[i];
        s_marks[i] = 0;
    }
}

void pal_profile_level_changed(void) {
    s_grace = PROFILE_GRACE_FRAMES;
    pal_profile_reset_worst();
}

uint16_t pal_profile_overruns(void) { return s_overruns; }

void pal_profile_mark(PalProfileSlot slot) {
    if (slot >= PAL_PROFILE_COUNT) return;
    uint16_t now = REG_TM3CNT_L;
    /* Suma en vez de asignar: un mismo tramo puede cerrarse en dos veces
       dentro del frame. */
    s_marks[slot] = (uint16_t)(s_marks[slot] + (uint16_t)(now - s_last_start));
    s_last_start = now;
}

uint16_t pal_profile_ticks(PalProfileSlot slot, bool worst) {
    if (slot >= PAL_PROFILE_COUNT) return 0;
    return worst ? s_worst[slot] : s_prev[slot];
}

uint16_t pal_profile_total(bool worst) {
    if (worst) return s_worst_total;
    uint32_t sum = 0;
    for (int i = 0; i < PAL_PROFILE_COUNT; i++) sum += s_prev[i];
    return (uint16_t)sum;
}

int pal_profile_percent(uint16_t ticks) {
    return (int)(((uint32_t)ticks * 100u) / TICKS_PER_FRAME);
}

void pal_profile_reset_worst(void) {
    for (int i = 0; i < PAL_PROFILE_COUNT; i++) s_worst[i] = 0;
    s_worst_total = 0;
    s_overruns = 0;
}
