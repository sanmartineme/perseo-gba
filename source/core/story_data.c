/* ARCHIVO GENERADO por tools/storygen/story_to_c.py - no editar a mano.
   Fuente: docs/prototipo_referencia.html
   (INTRO_STORY, ENDING_STORY, CREDITS y BOSS_DIALOG). */
#include "dialogue.h"

static const StoryPage INTRO[8] = {
    { 0, "Una noche cualquiera, en la superficie, Aurorita camina a casa tarareando una canción. No sabe que unas sombras la siguen de cerca.", SCENE_STREET_CALM },
    { "MATÓN 1", "Miren nada más... una gatita sola, tan tarde y tan lejos de casa.", SCENE_AMBUSH },
    { "MATÓN 2", "Betty paga bien por crías como esta. Agárrenla antes de que grite.", SCENE_AMBUSH },
    { "AURORITA", "¡N-no! ¡Suéltenme! ¡PERSEO! ¡AYÚDAME!", SCENE_SCARED },
    { "MATÓN 1", "Grita todo lo que quieras, princesa. Aquí abajo nadie te va a escuchar.", SCENE_CAGED },
    { "AURORITA", "Ríanse ahora... ¡pero como logre escapar, se las verán conmigo!", SCENE_DEFIANT },
    { 0, "Arrastran la jaula hacia una alcantarilla olvidada. La ciudad de arriba nunca sabrá lo que pasó esta noche.", SCENE_SEWER_DRAG },
    { 0, "Perseo regresa a casa y solo encuentra el silencio. Jura bajar a las cloacas de SILENCIO... y no volver a subir sin ella.", SCENE_VOW },
};

static const StoryPage ENDING[7] = {
    { 0, "Betty cae entre los escombros de su propio trono. Por primera vez en años, Silencio se queda... en silencio.", SCENE_BETTY_FALL },
    { 0, "Al fondo de la cámara, medio escondida entre cadenas oxidadas, una jaula tiembla. Perseo corre hacia ella sin pensarlo dos veces.", SCENE_APPROACH },
    { 0, "Con un golpe seco de su Ganchito mortal, el candado cede. Los barrotes se abren.", SCENE_OPEN },
    { "AURORITA", "¿...Perseo? ¡Sabía que vendrías! ¡Sabía que no ibas a dejarme aquí!", SCENE_REUNION },
    { "AURORITA", "¡Mi papito es el mejor! ¡Lo sabía, lo sabía!", SCENE_REUNION },
    { "PERSEO", "Nunca dejé de buscarte, hermanita. Ni un solo día. Vamos a casa.", SCENE_REUNION },
    { 0, "Juntos, dejan atrás las cloacas de Silencio y sus sombras, de vuelta hacia la luz de la superficie.", SCENE_EPILOGUE },
};

static const CreditPage CREDITS[6] = {
    { "PERSEO: SOMBRAS DE SILENCIO", 0, 0 },
    { "", "Diseño de personajes", "Carlos San Martín" },
    { "", "Diseño de escenarios", "Carlos San Martín" },
    { "", "Diseño de niveles", "Carlos San Martín" },
    { "", "Historia", "Perseo Andrés · Aurora Andrea · Mango Miguel" },
    { "FIN", "¡Feliz cumpleaños amor mío!", 0 },
};

static const StoryPage DLG_CAPATAZ[3] = {
    { "EL CAPATAZ", "¿Un gato curioso en MI vertedero? Betty paga bien por pieles como la tuya...", SCENE_NONE },
    { "PERSEO", "Solo quiero a mi hermana. Dime dónde está Aurorita.", SCENE_NONE },
    { "EL CAPATAZ", "¡Já! Ella ya está camino al Mercado Negro. Y tú no vas a seguirla... ¡vivo!", SCENE_NONE },
};

static const StoryPage DLG_REVISOR[3] = {
    { "EL REVISOR", "Billete, por favor... ¿no? Entonces este tren no tiene paradas para ti, gato.", SCENE_NONE },
    { "PERSEO", "Solo necesito pasar. Betty se llevó a mi hermana por estas vías.", SCENE_NONE },
    { "EL REVISOR", "Todos dicen lo mismo antes de quedarse aquí abajo... ¡para siempre!", SCENE_NONE },
};

static const StoryPage DLG_TOXICO[3] = {
    { "PERSEO", "Esta cosa... ¿esto es lo que queda de un animal? ¿Qué le hicieron?", SCENE_NONE },
    { "EL TÓXICO", "(un gruñido húmedo, sin palabras, solo hambre)", SCENE_NONE },
    { "PERSEO", "Lo siento. Pero no voy a dejar que nadie más termine así.", SCENE_NONE },
};

static const StoryPage DLG_MATON[3] = {
    { "EL GUARDIÁN", "Así que tú eres el gato que ha estado rompiendo cosas de Betty.", SCENE_NONE },
    { "PERSEO", "Y voy a seguir rompiéndolas hasta llegar a Aurorita.", SCENE_NONE },
    { "EL GUARDIÁN", "La Madriguera se traga a gatos más grandes que tú. Vas a ver.", SCENE_NONE },
};

static const StoryPage DLG_GUARDIA[3] = {
    { "EL GUARDIA", "(gruñe) Nadie pasa al Mercado sin el permiso de Betty. Nadie.", SCENE_NONE },
    { "PERSEO", "Vi la jaula vacía. Sé que la tuvieron aquí. Aparta.", SCENE_NONE },
    { "EL GUARDIA", "Llegas tarde, gato. Pero puedes acompañarla... al fondo del río.", SCENE_NONE },
};

static const StoryPage DLG_BETTY[3] = {
    { "BETTY", "Vaya, vaya. El gatito calló a todos mis muchachos. Tienes agallas, lo reconozco.", SCENE_NONE },
    { "PERSEO", "Betty. Se acabó. Devuélveme a Aurorita.", SCENE_NONE },
    { "BETTY", "¿\"Devolverla\"? Querido, ella ya es mercancía. Y tú vas a hacerme compañía en el fondo de Silencio.", SCENE_NONE },
};

/* Indexado por BossId: el orden es el del recorrido del juego. */
static const StoryPage *const BOSS_LINES[BOSS_COUNT] = {
    [BOSS_CAPATAZ] = DLG_CAPATAZ,
    [BOSS_REVISOR] = DLG_REVISOR,
    [BOSS_TOXICO] = DLG_TOXICO,
    [BOSS_MATON] = DLG_MATON,
    [BOSS_GUARDIA] = DLG_GUARDIA,
    [BOSS_BETTY] = DLG_BETTY,
};
static const uint8_t BOSS_LINE_COUNT[BOSS_COUNT] = {
    [BOSS_CAPATAZ] = 3,
    [BOSS_REVISOR] = 3,
    [BOSS_TOXICO] = 3,
    [BOSS_MATON] = 3,
    [BOSS_GUARDIA] = 3,
    [BOSS_BETTY] = 3,
};

const StoryPage *story_intro(uint8_t *count) {
    *count = 8;
    return INTRO;
}

const StoryPage *story_ending(uint8_t *count) {
    *count = 7;
    return ENDING;
}

const CreditPage *story_credits(uint8_t *count) {
    *count = 6;
    return CREDITS;
}

const StoryPage *boss_dialogue(BossId id, uint8_t *count) {
    if (id < 0 || id >= BOSS_COUNT) id = BOSS_CAPATAZ;
    *count = BOSS_LINE_COUNT[id];
    return BOSS_LINES[id];
}
