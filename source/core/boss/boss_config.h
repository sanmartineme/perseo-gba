/* =====================================================================
   boss_config.h — los 6 jefes como datos
   =====================================================================
   Traducción de BOSS_CONFIG{} del prototipo (docs/prototipo_referencia.html,
   línea ~964). La idea que hace que esto funcione ya estaba en el
   original y se conserva: **los seis jefes comparten un único motor de
   combate** (core/boss/boss_fsm.c) y se diferencian sólo por estos
   datos. Por eso hay seis enfrentamientos sin seis IAs que mantener.

   Betty es la excepción parcial: además del repertorio común tiene dos
   ataques propios (invocar refuerzos y su explosión de basura), que la
   FSM habilita mirando `is_final` y su id.
   ===================================================================== */
#ifndef PERSEO_CORE_BOSS_CONFIG_H
#define PERSEO_CORE_BOSS_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

typedef enum BossId {
    BOSS_CAPATAZ = 0,   /* La Ciudad Vertedero */
    BOSS_REVISOR,       /* Estación Abandonada */
    BOSS_TOXICO,        /* Zona de Residuos Tóxicos */
    BOSS_MATON,         /* El Guardián — Madriguera Subterránea */
    BOSS_GUARDIA,       /* Cámara de Comercio */
    BOSS_BETTY,         /* El Trono de Betty */
    BOSS_COUNT
} BossId;

typedef struct BossConfig {
    /* Texto del prototipo, tal cual: el nombre aparece sobre la barra de
       vida y el par victoryTitle/victoryDesc es el cartel que sale al
       vencerlo (F7-02 y el estado GS_BANNER). */
    const char *name;
    const char *victory_title;
    const char *victory_desc;
    int16_t hp;
    int16_t w, h;       /* caja de colisión, en píxeles */
    uint8_t sprite;     /* índice de su par de frames en la hoja de jefes */
    bool is_final;      /* Betty: habilita la fase de invocación */
    uint8_t next_level; /* a qué nivel abre la puerta al caer */
} BossConfig;

extern const BossConfig BOSS_CONFIGS[BOSS_COUNT];

#endif /* PERSEO_CORE_BOSS_CONFIG_H */
