/* Copyright (C) 2026 Nilorea Studio

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <https://www.gnu.org/licenses/>. */

/**
 * \file blockblaster_ui.c
 * \brief UI drawing and interaction functions.
 *
 * All button positions and sizes are derived from the virtual-canvas
 * dimensions WIN_W / WIN_H so that the layout scales correctly in both
 * windowed and fullscreen modes.  Corner radii and line widths are scaled
 * by UI_SCALE so they look correct on high-DPI and fullscreen displays.
 *
 * \author Castagnier Mickael aka Gull Ra Driel
 * \version 2.0
 * \date 21/02/2026
 */

#include "blockblaster_ui.h"

#include <allegro5/allegro_primitives.h>
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include "allegro_emscripten_fullscreen.h"

#include <emscripten/html5.h>
#endif

/* ======================================================================== */
/* Menu button layout macros                                                 */
/* ======================================================================== */

#define MENU_BUTTON_W ((float) WIN_W * (2.0f / 3.0f))
#define MENU_BUTTON_H ((float) WIN_H * 0.065f)
#define MENU_BUTTON_X (((float) WIN_W - MENU_BUTTON_W) * 0.5f)
#define MENU_ROW5_Y ((float) WIN_H * 0.200f)
#define MENU_BTN_START_EMPTY_Y ((float) WIN_H * 0.285f)
#define MENU_BTN_START_PARTIALFILL_Y ((float) WIN_H * 0.370f)
#define MENU_BTN_SOUND_Y ((float) WIN_H * 0.455f)
#define MENU_BTN_EXIT_Y ((float) WIN_H * 0.540f)

/* Row 5: two half-width buttons side by side */
#define MENU_ROW5_GAP ((float) WIN_W * 0.02f)
#define MENU_ROW5_BTN_W ((MENU_BUTTON_W - MENU_ROW5_GAP) * 0.5f)
#define MENU_TRAY_BTN_X MENU_BUTTON_X
#define MENU_GRID_BTN_X (MENU_BUTTON_X + MENU_ROW5_BTN_W + MENU_ROW5_GAP)

/* ======================================================================== */
/* Game-over overlay button layout macros                                    */
/* ======================================================================== */

#define GAMEOVER_BUTTON_W ((float) WIN_W * 0.467f)
#define GAMEOVER_BUTTON_H ((float) WIN_H * 0.058f)
#define GAMEOVER_BUTTON_X (((float) WIN_W - GAMEOVER_BUTTON_W) * 0.5f)
#define GAMEOVER_BUTTON_Y ((float) WIN_H * 0.72f)

#define GAMEOVER_EXIT_W ((float) WIN_W * 0.333f)
#define GAMEOVER_EXIT_H ((float) WIN_H * 0.058f)
#define GAMEOVER_EXIT_X (((float) WIN_W - GAMEOVER_EXIT_W) * 0.5f)
#define GAMEOVER_EXIT_Y ((float) WIN_H * 0.80f)

/* OK button for player name editing (same position as Back to menu) */
#define GAMEOVER_OK_W ((float) WIN_W * 0.25f)
#define GAMEOVER_OK_H ((float) WIN_H * 0.058f)
#define GAMEOVER_OK_X (((float) WIN_W - GAMEOVER_OK_W) * 0.5f)
#define GAMEOVER_OK_Y ((float) WIN_H * 0.55f)

/* ======================================================================== */
/* In-game exit + sound button layout macros                                 */
/* ======================================================================== */

#define PLAY_BTN_W ((float) WIN_W * 0.267f)
#define PLAY_BTN_H ((float) WIN_H * 0.062f)
#define PLAY_BTN_GAP ((float) WIN_W * 0.02f)
#define PLAY_PAIR_W (PLAY_BTN_W + PLAY_BTN_GAP + PLAY_BTN_W)

#define PLAY_EXIT_BUTTON_X (((float) WIN_W - PLAY_PAIR_W) * 0.5f)
#define PLAY_EXIT_BUTTON_W PLAY_BTN_W
#define PLAY_EXIT_BUTTON_H PLAY_BTN_H

#define PLAY_SOUND_BUTTON_X (PLAY_EXIT_BUTTON_X + PLAY_BTN_W + PLAY_BTN_GAP)
#define PLAY_SOUND_BUTTON_W PLAY_BTN_W
#define PLAY_SOUND_BUTTON_H PLAY_BTN_H

#define PLAY_BUTTON_Y ((float) WIN_H - PLAY_BTN_H - 5.0f)
#define PLAY_EXIT_BUTTON_Y PLAY_BUTTON_Y

/* ======================================================================== */
/* Exit-confirmation dialog layout macros                                    */
/* ======================================================================== */

#define CONFIRM_PANEL_W ((float) WIN_W * 0.5f)
#define CONFIRM_PANEL_H ((float) WIN_H * 0.189f)
#define CONFIRM_PANEL_X (((float) WIN_W - CONFIRM_PANEL_W) * 0.5f)
#define CONFIRM_PANEL_Y (((float) WIN_H - CONFIRM_PANEL_H) * 0.5f)

#define CONFIRM_BTN_W ((float) WIN_W * 0.167f)
#define CONFIRM_BTN_H ((float) WIN_H * 0.058f)
#define CONFIRM_BTN_Y                                                          \
    (CONFIRM_PANEL_Y + CONFIRM_PANEL_H - CONFIRM_BTN_H - 18.0f)
#define CONFIRM_YES_X (((float) WIN_W * 0.5f) - CONFIRM_BTN_W - 10.0f)
#define CONFIRM_NO_X (((float) WIN_W * 0.5f) + 10.0f)

/* ======================================================================== */
/* Internal helpers                                                          */
/* ======================================================================== */

/**
 * \brief Test whether point (px, py) lies inside the axis-aligned rectangle.
 *
 * \param px  Point X coordinate.
 * \param py  Point Y coordinate.
 * \param x1  Rectangle left edge.
 * \param y1  Rectangle top edge.
 * \param x2  Rectangle right edge.
 * \param y2  Rectangle bottom edge.
 * \return    true if the point is inside (edges inclusive).
 */
bool blockblaster_point_in_rect(float px, float py, float x1, float y1,
                                float x2, float y2)
{
    return (px >= x1 && px <= x2 && py >= y1 && py <= y2);
}

/**
 * \brief Draw a rounded-rectangle button with a centred text label.
 *
 * Corner radii and border width are scaled by UI_SCALE so they remain
 * proportional on high-DPI displays.
 */
static void draw_button(float x, float y, float w, float h, const char *label,
                        ALLEGRO_FONT *font, ALLEGRO_COLOR bg)
{
    float r = 10.0f * UI_SCALE;
    al_draw_filled_rounded_rectangle(x, y, x + w, y + h, r, r, bg);
    al_draw_rounded_rectangle(x, y, x + w, y + h, r, r, GRID_LINE_COLOR,
                              ROUNDED_LINE_WIDTH);
    float text_y = y + (h - al_get_font_line_height(font)) * 0.5f;
    al_draw_text(font, al_map_rgb(240, 240, 248), x + w * 0.5f, text_y,
                 ALLEGRO_ALIGN_CENTER, label);
}

/* ======================================================================== */
/* Menu                                                                      */
/* ======================================================================== */

/**
 * \brief Map a click position to a MenuAction enum.
 *
 * \param mx  Click X in virtual canvas coordinates.
 * \param my  Click Y in virtual canvas coordinates.
 * \return    The action corresponding to the button hit, or
 *            MENU_ACTION_NONE if no button was clicked.
 */
MenuAction blockblaster_menu_action_from_click(float mx, float my)
{
    if (blockblaster_point_in_rect(mx, my, MENU_BUTTON_X,
                                   MENU_BTN_START_EMPTY_Y,
                                   MENU_BUTTON_X + MENU_BUTTON_W,
                                   MENU_BTN_START_EMPTY_Y + MENU_BUTTON_H))
        return MENU_ACTION_START_EMPTY;
    if (blockblaster_point_in_rect(
            mx, my, MENU_BUTTON_X, MENU_BTN_START_PARTIALFILL_Y,
            MENU_BUTTON_X + MENU_BUTTON_W,
            MENU_BTN_START_PARTIALFILL_Y + MENU_BUTTON_H))
        return MENU_ACTION_START_PARTIAL;
    if (blockblaster_point_in_rect(mx, my, MENU_BUTTON_X, MENU_BTN_EXIT_Y,
                                   MENU_BUTTON_X + MENU_BUTTON_W,
                                   MENU_BTN_EXIT_Y + MENU_BUTTON_H))
        return MENU_ACTION_EXIT;
    if (blockblaster_point_in_rect(mx, my, MENU_BUTTON_X, MENU_BTN_SOUND_Y,
                                   MENU_BUTTON_X + MENU_BUTTON_W,
                                   MENU_BTN_SOUND_Y + MENU_BUTTON_H))
        return MENU_ACTION_TOGGLE_SOUND;
    if (blockblaster_point_in_rect(mx, my, MENU_TRAY_BTN_X, MENU_ROW5_Y,
                                   MENU_TRAY_BTN_X + MENU_ROW5_BTN_W,
                                   MENU_ROW5_Y + MENU_BUTTON_H))
        return MENU_ACTION_CYCLE_TRAY;
    if (blockblaster_point_in_rect(mx, my, MENU_GRID_BTN_X, MENU_ROW5_Y,
                                   MENU_GRID_BTN_X + MENU_ROW5_BTN_W,
                                   MENU_ROW5_Y + MENU_BUTTON_H))
        return MENU_ACTION_CYCLE_GRID;
    return MENU_ACTION_NONE;
}

/**
 * \brief Draw the top-5 high-score table.
 */
static void draw_high_score_table(const GameContext *gm, ALLEGRO_FONT *font,
                                  float cx, float start_y)
{
    al_draw_text(font, al_map_rgb(255, 230, 140), cx, start_y,
                 ALLEGRO_ALIGN_CENTER, "=== HIGH SCORES ===");

    float line_h = al_get_font_line_height(font) + 4.0f;
    float y = start_y + line_h;

    if (gm->high_score_count == 0) {
        al_draw_text(font, al_map_rgb(140, 140, 150), cx, y,
                     ALLEGRO_ALIGN_CENTER, "No scores yet");
        return;
    }

    for (int i = 0; i < gm->high_score_count && i < MAX_HIGH_SCORES; i++) {
        char buf[80];
        int gw = gm->high_scores[i].grid_w;
        int gh = gm->high_scores[i].grid_h;
        int tc = gm->high_scores[i].tray_count;
        if (gw <= 0)
            gw = 10;
        if (gh <= 0)
            gh = 10;
        if (tc <= 0)
            tc = 4;
        snprintf(buf, sizeof(buf), "%d. %dx%dx%d %-5s %ld %dx", i + 1, gw, gh,
                 tc, gm->high_scores[i].name, gm->high_scores[i].score,
                 gm->high_scores[i].highest_combo);
        ALLEGRO_COLOR col =
            (i == 0) ? al_map_rgb(255, 220, 110) : al_map_rgb(200, 200, 210);
        al_draw_text(font, col, cx, y, ALLEGRO_ALIGN_CENTER, buf);
        y += line_h;
    }
}

/**
 * \brief Draw the main menu screen.
 *
 * Renders the title, start-mode buttons, sound toggle, tray/grid setting
 * buttons, hint text, and the high-score table.
 *
 * \param gm    Game context.
 * \param font  Font used for all text.
 */
void blockblaster_draw_menu(const GameContext *gm, ALLEGRO_FONT *font)
{
    al_clear_to_color(al_map_rgb(14, 14, 18));

    float cx = (float) WIN_W * 0.5f;

    al_draw_text(font, al_map_rgb(250, 250, 250), cx, (float) WIN_H * 0.10f,
                 ALLEGRO_ALIGN_CENTER, "BLOCK BLASTER");
    al_draw_text(font, al_map_rgb(250, 250, 250), cx, (float) WIN_H * 0.13f,
                 ALLEGRO_ALIGN_CENTER, "A Nilorea Studio Game");
    al_draw_text(font, al_map_rgb(250, 250, 250), cx, (float) WIN_H * 0.16f,
                 ALLEGRO_ALIGN_CENTER, "Made with Allegro 5");

    draw_button(MENU_BUTTON_X, MENU_BTN_START_EMPTY_Y, MENU_BUTTON_W,
                MENU_BUTTON_H, "Empty grid", font, al_map_rgb(35, 55, 95));
    draw_button(MENU_BUTTON_X, MENU_BTN_START_PARTIALFILL_Y, MENU_BUTTON_W,
                MENU_BUTTON_H, "Partially filled grid", font,
                al_map_rgb(55, 65, 45));
    draw_button(MENU_BUTTON_X, MENU_BTN_EXIT_Y, MENU_BUTTON_W, MENU_BUTTON_H,
                "Exit", font, al_map_rgb(90, 30, 30));
    draw_button(MENU_BUTTON_X, MENU_BTN_SOUND_Y, MENU_BUTTON_W, MENU_BUTTON_H,
                gm->sound_on ? "Sound: ON" : "Sound: OFF", font,
                gm->sound_on ? al_map_rgb(30, 70, 50) : al_map_rgb(60, 40, 20));

    /* Row 5: Tray count + Grid size buttons */
    {
        char tray_label[32];
        snprintf(tray_label, sizeof(tray_label), "Tray: %d",
                 gm->setting_tray_count);
        /* Color from red (1) to green (4) */
        int tc = gm->setting_tray_count;
        int r_val = 90 - (tc - 1) * 20; /* 90 -> 30 */
        int g_val = 30 + (tc - 1) * 20; /* 30 -> 90 */
        if (r_val < 20)
            r_val = 20;
        if (g_val < 20)
            g_val = 20;
        draw_button(MENU_TRAY_BTN_X, MENU_ROW5_Y, MENU_ROW5_BTN_W,
                    MENU_BUTTON_H, tray_label, font,
                    al_map_rgb(r_val, g_val, 20));

        char grid_label[32];
        snprintf(grid_label, sizeof(grid_label), "Grid: %dx%d",
                 gm->setting_grid_size, gm->setting_grid_size);
        draw_button(MENU_GRID_BTN_X, MENU_ROW5_Y, MENU_ROW5_BTN_W,
                    MENU_BUTTON_H, grid_label, font, al_map_rgb(35, 50, 80));
    }

    al_draw_text(font, al_map_rgb(140, 140, 150), cx, (float) WIN_H * 0.63f,
                 ALLEGRO_ALIGN_CENTER, "Try to clear the board !");

    draw_high_score_table(gm, font, cx, (float) WIN_H * 0.68f);
}

/* ======================================================================== */
/* In-game buttons                                                           */
/* ======================================================================== */

/**
 * \brief Test whether the in-game "Exit" button was clicked.
 *
 * \param mx  Click X in virtual canvas coordinates.
 * \param my  Click Y in virtual canvas coordinates.
 * \return    true if the click hit the exit button.
 */
bool blockblaster_play_exit_clicked(float mx, float my)
{
    return blockblaster_point_in_rect(mx, my, PLAY_EXIT_BUTTON_X, PLAY_BUTTON_Y,
                                      PLAY_EXIT_BUTTON_X + PLAY_BTN_W,
                                      PLAY_BUTTON_Y + PLAY_BTN_H);
}

/**
 * \brief Draw the in-game "Exit" button.
 *
 * \param gm    Game context (unused, reserved for future use).
 * \param font  Font used for the button label.
 */
void blockblaster_draw_play_exit_button(const GameContext *gm,
                                        ALLEGRO_FONT *font)
{
    (void) gm;
    draw_button(PLAY_EXIT_BUTTON_X, PLAY_BUTTON_Y, PLAY_BTN_W, PLAY_BTN_H,
                "Exit", font, al_map_rgb(80, 28, 28));
}

/**
 * \brief Test whether the in-game "Sound" toggle button was clicked.
 *
 * \param mx  Click X.
 * \param my  Click Y.
 * \return    true if the click hit the sound button.
 */
bool blockblaster_play_sound_clicked(float mx, float my)
{
    return blockblaster_point_in_rect(mx, my, PLAY_SOUND_BUTTON_X,
                                      PLAY_BUTTON_Y,
                                      PLAY_SOUND_BUTTON_X + PLAY_SOUND_BUTTON_W,
                                      PLAY_BUTTON_Y + PLAY_SOUND_BUTTON_H);
}

/**
 * \brief Draw the in-game "Sound: ON/OFF" toggle button.
 *
 * \param gm    Game context (provides the current sound_on state).
 * \param font  Font used for the button label.
 */
void blockblaster_draw_play_sound_button(const GameContext *gm,
                                         ALLEGRO_FONT *font)
{
    draw_button(PLAY_SOUND_BUTTON_X, PLAY_BUTTON_Y, PLAY_SOUND_BUTTON_W,
                PLAY_SOUND_BUTTON_H, gm->sound_on ? "Sound: ON" : "Sound: OFF",
                font,
                gm->sound_on ? al_map_rgb(30, 70, 50) : al_map_rgb(60, 40, 20));
}

/* ======================================================================== */
/* Exit-confirmation dialog                                                  */
/* ======================================================================== */

/**
 * \brief Test whether the "Yes" button in the exit-confirm dialog was clicked.
 *
 * \param mx  Click X.
 * \param my  Click Y.
 * \return    true if the click hit "Yes".
 */
bool blockblaster_exit_confirm_yes_clicked(float mx, float my)
{
    return blockblaster_point_in_rect(mx, my, CONFIRM_YES_X, CONFIRM_BTN_Y,
                                      CONFIRM_YES_X + CONFIRM_BTN_W,
                                      CONFIRM_BTN_Y + CONFIRM_BTN_H);
}

/**
 * \brief Test whether the "No" button in the exit-confirm dialog was clicked.
 *
 * \param mx  Click X.
 * \param my  Click Y.
 * \return    true if the click hit "No".
 */
bool blockblaster_exit_confirm_no_clicked(float mx, float my)
{
    return blockblaster_point_in_rect(mx, my, CONFIRM_NO_X, CONFIRM_BTN_Y,
                                      CONFIRM_NO_X + CONFIRM_BTN_W,
                                      CONFIRM_BTN_Y + CONFIRM_BTN_H);
}

/**
 * \brief Draw the exit-confirmation dialog overlay.
 *
 * Dims the background, draws a panel with the question "Exit game?" and
 * two buttons ("Yes" / "No").
 *
 * \param font  Font used for text labels.
 */
void blockblaster_draw_exit_confirm(ALLEGRO_FONT *font)
{
    float cx = (float) WIN_W * 0.5f;
    float r = 14.0f * UI_SCALE;

    al_draw_filled_rectangle(0, 0, (float) WIN_W, (float) WIN_H,
                             al_map_rgba(0, 0, 0, 160));

    al_draw_filled_rounded_rectangle(
        CONFIRM_PANEL_X, CONFIRM_PANEL_Y, CONFIRM_PANEL_X + CONFIRM_PANEL_W,
        CONFIRM_PANEL_Y + CONFIRM_PANEL_H, r, r, al_map_rgba(20, 20, 30, 220));
    al_draw_rounded_rectangle(CONFIRM_PANEL_X, CONFIRM_PANEL_Y,
                              CONFIRM_PANEL_X + CONFIRM_PANEL_W,
                              CONFIRM_PANEL_Y + CONFIRM_PANEL_H, r, r,
                              GRID_LINE_COLOR, ROUNDED_LINE_WIDTH);

    al_draw_text(font, al_map_rgb(240, 240, 248), cx, CONFIRM_PANEL_Y + 30.0f,
                 ALLEGRO_ALIGN_CENTER, "Exit game?");

    draw_button(CONFIRM_YES_X, CONFIRM_BTN_Y, CONFIRM_BTN_W, CONFIRM_BTN_H,
                "Yes", font, al_map_rgb(90, 30, 30));
    draw_button(CONFIRM_NO_X, CONFIRM_BTN_Y, CONFIRM_BTN_W, CONFIRM_BTN_H, "No",
                font, al_map_rgb(35, 55, 95));
}

/* ======================================================================== */
/* Game-over overlay                                                         */
/* ======================================================================== */

/**
 * \brief Test whether the "Back to menu" button was clicked on the
 *        game-over overlay.
 *
 * \param mx  Click X.
 * \param my  Click Y.
 * \return    true if the click hit the button.
 */
bool blockblaster_gameover_restart_clicked(float mx, float my)
{
    return blockblaster_point_in_rect(mx, my, GAMEOVER_BUTTON_X,
                                      GAMEOVER_BUTTON_Y,
                                      GAMEOVER_BUTTON_X + GAMEOVER_BUTTON_W,
                                      GAMEOVER_BUTTON_Y + GAMEOVER_BUTTON_H);
}

/**
 * \brief Test whether the "Exit" button was clicked on the game-over overlay.
 *
 * \param mx  Click X.
 * \param my  Click Y.
 * \return    true if the click hit the button.
 */
bool blockblaster_gameover_exit_clicked(float mx, float my)
{
    return blockblaster_point_in_rect(mx, my, GAMEOVER_EXIT_X, GAMEOVER_EXIT_Y,
                                      GAMEOVER_EXIT_X + GAMEOVER_EXIT_W,
                                      GAMEOVER_EXIT_Y + GAMEOVER_EXIT_H);
}

/**
 * \brief Test whether the "OK" button was clicked during name editing.
 *
 * Returns false if the player is not currently editing their name.
 *
 * \param gm  Game context.
 * \param mx  Click X.
 * \param my  Click Y.
 * \return    true if the click hit "OK" while editing.
 */
bool blockblaster_gameover_ok_clicked(const GameContext *gm, float mx, float my)
{
    if (!gm->editing_name)
        return false;
    return blockblaster_point_in_rect(mx, my, GAMEOVER_OK_X, GAMEOVER_OK_Y,
                                      GAMEOVER_OK_X + GAMEOVER_OK_W,
                                      GAMEOVER_OK_Y + GAMEOVER_OK_H);
}

/**
 * \brief Test whether the player-name text field was clicked.
 *
 * Used on Android to re-open the soft keyboard when tapping the field.
 *
 * \param mx  Click X.
 * \param my  Click Y.
 * \return    true if the click hit the name input field.
 */
bool blockblaster_gameover_name_field_clicked(float mx, float my)
{
    float cx = (float) WIN_W * 0.5f;
    float field_w = (float) WIN_W * 0.35f;
    float field_h = (float) WIN_H * 0.06f;
    float field_x = cx - field_w * 0.5f;
    float field_y = (float) WIN_H * 0.32f;
    return blockblaster_point_in_rect(mx, my, field_x, field_y,
                                      field_x + field_w, field_y + field_h);
}

/**
 * \brief Draw the game-over overlay.
 *
 * Two sub-modes: when gm->editing_name is true, shows the name editor
 * with an OK button; otherwise displays the final score, the high-score
 * table, and "Back to menu" / "Exit" buttons.
 *
 * \param gm    Game context.
 * \param font  Font used for all text and buttons.
 */
void blockblaster_draw_gameover_overlay(const GameContext *gm,
                                        ALLEGRO_FONT *font)
{
    float cx = (float) WIN_W * 0.5f;
    float pmx = (float) WIN_W * 0.10f;
    float panel_top = (float) WIN_H * 0.10f;
    float panel_bot = (float) WIN_H * 0.90f;
    float r = 14.0f * UI_SCALE;

    /* Dim background */
    al_draw_filled_rectangle(0, 0, (float) WIN_W, (float) WIN_H,
                             al_map_rgba(8, 8, 12, 150));

    /* Panel */
    al_draw_filled_rounded_rectangle(pmx, panel_top, (float) WIN_W - pmx,
                                     panel_bot, r, r,
                                     al_map_rgba(20, 20, 30, 210));
    al_draw_rounded_rectangle(pmx, panel_top, (float) WIN_W - pmx, panel_bot, r,
                              r, GRID_LINE_COLOR, ROUNDED_LINE_WIDTH);

    al_draw_text(font, al_map_rgb(255, 120, 120), cx, (float) WIN_H * 0.14f,
                 ALLEGRO_ALIGN_CENTER, "GAME OVER");

    if (gm->editing_name) {
        /* Player name editing mode */
        al_draw_text(font, al_map_rgb(240, 240, 240), cx, (float) WIN_H * 0.25f,
                     ALLEGRO_ALIGN_CENTER, "Enter your name:");

        /* Name display field */
        float field_w = (float) WIN_W * 0.35f;
        float field_h = (float) WIN_H * 0.06f;
        float field_x = cx - field_w * 0.5f;
        float field_y = (float) WIN_H * 0.32f;

        al_draw_filled_rounded_rectangle(field_x, field_y, field_x + field_w,
                                         field_y + field_h, r * 0.5f, r * 0.5f,
                                         al_map_rgb(30, 30, 40));
        al_draw_rounded_rectangle(
            field_x, field_y, field_x + field_w, field_y + field_h, r * 0.5f,
            r * 0.5f, al_map_rgb(120, 190, 255), ROUNDED_LINE_WIDTH);

        /* Show name with blinking cursor */
        char display_name[MAX_PLAYER_NAME_LEN + 2];
        snprintf(display_name, sizeof(display_name), "%s_", gm->player_name);
        float text_y =
            field_y + (field_h - al_get_font_line_height(font)) * 0.5f;
        al_draw_text(font, al_map_rgb(255, 255, 255), cx, text_y,
                     ALLEGRO_ALIGN_CENTER, display_name);

        al_draw_textf(font, al_map_rgb(140, 140, 150), cx,
                      (float) WIN_H * 0.42f, ALLEGRO_ALIGN_CENTER,
                      "(%d/%d characters)", gm->name_cursor,
                      MAX_PLAYER_NAME_LEN);

        al_draw_textf(font, al_map_rgb(240, 240, 240), cx,
                      (float) WIN_H * 0.47f, ALLEGRO_ALIGN_CENTER,
                      "Final score: %ld", gm->score);

        /* OK button only */
        draw_button(GAMEOVER_OK_X, GAMEOVER_OK_Y, GAMEOVER_OK_W, GAMEOVER_OK_H,
                    "OK", font, al_map_rgb(35, 55, 95));
    } else {
        /* Normal game-over display with scores */
        al_draw_textf(font, al_map_rgb(240, 240, 240), cx,
                      (float) WIN_H * 0.22f, ALLEGRO_ALIGN_CENTER,
                      "Final score: %ld  (Player: %s)", gm->score,
                      gm->player_name);

        /* Top-5 high score table */
        draw_high_score_table(gm, font, cx, (float) WIN_H * 0.30f);

        /* Buttons */
        draw_button(GAMEOVER_BUTTON_X, GAMEOVER_BUTTON_Y, GAMEOVER_BUTTON_W,
                    GAMEOVER_BUTTON_H, "Back to menu", font,
                    al_map_rgb(90, 90, 60));
        draw_button(GAMEOVER_EXIT_X, GAMEOVER_EXIT_Y, GAMEOVER_EXIT_W,
                    GAMEOVER_EXIT_H, "Exit", font, al_map_rgb(60, 24, 24));
    }
}

/* ======================================================================== */
/* Fullscreen toggle                                                         */
/* ======================================================================== */

/**
 * \brief Toggle between windowed and fullscreen modes.
 *
 * On Emscripten delegates to the browser Fullscreen API; on desktop uses
 * ALLEGRO_FULLSCREEN_WINDOW and saves/restores the windowed size so the
 * user returns to the same window dimensions.
 *
 * \param gm  Game context (display handle and dimensions updated).
 */
void blockblaster_toggle_fullscreen(GameContext *gm)
{
    if (!gm || !gm->display)
        return;

#ifdef __EMSCRIPTEN__
    if (gm->is_fullscreen) {
        emscripten_exit_fullscreen();
    } else {
        web_request_fullscreen();
    }
    /* The on_fullscreen_change() callback handles display resize. */
#else
    static int windowed_width = 0;
    static int windowed_height = 0;

    uint32_t flags = al_get_display_flags(gm->display);
    bool is_fullscreen = (flags & ALLEGRO_FULLSCREEN_WINDOW) != 0;

    int adapter = al_get_new_display_adapter();
    ALLEGRO_MONITOR_INFO mi;
    bool has_monitor = al_get_monitor_info(adapter, &mi);

    if (!is_fullscreen) {
        windowed_width = al_get_display_width(gm->display);
        windowed_height = al_get_display_height(gm->display);
    }

    al_set_display_flag(gm->display, ALLEGRO_FULLSCREEN_WINDOW, !is_fullscreen);
    al_acknowledge_resize(gm->display);

    if (!is_fullscreen) {
        if (has_monitor) {
            gm->display_width = mi.x2 - mi.x1;
            gm->display_height = mi.y2 - mi.y1;
        } else {
            gm->display_width = al_get_display_width(gm->display);
            gm->display_height = al_get_display_height(gm->display);
        }
    } else {
        if (windowed_width > 0 && windowed_height > 0)
            al_resize_display(gm->display, windowed_width, windowed_height);
        if (has_monitor) {
            int x = mi.x1 + ((mi.x2 - mi.x1) - windowed_width) / 2;
            int y = mi.y1 + ((mi.y2 - mi.y1) - windowed_height) / 2;
            al_set_window_position(gm->display, x, y);
        }
        gm->display_width = al_get_display_width(gm->display);
        gm->display_height = al_get_display_height(gm->display);
    }
#endif
}
