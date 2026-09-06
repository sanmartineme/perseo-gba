/* =====================================================================
   camera.h — cámara de seguimiento
   =====================================================================
   Traducción de cam{}/updateCam() del prototipo (docs/prototipo_referencia.html,
   línea ~2918): sigue al jugador centrándolo en pantalla, clampeada a
   los límites del nivel para no mostrar nunca fuera del mundo.
   ===================================================================== */
#ifndef PERSEO_CORE_CAMERA_H
#define PERSEO_CORE_CAMERA_H

#include "fixed.h"
#include "level/level.h"

/* Resolución objetivo del juego (perfil GBA — ver
   docs/PLAN_MIGRACION_GBA_C.md, sección 2). Vive en core/ porque es una
   decisión de diseño del juego, no un detalle de hardware: el prototipo
   ya fue diseñado para esta resolución exacta. */
#define SCREEN_W 240
#define SCREEN_H 160

typedef struct Camera {
    fx_t x, y; /* esquina superior izquierda, en píxeles Q8.8 */
} Camera;

void camera_init(Camera *cam);

/* target_x/target_y: posición del jugador (esquina superior izquierda
   de su caja de colisión), igual que P.x/P.y en updateCam(). */
void camera_update(Camera *cam, fx_t target_x, fx_t target_y, const Level *lv);

#endif /* PERSEO_CORE_CAMERA_H */
