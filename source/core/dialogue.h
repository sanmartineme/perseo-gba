/* =====================================================================
   dialogue.h — texto narrativo del juego
   =====================================================================
   La cinemática de introducción, la del final, los créditos y los
   diálogos previos a cada jefe. Todo son datos: los emite
   tools/storygen/story_to_c.py leyendo INTRO_STORY, ENDING_STORY,
   CREDITS y BOSS_DIALOG del prototipo, así que el texto se corrige en un
   único sitio y no se transcribe a mano.

   NOTA sobre `scene`: cada viñeta del prototipo indicaba qué ilustración
   dibujar ('street_calm', 'ambush', 'caged'...), pintadas a mano sobre el
   canvas. Ese arte no existe todavía como sprites, así que el campo no
   se porta: guardar un identificador que nadie puede resolver sólo
   aparentaría que la ilustración está. Cuando haya arte de viñetas, se
   añade acá y se vuelve a generar.
   ===================================================================== */
#ifndef PERSEO_CORE_DIALOGUE_H
#define PERSEO_CORE_DIALOGUE_H

#include <stdint.h>
#include "boss/boss_config.h"

/* Una viñeta: quién habla (0 = narrador) y qué dice. */
typedef struct StoryPage {
    const char *who;
    const char *text;
} StoryPage;

/* Una pantalla de créditos: un título y hasta dos líneas debajo. */
typedef struct CreditPage {
    const char *title;
    const char *line1;
    const char *line2;
} CreditPage;

const StoryPage  *story_intro(uint8_t *count);
const StoryPage  *story_ending(uint8_t *count);
const CreditPage *story_credits(uint8_t *count);

/* Las tres réplicas que se cruzan antes de cada pelea. */
const StoryPage  *boss_dialogue(BossId id, uint8_t *count);

#endif /* PERSEO_CORE_DIALOGUE_H */
