/* =====================================================================
   boss_fsm.h — motor de combate compartido por los seis jefes
   =====================================================================
   Traducción de updateBoss() del prototipo (línea ~2049). Un solo
   conjunto de estados sirve para los seis encuentros; lo que cambia
   entre uno y otro son los datos de core/boss/boss_config.h y, en el
   caso de Betty, dos ataques extra que la FSM habilita por bandera.

   Ciclo normal: espera → entrada → elige ataque → telegrafía → ataca →
   vuelve a elegir. La telegrafía (`TELE`) es la que hace el combate
   justo: avisa antes de cada ataque, y se acorta cuando al jefe le queda
   poca vida, que es toda la subida de tensión de la segunda fase.
   ===================================================================== */
#ifndef PERSEO_CORE_BOSS_FSM_H
#define PERSEO_CORE_BOSS_FSM_H

#include "../entity.h"
#include "boss_config.h"

struct World;

typedef enum BossState {
    BOSS_WAIT = 0,   /* quieto hasta que el jugador entra en la arena */
    BOSS_INTRO,      /* entrada: rugido, polvo y temblor creciente */
    BOSS_CHOOSE,     /* elige el siguiente ataque */
    BOSS_TELE,       /* telegrafía: el aviso antes de atacar */
    BOSS_CHARGE,     /* embestida horizontal hasta chocar */
    BOSS_LEAP,       /* salto con impacto: al aterrizar suelta dos ondas */
    BOSS_THROW,      /* tanda de proyectiles de chatarra */
    BOSS_TRASHBLAST, /* sólo Betty: abanico de escombros de una vez */
    BOSS_SUMMON,     /* sólo Betty: llama dos ratas */
    BOSS_STUN,       /* aturdido tras estrellarse: la ventana para pegarle */
    BOSS_DIE
} BossState;

void boss_init(Entity *e, struct World *w);
void boss_update(Entity *e, struct World *w);

/* La FSM decide también qué frame mostrar, porque depende del estado
   (un jefe aturdido no se anima igual que uno caminando). */
int boss_sprite_frame(const Entity *e, uint32_t tick);

static inline const BossConfig *boss_config_of(const Entity *e) {
    int id = e->param;
    if (id < 0 || id >= BOSS_COUNT) id = BOSS_CAPATAZ;
    return &BOSS_CONFIGS[id];
}

#endif /* PERSEO_CORE_BOSS_FSM_H */
