/* =====================================================================
   save.h — la partida guardada
   =====================================================================
   Un único struct empaquetado que se escribe tal cual a la SRAM del
   cartucho. core/ sólo conoce el struct y la validación; que "por
   debajo" sea SRAM lo resuelve platform/gba/pal_gba_save.c, igual que
   con el vídeo y el audio.

   Se guarda solo, al encender una lámpara de control: el prototipo
   guardaba en localStorage sin pedir permiso, y esa es la costumbre que
   uno quiere en un juego así — nadie debería perder media hora por no
   haber ido a un menú.

   Las máscaras `chapas_taken[]` y `tiles_broken[]` que pedía la sección
   10 del plan estuvieron ausentes hasta la Fase 10, y a propósito:
   dependen de la posición exacta de cada chapa y de cada tile rompible,
   que se fijan al generar los niveles, y mientras sólo existían dos de
   los siete, fijar el formato habría sido fijarlo para romperlo. Con los
   siete niveles en su sitio ya se pudo cerrar: ahora una chapa recogida
   no vuelve a aparecer y una rejilla rota sigue rota.
   ===================================================================== */
#ifndef PERSEO_CORE_SAVE_H
#define PERSEO_CORE_SAVE_H

#include <stdint.h>
#include <stdbool.h>
#include "level/level.h"   /* LEVEL_COUNT */

/* 'PS' de Perseo/Silencio. Si no coincide, la SRAM tiene basura o el
   guardado de otro juego, y se ignora. */
#define SAVE_MAGIC   0x5053u
/* Sube cuando cambia el formato: un guardado viejo se descarta en vez de
   leerse mal. */
/* 2: entraron chapas_taken[] y tiles_broken[]. Un guardado de la
   versión 1 se descarta en vez de leerse mal — que es justo para lo que
   está este número. */
#define SAVE_VERSION 2

typedef struct SaveData {
    uint16_t magic;
    uint8_t  version;
    uint8_t  checksum;      /* de todo lo que viene después */

    uint8_t  difficulty;
    uint8_t  muted;
    uint8_t  abilities;     /* máscara de AbilityId */
    uint8_t  relics_found;
    uint8_t  relics_equipped;

    uint8_t  level;
    uint8_t  has_checkpoint;
    uint8_t  cp_level;
    int16_t  cp_tx, cp_ty;

    int16_t  chapas;
    uint16_t deaths;
    uint32_t play_frames;

    /* Qué se llevó ya de cada nivel: un bit por chapa y por rejilla
       oxidada, numeradas en el orden en que aparecen en los datos del
       nivel. Ver PlayerProgress en core/player.h, que es donde se
       explica el criterio y dónde vive en marcha. */
    uint32_t chapas_taken[LEVEL_COUNT];
    uint64_t tiles_broken[LEVEL_COUNT];
} SaveData;

/* Suma simple sobre los bytes que siguen al propio checksum. No es
   criptografía: sólo distingue un guardado íntegro de una SRAM a medio
   escribir o sin pila. */
uint8_t save_checksum(const SaveData *s);
void    save_seal(SaveData *s);       /* rellena magic/version/checksum */
bool    save_is_valid(const SaveData *s);

#endif /* PERSEO_CORE_SAVE_H */
