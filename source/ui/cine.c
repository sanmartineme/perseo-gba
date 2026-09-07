/* =====================================================================
   cine.c — las ilustraciones de las viñetas
   =====================================================================
   Traducción de drawIntroCine()/drawEndCine() del prototipo. Allá cada
   escena es una tanda de drawImage() escalados sobre un canvas; acá son
   sprites de hardware puestos en coordenadas de pantalla, y el escalado
   ya viene hecho en el arte (ver "scale" en tools/spritegen).

   Dos cosas que cambian respecto del original, y por qué:

   - **No hay transparencias.** El prototipo baja el alpha del personaje
     que no habla, para que se lea quién tiene la palabra. La GBA sólo
     puede mezclar capas enteras, no objetos sueltos, así que en su lugar
     el que habla se dibuja MÁS ADELANTE y el otro un poco atrás y más
     abajo. Se lee igual de bien y no cuesta nada.

   - **El orden de OAM es la profundidad.** El objeto de índice más bajo
     se dibuja encima, así que lo que va delante se pide primero. De ahí
     que la jaula se dibuje ANTES que Aurorita: así ella queda detrás de
     los barrotes, que es donde tiene que estar.
   ===================================================================== */
#include "cine.h"
#include "text.h"
#include "../core/dialogue.h"
#include "../core/particles.h"
#include "../platform/pal.h"

/* Alto de los personajes, que es lo que hay que restar para apoyarlos. */
#define ACTOR_H 32
#define ACTOR_W 32
#define FEET(y) ((y) - ACTOR_H)

/* Centro de la pantalla menos medio personaje. */
#define MID_X ((240 - ACTOR_W) / 2)

static bool is_intro_scene(uint8_t scene) {
    return scene >= SCENE_STREET_CALM && scene <= SCENE_VOW;
}

/* ---------------------------------------------------------------------
   Cielo nocturno
   ---------------------------------------------------------------------
   Las mismas veinte estrellas del prototipo, con su misma fórmula de
   posición y de parpadeo: son números arbitrarios pero reproducibles, y
   copiarlos tal cual es lo que hace que el cielo se vea igual. La luna
   va donde iba allá, restándole medio sprite porque acá las coordenadas
   son de la esquina y no del centro.
   --------------------------------------------------------------------- */
static void draw_night_sky(uint32_t frame) {
    pal_video_cine_actor(CINE_MOON, 240 - 30 - 8, 24 - 8, false);
    for (int i = 0; i < 20; i++) {
        if ((frame + (uint32_t)i * 9) % 50 >= 32) continue;   /* titila */
        pal_video_cine_dot((i * 53 + 41) % 240, (i * 31) % 54 + 6, PCOL_GREY);
    }
}

/* Vaivén de andar: el mismo sin(t/8)*2 del prototipo, resuelto con una
   tabla de cuatro pasos para no meter trigonometría por dos píxeles. */
static int bob(uint32_t t) {
    static const int8_t B[4] = { 0, 1, 0, -1 };
    return B[(t >> 3) & 3];
}

/* Avance de 0 a 256 en `frames`, para los desplazamientos largos. */
static int ramp(uint32_t t, uint32_t frames) {
    return t >= frames ? 256 : (int)((t * 256u) / frames);
}

/* ---------------------------------------------------------------------
   Las escenas
   --------------------------------------------------------------------- */
static void scene_street_calm(const Game *g) {
    /* Aurorita cruza la calle tarareando; al fondo, una sombra que la
       sigue. En el prototipo la sombra va al 30% de opacidad: acá se la
       deja quieta y a un lado, que es como se lee que está acechando. */
    pal_video_cine_actor(CINE_THUG1, 196, FEET(CINE_GROUND_Y) + 2, true);
    int x = 8 + (ramp(g->story_t, 150) * (150 - 8)) / 256;
    pal_video_cine_actor(CINE_AURORA, x, FEET(CINE_GROUND_Y) + bob(g->story_t), false);
}

static void scene_ambush(const Game *g, bool scared) {
    /* Dos matones le cierran el paso, uno por lado. En 'scared' uno la
       tiene agarrada: se acercan y ella tiembla. */
    bool speaking = g->story_scenes[g->story_idx].who &&
                    g->story_scenes[g->story_idx].who[0] == 'A';   /* AURORITA */
    int gap = scared ? 22 : 30;
    int jitter = scared ? (int)(g->story_t & 1) : 0;

    /* El que habla, primero: en OAM el primero va delante. */
    if (speaking) {
        pal_video_cine_actor(CINE_AURORA, MID_X + jitter, FEET(CINE_GROUND_Y), false);
        pal_video_cine_actor(CINE_THUG1, MID_X - gap, FEET(CINE_GROUND_Y) + 3, true);
        pal_video_cine_actor(CINE_THUG2, MID_X + gap, FEET(CINE_GROUND_Y) + 3, false);
    } else {
        pal_video_cine_actor(CINE_THUG1, MID_X - gap, FEET(CINE_GROUND_Y), true);
        pal_video_cine_actor(CINE_THUG2, MID_X + gap, FEET(CINE_GROUND_Y), false);
        pal_video_cine_actor(CINE_AURORA, MID_X + jitter, FEET(CINE_GROUND_Y) + 3, false);
    }
}

static void scene_caged(const Game *g, bool defiant) {
    /* Aurorita tras los barrotes; el matón se queda mirando con burla.
       La jaula va PRIMERO para que quede por delante de ella. */
    const int cage_x = 88;
    pal_video_cine_actor(CINE_CAGE, cage_x, FEET(CINE_GROUND_Y), false);
    pal_video_cine_actor(CINE_AURORA, cage_x, FEET(CINE_GROUND_Y), false);
    pal_video_cine_actor(CINE_THUG1, cage_x + 38, FEET(CINE_GROUND_Y) + 2, true);
    /* Su chispa de rabia, parpadeando encima de la jaula. */
    if (defiant && ((g->frame >> 3) & 1) == 0) {
        pal_video_cine_dot(cage_x + 13, FEET(CINE_GROUND_Y) - 8, PCOL_GOLD);
    }
}

static void scene_sewer_drag(const Game *g) {
    /* Plano abierto: arrastran la jaula hacia una alcantarilla que humea.
       El vapor son puntos grises que suben, que es lo que en el prototipo
       era un rectángulo semitransparente palpitando. */
    const int hole_x = 180;
    for (int i = 0; i < 4; i++) {
        uint32_t t = (g->story_t + (uint32_t)i * 13) % 40;
        pal_video_cine_dot(hole_x + 4 + i * 5, CINE_GROUND_Y - 4 - (int)(t >> 1), PCOL_GREY);
    }
    int x = 20 + (ramp(g->story_t, 160) * (hole_x - 56 - 20)) / 256;
    pal_video_cine_actor(CINE_CAGE, x, FEET(CINE_GROUND_Y), false);
    pal_video_cine_actor(CINE_AURORA, x, FEET(CINE_GROUND_Y), false);
    pal_video_cine_actor(CINE_THUG1, x - 26, FEET(CINE_GROUND_Y) + 2, false);
}

static void scene_vow(const Game *g) {
    /* Perseo solo, de noche, donde antes estaba su hermana. */
    pal_video_cine_actor(CINE_PERSEO, MID_X, FEET(CINE_GROUND_Y) + bob(g->story_t), false);
}

static void scene_betty_fall(const Game *g) {
    /* Betty entre los escombros de su propio trono. */
    pal_video_cine_actor(CINE_BETTY, MID_X, FEET(CINE_GROUND_Y) + 6, false);
    for (int i = 0; i < 6; i++) {
        uint32_t t = (g->story_t + (uint32_t)i * 11) % 60;
        if (t >= 30) continue;
        pal_video_cine_dot(MID_X - 20 + i * 14, CINE_GROUND_Y - 6 - (int)(t >> 2), PCOL_GREY);
    }
}

static void scene_approach(const Game *g) {
    /* La jaula tiembla al fondo; Perseo corre hacia ella. */
    int shake = (int)(g->story_t & 1);
    pal_video_cine_actor(CINE_CAGE, 168 + shake, FEET(CINE_GROUND_Y), false);
    pal_video_cine_actor(CINE_AURORA, 168 + shake, FEET(CINE_GROUND_Y), false);
    int x = 8 + (ramp(g->story_t, 140) * (120 - 8)) / 256;
    pal_video_cine_actor(CINE_PERSEO, x, FEET(CINE_GROUND_Y) + bob(g->story_t), false);
}

static void scene_open(const Game *g) {
    /* El candado cede: el golpe del Ganchito, en blanco. */
    const int cage_x = 140;
    pal_video_cine_actor(CINE_CAGE, cage_x, FEET(CINE_GROUND_Y), false);
    pal_video_cine_actor(CINE_AURORA, cage_x, FEET(CINE_GROUND_Y), false);
    pal_video_cine_actor(CINE_PERSEO, cage_x - 36, FEET(CINE_GROUND_Y), false);
    if (g->story_t < 24) {
        for (int i = 0; i < 5; i++) {
            pal_video_cine_dot(cage_x - 6 + (i * 5), FEET(CINE_GROUND_Y) + 8 + (i & 1) * 6,
                               PCOL_WHITE);
        }
    }
}

static void scene_reunion(const Game *g) {
    /* Los dos, ya sin barrotes. Delante, el que habla. */
    bool aurora_talks = g->story_scenes[g->story_idx].who &&
                        g->story_scenes[g->story_idx].who[0] == 'A';
    int px = MID_X - 20, ax = MID_X + 20;
    if (aurora_talks) {
        pal_video_cine_actor(CINE_AURORA, ax, FEET(CINE_GROUND_Y) + bob(g->story_t), false);
        pal_video_cine_actor(CINE_PERSEO, px, FEET(CINE_GROUND_Y) + 3, false);
    } else {
        pal_video_cine_actor(CINE_PERSEO, px, FEET(CINE_GROUND_Y) + bob(g->story_t), false);
        pal_video_cine_actor(CINE_AURORA, ax, FEET(CINE_GROUND_Y) + 3, false);
    }
}

static void scene_epilogue(const Game *g) {
    /* Se van juntos hacia la luz de la superficie. */
    int x = 40 + (ramp(g->story_t, 200) * 110) / 256;
    pal_video_cine_actor(CINE_PERSEO, x, FEET(CINE_GROUND_Y) + bob(g->story_t), false);
    pal_video_cine_actor(CINE_AURORA, x + 34, FEET(CINE_GROUND_Y) + bob(g->story_t + 4), false);
}

/* ------------------------------------------------------------------ */

bool ui_cine_has_scene(const Game *g) {
    if (!g->story_scenes || g->story_idx >= g->story_count) return false;
    return g->story_scenes[g->story_idx].scene != SCENE_NONE;
}

CineBackdrop ui_cine_backdrop(const Game *g) {
    if (!ui_cine_has_scene(g)) return CINE_BG_NONE;
    return is_intro_scene(g->story_scenes[g->story_idx].scene)
         ? CINE_BG_NIGHT : CINE_BG_THRONE;
}

void ui_cine_draw(const Game *g) {
    pal_video_cine_begin();
    if (!ui_cine_has_scene(g)) { pal_video_cine_end(); return; }

    uint8_t scene = g->story_scenes[g->story_idx].scene;

    /* Los personajes van antes que el cielo: en OAM el primero manda, y
       nadie quiere una estrella por delante de Aurorita. */
    switch (scene) {
        case SCENE_STREET_CALM: scene_street_calm(g); break;
        case SCENE_AMBUSH:      scene_ambush(g, false); break;
        case SCENE_SCARED:      scene_ambush(g, true);  break;
        case SCENE_CAGED:       scene_caged(g, false);  break;
        case SCENE_DEFIANT:     scene_caged(g, true);   break;
        case SCENE_SEWER_DRAG:  scene_sewer_drag(g); break;
        case SCENE_VOW:         scene_vow(g); break;
        case SCENE_BETTY_FALL:  scene_betty_fall(g); break;
        case SCENE_APPROACH:    scene_approach(g); break;
        case SCENE_OPEN:        scene_open(g); break;
        case SCENE_REUNION:     scene_reunion(g); break;
        case SCENE_EPILOGUE:    scene_epilogue(g); break;
        default: break;
    }

    /* El cielo estrellado es de la superficie: el desenlace ocurre bajo
       tierra, en el trono de Betty, y ahí no hay luna que valga. */
    if (is_intro_scene(scene)) draw_night_sky(g->frame);

    pal_video_cine_end();
}
