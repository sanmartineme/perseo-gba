/* =====================================================================
   audio.h — música y efectos
   =====================================================================
   El prototipo sintetizaba todo con Web Audio: osciladores cuadrados,
   triangulares y de sierra, más ruido blanco. Eso es, casi literalmente,
   lo que hacen los cuatro canales PSG que la GBA heredó de la Game Boy
   (dos cuadradas, una onda programable y ruido), así que el port no
   necesita samples ni un tracker: se reprograman los mismos registros.
   Ver docs/PLAN_MIGRACION_GBA_C.md, sección 2, "Decisión de audio".

   Este header es la frontera: `core/` describe QUÉ suena (notas, timbre,
   volumen) y `platform/gba/pal_gba_audio.c` resuelve CÓMO. Por eso las
   funciones se declaran acá, en core, y no en la PAL: así el código de
   juego puede pedir un sonido sin incluir nada de plataforma.
   ===================================================================== */
#ifndef PERSEO_CORE_AUDIO_H
#define PERSEO_CORE_AUDIO_H

#include <stdint.h>
#include <stdbool.h>

/* Timbre pedido. En la GBA los tres primeros se aproximan con el ciclo de
   trabajo de las cuadradas o con una tabla en el canal de onda. */
typedef enum WaveKind {
    WAVE_SQUARE = 0,
    WAVE_TRIANGLE,
    WAVE_SINE,
    WAVE_SAW,
    WAVE_COUNT
} WaveKind;

typedef enum SfxId {
    SFX_JUMP = 0, SFX_DJUMP, SFX_DASH, SFX_ATK, SFX_TRAZA, SFX_SPLAT,
    SFX_HIT_ENEMY, SFX_HURT, SFX_PICK, SFX_ABILITY, SFX_BREAK,
    SFX_DOOR, SFX_CHECK, SFX_BOSS, SFX_DENY,
    SFX_COUNT
} SfxId;

typedef enum SongId {
    SONG_NONE = 0,
    SONG_TITLE, SONG_LVL1, SONG_LVL2, SONG_LVL3, SONG_LVL4,
    SONG_LVL5, SONG_LVL6, SONG_LVL7,
    SONG_BOSS, SONG_BOSS_FINAL, SONG_END,
    SONG_COUNT
} SongId;

/* Un paso de efecto: una nota con barrido opcional. Los efectos del
   prototipo que encadenaban varios tonos con setTimeout (recoger algo,
   desbloquear una habilidad) se representan con varios pasos y su
   retardo. */
typedef struct SfxStep {
    uint16_t freq;      /* Hz; 0 = este paso no suena */
    uint16_t slide_to;  /* Hz de destino del barrido; 0 = sin barrido */
    uint8_t  delay;     /* frames a esperar antes de dispararlo */
    uint8_t  duration;  /* frames que dura */
    uint8_t  volume;    /* 0..15 */
    uint8_t  wave;      /* WaveKind */
} SfxStep;

#define SFX_MAX_STEPS 4

typedef struct SfxDef {
    SfxStep steps[SFX_MAX_STEPS];
    uint8_t step_count;
    uint8_t noise_dur;  /* frames de ruido; 0 = sin ruido */
    uint8_t noise_vol;  /* 0..15 */
} SfxDef;

/* Una canción es un par de patrones que se repiten (bajo y melodía) más
   una pista opcional de percusión, exactamente como en el prototipo.
   Las notas van en Hz, y 0 es silencio. */
typedef struct Song {
    uint16_t bpm;
    uint8_t wave_bass, wave_lead;
    uint8_t vol_bass, vol_lead;   /* 0..15 */
    bool echo;                    /* repite la nota más floja poco después */
    const uint16_t *bass; uint8_t bass_len;
    const uint16_t *lead; uint8_t lead_len;
    const uint8_t  *drum; uint8_t drum_len; /* 0 si la canción no tiene */
} Song;

const Song *audio_get_song(SongId id);
const SfxDef *audio_get_sfx(SfxId id);

/* ---------- Interfaz que implementa la plataforma ---------- */
void audio_init(void);
void audio_play_song(SongId id);   /* SONG_NONE para silenciar */
void audio_play_sfx(SfxId id);
void audio_update(void);           /* una vez por frame */
void audio_set_muted(bool muted);

#endif /* PERSEO_CORE_AUDIO_H */
