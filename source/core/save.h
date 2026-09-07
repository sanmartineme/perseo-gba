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

   DESVIACIÓN respecto de la sección 10 del plan: allí el struct llevaba
   además `collected_chapas[]` y `broken_tiles[][]`, máscaras de bits por
   nivel. No están. Esas máscaras dependen de la posición exacta de cada
   chapa y de cada tile rompible, que se fijan al generar los niveles, y
   hoy sólo existen dos de los siete. Meterlas ahora obligaría a fijar un
   formato que la Fase 8 rompería en cuanto entren los cinco niveles que
   faltan. Consecuencia real y conocida: al cargar, las chapas ya
   recogidas vuelven a estar en el mapa (el contador, en cambio, sí se
   conserva).
   ===================================================================== */
#ifndef PERSEO_CORE_SAVE_H
#define PERSEO_CORE_SAVE_H

#include <stdint.h>
#include <stdbool.h>

/* 'PS' de Perseo/Silencio. Si no coincide, la SRAM tiene basura o el
   guardado de otro juego, y se ignora. */
#define SAVE_MAGIC   0x5053u
/* Sube cuando cambia el formato: un guardado viejo se descarta en vez de
   leerse mal. */
#define SAVE_VERSION 1

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
} SaveData;

/* Suma simple sobre los bytes que siguen al propio checksum. No es
   criptografía: sólo distingue un guardado íntegro de una SRAM a medio
   escribir o sin pila. */
uint8_t save_checksum(const SaveData *s);
void    save_seal(SaveData *s);       /* rellena magic/version/checksum */
bool    save_is_valid(const SaveData *s);

#endif /* PERSEO_CORE_SAVE_H */
