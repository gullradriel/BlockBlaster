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
 * \file blockblaster_ui.h
 * \brief UI drawing functions and interaction helpers.
 *
 * Declares the MenuAction enumeration, hit-test helpers, drawing functions
 * and the fullscreen toggle used by the main game loop.
 *
 * \author Castagnier Mickael aka Gull Ra Driel
 * \version 2.0
 * \date 21/02/2026
 */

#ifndef __BLOCKBLASTER_UI__
#define __BLOCKBLASTER_UI__

#ifdef __cplusplus
extern "C" {
#endif

#include "blockblaster_context.h"

#include <allegro5/allegro_font.h>
#include <stdbool.h>

/**
 * \brief Actions that can be triggered by a click on the main menu.
 */
typedef enum {
    MENU_ACTION_NONE = 0,      /**< No button was clicked. */
    MENU_ACTION_START_EMPTY,   /**< Start a new game with an empty grid. */
    MENU_ACTION_START_PARTIAL, /**< Start a new game with a partially filled
                                  grid. */
    MENU_ACTION_EXIT,          /**< Exit the application. */
    MENU_ACTION_TOGGLE_SOUND,  /**< Toggle audio on or off. */
    MENU_ACTION_CYCLE_TRAY,    /**< Cycle tray pieces count (1-4). */
    MENU_ACTION_CYCLE_GRID     /**< Cycle grid size (10/15/20). */
} MenuAction;

bool blockblaster_point_in_rect(float px, float py, float x1, float y1,
                                float x2, float y2);

MenuAction blockblaster_menu_action_from_click(float mx, float my);

bool blockblaster_gameover_restart_clicked(float mx, float my);
bool blockblaster_gameover_exit_clicked(float mx, float my);
bool blockblaster_gameover_ok_clicked(const GameContext *gm, float mx,
                                      float my);
bool blockblaster_gameover_name_field_clicked(float mx, float my);
bool blockblaster_play_exit_clicked(float mx, float my);
bool blockblaster_play_sound_clicked(float mx, float my);
bool blockblaster_exit_confirm_yes_clicked(float mx, float my);
bool blockblaster_exit_confirm_no_clicked(float mx, float my);

void blockblaster_draw_menu(const GameContext *gm, ALLEGRO_FONT *font);
void blockblaster_draw_gameover_overlay(const GameContext *gm,
                                        ALLEGRO_FONT *font);
void blockblaster_draw_play_exit_button(const GameContext *gm,
                                        ALLEGRO_FONT *font);
void blockblaster_draw_play_sound_button(const GameContext *gm,
                                         ALLEGRO_FONT *font);
void blockblaster_draw_exit_confirm(ALLEGRO_FONT *font);
void blockblaster_toggle_fullscreen(GameContext *gm);

#ifdef __cplusplus
}
#endif

#endif /* __BLOCKBLASTER_UI__ */
