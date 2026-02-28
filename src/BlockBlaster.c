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
 * \file BlockBlaster.c
 * \brief Main entry point and event loop.
 *
 * All game logic, rendering, audio and UI are handled by their respective
 * modules.  This file contains only the Allegro initialisation, the main
 * event loop and the cleanup code.
 *
 * \author Castagnier Mickael aka Gull Ra Driel
 * \version 2.0
 * \date 19/02/2026
 */

#include "blockblaster_audio.h"
#include "blockblaster_context.h"
#include "blockblaster_game.h"
#include "blockblaster_render.h"
#include "blockblaster_ui.h"
#include "nilorea/n_log.h"

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_ttf.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(ALLEGRO_ANDROID)
#include <allegro5/allegro_android.h>
#endif

#if defined(__EMSCRIPTEN__)
#include "allegro_emscripten_fullscreen.h"
#include "allegro_emscripten_mouse.h"

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

/* Runtime virtual canvas dimensions (declared extern in
   blockblaster_context.h). These may change when switching between windowed and
   fullscreen modes. */
int WIN_W = WIN_W_DEFAULT;
int WIN_H = WIN_H_DEFAULT;

#if defined(ALLEGRO_ANDROID)
/** \brief Path to the app's private internal storage on Android. */
const char *android_internal_path = NULL;
#endif

/**
 * \brief Application entry point.
 *
 * Initialises Allegro (display, keyboard, mouse, audio, fonts, primitives),
 * creates the main event queue and timer, loads resources, and enters the
 * event loop.  The loop dispatches to the menu, play, and game-over states,
 * updating animations and input each frame.
 *
 * \param argc  Argument count (unused).
 * \param argv  Argument vector (unused).
 * \return      0 on success, 1 on fatal initialisation failure.
 */
int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    srand((unsigned) time(NULL));

    set_log_level(LOG_INFO);
#ifdef __EMSCRIPTEN__
    set_log_file_fd(stdout);
    blockblaster_emscripten_save_init();
#endif
    n_log(LOG_INFO, "Starting BlockBlaster...");

    if (!al_init()) {
        n_log(LOG_ERR, "Failed to init Allegro.");
        return 1;
    }
    if (!al_install_keyboard()) {
        n_log(LOG_ERR, "Failed to install keyboard.");
        return 1;
    }
    if (!al_install_mouse()) {
        n_log(LOG_ERR, "Failed to install mouse.");
        return 1;
    }
#ifdef ALLEGRO_ANDROID
    if (!al_install_touch_input())
        n_log(LOG_ERR, "Failed to install touch support.");
    al_set_mouse_emulation_mode(ALLEGRO_MOUSE_EMULATION_TRANSPARENT);
    al_android_set_apk_file_interface();
#endif
    if (!al_init_ttf_addon()) {
        n_log(LOG_ERR, "Failed to init TTF fonts addon");
        return 1;
    }
    if (al_install_audio() && al_init_acodec_addon()) {
        if (al_reserve_samples(32))
            audio_ok = true;
        else
            n_log(LOG_ERR, "Failed to reserve 32 audio samples");
    } else {
        n_log(LOG_ERR, "Failed to al_install_audio && al_init_acodec_addon");
    }

    al_set_new_display_option(ALLEGRO_DEPTH_SIZE, 16, ALLEGRO_SUGGEST);
#ifdef ALLEGRO_ANDROID
    al_set_new_display_flags(ALLEGRO_OPENGL | ALLEGRO_FULLSCREEN_WINDOW);
#else
    al_set_new_display_flags(ALLEGRO_OPENGL | ALLEGRO_WINDOWED |
                             ALLEGRO_RESIZABLE);
#endif
    ALLEGRO_DISPLAY *display = al_create_display(WIN_W, WIN_H);
    if (!display) {
        n_log(LOG_ERR, "Failed to create display");
        return 1;
    }

    if (!al_init_primitives_addon()) {
        n_log(LOG_ERR, "Failed to init primitives addon.");
        return 1;
    }

    ALLEGRO_TIMER *timer = al_create_timer(1.0 / REFRESH_RATE);
    ALLEGRO_EVENT_QUEUE *queue = al_create_event_queue();

    char font_path[512];
    blockblaster_get_data_path(FONT_FILENAME, font_path, sizeof(font_path));

    blockblaster_load_all_audio();

    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_mouse_event_source());
#ifdef ALLEGRO_ANDROID
    if (al_is_touch_input_installed())
        al_register_event_source(
            queue, al_get_touch_input_mouse_emulation_event_source());
#endif
    al_register_event_source(queue, al_get_timer_event_source(timer));

    GameContext gm = {0};
    gm.display = display;
    gm.display_width = al_get_display_width(display);
    gm.display_height = al_get_display_height(display);
    blockblaster_update_view_offset(&gm);
    gm.paused = false;
#ifndef __EMSCRIPTEN__
    gm.sound_on = blockblaster_load_sound_state();
    blockblaster_load_settings(&gm.setting_tray_count, &gm.setting_grid_size);
#else
    gm.sound_on = false;
    gm.setting_tray_count = 4;
    gm.setting_grid_size = 10;
#endif

    gm.font = blockblaster_reload_font(NULL, font_path,
                                       blockblaster_font_effective_scale(&gm));

#ifdef __EMSCRIPTEN__
    emscripten_set_fullscreenchange_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT,
                                             (void *) &gm, EM_TRUE,
                                             on_fullscreen_change);
    web_init_tab_visibility(timer, queue);
    web_init_key_char_capture();
#endif

    al_set_mouse_xy(display, WIN_W / 2, WIN_H / 2);
    al_show_mouse_cursor(display);

    gm.state = STATE_MENU;
    blockblaster_init_themes(gm.theme_table);

    /* Load high scores and player name */
#ifdef __EMSCRIPTEN__
    gm.high_score = 0;
    bool high_score_loaded = false;
    bool sound_state_loaded = false;
#else
    blockblaster_load_high_scores(&gm);
#endif
    blockblaster_load_player_name(gm.last_player_name,
                                  sizeof(gm.last_player_name));
    snprintf(gm.player_name, sizeof(gm.player_name), "%s", gm.last_player_name);

    bool running = true;
    bool redraw = true;
#ifdef ALLEGRO_ANDROID
    bool display_halted = false;
#endif

    al_start_timer(timer);

    while (running) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(queue, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            if (gm.state == STATE_PLAY)
                gm.confirm_exit = true;
            else
                running = false;

        } else if (ev.type == ALLEGRO_EVENT_TIMER) {
            redraw = true;

#ifdef __EMSCRIPTEN__
            if (blockblaster_emscripten_save_ready()) {
                if (!high_score_loaded) {
                    blockblaster_load_high_scores(&gm);
                    high_score_loaded = true;
                }
                if (!sound_state_loaded) {
                    bool prev_sound = gm.sound_on;
                    gm.sound_on = blockblaster_load_sound_state();
                    if (prev_sound && !gm.sound_on) {
                        blockblaster_stop_music();
                        music_current_track = -1;
                    }
                    blockblaster_load_settings(&gm.setting_tray_count,
                                               &gm.setting_grid_size);
                    sound_state_loaded = true;
                }
            }
#endif

            float dt = 1.0f / REFRESH_RATE;

            /* Screen shake */
            gm.cam_x = 0.0f;
            gm.cam_y = 0.0f;
            if (gm.shake_t > 0.0f) {
                gm.shake_t -= dt;
                if (gm.shake_t < 0.0f)
                    gm.shake_t = 0.0f;
                float t = gm.shake_t / SHAKE_TIME;
                float s = gm.shake_strength * t;
                gm.cam_x = blockblaster_frand(-s, s);
                gm.cam_y = blockblaster_frand(-s, s);
            }

            /* Particles */
            for (int i = 0; i < MAX_PARTICLES; i++) {
                Particle *p = &gm.particles[i];
                if (!p->alive)
                    continue;
                p->life -= dt;
                if (p->life <= 0.0f) {
                    p->alive = false;
                    continue;
                }
                p->vy += 520.0f * dt;
                p->vx *= (1.0f - 0.9f * dt);
                p->vy *= (1.0f - 0.2f * dt);
                p->x += p->vx * dt;
                p->y += p->vy * dt;
            }

            /* Bonus popups */
            for (int i = 0; i < MAX_BONUS_POPUPS; i++) {
                BonusPopup *b = &gm.bonus_popups[i];
                if (!b->alive)
                    continue;
                b->life -= dt;
                if (b->life <= 0.0f) {
                    b->alive = false;
                    continue;
                }
                b->y += b->vy * dt;
            }

            /* Pop timers */
            for (int y = 0; y < GRID_H; y++)
                for (int x = 0; x < GRID_W; x++)
                    if (gm.pop_t[y][x] > 0.0f) {
                        gm.pop_t[y][x] -= dt;
                        if (gm.pop_t[y][x] < 0.0f)
                            gm.pop_t[y][x] = 0.0f;
                    }

            /* Clear animation */
            if (gm.clearing) {
                gm.clear_t -= dt;
                if (gm.clear_t <= 0.0f)
                    blockblaster_finish_clear(&gm);
            }

            /* Return-to-tray animation */
            if (gm.returning) {
                gm.return_t -= dt;
                if (gm.return_t <= 0.0f) {
                    gm.returning = false;
                    gm.return_t = 0.0f;
                }
            }

            /* Combo popup */
            if (gm.combo_popup.alive) {
                gm.combo_popup.life -= dt;
                if (gm.combo_popup.life <= 0.0f) {
                    gm.combo_popup.alive = false;
                } else {
                    gm.combo_popup.x += gm.combo_popup.vx * dt;
                    gm.combo_popup.y += gm.combo_popup.vy * dt;
                    float p =
                        1.0f - (gm.combo_popup.life / gm.combo_popup.life0);
                    float ease = 1.0f - (1.0f - p) * (1.0f - p);
                    gm.combo_popup.scale = 0.35f + 0.95f * ease;
                }
            }

            if (gm.state == STATE_PLAY && gm.dragging && !gm.confirm_exit)
                blockblaster_update_drop_preview(&gm);

        } else if (ev.type == ALLEGRO_EVENT_MOUSE_AXES) {
            blockblaster_screen_to_virtual(&gm, ev.mouse.x, ev.mouse.y,
                                           &gm.mouse_x, &gm.mouse_y);
            if (gm.state == STATE_PLAY && gm.dragging && !gm.confirm_exit)
                blockblaster_update_drop_preview(&gm);

        } else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            float mouse_x = 0.0f, mouse_y = 0.0f;
            blockblaster_screen_to_virtual(&gm, ev.mouse.x, ev.mouse.y,
                                           &mouse_x, &mouse_y);
            gm.mouse_x = mouse_x;
            gm.mouse_y = mouse_y;

            if (ev.mouse.button == 1 && gm.state == STATE_MENU) {
                MenuAction action =
                    blockblaster_menu_action_from_click(mouse_x, mouse_y);
                if (action == MENU_ACTION_START_EMPTY)
                    blockblaster_start_game(&gm, 0);
                if (action == MENU_ACTION_START_PARTIAL)
                    blockblaster_start_game(&gm, 1);
                if (action == MENU_ACTION_EXIT)
                    running = false;
                if (action == MENU_ACTION_TOGGLE_SOUND) {
                    gm.sound_on = !gm.sound_on;
                    blockblaster_save_sound_state(gm.sound_on);
                    if (!gm.sound_on) {
                        blockblaster_stop_music();
                        music_current_track = -1;
                    }
                }
                if (action == MENU_ACTION_CYCLE_TRAY) {
                    gm.setting_tray_count++;
                    if (gm.setting_tray_count > 4)
                        gm.setting_tray_count = 1;
                    blockblaster_save_settings(gm.setting_tray_count,
                                               gm.setting_grid_size);
                    blockblaster_play_sfx(sfx_select, &gm);
                }
                if (action == MENU_ACTION_CYCLE_GRID) {
                    if (gm.setting_grid_size == 10)
                        gm.setting_grid_size = 15;
                    else if (gm.setting_grid_size == 15)
                        gm.setting_grid_size = 20;
                    else
                        gm.setting_grid_size = 10;
                    blockblaster_save_settings(gm.setting_tray_count,
                                               gm.setting_grid_size);
                    blockblaster_play_sfx(sfx_select, &gm);
                }
                if (action == MENU_ACTION_START_EMPTY ||
                    action == MENU_ACTION_START_PARTIAL ||
                    action == MENU_ACTION_EXIT)
                    blockblaster_play_sfx(sfx_select, &gm);
                if (action == MENU_ACTION_START_EMPTY ||
                    action == MENU_ACTION_START_PARTIAL) {
                    int rand_music = 2 + rand() % 3;
                    blockblaster_play_music_track(rand_music, &gm);
                }

            } else if (ev.mouse.button == 1 && gm.state == STATE_GAMEOVER) {
                if (gm.editing_name) {
                    if (blockblaster_gameover_ok_clicked(&gm, mouse_x,
                                                         mouse_y)) {
                        /* Finalise name */
                        if (gm.player_name[0] == '\0')
                            snprintf(gm.player_name, sizeof(gm.player_name),
                                     "PLAYR");
                        /* Re-insert score with possibly updated name */
                        blockblaster_load_high_scores(&gm);
                        blockblaster_insert_high_score(
                            &gm, gm.score, gm.highest_combo, gm.player_name);
                        blockblaster_save_high_scores(&gm);
                        snprintf(gm.last_player_name,
                                 sizeof(gm.last_player_name), "%s",
                                 gm.player_name);
                        blockblaster_save_player_name(gm.last_player_name);
                        gm.editing_name = false;
                        blockblaster_play_sfx(sfx_select, &gm);
#ifdef ALLEGRO_ANDROID
                        blockblaster_android_hide_keyboard();
#endif
                    }
#ifdef ALLEGRO_ANDROID
                    else if (blockblaster_gameover_name_field_clicked(
                                 mouse_x, mouse_y)) {
                        blockblaster_android_show_keyboard();
                    }
#endif
                } else {
                    if (blockblaster_gameover_restart_clicked(mouse_x,
                                                              mouse_y)) {
                        gm.state = STATE_MENU;
                        blockblaster_play_sfx(sfx_select, &gm);
                    }
                    if (blockblaster_gameover_exit_clicked(mouse_x, mouse_y))
                        running = false;
                }

            } else if (gm.state == STATE_PLAY && ev.mouse.button == 1) {
                if (gm.confirm_exit) {
                    if (blockblaster_exit_confirm_yes_clicked(mouse_x, mouse_y))
                        running = false;
                    else if (blockblaster_exit_confirm_no_clicked(mouse_x,
                                                                  mouse_y))
                        gm.confirm_exit = false;
                } else if (!gm.clearing && !gm.returning) {
                    if (blockblaster_play_exit_clicked(mouse_x, mouse_y))
                        gm.confirm_exit = true;
                    if (blockblaster_play_sound_clicked(mouse_x, mouse_y)) {
                        gm.sound_on = !gm.sound_on;
                        blockblaster_save_sound_state(gm.sound_on);
                        if (!gm.sound_on) {
                            blockblaster_stop_music();
                            music_current_track = -1;
                        } else {
                            int rand_music = 2 + rand() % 3;
                            blockblaster_play_music_track(rand_music, &gm);
                        }
                    }
                    for (int i = 0; i < PIECES_PER_SET; i++) {
                        if (gm.tray[i].used)
                            continue;
                        float x1, y1, x2, y2;
                        blockblaster_tray_piece_rect(i, &x1, &y1, &x2, &y2);
                        if (blockblaster_point_in_rect(mouse_x, mouse_y, x1, y1,
                                                       x2, y2)) {
                            blockblaster_play_sfx(sfx_select, &gm);
                            gm.dragging = true;
                            gm.dragging_index = i;
                            float local_x = mouse_x - x1;
                            float local_y = mouse_y - y1;
                            blockblaster_compute_grab_cell(
                                &gm.tray[i].shape, (x2 - x1), (y2 - y1),
                                local_x, local_y, &gm.grab_sx, &gm.grab_sy);
                            blockblaster_update_drop_preview(&gm);
                            break;
                        }
                    }
                }
            }

        } else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
            if (gm.state == STATE_PLAY && ev.mouse.button == 1 &&
                !gm.confirm_exit)
                blockblaster_try_drop(&gm);

        } else if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
            int kc = ev.keyboard.keycode;

            if (kc == ALLEGRO_KEY_ESCAPE) {
                if (gm.state == STATE_PLAY) {
                    gm.confirm_exit = !gm.confirm_exit;
                } else {
                    running = false;
                }
            }
            if (kc == ALLEGRO_KEY_F11) {
                blockblaster_toggle_fullscreen(&gm);
                blockblaster_update_view_offset(&gm);
                gm.font = blockblaster_reload_font(
                    gm.font, font_path, blockblaster_font_effective_scale(&gm));
            }

        } else if (ev.type == ALLEGRO_EVENT_KEY_CHAR) {
            /* Player name editing in game-over state.
             *
             * KEY_CHAR provides ev.keyboard.unichar which respects the
             * active keyboard layout on desktop and Android.  On
             * Emscripten, Allegro derives unichar from the physical
             * keycode (always QWERTY), so we read the layout-correct
             * character captured by the JavaScript keydown callback
             * instead. */
            if (gm.state == STATE_GAMEOVER && gm.editing_name) {
                int kc = ev.keyboard.keycode;
                int uc = ev.keyboard.unichar;

#ifdef __EMSCRIPTEN__
                char jc = web_consume_key_char();
                if (jc != '\0')
                    uc = (unsigned char) jc;
#endif

                if (uc > 0 && isalpha((unsigned char) uc)) {
                    if (gm.name_cursor < MAX_PLAYER_NAME_LEN) {
                        char ch = (char) toupper((unsigned char) uc);
                        gm.player_name[gm.name_cursor++] = ch;
                        gm.player_name[gm.name_cursor] = '\0';
                    }
                } else if (kc == ALLEGRO_KEY_BACKSPACE) {
                    if (gm.name_cursor > 0) {
                        gm.name_cursor--;
                        gm.player_name[gm.name_cursor] = '\0';
                    }
                } else if (kc == ALLEGRO_KEY_ENTER ||
                           kc == ALLEGRO_KEY_PAD_ENTER) {
                    if (gm.player_name[0] == '\0')
                        snprintf(gm.player_name, sizeof(gm.player_name),
                                 "PLAYR");
                    blockblaster_load_high_scores(&gm);
                    blockblaster_insert_high_score(
                        &gm, gm.score, gm.highest_combo, gm.player_name);
                    blockblaster_save_high_scores(&gm);
                    snprintf(gm.last_player_name, sizeof(gm.last_player_name),
                             "%s", gm.player_name);
                    blockblaster_save_player_name(gm.last_player_name);
                    gm.editing_name = false;
                    blockblaster_play_sfx(sfx_select, &gm);
#ifdef ALLEGRO_ANDROID
                    blockblaster_android_hide_keyboard();
#endif
                }
            }
#ifdef __EMSCRIPTEN__
            else {
                /* Consume the JS character for non-editing KEY_CHAR
                 * events so the ring buffer stays in sync. */
                (void) web_consume_key_char();
            }
#endif

        } else if (ev.type == ALLEGRO_EVENT_DISPLAY_RESIZE) {
            al_acknowledge_resize(display);
            gm.display_width = al_get_display_width(display);
            gm.display_height = al_get_display_height(display);
            blockblaster_update_view_offset(&gm);
            gm.font = blockblaster_reload_font(
                gm.font, font_path, blockblaster_font_effective_scale(&gm));
#ifdef ALLEGRO_ANDROID
        } else if (ev.type == ALLEGRO_EVENT_DISPLAY_SWITCH_OUT) {
            /* Android screen-lock or transient focus loss: stop the
               timer so events do not pile up while the display is off
               (same approach as the Emscripten tab-visibility handler). */
            if (gm.dragging) {
                gm.dragging = false;
                gm.can_drop_preview = false;
                blockblaster_clear_predicted(&gm);
            }
            if (gm.returning) {
                gm.returning = false;
                gm.return_t = 0.0f;
            }
            if (music_instance)
                al_set_sample_instance_playing(music_instance, false);
            al_stop_timer(timer);
#endif
        } else if (ev.type == ALLEGRO_EVENT_DISPLAY_HALT_DRAWING) {
#ifdef ALLEGRO_ANDROID
            display_halted = true;
#endif
            if (gm.dragging) {
                gm.dragging = false;
                gm.can_drop_preview = false;
                blockblaster_clear_predicted(&gm);
            }
            if (gm.returning) {
                gm.returning = false;
                gm.return_t = 0.0f;
            }
            if (music_instance)
                al_set_sample_instance_playing(music_instance, false);
            al_stop_timer(timer);
            al_acknowledge_drawing_halt(display);

        } else if (ev.type == ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING) {
#ifdef ALLEGRO_ANDROID
            display_halted = false;
#endif
            al_acknowledge_drawing_resume(display);
            al_flush_event_queue(queue);
            al_set_target_backbuffer(display);
            gm.display_width = al_get_display_width(display);
            gm.display_height = al_get_display_height(display);
            blockblaster_update_view_offset(&gm);
#ifdef ALLEGRO_ANDROID
            al_android_set_apk_file_interface();
#endif
            gm.font = blockblaster_reload_font(
                gm.font, font_path, blockblaster_font_effective_scale(&gm));
            if (music_instance)
                al_set_sample_instance_playing(music_instance, true);
            al_start_timer(timer);
#ifdef ALLEGRO_ANDROID
        } else if (ev.type == ALLEGRO_EVENT_DISPLAY_SWITCH_IN) {
            /* Unlocked / regained focus.  When the surface was never
               destroyed (screen-lock), RESUME_DRAWING will not arrive.
               Restart the timer ourselves.  If RESUME_DRAWING already
               handled the resume, the timer is running and we skip. */
            if (!display_halted && !al_get_timer_started(timer)) {
                al_flush_event_queue(queue);
                al_set_target_backbuffer(display);
                gm.display_width = al_get_display_width(display);
                gm.display_height = al_get_display_height(display);
                blockblaster_update_view_offset(&gm);
                al_android_set_apk_file_interface();
                gm.font = blockblaster_reload_font(
                    gm.font, font_path, blockblaster_font_effective_scale(&gm));
                if (music_instance)
                    al_set_sample_instance_playing(music_instance, true);
                al_start_timer(timer);
            }
#endif
        }

        /* ---- Draw ---- */
        if (redraw) {
            redraw = false;

            ALLEGRO_TRANSFORM base;
            al_build_transform(&base, gm.view_offset_x, gm.view_offset_y,
                               gm.scale, gm.scale, 0.0f);
            al_use_transform(&base);

            if (gm.state == STATE_MENU) {
                blockblaster_draw_menu(&gm, gm.font);
                blockblaster_play_music_track(0, &gm);
            } else if (gm.state == STATE_PLAY) {
                blockblaster_draw_play_scene(&gm);
                if (gm.confirm_exit)
                    blockblaster_draw_exit_confirm(gm.font);
            } else if (gm.state == STATE_GAMEOVER) {
                blockblaster_play_music_track(1, &gm);
                blockblaster_draw_play_scene(&gm);
                blockblaster_draw_gameover_overlay(&gm, gm.font);
            }

#ifdef __EMSCRIPTEN__
            if (gm.pending_resize) {
                gm.pending_resize = false;
                if (!gm.is_fullscreen || gm.pending_w <= 0 ||
                    gm.pending_h <= 0) {
                    gm.pending_w = WIN_W_DEFAULT;
                    gm.pending_h = WIN_H_DEFAULT;
                }
                al_resize_display(gm.display, gm.pending_w, gm.pending_h);
                al_set_target_backbuffer(gm.display);
                gm.display_width = al_get_display_width(gm.display);
                gm.display_height = al_get_display_height(gm.display);
                blockblaster_update_view_offset(&gm);
                gm.font = blockblaster_reload_font(
                    gm.font, font_path, blockblaster_font_effective_scale(&gm));
            }
#endif

            al_flip_display();
        }
    }

    n_log(LOG_INFO, "Exiting...");

    blockblaster_destroy_all_audio();
    al_destroy_font(gm.font);
    al_destroy_event_queue(queue);
    al_destroy_timer(timer);
    al_destroy_display(display);
    return 0;
}
