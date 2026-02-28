/* Copyright (C) 2026 Nilorea Studio — GPL-3.0-or-later (see COPYING). */

/**
 * \file blockblaster_render.c
 * \brief All rendering / drawing functions implementation.
 */

#include "blockblaster_render.h"

#include "blockblaster_game.h"
#include "blockblaster_ui.h"

#include <allegro5/allegro_primitives.h>
#include <math.h>

#if defined(__ANDROID__)
#include <allegro5/allegro_android.h>
#endif

/**
 * \brief Draw a filled rounded rectangle with a stroked outline.
 *
 * Used as the visual primitive for grid cells, tray slots, and buttons.
 *
 * \param x1      Left edge.
 * \param y1      Top edge.
 * \param x2      Right edge.
 * \param y2      Bottom edge.
 * \param r       Corner radius.
 * \param fill    Fill colour.
 * \param stroke  Outline colour.
 * \param width   Outline width in pixels.
 */
void blockblaster_draw_round_tile(float x1, float y1, float x2, float y2,
                                  float r, ALLEGRO_COLOR fill,
                                  ALLEGRO_COLOR stroke, float width)
{
    al_draw_filled_rounded_rectangle(x1, y1, x2, y2, r, r, fill);
    al_draw_rounded_rectangle(x1, y1, x2, y2, r, r, stroke, width);
}

/**
 * \brief Draw a small shape preview (used in the tray display).
 *
 * Each filled cell of the shape is rendered as a rounded tile at the given
 * cell size, inset slightly from its neighbours.
 *
 * \param s     Shape to draw.
 * \param px    X origin of the preview (top-left corner).
 * \param py    Y origin of the preview (top-left corner).
 * \param cell  Size of each cell in pixels.
 * \param col   Fill colour for the cells.
 */
void blockblaster_draw_shape_preview(const Shape *s, float px, float py,
                                     float cell, ALLEGRO_COLOR col)
{
    float r = cell * 0.20f;
    float gap = cell * 0.055f;
    for (int y = 0; y < s->h; y++) {
        for (int x = 0; x < s->w; x++) {
            if (!blockblaster_shape_cell(s, x, y))
                continue;
            float x1 = px + x * cell;
            float y1 = py + y * cell;
            float x2 = x1 + cell;
            float y2 = y1 + cell;
            blockblaster_draw_round_tile(x1 + gap, y1 + gap, x2 - gap, y2 - gap,
                                         r, col, al_map_rgb(30, 30, 35),
                                         ROUNDED_LINE_WIDTH);
        }
    }
}

/**
 * \brief Draw the game grid: background panel, cells, predicted-clear
 *        highlights, and the ghost drop preview.
 *
 * Occupied cells render with their assigned theme colour, applying a flash
 * tint during the clear animation and a pop scale on recent placements.
 * The ghost preview overlay shows where the dragged piece would land,
 * tinted green or red depending on placement validity.
 *
 * \param gm  Game context.
 */
void blockblaster_draw_grid(const GameContext *gm)
{
    float margin = 10.0f * UI_SCALE;

    /* Background panel */
    blockblaster_draw_round_tile(
        GRID_X - margin, GRID_Y - margin, GRID_X + GRID_W * CELL + margin,
        GRID_Y + GRID_H * CELL + margin, 10.0f * UI_SCALE,
        al_map_rgb(20, 20, 26), GRID_LINE_COLOR, GRID_LINE_WIDTH);

    /* Predicted-clear highlight */
    if (gm->dragging && gm->can_drop_preview && gm->has_predicted_clear) {
        Theme th = gm->tray[gm->dragging_index].theme;
        ALLEGRO_COLOR rowc =
            al_map_rgba_f(th.fill.r, th.fill.g, th.fill.b, 0.10f);
        ALLEGRO_COLOR colc =
            al_map_rgba_f(th.fill.r, th.fill.g, th.fill.b, 0.10f);

        for (int y = 0; y < GRID_H; y++) {
            if (!gm->pred_full_row[y])
                continue;
            float y1 = GRID_Y + y * CELL;
            float y2 = y1 + CELL;
            al_draw_filled_rectangle(GRID_X, y1, GRID_X + GRID_W * CELL, y2,
                                     rowc);
        }
        for (int x = 0; x < GRID_W; x++) {
            if (!gm->pred_full_col[x])
                continue;
            float x1 = GRID_X + x * CELL;
            float x2 = x1 + CELL;
            al_draw_filled_rectangle(x1, GRID_Y, x2, GRID_Y + GRID_H * CELL,
                                     colc);
        }
    }

    /* Grid cells */
    for (int y = 0; y < GRID_H; y++) {
        for (int x = 0; x < GRID_W; x++) {
            float x1 = GRID_X + x * CELL;
            float y1 = GRID_Y + y * CELL;
            float x2 = x1 + CELL;
            float y2 = y1 + CELL;

            al_draw_rectangle(x1, y1, x2, y2, GRID_LINE_COLOR, GRID_LINE_WIDTH);

            bool occ = gm->grid.occ[y][x];

            float flash = 0.0f;
            if (gm->clearing && gm->pending_clear[y][x])
                flash = blockblaster_clampf(gm->clear_t / CLEAR_FLASH_TIME,
                                            0.0f, 1.0f);

            float pop = blockblaster_clampf(gm->pop_t[y][x] / PLACE_POP_TIME,
                                            0.0f, 1.0f);

            if (occ || (gm->clearing && gm->pending_clear[y][x])) {
                Theme th;
                if (gm->grid.has_theme[y][x]) {
                    th = gm->grid.cell_theme[y][x];
                } else {
                    th.fill = al_map_rgb(120, 190, 255);
                    th.stroke = GRID_LINE_COLOR;
                }

                ALLEGRO_COLOR base = th.fill;
                ALLEGRO_COLOR stroke = th.stroke;

                if (flash > 0.0f)
                    base = al_map_rgba_f(1.0f, 0.85f, 0.45f, 1.0f);

                float scale = 1.0f + 0.12f * pop;
                float cx = (x1 + x2) * 0.5f;
                float cy = (y1 + y2) * 0.5f;
                float hw = (CELL * 0.42f) * scale;
                float hh = (CELL * 0.42f) * scale;

                float tx1 = cx - hw;
                float ty1 = cy - hh;
                float tx2 = cx + hw;
                float ty2 = cy + hh;

                blockblaster_draw_round_tile(tx1, ty1, tx2, ty2, CELL * 0.135f,
                                             base, stroke, ROUNDED_LINE_WIDTH);
            }
        }
    }

    /* Ghost preview */
    if (gm->dragging) {
        const Piece *p = &gm->tray[gm->dragging_index];
        if (!p->used) {
            Theme th = gm->tray[gm->dragging_index].theme;
            ALLEGRO_COLOR base = th.fill;
            ALLEGRO_COLOR c = gm->can_drop_preview
                                  ? al_map_rgba_f(base.r, base.g, base.b, 0.40f)
                                  : al_map_rgba(255, 90, 90, 120);
            float ghost_inset = 6.0f * UI_SCALE;
            for (int sy = 0; sy < p->shape.h; sy++) {
                for (int sx = 0; sx < p->shape.w; sx++) {
                    if (!blockblaster_shape_cell(&p->shape, sx, sy))
                        continue;
                    int gx = gm->preview_cell_x + sx;
                    int gy = gm->preview_cell_y + sy;
                    if (gx < 0 || gy < 0 || gx >= GRID_W || gy >= GRID_H)
                        continue;

                    float x1 = GRID_X + gx * CELL;
                    float y1 = GRID_Y + gy * CELL;
                    float x2 = x1 + CELL;
                    float y2 = y1 + CELL;
                    blockblaster_draw_round_tile(
                        x1 + ghost_inset, y1 + ghost_inset, x2 - ghost_inset,
                        y2 - ghost_inset, CELL * 0.135f, c,
                        al_map_rgba(0, 0, 0, 0), ROUNDED_LINE_WIDTH);
                }
            }
        }
    }
}

/**
 * \brief Draw the piece tray below the grid.
 *
 * Each tray slot is rendered as a rounded rectangle.  Used pieces show
 * "(placed)", the currently dragged piece shows "(placing)", and unused
 * pieces display a shape preview.  A label is drawn above the tray.
 *
 * \param gm  Game context.
 */
void blockblaster_draw_tray(const GameContext *gm)
{
    ALLEGRO_FONT *font = gm->font;
    for (int i = 0; i < PIECES_PER_SET; i++) {
        float x1, y1, x2, y2;
        blockblaster_tray_piece_rect(i, &x1, &y1, &x2, &y2);

        blockblaster_draw_round_tile(x1, y1, x2, y2, 12.0f * UI_SCALE,
                                     al_map_rgb(22, 22, 28), GRID_LINE_COLOR,
                                     GRID_LINE_WIDTH);

        if (gm->returning && gm->return_index == i) {
            al_draw_textf(font, al_map_rgb(120, 120, 130), x1 + (x2 - x1) / 2,
                          y1 + (y2 - y1) / 2, ALLEGRO_ALIGN_CENTER,
                          "(returning)");
            continue;
        }

        if (gm->tray[i].used) {
            al_draw_textf(font, al_map_rgb(120, 120, 130), x1 + (x2 - x1) / 2,
                          y1 + (y2 - y1) / 2, ALLEGRO_ALIGN_CENTER, "(placed)");
            continue;
        }
        if (gm->dragging && gm->dragging_index == i) {
            al_draw_textf(font, al_map_rgb(120, 120, 130), x1 + (x2 - x1) / 2,
                          y1 + (y2 - y1) / 2, ALLEGRO_ALIGN_CENTER,
                          "(placing)");
            continue;
        }

        const Shape *s = &gm->tray[i].shape;
        float pc = TRAY_BOX / 9.0f;
        float pw = s->w * pc;
        float ph = s->h * pc;
        float px = x1 + ((x2 - x1) - pw) * 0.5f;
        float py = y1 + ((y2 - y1) - ph) * 0.5f;

        blockblaster_draw_shape_preview(s, px, py, pc, gm->tray[i].theme.fill);
    }

    al_draw_textf(font, al_map_rgb(220, 220, 235), GRID_X, TRAY_Y - 34, 0,
                  "Pieces (drag onto grid):");
}

/**
 * \brief Draw the piece currently being dragged (or returning to the tray).
 *
 * The piece follows the mouse cursor with a shadow offset.  During the
 * return animation it smoothly interpolates from the release position
 * back to the tray slot centre while shrinking from grid cell size to
 * tray preview size.
 *
 * \param gm  Game context.
 */
void blockblaster_draw_floating_piece(const GameContext *gm)
{
    if (!gm->dragging && !gm->returning)
        return;

    int idx = gm->dragging ? gm->dragging_index : gm->return_index;
    const Piece *p = &gm->tray[idx];
    if (p->used)
        return;

    float mx = gm->mouse_x;
    float my = gm->mouse_y;
    float pc = (float) CELL;

#ifdef ALLEGRO_ANDROID
    my -= ANDROID_PIECE_Y_OFFSET * blockblaster_android_display_density();
#endif

    if (gm->returning) {
        float t =
            1.0f - blockblaster_clampf(gm->return_t / RETURN_TIME, 0.0f, 1.0f);
        t = blockblaster_smoothstep(t);
        mx = blockblaster_lerpf(gm->return_start_x, gm->return_end_x, t);
        my = blockblaster_lerpf(gm->return_start_y, gm->return_end_y, t);
        pc = blockblaster_lerpf((float) CELL, TRAY_BOX / 9.0f, t);
    }

    float r = pc * 0.22f;
    float shadow_dx = 4.0f * UI_SCALE;
    float shadow_dy = 6.0f * UI_SCALE;

    float px = mx - (gm->grab_sx + 0.5f) * pc;
    float py = my - (gm->grab_sy + 0.5f) * pc;

    /* Shadow */
    for (int sy = 0; sy < p->shape.h; sy++) {
        for (int sx = 0; sx < p->shape.w; sx++) {
            if (!blockblaster_shape_cell(&p->shape, sx, sy))
                continue;
            float x1 = px + sx * pc;
            float y1 = py + sy * pc;
            float x2 = x1 + pc;
            float y2 = y1 + pc;
            al_draw_filled_rounded_rectangle(x1 + shadow_dx, y1 + shadow_dy,
                                             x2 + shadow_dx, y2 + shadow_dy, r,
                                             r, al_map_rgba(0, 0, 0, 90));
        }
    }

    /* Tiles */
    ALLEGRO_COLOR fill = p->theme.fill;
    ALLEGRO_COLOR stroke = p->theme.stroke;
    float alpha = gm->returning ? 0.65f : 0.85f;
    ALLEGRO_COLOR fill_a = al_map_rgba_f(fill.r, fill.g, fill.b, alpha);

    for (int sy = 0; sy < p->shape.h; sy++) {
        for (int sx = 0; sx < p->shape.w; sx++) {
            if (!blockblaster_shape_cell(&p->shape, sx, sy))
                continue;
            float x1 = px + sx * pc;
            float y1 = py + sy * pc;
            float x2 = x1 + pc;
            float y2 = y1 + pc;
            blockblaster_draw_round_tile(x1, y1, x2, y2, r, fill_a, stroke,
                                         ROUNDED_LINE_WIDTH);
        }
    }
}

/**
 * \brief Draw the in-game HUD: score and combo indicator.
 *
 * \param gm  Game context.
 */
void blockblaster_draw_ui(const GameContext *gm)
{
    ALLEGRO_FONT *font = gm->font;
    al_draw_textf(font, al_map_rgb(245, 245, 245), GRID_X, 18, 0, "Score: %ld",
                  gm->score);

    if (gm->combo > 0) {
        al_draw_textf(font, al_map_rgb(255, 230, 140), GRID_X + GRID_W * CELL,
                      18, ALLEGRO_ALIGN_RIGHT, "Combo: x%d", gm->combo);
    }
}

/**
 * \brief Compose and draw the full play scene: grid, tray, HUD, particles,
 *        popups, and the floating piece.
 *
 * Sets up the camera shake transform before drawing the grid and restores
 * it afterwards for the tray and exit button which are drawn in screen
 * space.
 *
 * \param gm  Game context.
 */
void blockblaster_draw_play_scene(GameContext *gm)
{
    ALLEGRO_FONT *font = gm->font;

    al_clear_to_color(al_map_rgb(12, 12, 16));
    ALLEGRO_TRANSFORM old, t;
    al_copy_transform(&old, al_get_current_transform());

    al_copy_transform(&t, &old);
    al_translate_transform(&t, gm->cam_x, gm->cam_y);
    al_use_transform(&t);

    blockblaster_draw_grid(gm);
    blockblaster_draw_ui(gm);

    /* Particles */
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &gm->particles[i];
        if (!p->alive)
            continue;
        float a = blockblaster_clampf(p->life / p->life0, 0.0f, 1.0f);
        ALLEGRO_COLOR c = al_map_rgba_f(p->col.r, p->col.g, p->col.b, a);
        al_draw_filled_circle(p->x, p->y, p->size, c);
    }

    /* Combo popup */
    if (gm->combo_popup.alive) {
        float a = blockblaster_clampf(
            gm->combo_popup.life / gm->combo_popup.life0, 0.0f, 1.0f);
        ALLEGRO_COLOR c = al_map_rgba_f(
            gm->combo_popup.theme.fill.r, gm->combo_popup.theme.fill.g,
            gm->combo_popup.theme.fill.b, 0.95f * a);

        ALLEGRO_TRANSFORM old2, sc;
        al_copy_transform(&old2, al_get_current_transform());

        al_copy_transform(&sc, &old2);
        al_translate_transform(&sc, gm->combo_popup.x, gm->combo_popup.y);
        al_scale_transform(&sc, gm->combo_popup.scale, gm->combo_popup.scale);
        al_translate_transform(&sc, -gm->combo_popup.x, -gm->combo_popup.y);
        al_use_transform(&sc);

        al_draw_text(font, al_map_rgba(0, 0, 0, (int) (170 * a)),
                     gm->combo_popup.x + 3, gm->combo_popup.y + 3,
                     ALLEGRO_ALIGN_CENTER, gm->combo_popup.text);
        al_draw_text(font, c, gm->combo_popup.x, gm->combo_popup.y,
                     ALLEGRO_ALIGN_CENTER, gm->combo_popup.text);

        al_use_transform(&old2);
    }

    /* Bonus popups */
    for (int i = 0; i < MAX_BONUS_POPUPS; i++) {
        BonusPopup *b = &gm->bonus_popups[i];
        if (!b->alive)
            continue;

        float a = blockblaster_clampf(b->life / b->life0, 0.0f, 1.0f);
        ALLEGRO_COLOR tc =
            al_map_rgba_f(b->theme.fill.r, b->theme.fill.g, b->theme.fill.b, a);

        char buf[96];
        snprintf(buf, sizeof(buf), "+%d", b->points);

        al_draw_text(font, al_map_rgba(0, 0, 0, (int) (120 * a)), b->x + 2,
                     b->y + 2, ALLEGRO_ALIGN_RIGHT, buf);
        al_draw_text(font, tc, b->x, b->y, ALLEGRO_ALIGN_RIGHT, buf);
    }

    al_use_transform(&old);
    blockblaster_draw_tray(gm);
    blockblaster_draw_play_exit_button(gm, font);
    blockblaster_draw_play_sound_button(gm, font);
    blockblaster_draw_floating_piece(gm);
}
