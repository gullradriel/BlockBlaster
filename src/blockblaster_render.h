/* Copyright (C) 2026 Nilorea Studio — GPL-3.0-or-later (see COPYING). */

/**
 * \file blockblaster_render.h
 * \brief All rendering / drawing functions.
 */

#ifndef __BLOCKBLASTER_RENDER__
#define __BLOCKBLASTER_RENDER__

#ifdef __cplusplus
extern "C" {
#endif

#include "blockblaster_context.h"

/** \brief Draw a filled rounded rectangle with a scaled border. */
void blockblaster_draw_round_tile(float x1, float y1, float x2, float y2,
                                  float r, ALLEGRO_COLOR fill,
                                  ALLEGRO_COLOR stroke, float width);

/** \brief Draw a miniature shape preview at the given position. */
void blockblaster_draw_shape_preview(const Shape *s, float px, float py,
                                     float cell, ALLEGRO_COLOR col);

/** \brief Draw the play grid (cells, ghost preview, predicted-clear overlay).
 */
void blockblaster_draw_grid(const GameContext *gm);

/** \brief Draw the piece tray below the grid. */
void blockblaster_draw_tray(const GameContext *gm);

/** \brief Draw the floating (dragged or returning) piece. */
void blockblaster_draw_floating_piece(const GameContext *gm);

/** \brief Draw the in-game HUD (score, combo). */
void blockblaster_draw_ui(const GameContext *gm);

/** \brief Draw the complete in-game scene for one frame. */
void blockblaster_draw_play_scene(GameContext *gm);

#ifdef __cplusplus
}
#endif

#endif /* __BLOCKBLASTER_RENDER__ */
