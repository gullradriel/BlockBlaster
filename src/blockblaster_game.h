/* Copyright (C) 2026 Nilorea Studio — GPL-3.0-or-later (see COPYING). */

/**
 * \file blockblaster_game.h
 * \brief Game logic, utilities, save/load and platform helpers.
 */

#ifndef __BLOCKBLASTER_GAME__
#define __BLOCKBLASTER_GAME__

#ifdef __cplusplus
extern "C" {
#endif

#include "blockblaster_context.h"

/* ---- Utility ---- */
int blockblaster_irand(int a, int b);
float blockblaster_frand01(void);
float blockblaster_frand(float a, float b);
float blockblaster_clampf(float v, float lo, float hi);
float blockblaster_smoothstep(float t);
float blockblaster_lerpf(float a, float b, float t);
void blockblaster_shuffle_ints(int *a, int n);

/* ---- Platform ---- */
void blockblaster_get_data_path(const char *ressource, char *out,
                                size_t out_sz);
float blockblaster_font_effective_scale(const GameContext *gm);
ALLEGRO_FONT *blockblaster_reload_font(ALLEGRO_FONT *old_font,
                                       const char *font_path, float scale);
#ifdef ALLEGRO_ANDROID
float blockblaster_android_display_density(void);
void blockblaster_android_show_keyboard(void);
void blockblaster_android_hide_keyboard(void);
#endif

/* ---- Theme ---- */
void blockblaster_init_themes(Theme out[THEMES_COUNT]);
Theme blockblaster_random_theme(GameContext *gm);

/* ---- Grid ---- */
void blockblaster_grid_clear(Grid *g);
bool blockblaster_shape_cell(const Shape *s, int x, int y);
bool blockblaster_can_place_at(const Grid *g, const Shape *s, int gx, int gy);
bool blockblaster_any_valid_placement(const Grid *g, const Shape *s);
void blockblaster_place_shape(Grid *g, const Shape *s, int gx, int gy,
                              Theme theme);
int blockblaster_build_clear_mask(const Grid *g,
                                  bool out_mask[GRID_H_MAX][GRID_W_MAX]);
int blockblaster_count_cells_in_mask(const Grid *g,
                                     bool mask[GRID_H_MAX][GRID_W_MAX]);
void blockblaster_apply_clear_mask(Grid *g, bool mask[GRID_H_MAX][GRID_W_MAX]);

/* ---- Piece / tray ---- */
void blockblaster_refill_tray(GameContext *gm);
bool blockblaster_tray_all_used(const GameContext *gm);
bool blockblaster_none_placeable(const GameContext *gm);
void blockblaster_random_fill(Grid *g, int count);
void blockblaster_tray_piece_rect(int i, float *x1, float *y1, float *x2,
                                  float *y2);
void blockblaster_compute_grab_cell(const Shape *s, float rect_w, float rect_h,
                                    float local_x, float local_y, int *out_sx,
                                    int *out_sy);

/* ---- Score ---- */
int blockblaster_score_move(GameContext *gm, int placed_cells,
                            int lines_cleared, int cleared_cells,
                            int *out_clear_gain, float *out_mult);

/* ---- Animation ---- */
void blockblaster_begin_clear(GameContext *gm,
                              bool mask[GRID_H_MAX][GRID_W_MAX]);
void blockblaster_finish_clear(GameContext *gm);
void blockblaster_start_return(GameContext *gm, int tray_index);
void blockblaster_clear_predicted(GameContext *gm);
void blockblaster_compute_predicted_clear(GameContext *gm, const Piece *p,
                                          int gx, int gy);

/* ---- Input / drop ---- */
void blockblaster_update_drop_preview(GameContext *gm);
void blockblaster_try_drop(GameContext *gm);

/* ---- Particles ---- */
void blockblaster_spawn_particles(GameContext *gm, float x, float y, Theme t,
                                  int count);
void blockblaster_spawn_particles_scaled(GameContext *gm, float x, float y,
                                         Theme t, int count, float size_min,
                                         float size_max, float speed_min,
                                         float speed_max);
void blockblaster_spawn_bonus_popup(GameContext *gm, float x, float y,
                                    int points, float mult, Theme t);
void blockblaster_start_combo_popup(GameContext *gm, float mult, Theme theme);

/* ---- Game flow ---- */
void blockblaster_start_game(GameContext *gm, int mode);
void blockblaster_set_gameover(GameContext *gm);

/* ---- View ---- */
void blockblaster_update_view_offset(GameContext *gm);
void blockblaster_screen_to_virtual(const GameContext *gm, float sx, float sy,
                                    float *vx, float *vy);

/* ---- Save / load ---- */
void blockblaster_save_high_scores(const GameContext *gm);
void blockblaster_load_high_scores(GameContext *gm);
void blockblaster_insert_high_score(GameContext *gm, long score, int combo,
                                    const char *name);
void blockblaster_save_sound_state(bool on);
bool blockblaster_load_sound_state(void);
void blockblaster_save_player_name(const char *name);
void blockblaster_load_player_name(char *out, size_t out_sz);
void blockblaster_save_settings(int tray_count, int grid_size);
void blockblaster_load_settings(int *tray_count, int *grid_size);
void blockblaster_apply_settings(GameContext *gm);

#if defined(__EMSCRIPTEN__)
void blockblaster_emscripten_save_init(void);
bool blockblaster_emscripten_save_ready(void);
void blockblaster_emscripten_save_flush(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __BLOCKBLASTER_GAME__ */
