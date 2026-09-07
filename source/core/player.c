#include "player.h"

/* Constantes de física — traducción valor-por-valor de las constantes
   locales de updatePlayer() en el prototipo (SPD, ACC, GRV, JV, MAXF) y
   de los literales usados en su cuerpo (0.8, 2.6, 3.8, 1.6, 3.4, 0.7).
   Se mantienen con magnitud positiva, igual que el original, y se
   niegan en el punto de uso (ej. "P.vy=-JV") para que el port se pueda
   comparar línea a línea contra la fuente. */
#define PLAYER_SPEED          FX_C(1.35)  /* SPD */
#define PLAYER_ACCEL          FX_C(0.30)  /* ACC */
#define PLAYER_GRAVITY        FX_C(0.22)  /* GRV */
#define PLAYER_JUMP_V         FX_C(4.2)   /* JV  */
#define PLAYER_MAX_FALL       FX_C(4.0)   /* MAXF */
#define PLAYER_FRICTION       FX_C(0.7)
#define PLAYER_WALLSLIDE_MAXV FX_C(0.8)
#define PLAYER_WALLJUMP_VY    FX_C(4.0)
#define PLAYER_WALLJUMP_VX    FX_C(2.6)
#define PLAYER_DJUMP_VY       FX_C(3.8)
#define PLAYER_VARJUMP_CAP    FX_C(1.6)
#define PLAYER_DASH_VX        FX_C(3.4)

#define PLAYER_JUMP_BUFFER_FRAMES 7
#define PLAYER_COYOTE_FRAMES      7
#define PLAYER_DASH_FRAMES        11
#define PLAYER_DASH_COOLDOWN      32

void player_init(Player *p, fx_t spawn_x, fx_t spawn_y) {
    Player zero = {0};
    *p = zero;
    p->x = spawn_x;
    p->y = spawn_y;
    p->w = 10;
    p->h = 13;
    p->face = 1;
    p->can_double_jump = true;
}

void player_update(Player *p, const Level *lv, const PlayerInput *in) {
    /* --- Jump buffer: ventana de 7 frames para que un salto pulsado un
       poco antes de tocar el suelo/muro igual cuente. --- */
    if (in->jump_pressed) p->jump_buffer = PLAYER_JUMP_BUFFER_FRAMES;
    else if (p->jump_buffer > 0) p->jump_buffer--;

    /* El cooldown del dash se descuenta ANTES de evaluar si se puede
       arrancar uno nuevo este mismo frame — mismo orden que el
       prototipo (dashCd-- ocurre antes del "if(dPress&&...)"). */
    if (p->dash_cd > 0) p->dash_cd--;

    /* --- Dash Sombrío --- */
    if (in->dash_pressed && p->ab.dash && p->dash_cd == 0 && p->dash_t == 0) {
        p->dash_t = PLAYER_DASH_FRAMES;
        p->dash_cd = PLAYER_DASH_COOLDOWN;
        p->vy = 0;
        /* SFX.dash() y el rastro de partículas llegan en las Fases 6/4
           (audio y sistema de partículas todavía no existen). */
    }

    if (p->dash_t > 0) {
        p->dash_t--;
        p->vx = fx_mul(fx_from_int(p->face), PLAYER_DASH_VX);
        p->vy = 0;
    } else {
        /* Movimiento horizontal */
        if (in->left) {
            p->vx = fx_max(fx_sub(p->vx, PLAYER_ACCEL), fx_neg(PLAYER_SPEED));
            p->face = -1;
        } else if (in->right) {
            p->vx = fx_min(fx_add(p->vx, PLAYER_ACCEL), PLAYER_SPEED);
            p->face = 1;
        } else {
            p->vx = fx_mul(p->vx, PLAYER_FRICTION);
        }
        /* Gravedad */
        p->vy = fx_min(fx_add(p->vy, PLAYER_GRAVITY), PLAYER_MAX_FALL);
    }

    /* --- Muro (Garra Felina) --- */
    p->wall = 0;
    if (p->ab.climb && !p->on_ground && p->dash_t == 0) {
        int8_t dir = in->left ? -1 : (in->right ? 1 : 0);
        if (dir != 0) {
            fx_t probe_x = fx_add(p->x, fx_from_int((int32_t)dir * 2));
            fx_t probe_y = fx_add(p->y, fx_from_int(2));
            if (level_rect_solid(lv, probe_x, probe_y, p->w, p->h - 4)) {
                p->wall = dir;
                if (p->vy > PLAYER_WALLSLIDE_MAXV) p->vy = PLAYER_WALLSLIDE_MAXV;
                p->can_double_jump = true;
            }
        }
    }

    /* --- Saltos: suelo/coyote, muro o Salto Doble, por prioridad --- */
    if (p->jump_buffer > 0) {
        if (p->on_ground || p->coyote > 0) {
            p->vy = fx_neg(PLAYER_JUMP_V);
            p->on_ground = false;
            p->coyote = 0;
            p->jump_buffer = 0;
            p->spun = false;
        } else if (p->wall != 0) {
            p->vy = fx_neg(PLAYER_WALLJUMP_VY);
            p->vx = fx_neg(fx_mul(fx_from_int((int32_t)p->wall), PLAYER_WALLJUMP_VX));
            p->face = (int8_t)-p->wall;
            p->jump_buffer = 0;
            p->spun = false;
        } else if (p->ab.double_jump && p->can_double_jump) {
            p->vy = fx_neg(PLAYER_DJUMP_VY);
            p->can_double_jump = false;
            p->jump_buffer = 0;
            p->spun = true;
        }
    }
    /* Salto de altura variable: soltar el botón corta el impulso hacia
       arriba (no aplica en pleno dash). */
    if (!in->jump_held && p->vy < fx_neg(PLAYER_VARJUMP_CAP) && p->dash_t == 0) {
        p->vy = fx_neg(PLAYER_VARJUMP_CAP);
    }

    /* --- Colisión horizontal ---
       A diferencia del prototipo, acá NO se rompen rejillas oxidadas
       (TILE_RUST) al dashear todavía: esa interacción con el nivel
       llega junto al sistema de niveles reales y de partículas (Fase
       3/4) — por ahora todo tile sólido simplemente detiene al
       jugador, rejilla incluida. */
    p->x = fx_add(p->x, p->vx);
    {
        int16_t tx0 = fx_to_tile(p->x);
        int16_t tx1 = fx_to_tile(fx_add(p->x, fx_from_int(p->w - 1)));
        int16_t ty0 = fx_to_tile(p->y);
        int16_t ty1 = fx_to_tile(fx_add(p->y, fx_from_int(p->h - 1)));
        for (int16_t ty = ty0; ty <= ty1; ty++) {
            for (int16_t tx = tx0; tx <= tx1; tx++) {
                if (tile_is_solid(level_tile_at(lv, tx, ty))) {
                    if (p->vx > 0) p->x = fx_sub(fx_from_int((int32_t)tx * TILE_SIZE), fx_from_int(p->w));
                    else if (p->vx < 0) p->x = fx_from_int((int32_t)tx * TILE_SIZE + TILE_SIZE);
                    p->vx = 0;
                }
            }
        }
    }

    /* --- Colisión vertical --- */
    fx_t prev_feet = fx_add(p->y, fx_from_int(p->h));
    p->y = fx_add(p->y, p->vy);
    p->on_ground = false;
    {
        int16_t tx0 = fx_to_tile(p->x);
        int16_t tx1 = fx_to_tile(fx_add(p->x, fx_from_int(p->w - 1)));
        if (p->vy >= 0) {
            /* Se usa (y+h)/8 y NO (y+h-1)/8 — ver la nota del prototipo:
               con "-1" el aterrizaje se detecta un píxel tarde y
               produce parpadeo entre los estados de caminar/caer. */
            int16_t ty = fx_to_tile(fx_add(p->y, fx_from_int(p->h)));
            for (int16_t tx = tx0; tx <= tx1; tx++) {
                uint8_t id = level_tile_at(lv, tx, ty);
                bool plat = id == TILE_PLATFORM
                    && fx_to_int(prev_feet) <= (int32_t)ty * TILE_SIZE + 1
                    && !in->down;
                if (tile_is_solid(id) || plat) {
                    p->y = fx_sub(fx_from_int((int32_t)ty * TILE_SIZE), fx_from_int(p->h));
                    p->vy = 0;
                    p->on_ground = true;
                    p->can_double_jump = true;
                    p->coyote = PLAYER_COYOTE_FRAMES;
                    p->spun = false;
                }
            }
        } else {
            int16_t ty = fx_to_tile(p->y);
            for (int16_t tx = tx0; tx <= tx1; tx++) {
                if (tile_is_solid(level_tile_at(lv, tx, ty))) {
                    p->y = fx_from_int((int32_t)ty * TILE_SIZE + TILE_SIZE);
                    p->vy = 0;
                }
            }
        }
    }
    if (!p->on_ground && p->coyote > 0) p->coyote--;

    /* --- Animación de caminata (bandera; los frames de arte real
       llegan en la Fase 3) --- */
    p->walking = p->on_ground && p->dash_t == 0 && (in->left || in->right);
    if (p->walking) p->walk_anim++; else p->walk_anim = 0;

    /* Deliberadamente fuera de la Fase 1 (se agregan junto con el
       sistema correspondiente — ver docs/TAREAS_MIGRACION_GBA.md):
         - Tiles dañinos / caída al vacío -> Fase 4 (F4-13, sistema de daño).
         - Ganchito Mortal / Lanzamiento de Traza -> Fase 4 (F4-03/F4-04).
         - Reliquias (Pata de la Suerte, etc.) -> Fase 7. */
}

/* Traducción directa de drawPlayer() del prototipo (línea ~2318): misma
   prioridad de estados y mismos tiempos. Las poses de ataque ya están en la
   hoja de sprites y contempladas en el enum, pero todavía no se pueden
   seleccionar porque el jugador aún no tiene contadores de combate — eso
   llega con el Ganchito Mortal en la Fase 4 (F4-03). */
PlayerAnim player_get_anim(const Player *p, uint32_t tick) {
    PlayerAnim a = { PFRAME_IDLE1, 0, p->face < 0, false };

    if (p->dash_t > 0 || p->spun) {
        a.frame = PFRAME_DASH;
    } else if (!p->on_ground) {
        a.frame = (p->vy < 0) ? PFRAME_JUMP : PFRAME_FALL;
    } else if (p->walking) {
        /* Ciclo de 4 fases, una cada 4 frames, con "contoneo" de 1 px en las
           fases de contacto (1 y 3) — el vaivén de peso que pide
           docs/Estilo_Grafico_Perseo.md. */
        uint32_t wf = (p->walk_anim >> 2) & 3;
        a.frame = (uint8_t)(PFRAME_WALK1 + wf);
        a.bob = (wf == 1 || wf == 3) ? 1 : 0;
    } else {
        a.frame = ((tick >> 5) & 1) ? PFRAME_IDLE1 : PFRAME_IDLE2;
    }
    return a;
}
