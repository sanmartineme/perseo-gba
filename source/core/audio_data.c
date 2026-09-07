/* ARCHIVO GENERADO por tools/audiogen/songs_to_psg.py — no editar a mano.
   Fuente: docs/prototipo_referencia.html (SONGS y SFX).

   Las notas van en Hz tal como estaban en el prototipo; la capa de
   audio de la GBA las convierte al registro de frecuencia de cada
   canal. Los volumenes ya vienen convertidos de la ganancia de Web
   Audio (0.026..0.09) al rango 0..15 del PSG. */
#include "audio.h"

static const uint16_t title_bass[] = { 87, 0, 0, 98, 0, 0, 73, 0, 0, 82, 0, 0, 65, 0, 0, 0 };
static const uint16_t title_lead[] = { 0, 0, 220, 0, 0, 196, 0, 0, 175, 0, 0, 196, 0, 0, 0, 0 };
static const uint16_t lvl1_bass[] = { 110, 0, 0, 110, 0, 131, 0, 0, 98, 0, 0, 98, 0, 87, 0, 0 };
static const uint16_t lvl1_lead[] = { 0, 0, 220, 0, 262, 0, 0, 196, 0, 0, 220, 0, 175, 0, 196, 0 };
static const uint16_t lvl2_bass[] = { 98, 98, 0, 98, 117, 117, 0, 131, 98, 98, 0, 98, 87, 87, 0, 73 };
static const uint16_t lvl2_lead[] = { 392, 0, 349, 0, 330, 0, 392, 0, 440, 0, 349, 0, 392, 0, 330, 0 };
static const uint8_t lvl2_drum[] = { 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1 };
static const uint16_t lvl3_bass[] = { 82, 0, 0, 82, 0, 0, 73, 0, 0, 73, 0, 98, 0, 0, 65, 0 };
static const uint16_t lvl3_lead[] = { 0, 330, 0, 0, 294, 0, 0, 262, 0, 0, 294, 0, 247, 0, 0, 0 };
static const uint16_t lvl4_bass[] = { 62, 0, 69, 0, 0, 58, 0, 0, 62, 0, 77, 0, 0, 55, 0, 0 };
static const uint16_t lvl4_lead[] = { 0, 0, 415, 0, 0, 0, 392, 0, 0, 370, 0, 0, 0, 349, 0, 0 };
static const uint16_t lvl5_bass[] = { 73, 73, 87, 0, 73, 73, 98, 0, 65, 65, 82, 0, 73, 73, 0, 0 };
static const uint16_t lvl5_lead[] = { 293, 0, 0, 349, 0, 293, 0, 0, 261, 0, 0, 329, 0, 293, 0, 0 };
static const uint8_t lvl5_drum[] = { 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1 };
static const uint16_t lvl6_bass[] = { 87, 0, 0, 0, 98, 0, 0, 0, 82, 0, 0, 0, 73, 0, 0, 0 };
static const uint16_t lvl6_lead[] = { 0, 0, 349, 0, 0, 0, 392, 0, 0, 0, 330, 0, 0, 0, 294, 0 };
static const uint8_t lvl6_drum[] = { 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0 };
static const uint16_t lvl7_bass[] = { 65, 65, 0, 73, 65, 65, 0, 82, 58, 58, 0, 65, 55, 0, 0, 0 };
static const uint16_t lvl7_lead[] = { 391, 0, 349, 0, 391, 0, 440, 0, 349, 0, 391, 0, 329, 0, 0, 0 };
static const uint8_t lvl7_drum[] = { 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1 };
static const uint16_t boss_bass[] = { 73, 73, 0, 73, 87, 0, 73, 0, 73, 73, 0, 98, 92, 0, 87, 0 };
static const uint16_t boss_lead[] = { 587, 0, 554, 0, 587, 0, 0, 440, 0, 466, 0, 440, 0, 392, 0, 0 };
static const uint8_t boss_drum[] = { 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0 };
static const uint16_t bossFinal_bass[] = { 49, 49, 0, 55, 49, 0, 58, 0, 46, 46, 0, 55, 52, 0, 49, 0 };
static const uint16_t bossFinal_lead[] = { 622, 0, 587, 0, 622, 554, 0, 0, 494, 0, 466, 0, 440, 415, 0, 0 };
static const uint8_t bossFinal_drum[] = { 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1 };
static const uint16_t end_bass[] = { 131, 0, 98, 0, 110, 0, 87, 0 };
static const uint16_t end_lead[] = { 523, 0, 392, 440, 0, 349, 0, 0 };

static const Song SONGS[SONG_COUNT] = {
    [SONG_NONE] = { 0 },
    [SONG_TITLE] = {
        .bpm = 76,
        .wave_bass = WAVE_TRIANGLE, .wave_lead = WAVE_TRIANGLE,
        .vol_bass = 7, .vol_lead = 5,
        .echo = false,
        .bass = title_bass, .bass_len = sizeof(title_bass) / 2,
        .lead = title_lead, .lead_len = sizeof(title_lead) / 2,
        .drum = 0, .drum_len = 0,
    },
    [SONG_LVL1] = {
        .bpm = 100,
        .wave_bass = WAVE_SINE, .wave_lead = WAVE_TRIANGLE,
        .vol_bass = 8, .vol_lead = 5,
        .echo = true,
        .bass = lvl1_bass, .bass_len = sizeof(lvl1_bass) / 2,
        .lead = lvl1_lead, .lead_len = sizeof(lvl1_lead) / 2,
        .drum = 0, .drum_len = 0,
    },
    [SONG_LVL2] = {
        .bpm = 126,
        .wave_bass = WAVE_SQUARE, .wave_lead = WAVE_SAW,
        .vol_bass = 8, .vol_lead = 4,
        .echo = false,
        .bass = lvl2_bass, .bass_len = sizeof(lvl2_bass) / 2,
        .lead = lvl2_lead, .lead_len = sizeof(lvl2_lead) / 2,
        .drum = lvl2_drum, .drum_len = sizeof(lvl2_drum),
    },
    [SONG_LVL3] = {
        .bpm = 92,
        .wave_bass = WAVE_TRIANGLE, .wave_lead = WAVE_SINE,
        .vol_bass = 8, .vol_lead = 5,
        .echo = true,
        .bass = lvl3_bass, .bass_len = sizeof(lvl3_bass) / 2,
        .lead = lvl3_lead, .lead_len = sizeof(lvl3_lead) / 2,
        .drum = 0, .drum_len = 0,
    },
    [SONG_LVL4] = {
        .bpm = 110,
        .wave_bass = WAVE_SQUARE, .wave_lead = WAVE_TRIANGLE,
        .vol_bass = 7, .vol_lead = 4,
        .echo = true,
        .bass = lvl4_bass, .bass_len = sizeof(lvl4_bass) / 2,
        .lead = lvl4_lead, .lead_len = sizeof(lvl4_lead) / 2,
        .drum = 0, .drum_len = 0,
    },
    [SONG_LVL5] = {
        .bpm = 132,
        .wave_bass = WAVE_SAW, .wave_lead = WAVE_SQUARE,
        .vol_bass = 9, .vol_lead = 5,
        .echo = false,
        .bass = lvl5_bass, .bass_len = sizeof(lvl5_bass) / 2,
        .lead = lvl5_lead, .lead_len = sizeof(lvl5_lead) / 2,
        .drum = lvl5_drum, .drum_len = sizeof(lvl5_drum),
    },
    [SONG_LVL6] = {
        .bpm = 118,
        .wave_bass = WAVE_SINE, .wave_lead = WAVE_SQUARE,
        .vol_bass = 7, .vol_lead = 4,
        .echo = true,
        .bass = lvl6_bass, .bass_len = sizeof(lvl6_bass) / 2,
        .lead = lvl6_lead, .lead_len = sizeof(lvl6_lead) / 2,
        .drum = lvl6_drum, .drum_len = sizeof(lvl6_drum),
    },
    [SONG_LVL7] = {
        .bpm = 138,
        .wave_bass = WAVE_SAW, .wave_lead = WAVE_SAW,
        .vol_bass = 10, .vol_lead = 5,
        .echo = false,
        .bass = lvl7_bass, .bass_len = sizeof(lvl7_bass) / 2,
        .lead = lvl7_lead, .lead_len = sizeof(lvl7_lead) / 2,
        .drum = lvl7_drum, .drum_len = sizeof(lvl7_drum),
    },
    [SONG_BOSS] = {
        .bpm = 150,
        .wave_bass = WAVE_SAW, .wave_lead = WAVE_SQUARE,
        .vol_bass = 10, .vol_lead = 5,
        .echo = false,
        .bass = boss_bass, .bass_len = sizeof(boss_bass) / 2,
        .lead = boss_lead, .lead_len = sizeof(boss_lead) / 2,
        .drum = boss_drum, .drum_len = sizeof(boss_drum),
    },
    [SONG_BOSS_FINAL] = {
        .bpm = 172,
        .wave_bass = WAVE_SAW, .wave_lead = WAVE_SAW,
        .vol_bass = 12, .vol_lead = 6,
        .echo = true,
        .bass = bossFinal_bass, .bass_len = sizeof(bossFinal_bass) / 2,
        .lead = bossFinal_lead, .lead_len = sizeof(bossFinal_lead) / 2,
        .drum = bossFinal_drum, .drum_len = sizeof(bossFinal_drum),
    },
    [SONG_END] = {
        .bpm = 96,
        .wave_bass = WAVE_TRIANGLE, .wave_lead = WAVE_SQUARE,
        .vol_bass = 8, .vol_lead = 6,
        .echo = false,
        .bass = end_bass, .bass_len = sizeof(end_bass) / 2,
        .lead = end_lead, .lead_len = sizeof(end_lead) / 2,
        .drum = 0, .drum_len = 0,
    },
};

static const SfxDef SFXS[SFX_COUNT] = {
    [SFX_JUMP] = {
        .steps = {
            { 220, 440, 0, 6, 8, WAVE_SQUARE },
        },
        .step_count = 1,
    },
    [SFX_DJUMP] = {
        .steps = {
            { 330, 660, 0, 7, 8, WAVE_SQUARE },
        },
        .step_count = 1,
    },
    [SFX_DASH] = {
        .steps = {
            { 150, 80, 0, 6, 6, WAVE_SAW },
        },
        .step_count = 1,
        .noise_dur = 7, .noise_vol = 10,
    },
    [SFX_ATK] = {
        .steps = {
            { 700, 300, 0, 4, 6, WAVE_SQUARE },
        },
        .step_count = 1,
    },
    [SFX_TRAZA] = {
        .steps = {
            { 300, 140, 0, 6, 8, WAVE_TRIANGLE },
        },
        .step_count = 1,
        .noise_dur = 4, .noise_vol = 6,
    },
    [SFX_SPLAT] = {
        .steps = {
            { 120, 60, 0, 8, 8, WAVE_SQUARE },
        },
        .step_count = 1,
        .noise_dur = 10, .noise_vol = 12,
    },
    [SFX_HIT_ENEMY] = {
        .steps = {
            { 180, 90, 0, 5, 8, WAVE_SQUARE },
        },
        .step_count = 1,
        .noise_dur = 5, .noise_vol = 11,
    },
    [SFX_HURT] = {
        .steps = {
            { 140, 60, 0, 15, 11, WAVE_SAW },
        },
        .step_count = 1,
        .noise_dur = 9, .noise_vol = 10,
    },
    [SFX_PICK] = {
        .steps = {
            { 880, 0, 0, 4, 8, WAVE_SQUARE },
            { 1320, 0, 4, 6, 8, WAVE_SQUARE },
        },
        .step_count = 2,
    },
    [SFX_ABILITY] = {
        .steps = {
            { 440, 0, 0, 11, 10, WAVE_SQUARE },
            { 554, 0, 7, 11, 10, WAVE_SQUARE },
            { 659, 0, 13, 11, 10, WAVE_SQUARE },
            { 880, 0, 20, 11, 10, WAVE_SQUARE },
        },
        .step_count = 4,
    },
    [SFX_BREAK] = {
        .steps = {
            { 100, 50, 0, 9, 10, WAVE_SQUARE },
        },
        .step_count = 1,
        .noise_dur = 12, .noise_vol = 14,
    },
    [SFX_DOOR] = {
        .steps = {
            { 110, 55, 0, 24, 8, WAVE_SQUARE },
        },
        .step_count = 1,
        .noise_dur = 18, .noise_vol = 6,
    },
    [SFX_CHECK] = {
        .steps = {
            { 523, 0, 0, 6, 10, WAVE_TRIANGLE },
            { 784, 0, 6, 9, 10, WAVE_TRIANGLE },
        },
        .step_count = 2,
    },
    [SFX_BOSS] = {
        .steps = {
            { 80, 40, 0, 30, 14, WAVE_SAW },
        },
        .step_count = 1,
        .noise_dur = 24, .noise_vol = 11,
    },
    [SFX_DENY] = {
        .steps = {
            { 160, 120, 0, 5, 8, WAVE_SQUARE },
        },
        .step_count = 1,
    },
};

const Song *audio_get_song(SongId id) {
    if (id < 0 || id >= SONG_COUNT) id = SONG_NONE;
    return &SONGS[id];
}

const SfxDef *audio_get_sfx(SfxId id) {
    if (id < 0 || id >= SFX_COUNT) id = SFX_JUMP;
    return &SFXS[id];
}
