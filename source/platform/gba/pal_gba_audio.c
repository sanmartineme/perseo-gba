/* =====================================================================
   pal_gba_audio.c — implementación GBA de core/audio.h
   =====================================================================
   El prototipo sintetizaba con Web Audio: osciladores + ruido blanco,
   cada nota con su rampa de ganancia. Los cuatro canales PSG que la GBA
   heredó de la Game Boy hacen exactamente eso en hardware, así que acá
   no hay mezclador ni samples: se reprograman registros y el hardware
   sigue sonando solo mientras el juego hace otra cosa. Coste por frame:
   unas pocas escrituras de 16 bits.

   Reparto de canales (la decisión de diseño de este archivo):
     ch1  cuadrada          -> TONOS DE EFECTO, con barrido manual.
          Casi todos los efectos del prototipo son un tono que cae o
          sube (`slide`), y tenerlos en su propio canal evita que la
          música se los coma.
     ch2  cuadrada          -> melodía.
     ch3  onda programable  -> bajo. Es el único que reproduce una tabla
          arbitraria, así que acá sí salen triangular, seno y sierra de
          verdad y no una aproximación.
     ch4  ruido             -> percusión y el ruido de los efectos.
          Los dos comparten canal: el efecto tiene prioridad, porque un
          golpe que no suena se nota mucho más que un charles perdido.

   La melodía y los efectos son cuadradas obligadas, así que su timbre
   se aproxima con el ciclo de trabajo (duty): es lo más cerca que llega
   el hardware a distinguir una sierra de una cuadrada.

   Ver docs/PLAN_MIGRACION_GBA_C.md, sección 5 (Fase 6).
   ===================================================================== */
#include <tonc.h>
#include "../../core/audio.h"

/* --- Bits que libtonc no nombra (GBATEK) --- */
/* REG_SND3SEL */
#define S3SEL_BANK1     0x0020  /* toca el banco 1; las escrituras van al 0 */
#define S3SEL_ON        0x0080
/* REG_SND3CNT: volumen en los bits 13-14 */
#define S3VOL_0         (0 << 13)
#define S3VOL_100       (1 << 13)
#define S3VOL_50        (2 << 13)
#define S3VOL_25        (3 << 13)

/* Envolvente de las cuadradas y del ruido. OJO: la dirección es 0 =
   baja, 1 = sube (GBATEK); las constantes SSQR_INC/SSQR_DEC de libtonc
   están al revés, así que acá se arma el campo a mano. */
#define ENV(vol, up, step) (((vol) << 12) | ((up) << 11) | ((step) << 8))

/* Duraciones de nota de playSong() en el prototipo, pasadas a frames:
   0.16 s el bajo, 0.10 s la melodía, 0.045 s la percusión. */
#define BASS_FRAMES   10
#define LEAD_FRAMES    6
#define DRUM_FRAMES    3
#define DRUM_VOL       6    /* ganancia 0.035 del prototipo */
/* Retardos del eco: 95 ms y 115 ms. */
#define ECHO_BASS_DELAY 6
#define ECHO_LEAD_DELAY 7

/* ---------------------------------------------------------------------
   Tablas de onda del canal 3, 32 muestras de 4 bits cada una.
   --------------------------------------------------------------------- */
static const uint8_t WAVE_TABLES[WAVE_COUNT][32] = {
    /* WAVE_SQUARE */
    { 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    /* WAVE_TRIANGLE */
    {  0, 2, 4, 6, 8,10,12,14,15,14,12,10, 8, 6, 4, 2,
       0, 2, 4, 6, 8,10,12,14,15,14,12,10, 8, 6, 4, 2 },
    /* WAVE_SINE */
    {  8,10,11,13,14,15,15,15,15,15,14,13,11,10, 8, 7,
       5, 4, 2, 1, 0, 0, 0, 0, 0, 0, 1, 2, 4, 5, 7, 8 },
    /* WAVE_SAW */
    {  0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7,
       8, 8, 9, 9,10,10,11,11,12,12,13,13,14,14,15,15 },
};

/* Ciclo de trabajo con el que se imita cada timbre en las cuadradas. */
static const uint16_t DUTY_OF[WAVE_COUNT] = {
    SSQR_DUTY1_2,   /* cuadrada: la de verdad */
    SSQR_DUTY1_8,   /* triangular: la más suave que hay disponible */
    SSQR_DUTY1_2,   /* seno: indistinguible de la cuadrada acá */
    SSQR_DUTY1_4,   /* sierra: más áspera que la cuadrada */
};

/* ---------------------------------------------------------------------
   Estado
   --------------------------------------------------------------------- */
static bool s_ready = false;
static bool s_muted = false;

static SongId       s_song_id = SONG_NONE;
static const Song  *s_song = 0;
static uint16_t     s_step_frames = 1;  /* duración de un paso del secuenciador */
static uint16_t     s_frame_in_step = 0;
static uint32_t     s_music_step = 0;
/* Ecos pendientes: frames que faltan (0 = ninguno) y la nota a repetir. */
static uint8_t  s_echo_bass_t = 0, s_echo_lead_t = 0;
static uint16_t s_echo_bass_f = 0, s_echo_lead_f = 0;

/* Efecto en curso. `s_sfx_next` es el próximo paso que falta disparar:
   así los efectos encadenados del prototipo (recoger, habilidad) se
   resuelven contando frames en vez de con setTimeout. */
static const SfxDef *s_sfx = 0;
static uint8_t s_sfx_frame = 0, s_sfx_next = 0;
/* Barrido manual del ch1: se interpola la frecuencia frame a frame en
   vez de usar el barrido por hardware, que sólo hace saltos geométricos
   y no la rampa lineal del prototipo. */
static uint16_t s_slide_from = 0, s_slide_to = 0;
static uint8_t  s_slide_len = 0, s_slide_t = 0;
/* Frames en los que el ruido pertenece al efecto y la percusión calla. */
static uint8_t s_noise_lock = 0;

/* ---------------------------------------------------------------------
   Conversión de Hz al registro de frecuencia de cada canal
   --------------------------------------------------------------------- */
static inline uint16_t rate_square(uint16_t hz) {
    if (hz < 64) hz = 64;               /* por debajo el registro se desborda */
    int32_t r = 2048 - (int32_t)(131072u / hz);
    if (r < 0) r = 0;
    if (r > 2047) r = 2047;
    return (uint16_t)r;
}

static inline uint16_t rate_wave(uint16_t hz) {
    if (hz < 32) hz = 32;
    int32_t r = 2048 - (int32_t)(65536u / hz);
    if (r < 0) r = 0;
    if (r > 2047) r = 2047;
    return (uint16_t)r;
}

/* Paso de envolvente para que la nota se apague en `frames`. El tiempo
   total de caída es vol * step / 64 segundos. */
static inline uint16_t env_step_for(uint8_t vol, uint16_t frames) {
    if (vol == 0) return 1;
    uint32_t s = (16u * frames) / (15u * vol);
    if (s < 1) s = 1;
    if (s > 7) s = 7;
    return (uint16_t)s;
}

/* Arranca un barrido manual en el ch1. Cinco efectos del prototipo
   (splat, hurt, break, door, boss) caen por debajo de 64 Hz, que es el
   piso fisico de las cuadradas de la GBA. Si se dejara el destino
   original, la caida llegaria al piso a mitad de camino y el resto del
   efecto seria un tono plano; recortando el destino ANTES de interpolar,
   la caida se reparte a lo largo de toda su duracion y conserva el
   gesto, que es lo que se oye. */
#define SQ_MIN_HZ 64

static void slide_start(uint16_t from, uint16_t to, uint8_t frames) {
    s_slide_from = from < SQ_MIN_HZ ? SQ_MIN_HZ : from;
    s_slide_to   = to   < SQ_MIN_HZ ? SQ_MIN_HZ : to;
    s_slide_len  = frames;
    s_slide_t    = 0;
}

/* ---------------------------------------------------------------------
   Disparo de cada canal
   --------------------------------------------------------------------- */
static void ch1_tone(uint16_t hz, uint8_t vol, uint8_t wave, uint16_t frames) {
    REG_SND1SWEEP = 0;   /* el barrido lo hace audio_update() a mano */
    REG_SND1CNT  = DUTY_OF[wave & 3] | ENV(vol, 0, env_step_for(vol, frames));
    REG_SND1FREQ = rate_square(hz) | SFREQ_RESET;
}

static void ch2_note(uint16_t hz, uint8_t vol, uint8_t wave, uint16_t frames) {
    REG_SND2CNT  = DUTY_OF[wave & 3] | ENV(vol, 0, env_step_for(vol, frames));
    REG_SND2FREQ = rate_square(hz) | SFREQ_RESET;
}

/* El canal de onda no tiene envolvente: sólo cuatro volúmenes fijos y un
   contador de longitud, así que la nota se corta en vez de apagarse. */
static void ch3_note(uint16_t hz, uint8_t vol, uint16_t frames) {
    uint16_t v = vol >= 10 ? S3VOL_100 : (vol >= 5 ? S3VOL_50 : (vol >= 1 ? S3VOL_25 : S3VOL_0));
    int32_t len = 256 - ((int32_t)frames * 256) / 60;
    if (len < 0) len = 0;
    REG_SND3CNT  = v | (uint16_t)len;
    REG_SND3FREQ = rate_wave(hz) | SFREQ_TIMED | SFREQ_RESET;
}

static void ch4_hit(uint8_t vol, uint16_t frames, uint16_t shift) {
    REG_SND4CNT  = ENV(vol, 0, env_step_for(vol, frames));
    REG_SND4FREQ = (shift << 4) | SFREQ_RESET;   /* ratio 0 = el más agudo */
}

/* Carga una tabla de onda en el banco 0 del ch3 y lo deja sonando desde
   ahí. Las escrituras a WAVE_RAM van al banco que NO está seleccionado,
   de ahí el baile de S3SEL_BANK1. */
static void wave_load(uint8_t wave) {
    const uint8_t *t = WAVE_TABLES[wave & 3];
    REG_SND3SEL = S3SEL_BANK1;                 /* apunta al 1: escribo en el 0 */
    for (int i = 0; i < 4; i++) {
        uint32_t w = 0;
        for (int j = 0; j < 4; j++) {
            /* Cada byte lleva dos muestras, la primera en el nibble alto. */
            uint32_t b = ((uint32_t)t[i * 8 + j * 2] << 4) | t[i * 8 + j * 2 + 1];
            w |= b << (j * 8);
        }
        /* Los parentesis no sobran: la macro de libtonc es un cast sin
           envolver, y sin ellos el indice se aplicaria antes del cast. */
        (REG_WAVE_RAM)[i] = w;
    }
    REG_SND3SEL = S3SEL_ON;                    /* toca el banco 0 */
}

/* ---------------------------------------------------------------------
   Interfaz pública
   --------------------------------------------------------------------- */
void audio_init(void) {
    REG_SNDSTAT   = SSTAT_ENABLE;   /* obligatorio antes de tocar nada más */
    REG_SNDDSCNT  = SDS_DMG100;
    REG_SNDDMGCNT = SDMG_BUILD_LR(SDMG_SQR1 | SDMG_SQR2 | SDMG_WAVE | SDMG_NOISE, 7);
    REG_SND1SWEEP = 0;
    REG_SND4FREQ  = 0;
    wave_load(WAVE_TRIANGLE);
    s_ready = true;
    s_song_id = SONG_NONE;
    s_song = 0;
    s_sfx = 0;
    s_slide_len = 0;
    s_noise_lock = 0;
}

void audio_play_song(SongId id) {
    if (!s_ready || id == s_song_id) return;
    s_song_id = id;
    s_music_step = 0;
    s_frame_in_step = 0;
    s_echo_bass_t = s_echo_lead_t = 0;
    REG_SND2CNT = 0;
    REG_SND3SEL = 0;
    if (id == SONG_NONE) { s_song = 0; return; }

    s_song = audio_get_song(id);
    /* El prototipo corría un setInterval de 60000/bpm/2 ms: a 60 fps eso
       es 1800/bpm frames por paso. */
    uint16_t f = s_song->bpm ? (uint16_t)(1800u / s_song->bpm) : 15;
    s_step_frames = f < 1 ? 1 : f;
    wave_load(s_song->wave_bass);
}

void audio_play_sfx(SfxId id) {
    if (!s_ready || s_muted) return;
    const SfxDef *d = audio_get_sfx(id);
    s_sfx = d;
    s_sfx_frame = 0;
    s_sfx_next = 0;
    s_slide_len = 0;
    if (d->noise_dur) {
        ch4_hit(d->noise_vol, d->noise_dur, 5);
        s_noise_lock = d->noise_dur;
    }
    /* El primer paso casi siempre va sin retardo; audio_update() se
       encarga igual, pero dispararlo acá evita un frame de latencia
       justo en los sonidos que responden a un botón. */
    if (d->step_count > 0 && d->steps[0].delay == 0) {
        const SfxStep *st = &d->steps[0];
        if (st->freq) {
            ch1_tone(st->freq, st->volume, st->wave, st->duration);
            if (st->slide_to) slide_start(st->freq, st->slide_to, st->duration);
        }
        s_sfx_next = 1;
    }
}

void audio_set_muted(bool muted) {
    s_muted = muted;
    if (!s_ready) return;
    if (muted) {
        REG_SNDDMGCNT = 0;
        REG_SND1CNT = 0;
        REG_SND2CNT = 0;
        REG_SND4CNT = 0;
        REG_SND3SEL = 0;
        s_sfx = 0;
        s_slide_len = 0;
    } else {
        REG_SNDDMGCNT = SDMG_BUILD_LR(SDMG_SQR1 | SDMG_SQR2 | SDMG_WAVE | SDMG_NOISE, 7);
        if (s_song) wave_load(s_song->wave_bass);
    }
}

bool audio_is_muted(void) { return s_muted; }

void audio_update(void) {
    if (!s_ready || s_muted) return;

    /* ---- barrido manual del tono de efecto (ch1) ---- */
    if (s_slide_len) {
        s_slide_t++;
        int32_t d = (int32_t)s_slide_to - (int32_t)s_slide_from;
        int32_t hz = (int32_t)s_slide_from + (d * s_slide_t) / s_slide_len;
        /* Sin SFREQ_RESET: cambiar la frecuencia no debe reiniciar la
           envolvente, o el efecto no se apagaría nunca. */
        REG_SND1FREQ = rate_square((uint16_t)hz);
        if (s_slide_t >= s_slide_len) s_slide_len = 0;
    }

    /* ---- pasos encadenados del efecto en curso ---- */
    if (s_sfx) {
        s_sfx_frame++;
        while (s_sfx_next < s_sfx->step_count &&
               s_sfx->steps[s_sfx_next].delay <= s_sfx_frame) {
            const SfxStep *st = &s_sfx->steps[s_sfx_next];
            if (st->freq) {
                ch1_tone(st->freq, st->volume, st->wave, st->duration);
                if (st->slide_to) slide_start(st->freq, st->slide_to, st->duration);
                else s_slide_len = 0;
            }
            s_sfx_next++;
        }
        if (s_sfx_next >= s_sfx->step_count && s_slide_len == 0) s_sfx = 0;
    }
    if (s_noise_lock) s_noise_lock--;

    /* ---- ecos pendientes ---- */
    if (s_echo_bass_t && --s_echo_bass_t == 0 && s_song) {
        ch3_note(s_echo_bass_f, (uint8_t)((s_song->vol_bass * 2) / 5), BASS_FRAMES - 1);
    }
    if (s_echo_lead_t && --s_echo_lead_t == 0 && s_song) {
        ch2_note(s_echo_lead_f, (uint8_t)((s_song->vol_lead * 7) / 20),
                 s_song->wave_lead, LEAD_FRAMES);
    }

    /* ---- secuenciador ---- */
    if (!s_song) return;
    if (s_frame_in_step > 0) { s_frame_in_step--; return; }
    s_frame_in_step = s_step_frames - 1;

    const Song *s = s_song;
    if (s->bass_len) {
        uint16_t n = s->bass[s_music_step % s->bass_len];
        if (n) {
            ch3_note(n, s->vol_bass, BASS_FRAMES);
            if (s->echo) { s_echo_bass_f = n; s_echo_bass_t = ECHO_BASS_DELAY; }
        }
    }
    if (s->lead_len) {
        uint16_t n = s->lead[s_music_step % s->lead_len];
        if (n) {
            ch2_note(n, s->vol_lead, s->wave_lead, LEAD_FRAMES);
            if (s->echo) { s_echo_lead_f = n; s_echo_lead_t = ECHO_LEAD_DELAY; }
        }
    }
    /* La percusión cede el canal de ruido a los efectos. */
    if (s->drum_len && !s_noise_lock) {
        if (s->drum[s_music_step % s->drum_len]) ch4_hit(DRUM_VOL, DRUM_FRAMES, 4);
    }
    s_music_step++;
}
