/* =====================================================================
   ui_tiles.h — los tiles macizos de la capa de interfaz
   =====================================================================
   Vive aparte porque lo usan dos módulos y no conviene que se copien las
   constantes: pal_gba_text.c los DIBUJA (es su dueño) y pal_gba_video.c
   los TOMA PRESTADOS para el fondo de las viñetas.

   El préstamo tiene motivo: son los únicos tiles de un color plano que
   hay en toda la ROM, y con un tile plano más un banco de paleta se
   dibuja cualquier rectángulo del color que haga falta — la silueta de
   la ciudad, una ventana encendida, las franjas del trono. La
   alternativa era arte nuevo para algo que ya existía.
   ===================================================================== */
#ifndef PERSEO_PLATFORM_GBA_UI_TILES_H
#define PERSEO_PLATFORM_GBA_UI_TILES_H

/* Charblock donde viven la fuente y los tres tiles macizos. */
#define UI_CBB 2

/* Los tres macizos, al principio del charblock. */
#define TILE_BLANK 0   /* todo el índice 0: transparente */
#define TILE_DARK  1   /* todo el índice 1: la tinta del banco */
#define TILE_EDGE  2   /* todo el índice 2: el borde de los paneles */

#endif /* PERSEO_PLATFORM_GBA_UI_TILES_H */
