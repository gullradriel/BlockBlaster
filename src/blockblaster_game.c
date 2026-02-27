/* Copyright (C) 2026 Nilorea Studio — GPL-3.0-or-later (see COPYING). */

/**
 * \file blockblaster_game.c
 * \brief Game logic, utilities, save/load, particles, and platform helpers.
 */

#include "blockblaster_game.h"

#include "blockblaster_audio.h"
#include "nilorea/n_common.h"
#include "nilorea/n_log.h"

#include <allegro5/allegro_ttf.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(__ANDROID__)
#include <allegro5/allegro_android.h>
#endif

/* Runtime grid dimensions and tray count (declared extern in context.h). */
int GRID_W = 10;
int GRID_H = 10;
int PIECES_PER_SET = 4;

/* Display scale factor used by line-width macros to guarantee a minimum
   physical pixel width.  Updated every time the view offset is recalculated. */
float g_display_scale = 1.0f;

/* ======================================================================== */
/* Utility                                                                   */
/* ======================================================================== */

/**
 * \brief Return a random integer in the inclusive range [a, b].
 *
 * If b <= a the function returns a directly.
 *
 * \param a  Lower bound (inclusive).
 * \param b  Upper bound (inclusive).
 * \return   Pseudo-random integer in [a, b].
 */
int blockblaster_irand(int a, int b)
{
    if (b <= a)
        return a;
    return a + (rand() % (b - a + 1));
}

/**
 * \brief Return a random float in [0.0, 1.0].
 * \return  Pseudo-random float uniformly distributed in [0, 1].
 */
float blockblaster_frand01(void)
{
    return (float) rand() / (float) RAND_MAX;
}

/**
 * \brief Return a random float in [a, b].
 *
 * \param a  Lower bound.
 * \param b  Upper bound.
 * \return   Pseudo-random float in [a, b].
 */
float blockblaster_frand(float a, float b)
{
    return a + (b - a) * blockblaster_frand01();
}

/**
 * \brief Clamp a float to the range [lo, hi].
 *
 * \param v   Value to clamp.
 * \param lo  Lower bound.
 * \param hi  Upper bound.
 * \return    Clamped value.
 */
float blockblaster_clampf(float v, float lo, float hi)
{
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

/**
 * \brief Hermite smoothstep interpolation: 3t^2 - 2t^3.
 *
 * Maps t in [0,1] to a smooth S-curve useful for easing animations.
 *
 * \param t  Input parameter, expected in [0, 1].
 * \return   Smoothly interpolated value in [0, 1].
 */
float blockblaster_smoothstep(float t)
{
    return t * t * (3.0f - 2.0f * t);
}

/**
 * \brief Linear interpolation between a and b by factor t.
 *
 * \param a  Start value (returned when t == 0).
 * \param b  End value (returned when t == 1).
 * \param t  Interpolation factor, typically in [0, 1].
 * \return   Interpolated value a + (b - a) * t.
 */
float blockblaster_lerpf(float a, float b, float t)
{
    return a + (b - a) * t;
}

/**
 * \brief Fisher-Yates shuffle of an integer array in-place.
 *
 * \param a  Array of integers to shuffle.
 * \param n  Number of elements in the array.
 */
void blockblaster_shuffle_ints(int *a, int n)
{
    for (int i = n - 1; i > 0; i--) {
        int j = blockblaster_irand(0, i);
        int tmp = a[i];
        a[i] = a[j];
        a[j] = tmp;
    }
}

/* ======================================================================== */
/* Platform                                                                  */
/* ======================================================================== */

/**
 * \brief Build the platform-specific path to a DATA/ resource file.
 *
 * On Emscripten the path prefix is "/DATA/", on Android the bare filename
 * is used (APK file interface), and on desktop "./DATA/" is prepended.
 *
 * \param ressource  File name of the resource (e.g. "place.ogg").
 * \param out        Output buffer receiving the full path.
 * \param out_sz     Size of the output buffer in bytes.
 */
void blockblaster_get_data_path(const char *ressource, char *out, size_t out_sz)
{
#if defined(__EMSCRIPTEN__)
    snprintf(out, out_sz, "%s%s", "/DATA/", ressource);
#elif defined(ALLEGRO_ANDROID)
    snprintf(out, out_sz, "%s", ressource);
#else
    snprintf(out, out_sz, "%s%s", "./DATA/", ressource);
#endif
}

#ifdef ALLEGRO_ANDROID
static float s_android_density = 0.0f;

/**
 * \brief Query the Android display density via JNI (cached after first call).
 *
 * Reads DisplayMetrics.density through JNI reflection.  Returns a default
 * of 1.4 if the JNI call chain fails.
 *
 * \return  Display density factor (e.g. 1.0 for mdpi, 2.0 for xhdpi).
 */
float blockblaster_android_display_density(void)
{
    if (s_android_density > 0.0f)
        return s_android_density;

    float result = 1.4f;

    JNIEnv *env = al_android_get_jni_env();
    jobject activity = env ? al_android_get_activity() : NULL;
    if (env && activity) {
        jclass actClass = (*env)->GetObjectClass(env, activity);
        jmethodID getResources = (*env)->GetMethodID(
            env, actClass, "getResources", "()Landroid/content/res/Resources;");
        jobject resources =
            getResources ? (*env)->CallObjectMethod(env, activity, getResources)
                         : NULL;
        if (resources) {
            jclass resClass = (*env)->GetObjectClass(env, resources);
            jmethodID getMetrics =
                (*env)->GetMethodID(env, resClass, "getDisplayMetrics",
                                    "()Landroid/util/DisplayMetrics;");
            jobject metrics =
                getMetrics
                    ? (*env)->CallObjectMethod(env, resources, getMetrics)
                    : NULL;
            if (metrics) {
                jclass dmClass = (*env)->GetObjectClass(env, metrics);
                jfieldID fDensity =
                    (*env)->GetFieldID(env, dmClass, "density", "F");
                if (fDensity) {
                    jfloat d = (*env)->GetFloatField(env, metrics, fDensity);
                    if (d > 0.0f)
                        result = (float) d;
                }
                (*env)->DeleteLocalRef(env, dmClass);
                (*env)->DeleteLocalRef(env, metrics);
            }
            (*env)->DeleteLocalRef(env, resClass);
            (*env)->DeleteLocalRef(env, resources);
        }
        (*env)->DeleteLocalRef(env, actClass);
        if ((*env)->ExceptionCheck(env))
            (*env)->ExceptionClear(env);
    }

    s_android_density = result;
    return result;
}

/**
 * \brief Show the Android soft keyboard using InputMethodManager directly.
 *
 * Previous approach relied on a custom Java method (showSoftKeyboard) in
 * AllegroActivity which may not exist in the actually compiled Activity
 * subclass.  This version calls InputMethodManager.toggleSoftInput() via
 * JNI so it works regardless of the Activity class hierarchy.
 */
void blockblaster_android_show_keyboard(void)
{
    JNIEnv *env = al_android_get_jni_env();
    jobject activity = env ? al_android_get_activity() : NULL;
    if (!env || !activity) {
        n_log(LOG_ERR, "android_show_keyboard: no JNI env or activity");
        return;
    }

    /* Context.INPUT_METHOD_SERVICE */
    jclass ctx_cls = (*env)->FindClass(env, "android/content/Context");
    jfieldID ims_fid = (*env)->GetStaticFieldID(
        env, ctx_cls, "INPUT_METHOD_SERVICE", "Ljava/lang/String;");
    jstring ims_str = (*env)->GetStaticObjectField(env, ctx_cls, ims_fid);

    /* activity.getSystemService(INPUT_METHOD_SERVICE) */
    jmethodID get_ss =
        (*env)->GetMethodID(env, ctx_cls, "getSystemService",
                            "(Ljava/lang/String;)Ljava/lang/Object;");
    jobject imm = (*env)->CallObjectMethod(env, activity, get_ss, ims_str);

    if (imm) {
        jclass imm_cls = (*env)->FindClass(
            env, "android/view/inputmethod/InputMethodManager");
        /* toggleSoftInput(SHOW_FORCED=2, HIDE_IMPLICIT_ONLY=1) */
        jmethodID toggle =
            (*env)->GetMethodID(env, imm_cls, "toggleSoftInput", "(II)V");
        if (toggle)
            (*env)->CallVoidMethod(env, imm, toggle, 2, 1);
        (*env)->DeleteLocalRef(env, imm_cls);
        (*env)->DeleteLocalRef(env, imm);
    }

    (*env)->DeleteLocalRef(env, ims_str);
    (*env)->DeleteLocalRef(env, ctx_cls);
    if ((*env)->ExceptionCheck(env))
        (*env)->ExceptionClear(env);

    n_log(LOG_INFO, "android_show_keyboard: toggleSoftInput called");
}

/**
 * \brief Hide the Android soft keyboard using InputMethodManager directly.
 */
void blockblaster_android_hide_keyboard(void)
{
    JNIEnv *env = al_android_get_jni_env();
    jobject activity = env ? al_android_get_activity() : NULL;
    if (!env || !activity) {
        n_log(LOG_ERR, "android_hide_keyboard: no JNI env or activity");
        return;
    }

    /* Context.INPUT_METHOD_SERVICE */
    jclass ctx_cls = (*env)->FindClass(env, "android/content/Context");
    jfieldID ims_fid = (*env)->GetStaticFieldID(
        env, ctx_cls, "INPUT_METHOD_SERVICE", "Ljava/lang/String;");
    jstring ims_str = (*env)->GetStaticObjectField(env, ctx_cls, ims_fid);

    /* activity.getSystemService(INPUT_METHOD_SERVICE) */
    jmethodID get_ss =
        (*env)->GetMethodID(env, ctx_cls, "getSystemService",
                            "(Ljava/lang/String;)Ljava/lang/Object;");
    jobject imm = (*env)->CallObjectMethod(env, activity, get_ss, ims_str);

    if (imm) {
        /* activity.getWindow().getDecorView().getWindowToken() */
        jclass act_cls = (*env)->GetObjectClass(env, activity);
        jmethodID get_win = (*env)->GetMethodID(env, act_cls, "getWindow",
                                                "()Landroid/view/Window;");
        jobject window = (*env)->CallObjectMethod(env, activity, get_win);

        jclass win_cls = (*env)->GetObjectClass(env, window);
        jmethodID get_dv = (*env)->GetMethodID(env, win_cls, "getDecorView",
                                               "()Landroid/view/View;");
        jobject decor = (*env)->CallObjectMethod(env, window, get_dv);

        jclass view_cls = (*env)->FindClass(env, "android/view/View");
        jmethodID get_tok = (*env)->GetMethodID(env, view_cls, "getWindowToken",
                                                "()Landroid/os/IBinder;");
        jobject token = (*env)->CallObjectMethod(env, decor, get_tok);

        jclass imm_cls = (*env)->FindClass(
            env, "android/view/inputmethod/InputMethodManager");
        jmethodID hide =
            (*env)->GetMethodID(env, imm_cls, "hideSoftInputFromWindow",
                                "(Landroid/os/IBinder;I)Z");
        if (hide && token)
            (*env)->CallBooleanMethod(env, imm, hide, token, 0);

        (*env)->DeleteLocalRef(env, imm_cls);
        (*env)->DeleteLocalRef(env, token);
        (*env)->DeleteLocalRef(env, view_cls);
        (*env)->DeleteLocalRef(env, decor);
        (*env)->DeleteLocalRef(env, win_cls);
        (*env)->DeleteLocalRef(env, window);
        (*env)->DeleteLocalRef(env, act_cls);
        (*env)->DeleteLocalRef(env, imm);
    }

    (*env)->DeleteLocalRef(env, ims_str);
    (*env)->DeleteLocalRef(env, ctx_cls);
    if ((*env)->ExceptionCheck(env))
        (*env)->ExceptionClear(env);

    n_log(LOG_INFO, "android_hide_keyboard: hideSoftInputFromWindow called");
}
#endif /* ALLEGRO_ANDROID */

/**
 * \brief Compute the effective font scale for the current display.
 *
 * Combines the display transform scale with the ratio of the current
 * virtual canvas to the default 600x900 canvas.  On Android the device
 * pixel density is factored in so text remains readable on high-DPI screens.
 *
 * \param gm  Game context (provides scale and display dimensions).
 * \return    Effective scale factor to pass to blockblaster_reload_font().
 */
float blockblaster_font_effective_scale(const GameContext *gm)
{
    float sx = (float) WIN_W / (float) WIN_W_DEFAULT;
    float sy = (float) WIN_H / (float) WIN_H_DEFAULT;
    float s = (sx < sy ? sx : sy);
#ifdef ALLEGRO_ANDROID
    float density_scale =
        gm->scale * blockblaster_android_display_density() * 0.65f;
    float ratio_scale = gm->scale * s * 0.65f;
    return density_scale > ratio_scale ? density_scale : ratio_scale;
#else
    return gm->scale * s;
#endif
}

/**
 * \brief Destroy the current font and load a new one at the given scale.
 *
 * If the TTF font cannot be loaded, the Allegro built-in bitmap font is
 * returned as a fallback.  The minimum rendered size is clamped to 8 pixels.
 *
 * \param old_font   Previous font to destroy (may be NULL on first call).
 * \param font_path  Full path to the TTF font file.
 * \param scale      Scale factor; the base size is 26 px multiplied by this.
 * \return           Newly loaded ALLEGRO_FONT pointer.
 */
ALLEGRO_FONT *blockblaster_reload_font(ALLEGRO_FONT *old_font,
                                       const char *font_path, float scale)
{
    if (old_font)
        al_destroy_font(old_font);
    int size = (int) (26.0f * scale);
    if (size < 8)
        size = 8;
    ALLEGRO_FONT *f = al_load_ttf_font(font_path, size, 0);
    if (!f)
        f = al_create_builtin_font();
    return f;
}

/* ======================================================================== */
/* Theme                                                                     */
/* ======================================================================== */

/**
 * \brief Populate the theme palette with predefined fill/stroke colour pairs.
 *
 * Called once at startup.  Each theme pairs a bright fill colour with a
 * dark stroke so that pieces and grid cells stand out against the dark
 * background.
 *
 * \param out  Array of THEMES_COUNT Theme structs to fill.
 */
void blockblaster_init_themes(Theme out[THEMES_COUNT])
{
    out[0] = (Theme) {al_map_rgb(120, 190, 255), al_map_rgb(18, 18, 26)};
    out[1] = (Theme) {al_map_rgb(255, 220, 110), al_map_rgb(26, 20, 14)};
    out[2] = (Theme) {al_map_rgb(160, 240, 170), al_map_rgb(18, 26, 18)};
    out[3] = (Theme) {al_map_rgb(255, 140, 160), al_map_rgb(26, 18, 22)};
    out[4] = (Theme) {al_map_rgb(190, 160, 255), al_map_rgb(20, 18, 26)};
    out[5] = (Theme) {al_map_rgb(255, 180, 120), al_map_rgb(26, 20, 18)};
    out[6] = (Theme) {al_map_rgb(140, 240, 240), al_map_rgb(16, 24, 26)};
    out[7] = (Theme) {al_map_rgb(240, 240, 140), al_map_rgb(26, 26, 18)};
}

/**
 * \brief Pick a random theme from the game's theme table.
 *
 * \param gm  Game context containing the initialised theme_table.
 * \return    A copy of a randomly selected Theme.
 */
Theme blockblaster_random_theme(GameContext *gm)
{
    int idx = blockblaster_irand(0, THEMES_COUNT - 1);
    return gm->theme_table[idx];
}

/* ======================================================================== */
/* Grid                                                                      */
/* ======================================================================== */

/**
 * \brief Reset every cell in the grid to unoccupied with no theme.
 *
 * \param g  Grid to clear.
 */
void blockblaster_grid_clear(Grid *g)
{
    for (int y = 0; y < GRID_H; y++)
        for (int x = 0; x < GRID_W; x++) {
            g->occ[y][x] = false;
            g->has_theme[y][x] = false;
            g->cell_theme[y][x] = (Theme) {0};
        }
}

/**
 * \brief Test whether a shape occupies the cell at (x, y).
 *
 * Out-of-bounds coordinates return false.
 *
 * \param s  Shape to query.
 * \param x  Column within the shape.
 * \param y  Row within the shape.
 * \return   true if the cell is filled.
 */
bool blockblaster_shape_cell(const Shape *s, int x, int y)
{
    if (x < 0 || y < 0 || x >= s->w || y >= s->h)
        return false;
    return s->cells[y][x];
}

/**
 * \brief Test whether shape s can be placed at grid position (gx, gy).
 *
 * Returns false if any filled cell of the shape would land outside the
 * grid or on an already-occupied cell.
 *
 * \param g   Current grid state.
 * \param s   Shape to test.
 * \param gx  Target column for the shape's top-left corner.
 * \param gy  Target row for the shape's top-left corner.
 * \return    true if placement is valid.
 */
bool blockblaster_can_place_at(const Grid *g, const Shape *s, int gx, int gy)
{
    for (int sy = 0; sy < s->h; sy++) {
        for (int sx = 0; sx < s->w; sx++) {
            if (!blockblaster_shape_cell(s, sx, sy))
                continue;
            int x = gx + sx;
            int y = gy + sy;
            if (x < 0 || y < 0 || x >= GRID_W || y >= GRID_H)
                return false;
            if (g->occ[y][x])
                return false;
        }
    }
    return true;
}

/**
 * \brief Scan the entire grid for at least one valid placement of shape s.
 *
 * \param g  Current grid state.
 * \param s  Shape to test.
 * \return   true if at least one position allows the shape to be placed.
 */
bool blockblaster_any_valid_placement(const Grid *g, const Shape *s)
{
    for (int gy = 0; gy < GRID_H; gy++)
        for (int gx = 0; gx < GRID_W; gx++)
            if (blockblaster_can_place_at(g, s, gx, gy))
                return true;
    return false;
}

/**
 * \brief Stamp shape s onto the grid at (gx, gy) with the given theme.
 *
 * Each filled cell of the shape sets the corresponding grid cell to
 * occupied and assigns the colour theme.  Caller must ensure the placement
 * is valid (see blockblaster_can_place_at()).
 *
 * \param g      Grid to modify.
 * \param s      Shape to place.
 * \param gx     Column for the shape's top-left corner.
 * \param gy     Row for the shape's top-left corner.
 * \param theme  Colour theme applied to the newly occupied cells.
 */
void blockblaster_place_shape(Grid *g, const Shape *s, int gx, int gy,
                              Theme theme)
{
    for (int sy = 0; sy < s->h; sy++) {
        for (int sx = 0; sx < s->w; sx++) {
            if (!blockblaster_shape_cell(s, sx, sy))
                continue;
            int x = gx + sx;
            int y = gy + sy;
            if (x >= 0 && y >= 0 && x < GRID_W && y < GRID_H) {
                g->occ[y][x] = true;
                g->cell_theme[y][x] = theme;
                g->has_theme[y][x] = true;
            }
        }
    }
}

/* Return true if every cell in row y is occupied. */
static bool is_row_full(bool grid[GRID_H_MAX][GRID_W_MAX], int y)
{
    for (int x = 0; x < GRID_W; x++)
        if (!grid[y][x])
            return false;
    return true;
}

/* Return true if every cell in column x is occupied. */
static bool is_col_full(bool grid[GRID_H_MAX][GRID_W_MAX], int x)
{
    for (int y = 0; y < GRID_H; y++)
        if (!grid[y][x])
            return false;
    return true;
}

/**
 * \brief Build a boolean mask of cells that belong to fully completed rows
 *        or columns.
 *
 * \param g         Current grid state.
 * \param out_mask  Output mask; true for every cell in a full row or column.
 * \return          Total number of full lines (rows + columns).
 */
int blockblaster_build_clear_mask(const Grid *g,
                                  bool out_mask[GRID_H_MAX][GRID_W_MAX])
{
    bool full_row[GRID_H_MAX] = {0};
    bool full_col[GRID_W_MAX] = {0};

    for (int y = 0; y < GRID_H; y++)
        full_row[y] = is_row_full((bool (*)[GRID_W_MAX]) g->occ, y);
    for (int x = 0; x < GRID_W; x++)
        full_col[x] = is_col_full((bool (*)[GRID_W_MAX]) g->occ, x);

    int lines = 0;
    for (int y = 0; y < GRID_H; y++)
        if (full_row[y])
            lines++;
    for (int x = 0; x < GRID_W; x++)
        if (full_col[x])
            lines++;

    for (int y = 0; y < GRID_H; y++)
        for (int x = 0; x < GRID_W; x++)
            out_mask[y][x] = (full_row[y] || full_col[x]);
    return lines;
}

/**
 * \brief Count how many occupied cells are flagged by the clear mask.
 *
 * \param g     Current grid state.
 * \param mask  Boolean clear mask (from blockblaster_build_clear_mask()).
 * \return      Number of occupied cells under the mask.
 */
int blockblaster_count_cells_in_mask(const Grid *g,
                                     bool mask[GRID_H_MAX][GRID_W_MAX])
{
    int c = 0;
    for (int y = 0; y < GRID_H; y++)
        for (int x = 0; x < GRID_W; x++)
            if (mask[y][x] && g->occ[y][x])
                c++;
    return c;
}

/**
 * \brief Remove all cells marked by the clear mask from the grid.
 *
 * Sets occ and has_theme to false for each flagged cell.
 *
 * \param g     Grid to modify.
 * \param mask  Boolean clear mask.
 */
void blockblaster_apply_clear_mask(Grid *g, bool mask[GRID_H_MAX][GRID_W_MAX])
{
    for (int y = 0; y < GRID_H; y++)
        for (int x = 0; x < GRID_W; x++)
            if (mask[y][x]) {
                g->occ[y][x] = false;
                g->has_theme[y][x] = false;
            }
}

/* ======================================================================== */
/* Bag randomizer                                                            */
/* ======================================================================== */

/**
 * \brief Pick a shape index weighted by the current difficulty curve.
 *
 * Uses a linear interpolation between easy-biased and hard-biased weight
 * distributions based on the current score.  At score 0 easy shapes (low
 * index) are strongly favoured; at DIFFICULTY_MAX_SCORE hard shapes (high
 * index) are favoured.  Every shape always retains at least
 * MIN_DIFFICULTY_WEIGHT probability.
 *
 * \param score  Current player score used to compute difficulty.
 * \return       Index into the SHAPES[] array.
 */
static int weighted_shape_index(int score)
{
    float t = (float) score / (float) DIFFICULTY_MAX_SCORE;
    if (t < 0.0f)
        t = 0.0f;
    if (t > 1.0f)
        t = 1.0f;

    float weights[128];
    float total = 0.0f;
    for (int i = 0; i < SHAPES_COUNT; i++) {
        float d =
            (SHAPES_COUNT > 1) ? (float) i / (float) (SHAPES_COUNT - 1) : 0.5f;
        float w = MIN_DIFFICULTY_WEIGHT + (1.0f - MIN_DIFFICULTY_WEIGHT) *
                                              ((1.0f - d) * (1.0f - t) + d * t);
        weights[i] = w;
        total += w;
    }

    float r = blockblaster_frand(0.0f, total);
    float acc = 0.0f;
    for (int i = 0; i < SHAPES_COUNT; i++) {
        acc += weights[i];
        if (r < acc)
            return i;
    }
    return SHAPES_COUNT - 1;
}

/* Fill the bag with BAG_SIZE shape indices using the weighted picker, then
   shuffle so consecutive draws from the same bag are randomised. */
static void bag_refill(GameContext *gm)
{
    gm->bag_len = BAG_SIZE;
    for (int i = 0; i < gm->bag_len; i++)
        gm->bag[i] = weighted_shape_index(gm->score);
    blockblaster_shuffle_ints(gm->bag, gm->bag_len);
    gm->bag_pos = 0;
}

/* Draw the next shape index from the bag, refilling when exhausted. */
static int bag_next_shape_index(GameContext *gm)
{
    if (gm->bag_len <= 0 || gm->bag_pos >= gm->bag_len)
        bag_refill(gm);
    return gm->bag[gm->bag_pos++];
}

/* ======================================================================== */
/* Piece / tray                                                              */
/* ======================================================================== */

/**
 * \brief Assign new shapes (and themes) to all tray slots.
 *
 * In theme_mode 1 all pieces share a single random theme; otherwise each
 * piece gets its own.  Shapes are drawn from the bag randomizer.
 *
 * \param gm  Game context.
 */
void blockblaster_refill_tray(GameContext *gm)
{
    if (gm->theme_mode == 1)
        gm->set_theme = blockblaster_random_theme(gm);

    for (int i = 0; i < PIECES_PER_SET; i++) {
        gm->tray[i].used = false;
        int si = bag_next_shape_index(gm);
        gm->tray[i].shape = SHAPES[si];
        if (gm->theme_mode == 1)
            gm->tray[i].theme = gm->set_theme;
        else
            gm->tray[i].theme = blockblaster_random_theme(gm);
    }
}

/**
 * \brief Check whether every tray slot has been placed on the grid.
 *
 * \param gm  Game context.
 * \return    true if all pieces are used.
 */
bool blockblaster_tray_all_used(const GameContext *gm)
{
    for (int i = 0; i < PIECES_PER_SET; i++)
        if (!gm->tray[i].used)
            return false;
    return true;
}

/**
 * \brief Check whether none of the remaining tray pieces can be placed.
 *
 * Used to detect game-over: if no unused piece has a valid placement the
 * game ends.
 *
 * \param gm  Game context.
 * \return    true if no remaining piece fits on the grid.
 */
bool blockblaster_none_placeable(const GameContext *gm)
{
    for (int i = 0; i < PIECES_PER_SET; i++) {
        if (gm->tray[i].used)
            continue;
        if (blockblaster_any_valid_placement(&gm->grid, &gm->tray[i].shape))
            return false;
    }
    return true;
}

/**
 * \brief Randomly occupy count cells on the grid (partial-fill start mode).
 *
 * Cells are chosen at random; already-occupied cells are skipped.  The
 * loop gives up after 5000 attempts to prevent infinite spinning on a
 * nearly-full grid.
 *
 * \param g      Grid to fill.
 * \param count  Desired number of cells to occupy.
 */
void blockblaster_random_fill(Grid *g, int count)
{
    int tries = 0;
    while (count > 0 && tries < 5000) {
        tries++;
        int x = blockblaster_irand(0, GRID_W - 1);
        int y = blockblaster_irand(0, GRID_H - 1);
        if (!g->occ[y][x]) {
            g->occ[y][x] = true;
            count--;
        }
    }
}

/**
 * \brief Compute the bounding rectangle of the i-th tray slot.
 *
 * \param i   Tray slot index (0 .. PIECES_PER_SET-1).
 * \param x1  Output: left edge (virtual pixels).
 * \param y1  Output: top edge (virtual pixels).
 * \param x2  Output: right edge (virtual pixels).
 * \param y2  Output: bottom edge (virtual pixels).
 */
void blockblaster_tray_piece_rect(int i, float *x1, float *y1, float *x2,
                                  float *y2)
{
    float bx = TRAY_X + i * (TRAY_BOX + TRAY_BOX_GAP);
    float by = TRAY_Y;
    *x1 = bx;
    *y1 = by;
    *x2 = bx + TRAY_BOX;
    *y2 = by + TRAY_BOX;
}

/**
 * \brief Determine which cell of the shape the player grabbed.
 *
 * Converts a click position local to the tray slot rectangle into the
 * shape cell that was closest to the click.  If the click lands directly
 * on a filled cell that cell is returned; otherwise the nearest filled
 * cell is chosen (Euclidean distance).
 *
 * \param s        Shape being picked up.
 * \param rect_w   Width of the tray slot rectangle.
 * \param rect_h   Height of the tray slot rectangle.
 * \param local_x  X position of the click relative to the slot's top-left.
 * \param local_y  Y position of the click relative to the slot's top-left.
 * \param out_sx   Output: shape column of the grabbed cell.
 * \param out_sy   Output: shape row of the grabbed cell.
 */
void blockblaster_compute_grab_cell(const Shape *s, float rect_w, float rect_h,
                                    float local_x, float local_y, int *out_sx,
                                    int *out_sy)
{
    float pc = TRAY_BOX / 9.0f;
    float pw = s->w * pc;
    float ph = s->h * pc;
    float px = (rect_w - pw) * 0.5f;
    float py = (rect_h - ph) * 0.5f;

    float rx = local_x - px;
    float ry = local_y - py;

    int sx = (int) floorf(rx / pc);
    int sy = (int) floorf(ry / pc);

    if (sx >= 0 && sy >= 0 && sx < s->w && sy < s->h &&
        blockblaster_shape_cell(s, sx, sy)) {
        *out_sx = sx;
        *out_sy = sy;
        return;
    }

    float best_d2 = 1e30f;
    int best_x = 0, best_y = 0;

    for (int y = 0; y < s->h; y++) {
        for (int x = 0; x < s->w; x++) {
            if (!blockblaster_shape_cell(s, x, y))
                continue;
            float cell_cx = (x + 0.5f) * pc;
            float cell_cy = (y + 0.5f) * pc;
            float dx = cell_cx - rx;
            float dy = cell_cy - ry;
            float d2 = dx * dx + dy * dy;
            if (d2 < best_d2) {
                best_d2 = d2;
                best_x = x;
                best_y = y;
            }
        }
    }
    *out_sx = best_x;
    *out_sy = best_y;
}

/* ======================================================================== */
/* Score                                                                     */
/* ======================================================================== */

/**
 * \brief Calculate and apply the score for a single placement move.
 *
 * Awards points for placed cells, cleared cells, line bonuses and multi-line
 * bonuses.  Maintains the combo counter: consecutive clearing moves increase
 * the multiplier; three consecutive non-clearing moves reset it.
 *
 * \param gm             Game context (score, combo state updated in-place).
 * \param placed_cells   Number of cells the placed shape occupies.
 * \param lines_cleared  Number of full rows + columns cleared this move.
 * \param cleared_cells  Total occupied cells removed by those lines.
 * \param out_clear_gain If non-NULL, receives the clear-only portion of the
 *                       score gain (before placement points).
 * \param out_mult       If non-NULL, receives the effective multiplier used.
 * \return               Total score gained this move (placed + cleared).
 */
int blockblaster_score_move(GameContext *gm, int placed_cells,
                            int lines_cleared, int cleared_cells,
                            int *out_clear_gain, float *out_mult)
{
    int gained_total = 0;
    int clear_gain = 0;
    float mult = 1.0f;

    int place_gain = placed_cells * SCORE_PER_PLACED_CELL;
    gained_total += place_gain;

    if (lines_cleared > 0) {
        gm->combo += lines_cleared;
        if (gm->combo > gm->highest_combo)
            gm->highest_combo = gm->combo;
        gm->combo_miss = 0;

        mult = 1.0f + (float) gm->combo;
        if (mult > MAX_MULTIPLIER)
            mult = MAX_MULTIPLIER;

        int base_clear = cleared_cells * SCORE_PER_CLEARED_CELL;
        int line_bonus = lines_cleared * SCORE_PER_LINE_BONUS;
        int multi_bonus = (lines_cleared > 1)
                              ? (SCORE_MULTI_LINE_BONUS * (lines_cleared - 1))
                              : 0;

        int subtotal = base_clear + line_bonus + multi_bonus;
        clear_gain = (int) roundf((float) subtotal * mult);
        gained_total += clear_gain;
    } else {
        if (gm->combo > 0) {
            gm->combo_miss += 1;
            if (gm->combo_miss > 3) {
                gm->combo = 0;
                gm->combo_miss = 0;
                gm->last_move_mult = 1.0f;
                mult = 1.0f;
            } else {
                mult = gm->last_move_mult;
            }
        } else {
            gm->last_move_mult = 1.0f;
            mult = 1.0f;
        }
    }
    if (out_clear_gain)
        *out_clear_gain = clear_gain;
    if (out_mult)
        *out_mult = mult;
    gm->last_move_mult = mult;
    gm->score += gained_total;
    if (gm->score > gm->high_score)
        gm->high_score = gm->score;
    return gained_total;
}

/* ======================================================================== */
/* Animation                                                                 */
/* ======================================================================== */

/**
 * \brief Start the clear-flash animation for the cells in mask.
 *
 * Copies the mask into gm->pending_clear and sets the clearing flag so
 * that input is blocked and the flash timer begins counting down.
 *
 * \param gm    Game context.
 * \param mask  Boolean mask of cells to clear after the animation.
 */
void blockblaster_begin_clear(GameContext *gm,
                              bool mask[GRID_H_MAX][GRID_W_MAX])
{
    gm->clearing = true;
    gm->clear_t = CLEAR_FLASH_TIME;
    for (int y = 0; y < GRID_H; y++)
        for (int x = 0; x < GRID_W; x++)
            gm->pending_clear[y][x] = mask[y][x];
}

/**
 * \brief Complete the clear animation: remove flagged cells and check for
 *        game-over.
 *
 * Called when clear_t reaches zero.  Applies the pending clear mask to the
 * grid, resets animation state, and triggers game-over if no remaining
 * piece can be placed.
 *
 * \param gm  Game context.
 */
void blockblaster_finish_clear(GameContext *gm)
{
    blockblaster_apply_clear_mask(&gm->grid, gm->pending_clear);
    for (int y = 0; y < GRID_H; y++)
        for (int x = 0; x < GRID_W; x++)
            gm->pending_clear[y][x] = false;

    gm->clearing = false;
    gm->clear_t = 0.0f;

    if (gm->state == STATE_PLAY && blockblaster_none_placeable(gm)) {
        n_log(LOG_INFO, "Game over (post-clear): none of the offered pieces "
                        "can be placed.");
        blockblaster_set_gameover(gm);
    }
}

/**
 * \brief Begin the return-to-tray animation for a piece that failed to drop.
 *
 * Records the starting position (current mouse) and the target position
 * (centre of the tray slot) so the piece smoothly animates back.
 *
 * \param gm          Game context.
 * \param tray_index  Tray slot index of the returning piece.
 */
void blockblaster_start_return(GameContext *gm, int tray_index)
{
    float x1, y1, x2, y2;
    blockblaster_tray_piece_rect(tray_index, &x1, &y1, &x2, &y2);

    gm->returning = true;
    gm->return_index = tray_index;
    gm->return_t = RETURN_TIME;
    gm->return_start_x = gm->mouse_x;
#ifdef ALLEGRO_ANDROID
    gm->return_start_y =
        gm->mouse_y -
        ANDROID_PIECE_Y_OFFSET * blockblaster_android_display_density();
#else
    gm->return_start_y = gm->mouse_y;
#endif
    gm->return_end_x = (x1 + x2) * 0.5f;
    gm->return_end_y = (y1 + y2) * 0.5f;

    blockblaster_clear_predicted(gm);
}

/**
 * \brief Reset the predicted-clear highlight arrays to all-false.
 *
 * \param gm  Game context.
 */
void blockblaster_clear_predicted(GameContext *gm)
{
    gm->has_predicted_clear = false;
    for (int y = 0; y < GRID_H; y++)
        gm->pred_full_row[y] = false;
    for (int x = 0; x < GRID_W; x++)
        gm->pred_full_col[x] = false;
}

/**
 * \brief Predict which rows and columns would be cleared if piece p were
 *        placed at (gx, gy).
 *
 * Builds a temporary copy of the grid with the piece stamped on, then
 * scans for full rows/columns.  The results are stored in
 * gm->pred_full_row[] and gm->pred_full_col[] for the renderer to
 * highlight.
 *
 * \param gm  Game context (predicted arrays updated in-place).
 * \param p   Piece being considered for placement.
 * \param gx  Grid column of the piece's top-left corner.
 * \param gy  Grid row of the piece's top-left corner.
 */
void blockblaster_compute_predicted_clear(GameContext *gm, const Piece *p,
                                          int gx, int gy)
{
    bool temp[GRID_H_MAX][GRID_W_MAX];
    for (int y = 0; y < GRID_H; y++)
        for (int x = 0; x < GRID_W; x++)
            temp[y][x] = gm->grid.occ[y][x];

    for (int sy = 0; sy < p->shape.h; sy++) {
        for (int sx = 0; sx < p->shape.w; sx++) {
            if (!blockblaster_shape_cell(&p->shape, sx, sy))
                continue;
            int x = gx + sx;
            int y = gy + sy;
            if (x >= 0 && y >= 0 && x < GRID_W && y < GRID_H)
                temp[y][x] = true;
        }
    }

    gm->has_predicted_clear = true;
    for (int y = 0; y < GRID_H; y++)
        gm->pred_full_row[y] = is_row_full(temp, y);
    for (int x = 0; x < GRID_W; x++)
        gm->pred_full_col[x] = is_col_full(temp, x);
}

/* ======================================================================== */
/* Input / drop                                                              */
/* ======================================================================== */

/**
 * \brief Recalculate the ghost preview and predicted-clear overlay.
 *
 * Converts the current mouse position to grid coordinates, snaps to the
 * grab anchor, and checks placement validity.  If valid, the predicted
 * clear mask is also updated.
 *
 * \param gm  Game context (preview state updated in-place).
 */
void blockblaster_update_drop_preview(GameContext *gm)
{
    gm->can_drop_preview = false;
    gm->preview_cell_x = -999;
    gm->preview_cell_y = -999;

    if (!gm->dragging) {
        blockblaster_clear_predicted(gm);
        return;
    }

    Piece *p = &gm->tray[gm->dragging_index];
    if (p->used) {
        blockblaster_clear_predicted(gm);
        return;
    }

    float gx1 = GRID_X, gy1 = GRID_Y;
    float gx2 = GRID_X + GRID_W * CELL;
    float gy2 = GRID_Y + GRID_H * CELL;

    float my = gm->mouse_y;
#ifdef ALLEGRO_ANDROID
    my -= ANDROID_PIECE_Y_OFFSET * blockblaster_android_display_density();
#endif

    if (gm->mouse_x < gx1 || gm->mouse_x >= gx2 || my < gy1 || my >= gy2) {
        blockblaster_clear_predicted(gm);
        return;
    }

    int mouse_gx = (int) floorf((gm->mouse_x - GRID_X) / CELL);
    int mouse_gy = (int) floorf((my - GRID_Y) / CELL);

    int gx = mouse_gx - gm->grab_sx;
    int gy = mouse_gy - gm->grab_sy;

    gm->preview_cell_x = gx;
    gm->preview_cell_y = gy;
    gm->can_drop_preview =
        blockblaster_can_place_at(&gm->grid, &p->shape, gx, gy);

    if (gm->can_drop_preview)
        blockblaster_compute_predicted_clear(gm, p, gx, gy);
    else
        blockblaster_clear_predicted(gm);
}

/**
 * \brief Attempt to place the currently dragged piece onto the grid.
 *
 * If the preview position is valid the piece is stamped, score is
 * calculated, particles/popups are spawned, and clearing begins if any
 * lines are completed.  On an invalid drop the piece animates back to
 * the tray.
 *
 * \param gm  Game context.
 */
void blockblaster_try_drop(GameContext *gm)
{
    if (!gm->dragging)
        return;
    if (gm->clearing)
        return;

    float old_mult = gm->last_move_mult;

    int drop_index = gm->dragging_index;
    Piece *p = &gm->tray[drop_index];
    gm->dragging = false;

    if (p->used) {
        blockblaster_play_sfx(sfx_send_to_tray, gm);
        return;
    }
    if (!gm->can_drop_preview) {
        blockblaster_play_sfx(sfx_send_to_tray, gm);
        blockblaster_start_return(gm, drop_index);
        return;
    }

    blockblaster_play_sfx(sfx_place, gm);

    blockblaster_place_shape(&gm->grid, &p->shape, gm->preview_cell_x,
                             gm->preview_cell_y, p->theme);

    int placed_cells = 0;
    for (int sy = 0; sy < p->shape.h; sy++) {
        for (int sx = 0; sx < p->shape.w; sx++) {
            if (!blockblaster_shape_cell(&p->shape, sx, sy))
                continue;
            placed_cells++;
            int gx = gm->preview_cell_x + sx;
            int gy = gm->preview_cell_y + sy;
            if (gx >= 0 && gy >= 0 && gx < GRID_W && gy < GRID_H)
                gm->pop_t[gy][gx] = PLACE_POP_TIME;
        }
    }

    bool mask[GRID_H_MAX][GRID_W_MAX];
    int lines = blockblaster_build_clear_mask(&gm->grid, mask);
    int cleared_cells = 0;
    if (lines > 0) {
        cleared_cells = blockblaster_count_cells_in_mask(&gm->grid, mask);
        blockblaster_play_sfx(sfx_break_lines, gm);
    }

    int clear_gain = 0;
    float applied_mult = 1.0f;
    blockblaster_score_move(gm, placed_cells, lines, cleared_cells, &clear_gain,
                            &applied_mult);

    if (lines > 0 && applied_mult > old_mult + 0.001f)
        blockblaster_start_combo_popup(gm, applied_mult, p->theme);

    if (lines > 0) {
        int spawned = 0;
        for (int y = 0; y < GRID_H; y++) {
            for (int x = 0; x < GRID_W; x++) {
                if (!mask[y][x] || !gm->grid.occ[y][x])
                    continue;
                float cx = GRID_X + x * CELL + CELL * 0.5f;
                float cy = GRID_Y + y * CELL + CELL * 0.5f;
                int n = PARTICLES_PER_CLEARED_CELL;
                if (spawned + n > PARTICLES_CAP_PER_CLEAR)
                    n = PARTICLES_CAP_PER_CLEAR - spawned;
                if (n <= 0)
                    break;
                blockblaster_spawn_particles(gm, cx, cy, p->theme, n);
                spawned += n;
            }
            if (spawned >= PARTICLES_CAP_PER_CLEAR)
                break;
        }
        if (clear_gain > 0) {
            float bx = GRID_X + (float) GRID_W * CELL;
            float by = GRID_Y + (float) GRID_H * CELL + 5.0f;
            blockblaster_spawn_bonus_popup(gm, bx, by, clear_gain, applied_mult,
                                           p->theme);
            blockblaster_spawn_particles(gm, bx, by, p->theme, BONUS_PARTICLES);
        }
        blockblaster_begin_clear(gm, mask);
    }

    if (lines >= 2) {
        gm->shake_t = SHAKE_TIME;
        gm->shake_strength = SHAKE_STRENGTH * (1.0f + (lines - 2) * 0.35f) *
                             SHAKE_MULTILINE_BOOST;
    } else if (lines == 1) {
        gm->shake_t = SHAKE_TIME * 0.7f;
        gm->shake_strength = SHAKE_STRENGTH * 0.6f;
    }

    p->used = true;

    if (blockblaster_tray_all_used(gm))
        blockblaster_refill_tray(gm);

    if (!gm->clearing && blockblaster_none_placeable(gm)) {
        n_log(LOG_INFO, "Game over: none of the offered pieces can be placed.");
        blockblaster_set_gameover(gm);
    }
}

/* ======================================================================== */
/* Particles                                                                 */
/* ======================================================================== */

/**
 * \brief Spawn count particles at (x, y) with configurable size and speed.
 *
 * Each particle is given a random launch angle, speed in [speed_min,
 * speed_max], and size in [size_min, size_max] scaled by the current font
 * scale.  If the particle pool is full, remaining particles are silently
 * dropped.
 *
 * \param gm         Game context (particle pool updated in-place).
 * \param x          Horizontal spawn centre (virtual pixels).
 * \param y          Vertical spawn centre (virtual pixels).
 * \param t          Colour theme for the particles.
 * \param count      Number of particles to spawn.
 * \param size_min   Minimum particle radius before scaling.
 * \param size_max   Maximum particle radius before scaling.
 * \param speed_min  Minimum launch speed (pixels/second).
 * \param speed_max  Maximum launch speed (pixels/second).
 */
void blockblaster_spawn_particles_scaled(GameContext *gm, float x, float y,
                                         Theme t, int count, float size_min,
                                         float size_max, float speed_min,
                                         float speed_max)
{
    for (int k = 0; k < count; k++) {
        int idx = -1;
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (!gm->particles[i].alive) {
                idx = i;
                break;
            }
        }
        if (idx < 0)
            return;

        Particle *p = &gm->particles[idx];
        float ang = blockblaster_frand(0.0f, 6.2831853f);
        float spd = blockblaster_frand(speed_min, speed_max);

        p->x = x + blockblaster_frand(-6.0f, 6.0f);
        p->y = y + blockblaster_frand(-6.0f, 6.0f);
        p->vx = cosf(ang) * spd;
        p->vy = sinf(ang) * spd - blockblaster_frand(10.0f, 90.0f);
        p->life0 = p->life =
            blockblaster_frand(PARTICLE_LIFE_MIN, PARTICLE_LIFE_MAX);
        float sc = blockblaster_font_effective_scale(gm);
        if (sc <= 0.0f)
            sc = 1.0f;
        p->size = blockblaster_frand(size_min, size_max) * sc;
        p->col = t.fill;
        p->alive = true;
    }
}

/**
 * \brief Spawn count particles with default size and speed ranges.
 *
 * Convenience wrapper around blockblaster_spawn_particles_scaled() using
 * the standard particle size (3.5 - 7.0) and speed (PARTICLE_SPEED_MIN -
 * PARTICLE_SPEED_MAX) ranges.
 *
 * \param gm     Game context.
 * \param x      Horizontal spawn centre (virtual pixels).
 * \param y      Vertical spawn centre (virtual pixels).
 * \param t      Colour theme for the particles.
 * \param count  Number of particles to spawn.
 */
void blockblaster_spawn_particles(GameContext *gm, float x, float y, Theme t,
                                  int count)
{
    blockblaster_spawn_particles_scaled(gm, x, y, t, count, 3.5f, 7.0f,
                                        PARTICLE_SPEED_MIN, PARTICLE_SPEED_MAX);
}

/**
 * \brief Spawn an animated "+N points" popup at (x, y).
 *
 * The popup drifts upward and fades out over BONUS_LIFE seconds.  If the
 * popup pool is full the request is silently ignored.
 *
 * \param gm      Game context.
 * \param x       Horizontal position (virtual pixels).
 * \param y       Vertical position (virtual pixels).
 * \param points  Point value to display.
 * \param mult    Multiplier value to display alongside the points.
 * \param t       Colour theme for the popup text.
 */
void blockblaster_spawn_bonus_popup(GameContext *gm, float x, float y,
                                    int points, float mult, Theme t)
{
    for (int i = 0; i < MAX_BONUS_POPUPS; i++) {
        if (!gm->bonus_popups[i].alive) {
            gm->bonus_popups[i].alive = true;
            gm->bonus_popups[i].x = x;
            gm->bonus_popups[i].y = y;
            gm->bonus_popups[i].vy = -BONUS_RISE_SPEED;
            gm->bonus_popups[i].life0 = gm->bonus_popups[i].life = BONUS_LIFE;
            gm->bonus_popups[i].points = points;
            gm->bonus_popups[i].mult = mult;
            gm->bonus_popups[i].theme = t;
            return;
        }
    }
}

/**
 * \brief Show the centred "COMBO xN" popup and trigger a particle burst.
 *
 * The popup scales up with an ease-out animation and drifts across the
 * grid.  The particle burst intensity scales with the multiplier value.
 *
 * \param gm     Game context.
 * \param mult   Current combo multiplier value.
 * \param theme  Colour theme for the popup and particles.
 */
void blockblaster_start_combo_popup(GameContext *gm, float mult, Theme theme)
{
    gm->combo_popup.alive = true;
    gm->combo_popup.life0 = gm->combo_popup.life = COMBO_POP_LIFE;
    gm->combo_popup.scale = 0.35f;
    gm->combo_popup.mult = mult;
    gm->combo_popup.theme = theme;

    float grid_w_px = (float) GRID_W * CELL;
    float grid_h_px = (float) GRID_H * CELL;
    gm->combo_popup.x = GRID_X;
    gm->combo_popup.y = GRID_Y;
    gm->combo_popup.vx = grid_w_px / COMBO_POP_LIFE;
    gm->combo_popup.vy = grid_h_px / COMBO_POP_LIFE;

    snprintf(gm->combo_popup.text, sizeof(gm->combo_popup.text), "COMBO x%d",
             gm->combo);

    float mclamp = mult;
    if (mclamp < 1.0f)
        mclamp = 1.0f;
    if (mclamp > MAX_MULTIPLIER)
        mclamp = MAX_MULTIPLIER;

    int count = COMBO_POP_PARTICLES_BASE + (int) (mclamp * 10.0f);
    if (count > 220)
        count = 220;

    float sz_min = 3.5f + 0.28f * mclamp;
    float sz_max = 7.0f + 0.55f * mclamp;
    float sp_min = 80.0f + 14.0f * mclamp;
    float sp_max = 180.0f + 26.0f * mclamp;

    blockblaster_spawn_particles_scaled(gm, GRID_X, GRID_Y, theme, count,
                                        sz_min, sz_max, sp_min, sp_max);
}

/* ======================================================================== */
/* Game flow                                                                 */
/* ======================================================================== */

/**
 * \brief Transition to the game-over state and open the name editor.
 *
 * Pre-fills the player name with the last used name (or "PLAYR" as
 * default) and switches to STATE_GAMEOVER.  Score insertion is deferred
 * until the player confirms their name.  On Android the soft keyboard is
 * shown automatically.
 *
 * \param gm  Game context.
 */
void blockblaster_set_gameover(GameContext *gm)
{
    /* Pre-fill with last player name; the player can edit before confirming. */
    if (gm->player_name[0] == '\0')
        snprintf(gm->player_name, MAX_PLAYER_NAME_LEN + 1, "%s",
                 gm->last_player_name);
    if (gm->player_name[0] == '\0')
        snprintf(gm->player_name, MAX_PLAYER_NAME_LEN + 1, "PLAYR");

    gm->editing_name = true;
    gm->name_cursor = (int) strlen(gm->player_name);
    gm->state = STATE_GAMEOVER;

#ifdef ALLEGRO_ANDROID
    blockblaster_android_show_keyboard();
#endif

    /* Score insertion is deferred until the player confirms their name
     * (OK button or Enter key in the game-over overlay). */
}

/**
 * \brief Initialise a new game session.
 *
 * Applies the player's chosen grid size and tray count, clears the grid,
 * optionally pre-fills cells (mode 1), refills the tray, and resets all
 * animation and scoring state.
 *
 * \param gm    Game context (fully reset by this call).
 * \param mode  0 = empty grid, 1 = partially filled grid.
 */
void blockblaster_start_game(GameContext *gm, int mode)
{
    /* Apply chosen grid size and tray count before anything else. */
    blockblaster_apply_settings(gm);

    gm->state = STATE_PLAY;
    gm->score = 0;
    gm->combo = 0;
    gm->highest_combo = 0;
    gm->last_move_mult = 1.0f;
    gm->combo_miss = 0;
    gm->combo_popup.alive = false;

    gm->dragging = false;
    gm->dragging_index = -1;
    gm->returning = false;
    gm->return_t = 0.0f;
    gm->return_index = -1;
    blockblaster_clear_predicted(gm);

    gm->clearing = false;
    gm->clear_t = 0.0f;
    for (int y = 0; y < GRID_H; y++)
        for (int x = 0; x < GRID_W; x++) {
            gm->pending_clear[y][x] = false;
            gm->pop_t[y][x] = 0.0f;
        }

    gm->start_mode = mode;
    blockblaster_grid_clear(&gm->grid);

    if (mode == 1) {
        int fill = blockblaster_irand(FILL_MIN, FILL_MAX);
        blockblaster_random_fill(&gm->grid, fill);
        bool tmp[GRID_H_MAX][GRID_W_MAX];
        int lines = blockblaster_build_clear_mask(&gm->grid, tmp);
        if (lines > 0)
            blockblaster_apply_clear_mask(&gm->grid, tmp);
    }

    blockblaster_refill_tray(gm);

    if (blockblaster_none_placeable(gm)) {
        n_log(LOG_INFO,
              "Immediate game over: none of the offered pieces can be placed.");
        blockblaster_set_gameover(gm);
    }

    gm->shake_t = 0.0f;
    gm->shake_strength = 0.0f;
    gm->cam_x = gm->cam_y = 0.0f;

    for (int i = 0; i < MAX_PARTICLES; i++)
        gm->particles[i].alive = false;
    for (int i = 0; i < MAX_BONUS_POPUPS; i++)
        gm->bonus_popups[i].alive = false;

    gm->theme_mode = 1;
    gm->set_theme = blockblaster_random_theme(gm);

    /* Prepare player name from last saved name */
    snprintf(gm->player_name, sizeof(gm->player_name), "%s",
             gm->last_player_name);
    gm->editing_name = false;
    gm->name_cursor = 0;
}

/* ======================================================================== */
/* View                                                                      */
/* ======================================================================== */

/**
 * \brief Recalculate the display transform and virtual canvas dimensions.
 *
 * In fullscreen the virtual canvas matches the physical display so scale
 * is 1.0 and there is no letterboxing.  In windowed mode the virtual
 * canvas is the default 600x900 and a uniform scale + offset centres it
 * within the window.
 *
 * Also updates g_display_scale (used by the line-width macros) and kills
 * any active combo popup so it doesn't render at stale coordinates.
 *
 * \param gm  Game context (display dimensions, scale, offsets updated).
 */
void blockblaster_update_view_offset(GameContext *gm)
{
    if (!gm)
        return;

#ifdef ALLEGRO_ANDROID
    const bool is_fs = true;
#else
    bool is_fs = gm->is_fullscreen;
    if (!is_fs && gm->display)
        is_fs = (al_get_display_flags(gm->display) &
                 ALLEGRO_FULLSCREEN_WINDOW) != 0;
#endif

    if (is_fs) {
        WIN_W = gm->display_width;
        WIN_H = gm->display_height;
        gm->scale = 1.0f;
        gm->view_offset_x = 0.0f;
        gm->view_offset_y = 0.0f;
    } else {
        WIN_W = WIN_W_DEFAULT;
        WIN_H = WIN_H_DEFAULT;
        float sx = (float) gm->display_width / (float) WIN_W;
        float sy = (float) gm->display_height / (float) WIN_H;
        gm->scale = (sx < sy) ? sx : sy;
        if (gm->scale < 0.1f)
            gm->scale = 0.1f;
        float scaled_w = (float) WIN_W * gm->scale;
        float scaled_h = (float) WIN_H * gm->scale;
        gm->view_offset_x =
            floorf(((float) gm->display_width - scaled_w) * 0.5f);
        gm->view_offset_y =
            floorf(((float) gm->display_height - scaled_h) * 0.5f);
    }

    g_display_scale = gm->scale;
    gm->combo_popup.alive = false;
}

/**
 * \brief Convert physical screen coordinates to virtual canvas coordinates.
 *
 * Applies the inverse of the base transform (offset + scale) used for
 * rendering.
 *
 * \param gm  Game context (provides view_offset and scale).
 * \param sx  Screen X (physical pixels).
 * \param sy  Screen Y (physical pixels).
 * \param vx  Output: virtual X (may be NULL).
 * \param vy  Output: virtual Y (may be NULL).
 */
void blockblaster_screen_to_virtual(const GameContext *gm, float sx, float sy,
                                    float *vx, float *vy)
{
    float scale = (gm->scale > 0.0f) ? gm->scale : 1.0f;
    if (vx)
        *vx = (sx - gm->view_offset_x) / scale;
    if (vy)
        *vy = (sy - gm->view_offset_y) / scale;
}

/* ======================================================================== */
/* Save / load                                                               */
/* ======================================================================== */

#if defined(__EMSCRIPTEN__)
/**
 * \brief Mount the IDBFS filesystem at /save and sync from IndexedDB.
 *
 * Must be called once during startup.  Sets Module.bbSavesReady to 1
 * after the async sync completes (or immediately if IDBFS is unavailable).
 */
void blockblaster_emscripten_save_init(void)
{
    EM_ASM({
        Module.bbSavesReady = 0;
        if (!FS || !FS.filesystems || !FS.filesystems.IDBFS) {
            console.warn(
                "IDBFS not available; saves will be in-memory only (MEMFS). " +
                "Build with -sFILESYSTEM=1 and link -lidbfs.js");
            Module.bbSavesReady = 1;
            return;
        }
        if (!FS.analyzePath('/save').exists)
            FS.mkdir('/save');
        FS.mount(FS.filesystems.IDBFS, {}, '/save');
        FS.syncfs(
            true, function(err) {
                if (err)
                    console.error('syncfs(load) failed', err);
                Module.bbSavesReady = 1;
            });
    });
}

/**
 * \brief Check whether the IDBFS async mount has completed.
 * \return  true once saves are ready to read/write.
 */
bool blockblaster_emscripten_save_ready(void)
{
    return EM_ASM_INT({ return Module.bbSavesReady ? 1 : 0; }) != 0;
}

/* Flush in-memory IDBFS changes to the browser's IndexedDB store. */
static void emscripten_save_flush_internal(void)
{
    EM_ASM({
        if (Module.bbSavesReady && FS && FS.syncfs)
            FS.syncfs(
                false, function(err) {
                    if (err)
                        console.error('syncfs(save) failed', err);
                });
    });
}

/**
 * \brief Public wrapper to flush IDBFS changes to IndexedDB.
 */
void blockblaster_emscripten_save_flush(void)
{
    emscripten_save_flush_internal();
}
#endif

/**
 * \brief Persist the high-score table to disk.
 *
 * On Android writes via the Allegro standard file interface; on desktop
 * and Emscripten uses SAVE_DIR.  After writing, an IDBFS sync is issued
 * on Emscripten so the data reaches IndexedDB.
 *
 * \param gm  Game context containing the high_scores array.
 */
void blockblaster_save_high_scores(const GameContext *gm)
{
#ifdef ALLEGRO_ANDROID
    al_set_standard_file_interface();
    ALLEGRO_PATH *path = al_get_standard_path(ALLEGRO_USER_DATA_PATH);
    al_set_path_filename(path, SCORES_FILENAME);
    ALLEGRO_FILE *af = al_fopen(al_path_cstr(path, '/'), "wb");
    al_destroy_path(path);
    if (!af) {
        al_android_set_apk_file_interface();
        return;
    }
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "%d\n", gm->high_score_count);
    for (int i = 0; i < gm->high_score_count; i++) {
        if (off >= (int) sizeof(buf) - 1)
            break;
        off +=
            snprintf(buf + off, sizeof(buf) - off, "%d %d %d %ld %d %.5s\n",
                     gm->high_scores[i].grid_w, gm->high_scores[i].grid_h,
                     gm->high_scores[i].tray_count, gm->high_scores[i].score,
                     gm->high_scores[i].highest_combo, gm->high_scores[i].name);
    }
    al_fwrite(af, buf, off);
    al_fclose(af);
    al_android_set_apk_file_interface();
#else
    char path[512];
    snprintf(path, sizeof(path), "%s%s", SAVE_DIR, SCORES_FILENAME);
    FILE *f = fopen(path, "w");
    if (!f)
        return;
    fprintf(f, "%d\n", gm->high_score_count);
    for (int i = 0; i < gm->high_score_count; i++) {
        fprintf(f, "%d %d %d %ld %d %.5s\n", gm->high_scores[i].grid_w,
                gm->high_scores[i].grid_h, gm->high_scores[i].tray_count,
                gm->high_scores[i].score, gm->high_scores[i].highest_combo,
                gm->high_scores[i].name);
    }
    fclose(f);
#endif

#ifdef __EMSCRIPTEN__
    emscripten_save_flush_internal();
#endif
    n_log(LOG_INFO, "Saved %d high scores", gm->high_score_count);
}

/**
 * \brief Load the high-score table from disk.
 *
 * Supports both the current multi-entry format (grid_w grid_h tray_count
 * score combo name) and the legacy single-score format (score combo).
 * Missing or malformed files result in an empty table.
 *
 * \param gm  Game context (high_scores and high_score_count updated).
 */
void blockblaster_load_high_scores(GameContext *gm)
{
    gm->high_score_count = 0;
    memset(gm->high_scores, 0, sizeof(gm->high_scores));
    gm->high_score = 0;

#ifdef ALLEGRO_ANDROID
    al_set_standard_file_interface();
    ALLEGRO_PATH *path = al_get_standard_path(ALLEGRO_USER_DATA_PATH);
    al_set_path_filename(path, SCORES_FILENAME);
    ALLEGRO_FILE *af = al_fopen(al_path_cstr(path, '/'), "rb");
    al_destroy_path(path);
    if (!af) {
        al_android_set_apk_file_interface();
        n_log(LOG_INFO, "No scores file yet");
        return;
    }
    char buf[512] = {0};
    al_fread(af, buf, sizeof(buf) - 1);
    al_fclose(af);
    al_android_set_apk_file_interface();

    int count = 0;
    const char *p = buf;
    if (sscanf(p, "%d", &count) != 1)
        return;
    while (*p && *p != '\n')
        p++;
    if (*p)
        p++;
    if (count < 0)
        count = 0;
    if (count > MAX_HIGH_SCORES)
        count = MAX_HIGH_SCORES;
    gm->high_score_count = count;
    for (int i = 0; i < count; i++) {
        char name[MAX_PLAYER_NAME_LEN + 2] = {0};
        int gw = 10, gh = 10, tc = 4;
        /* Try new format: grid_w grid_h tray_count score combo name */
        if (sscanf(p, "%d %d %d %ld %d %5s", &gw, &gh, &tc,
                   &gm->high_scores[i].score, &gm->high_scores[i].highest_combo,
                   name) >= 6) {
            gm->high_scores[i].grid_w = gw;
            gm->high_scores[i].grid_h = gh;
            gm->high_scores[i].tray_count = tc;
        } else if (sscanf(p, "%ld %d %5s", &gm->high_scores[i].score,
                          &gm->high_scores[i].highest_combo, name) >= 2) {
            gm->high_scores[i].grid_w = 10;
            gm->high_scores[i].grid_h = 10;
            gm->high_scores[i].tray_count = 4;
        } else {
            break;
        }
        snprintf(gm->high_scores[i].name, MAX_PLAYER_NAME_LEN + 1, "%s", name);
        while (*p && *p != '\n')
            p++;
        if (*p)
            p++;
    }
#else
    char path[512];
    snprintf(path, sizeof(path), "%s%s", SAVE_DIR, SCORES_FILENAME);
    FILE *f = fopen(path, "r");
    if (!f) {
        /* Try legacy single-score format for backward compatibility */
        snprintf(path, sizeof(path), "%s%s", SAVE_DIR, HIGHSCORE_FILENAME);
        f = fopen(path, "r");
        if (!f) {
            n_log(LOG_INFO, "No scores file yet");
            return;
        }
        long hs = 0;
        int hc = 0;
        if (fscanf(f, "%ld %d", &hs, &hc) >= 1) {
            gm->high_score_count = 1;
            gm->high_scores[0].score = hs;
            gm->high_scores[0].highest_combo = hc;
            snprintf(gm->high_scores[0].name, sizeof(gm->high_scores[0].name),
                     "PLAYR");
        }
        fclose(f);
        goto done;
    }
    int count = 0;
    if (fscanf(f, "%d", &count) != 1) {
        fclose(f);
        return;
    }
    if (count < 0)
        count = 0;
    if (count > MAX_HIGH_SCORES)
        count = MAX_HIGH_SCORES;
    gm->high_score_count = count;
    for (int i = 0; i < count; i++) {
        char name[MAX_PLAYER_NAME_LEN + 2] = {0};
        int gw = 10, gh = 10, tc = 4;
        long saved_pos = ftell(f);
        /* Try new format: grid_w grid_h tray_count score combo name */
        if (fscanf(f, "%d %d %d %ld %d %5s", &gw, &gh, &tc,
                   &gm->high_scores[i].score, &gm->high_scores[i].highest_combo,
                   name) >= 6) {
            gm->high_scores[i].grid_w = gw;
            gm->high_scores[i].grid_h = gh;
            gm->high_scores[i].tray_count = tc;
        } else {
            /* Fall back to old format: score combo name */
            fseek(f, saved_pos, SEEK_SET);
            if (fscanf(f, "%ld %d %5s", &gm->high_scores[i].score,
                       &gm->high_scores[i].highest_combo, name) < 2)
                break;
            gm->high_scores[i].grid_w = 10;
            gm->high_scores[i].grid_h = 10;
            gm->high_scores[i].tray_count = 4;
        }
        snprintf(gm->high_scores[i].name, MAX_PLAYER_NAME_LEN + 1, "%s", name);
    }
    fclose(f);
done:
#endif

    /* Derive high_score from the best entry */
    if (gm->high_score_count > 0)
        gm->high_score = gm->high_scores[0].score;

    /* Clamp values */
    for (int i = 0; i < gm->high_score_count; i++) {
        if (gm->high_scores[i].score < 0)
            gm->high_scores[i].score = 0;
        if (gm->high_scores[i].highest_combo < 0)
            gm->high_scores[i].highest_combo = 0;
        if (gm->high_scores[i].name[0] == '\0')
            snprintf(gm->high_scores[i].name, sizeof(gm->high_scores[i].name),
                     "PLAYR");
    }

    n_log(LOG_INFO, "Loaded %d high scores", gm->high_score_count);
}

/**
 * \brief Insert a new entry into the high-score table (sorted descending).
 *
 * If the score qualifies for the top MAX_HIGH_SCORES the entry is
 * inserted at the correct position and lower entries are shifted down.
 * Entries beyond MAX_HIGH_SCORES are discarded.
 *
 * \param gm     Game context.
 * \param score  Score to insert.
 * \param combo  Highest combo achieved during the session.
 * \param name   Player name (up to MAX_PLAYER_NAME_LEN characters).
 */
void blockblaster_insert_high_score(GameContext *gm, long score, int combo,
                                    const char *name)
{
    /* Find insertion point (sorted descending by score) */
    int pos = gm->high_score_count;
    for (int i = 0; i < gm->high_score_count; i++) {
        if (score > gm->high_scores[i].score) {
            pos = i;
            break;
        }
    }
    if (pos >= MAX_HIGH_SCORES)
        return; /* Didn't make the top 5 */

    /* Shift lower entries down */
    int new_count = gm->high_score_count + 1;
    if (new_count > MAX_HIGH_SCORES)
        new_count = MAX_HIGH_SCORES;
    for (int i = new_count - 1; i > pos; i--)
        gm->high_scores[i] = gm->high_scores[i - 1];

    /* Insert */
    gm->high_scores[pos].grid_w = GRID_W;
    gm->high_scores[pos].grid_h = GRID_H;
    gm->high_scores[pos].tray_count = PIECES_PER_SET;
    gm->high_scores[pos].score = score;
    gm->high_scores[pos].highest_combo = combo;
    snprintf(gm->high_scores[pos].name, sizeof(gm->high_scores[pos].name), "%s",
             name);

    gm->high_score_count = new_count;

    /* Update derived high_score */
    gm->high_score = gm->high_scores[0].score;
}

/**
 * \brief Persist the sound on/off state to disk.
 *
 * \param on  true to save "sound enabled", false for "sound disabled".
 */
void blockblaster_save_sound_state(bool on)
{
#ifdef ALLEGRO_ANDROID
    al_set_standard_file_interface();
    ALLEGRO_PATH *path = al_get_standard_path(ALLEGRO_USER_DATA_PATH);
    al_set_path_filename(path, SOUND_STATE_FILENAME);
    ALLEGRO_FILE *f = al_fopen(al_path_cstr(path, '/'), "wb");
    al_destroy_path(path);
    if (!f) {
        al_android_set_apk_file_interface();
        return;
    }
    char buf[8];
    int len = snprintf(buf, sizeof(buf), "%d\n", on ? 1 : 0);
    al_fwrite(f, buf, len);
    al_fclose(f);
    al_android_set_apk_file_interface();
#else
    char path[512];
    snprintf(path, sizeof(path), "%s%s", SAVE_DIR, SOUND_STATE_FILENAME);
    FILE *f = fopen(path, "w");
    if (!f)
        return;
    fprintf(f, "%d\n", on ? 1 : 0);
    fclose(f);
#endif

#ifdef __EMSCRIPTEN__
    emscripten_save_flush_internal();
#endif
    n_log(LOG_INFO, "sound_state saved: %d", on ? 1 : 0);
}

/**
 * \brief Load the persisted sound on/off state from disk.
 *
 * If no file is found the default (on) is saved and returned.
 *
 * \return  true if sound is enabled, false otherwise.
 */
bool blockblaster_load_sound_state(void)
{
    int v = 1;

#ifdef ALLEGRO_ANDROID
    al_set_standard_file_interface();
    ALLEGRO_PATH *path = al_get_standard_path(ALLEGRO_USER_DATA_PATH);
    al_set_path_filename(path, SOUND_STATE_FILENAME);
    ALLEGRO_FILE *f = al_fopen(al_path_cstr(path, '/'), "rb");
    al_destroy_path(path);
    if (!f) {
        blockblaster_save_sound_state(true);
        return true;
    }
    char buf[8] = {0};
    al_fread(f, buf, sizeof(buf) - 1);
    al_fclose(f);
    al_android_set_apk_file_interface();
    if (sscanf(buf, "%d", &v) != 1)
        v = 1;
#else
    char path[512];
    snprintf(path, sizeof(path), "%s%s", SAVE_DIR, SOUND_STATE_FILENAME);
    FILE *f = fopen(path, "r");
    if (!f) {
        blockblaster_save_sound_state(true);
        return true;
    }
    if (fscanf(f, "%d", &v) != 1)
        v = 1;
    fclose(f);
#endif

    return v != 0;
}

/**
 * \brief Persist the last-used player name to disk.
 *
 * \param name  Player name string (up to MAX_PLAYER_NAME_LEN characters).
 */
void blockblaster_save_player_name(const char *name)
{
#ifdef ALLEGRO_ANDROID
    al_set_standard_file_interface();
    ALLEGRO_PATH *path = al_get_standard_path(ALLEGRO_USER_DATA_PATH);
    al_set_path_filename(path, PLAYER_NAME_FILENAME);
    ALLEGRO_FILE *f = al_fopen(al_path_cstr(path, '/'), "wb");
    al_destroy_path(path);
    if (!f) {
        al_android_set_apk_file_interface();
        return;
    }
    char buf[MAX_PLAYER_NAME_LEN + 2];
    int len = snprintf(buf, sizeof(buf), "%.5s\n", name);
    al_fwrite(f, buf, len);
    al_fclose(f);
    al_android_set_apk_file_interface();
#else
    char path[512];
    snprintf(path, sizeof(path), "%s%s", SAVE_DIR, PLAYER_NAME_FILENAME);
    FILE *f = fopen(path, "w");
    if (!f)
        return;
    fprintf(f, "%.5s\n", name);
    fclose(f);
#endif

#ifdef __EMSCRIPTEN__
    emscripten_save_flush_internal();
#endif
}

/**
 * \brief Load the last-used player name from disk.
 *
 * If no saved name is found, "PLAYR" is written to out as a default.
 *
 * \param out     Output buffer to receive the name.
 * \param out_sz  Size of the output buffer in bytes.
 */
void blockblaster_load_player_name(char *out, size_t out_sz)
{
    snprintf(out, out_sz, "PLAYR");

#ifdef ALLEGRO_ANDROID
    al_set_standard_file_interface();
    ALLEGRO_PATH *path = al_get_standard_path(ALLEGRO_USER_DATA_PATH);
    al_set_path_filename(path, PLAYER_NAME_FILENAME);
    ALLEGRO_FILE *f = al_fopen(al_path_cstr(path, '/'), "rb");
    al_destroy_path(path);
    if (!f) {
        al_android_set_apk_file_interface();
        return;
    }
    char buf[MAX_PLAYER_NAME_LEN + 2] = {0};
    al_fread(f, buf, sizeof(buf) - 1);
    al_fclose(f);
    al_android_set_apk_file_interface();
    char name[MAX_PLAYER_NAME_LEN + 1] = {0};
    if (sscanf(buf, "%5s", name) == 1 && name[0] != '\0') {
        snprintf(out, out_sz, "%s", name);
    }
#else
    char path[512];
    snprintf(path, sizeof(path), "%s%s", SAVE_DIR, PLAYER_NAME_FILENAME);
    FILE *f = fopen(path, "r");
    if (!f)
        return;
    char name[MAX_PLAYER_NAME_LEN + 1] = {0};
    if (fscanf(f, "%5s", name) == 1 && name[0] != '\0') {
        snprintf(out, out_sz, "%s", name);
    }
    fclose(f);
#endif
}

/* ======================================================================== */
/* Settings save / load                                                      */
/* ======================================================================== */

/**
 * \brief Persist the tray count and grid size settings to disk.
 *
 * \param tray_count  Number of pieces per tray set (1 - 4).
 * \param grid_size   Grid side length (10, 15, or 20).
 */
void blockblaster_save_settings(int tray_count, int grid_size)
{
#ifdef ALLEGRO_ANDROID
    al_set_standard_file_interface();
    ALLEGRO_PATH *path = al_get_standard_path(ALLEGRO_USER_DATA_PATH);
    al_set_path_filename(path, SETTINGS_FILENAME);
    ALLEGRO_FILE *f = al_fopen(al_path_cstr(path, '/'), "wb");
    al_destroy_path(path);
    if (!f) {
        al_android_set_apk_file_interface();
        return;
    }
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "%d %d\n", tray_count, grid_size);
    al_fwrite(f, buf, len);
    al_fclose(f);
    al_android_set_apk_file_interface();
#else
    char path[512];
    snprintf(path, sizeof(path), "%s%s", SAVE_DIR, SETTINGS_FILENAME);
    FILE *f = fopen(path, "w");
    if (!f)
        return;
    fprintf(f, "%d %d\n", tray_count, grid_size);
    fclose(f);
#endif

#ifdef __EMSCRIPTEN__
    emscripten_save_flush_internal();
#endif
    n_log(LOG_INFO, "Settings saved: tray=%d grid=%d", tray_count, grid_size);
}

/**
 * \brief Load the tray count and grid size settings from disk.
 *
 * On failure or out-of-range values, defaults (tray=4, grid=10) are used.
 *
 * \param tray_count  Output: number of pieces per set.
 * \param grid_size   Output: grid side length.
 */
void blockblaster_load_settings(int *tray_count, int *grid_size)
{
    *tray_count = 4;
    *grid_size = 10;

#ifdef ALLEGRO_ANDROID
    al_set_standard_file_interface();
    ALLEGRO_PATH *path = al_get_standard_path(ALLEGRO_USER_DATA_PATH);
    al_set_path_filename(path, SETTINGS_FILENAME);
    ALLEGRO_FILE *f = al_fopen(al_path_cstr(path, '/'), "rb");
    al_destroy_path(path);
    if (!f) {
        al_android_set_apk_file_interface();
        return;
    }
    char buf[32] = {0};
    al_fread(f, buf, sizeof(buf) - 1);
    al_fclose(f);
    al_android_set_apk_file_interface();
    int tc = 4, gs = 10;
    if (sscanf(buf, "%d %d", &tc, &gs) >= 2) {
        *tray_count = tc;
        *grid_size = gs;
    }
#else
    char path[512];
    snprintf(path, sizeof(path), "%s%s", SAVE_DIR, SETTINGS_FILENAME);
    FILE *f = fopen(path, "r");
    if (!f)
        return;
    int tc = 4, gs = 10;
    if (fscanf(f, "%d %d", &tc, &gs) >= 2) {
        *tray_count = tc;
        *grid_size = gs;
    }
    fclose(f);
#endif

    /* Clamp to valid ranges */
    if (*tray_count < 1)
        *tray_count = 1;
    if (*tray_count > 4)
        *tray_count = 4;
    if (*grid_size != 10 && *grid_size != 15 && *grid_size != 20)
        *grid_size = 10;

    n_log(LOG_INFO, "Settings loaded: tray=%d grid=%d", *tray_count,
          *grid_size);
}

/**
 * \brief Apply the persisted settings to the runtime grid/tray globals.
 *
 * Must be called before starting a new game so that GRID_W, GRID_H and
 * PIECES_PER_SET reflect the player's choices.
 *
 * \param gm  Game context containing setting_tray_count and
 * setting_grid_size.
 */
void blockblaster_apply_settings(GameContext *gm)
{
    PIECES_PER_SET = gm->setting_tray_count;
    GRID_W = gm->setting_grid_size;
    GRID_H = gm->setting_grid_size;
}
