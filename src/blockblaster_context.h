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
 * \file blockblaster_context.h
 * \brief Game context structures, constants and layout macros.
 * \author Castagnier Mickael
 * \version 1.0
 * \date 19/02/2026
 */

#ifndef __GAME_CONTEXT__
#define __GAME_CONTEXT__

#ifdef __cplusplus
extern "C" {
#endif

#include <allegro5/allegro.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_font.h>
#include <stdio.h>
#include <string.h>

/**
 * \defgroup GRID Grid dimensions
 * \brief Constants that define the size of the play grid.
 * @{
 */

/** \brief Maximum number of columns (compile-time, used for array sizes). */
#define GRID_W_MAX 20

/** \brief Maximum number of rows (compile-time, used for array sizes). */
#define GRID_H_MAX 20

/** \brief Current number of columns in the play grid (runtime). */
extern int GRID_W;

/** \brief Current number of rows in the play grid (runtime). */
extern int GRID_H;

/** \brief color of the tray and grid bordes */
#define GRID_LINE_COLOR al_map_rgb(180, 180, 190)

/** \brief Base line width of the tray and grid border (scaled by UI_SCALE). */
#define GRID_LINE_WIDTH_BASE 3.0f

/** \brief Base line width of the rounded rectangles (scaled by UI_SCALE). */
#define ROUNDED_LINE_WIDTH_BASE 3.0f

/** @} */

/**
 * \defgroup CANVAS Virtual canvas and display
 * \brief Constants governing the virtual canvas and refresh rate.
 *
 * WIN_W and WIN_H are the virtual-canvas dimensions.  In windowed mode they
 * remain at their defaults (600 x 900).  In fullscreen mode (Android always;
 * desktop and Emscripten when toggled) update_view_offset() sets them to the
 * native display dimensions so the whole screen is used with no letterboxing
 * and scale = 1.0.
 * @{
 */

/** \brief Default virtual canvas width in pixels (windowed mode). */
#define WIN_W_DEFAULT 600

/** \brief Default virtual canvas height in pixels (windowed mode). */
#define WIN_H_DEFAULT 900

/** \brief Target frames per second for the game timer. */
#define REFRESH_RATE 30.0f

/** \brief Current virtual canvas width; updated at runtime by
 * update_view_offset(). */
extern int WIN_W;

/** \brief Current virtual canvas height; updated at runtime by
 * update_view_offset(). */
extern int WIN_H;

/** @} */

/**
 * \defgroup PIECES Piece tray
 * \brief Constants related to the piece tray presented each turn.
 * @{
 */

/** \brief Maximum pieces per set (compile-time, used for array sizes). */
#define PIECES_PER_SET_MAX 4

/** \brief Current number of pieces offered to the player each turn (runtime).
 */
extern int PIECES_PER_SET;

/** \brief Current display scale factor (runtime).
 *
 * Set by blockblaster_update_view_offset() whenever the window is resized.
 * Used by the line-width macros to guarantee a minimum physical pixel width
 * so that grid lines remain visible even at very small window sizes.
 */
extern float g_display_scale;

/** @} */

/**
 * \defgroup LAYOUT Layout macros
 * \brief Macros deriving all UI positions and sizes from WIN_W / WIN_H.
 *
 * CELL is constrained by both axes so the grid and tray always fit inside
 * the virtual canvas:
 *  - Width  constraint: CELL_W = (WIN_W - 2*GRID_MARGIN) / GRID_W
 *  - Height constraint: CELL_H = (WIN_H-171) / (GRID_H + GRID_W/PIECES_PER_SET
 * + 0.5)
 *  - CELL = min(CELL_W, CELL_H)
 *
 * The exit button is pinned independently to WIN_H - btn_h - 5, so only the
 * grid and tray stack drives the height constraint.
 * @{
 */

/** \brief Horizontal margin (px) between the virtual canvas edge and the grid.
 */
#define GRID_MARGIN 5.0f

/** \brief Cell size derived from the canvas width. */
#define CELL_W ((WIN_W - 2.0f * GRID_MARGIN) / (float) GRID_W)

/** \brief Cell size derived from the canvas height.
 *
 * The divisor is GRID_H + GRID_W/PIECES_PER_SET + 0.5 so the grid and tray
 * always fit vertically.  At the default 10x10/4 this equals 13.0. */
#define CELL_H                                                                 \
    (((float) WIN_H - 171.0f) /                                                \
     ((float) GRID_H + (float) GRID_W / (float) PIECES_PER_SET + 0.5f))

/**
 * \brief Actual cell size in pixels, the minimum of CELL_W and CELL_H.
 *
 * Using the minimum ensures both the grid and the tray remain fully visible
 * regardless of the canvas aspect ratio.
 */
#define CELL (CELL_W < CELL_H ? CELL_W : CELL_H)

/**
 * \brief CELL size at the default 600x900 virtual canvas.
 *
 * At 600x900: CELL_W = (600-10)/10 = 59, CELL_H = (900-171)/13 ≈ 56.08,
 * CELL = min(59, 56.08) ≈ 56.  Used as the reference for UI_SCALE.
 */
#define CELL_DEFAULT 56.0f

/**
 * \brief Scale factor for UI elements (corner radii, line widths, margins).
 *
 * Equals 1.0 at the default 600x900 canvas and scales proportionally with
 * the actual cell size on high-DPI or fullscreen displays.
 */
#define UI_SCALE (CELL / CELL_DEFAULT)

/** \brief Minimum line width in virtual pixels, ensuring at least 1 physical
 * pixel after the display transform.  When the window is shrunk the display
 * scale drops and this value rises to compensate, preventing sub-pixel lines
 * from becoming invisible. */
#define LINE_WIDTH_MIN (1.0f / g_display_scale)

/** \brief Scaled line width for grid and tray borders.
 * Clamped so the line never falls below 1 physical pixel. */
#define GRID_LINE_WIDTH                                                        \
    ((GRID_LINE_WIDTH_BASE * UI_SCALE) > LINE_WIDTH_MIN                        \
         ? (GRID_LINE_WIDTH_BASE * UI_SCALE)                                   \
         : LINE_WIDTH_MIN)

/** \brief Scaled line width for rounded rectangles.
 * Clamped so the line never falls below 1 physical pixel. */
#define ROUNDED_LINE_WIDTH                                                     \
    ((ROUNDED_LINE_WIDTH_BASE * UI_SCALE) > LINE_WIDTH_MIN                     \
         ? (ROUNDED_LINE_WIDTH_BASE * UI_SCALE)                                \
         : LINE_WIDTH_MIN)

/**
 * \brief Horizontal position (px) of the left edge of the play grid.
 *
 * The grid is centred horizontally within the virtual canvas.  When
 * CELL == CELL_W the grid fills edge-to-edge with GRID_MARGIN padding.
 */
#define GRID_X (((float) WIN_W - (float) GRID_W * CELL) * 0.5f)

/** \brief Vertical position (px) of the top edge of the play grid. */
#define GRID_Y 40.0f

/**
 * \brief Vertical position (px) of the top of the piece tray.
 *
 * The tray sits below the grid with a 60 px gap.
 */
#define TRAY_Y (GRID_Y + (float) GRID_H * CELL + 60.0f)

/**
 * \brief Horizontal position (px) of the left edge of the first tray slot.
 *
 * The tray is aligned with the left edge of the grid.
 */
#define TRAY_X GRID_X

/**
 * \brief Fixed horizontal gap (px) between adjacent tray slots.
 *
 * A small constant gap so slots never touch regardless of PIECES_PER_SET.
 */
#define TRAY_BOX_GAP 4.0f

/**
 * \brief Width (and height, px) of each tray slot box.
 *
 * Computed so that PIECES_PER_SET boxes plus (PIECES_PER_SET-1) gaps span
 * exactly GRID_W * CELL:
 *   TRAY_BOX * PIECES_PER_SET + TRAY_BOX_GAP * (PIECES_PER_SET - 1)
 *     = GRID_W * CELL
 *
 * With PIECES_PER_SET > 3 the old fixed CELL*3 box exceeded the grid width
 * and caused overlap; this formula always fits.
 */
#define TRAY_BOX                                                               \
    (((float) GRID_W * CELL - (float) (PIECES_PER_SET - 1) * TRAY_BOX_GAP) /   \
     (float) PIECES_PER_SET)

/** @} */

/**
 * \defgroup ANDROID_TOUCH Android touch input
 * \brief Constants specific to the Android touchscreen build.
 * @{
 */

/**
 * \brief Vertical offset (virtual pixels) applied to the floating piece on
 * Android.
 *
 * Shifts the dragged piece upward so it is not obscured by the player's
 * finger during a drag gesture.
 */
#define ANDROID_PIECE_Y_OFFSET 70.0f

/** @} */

/**
 * \defgroup DIFFICULTY Difficulty ramp
 * \brief Constants governing the score-based difficulty ramp.
 *
 * At score 0 the bag is filled with easy shapes.  At DIFFICULTY_MAX_SCORE
 * the bag favours the hardest shapes.  Easy shapes are never completely absent
 * even at maximum difficulty, and hard shapes always have some chance even at
 * score 0 (see MIN_DIFFICULTY_WEIGHT).
 *
 * The weight for each shape is:
 *   w(d,t) = MIN_DIFFICULTY_WEIGHT + (1 - MIN_DIFFICULTY_WEIGHT) * lerp(1-d, d,
 * t)
 *
 * where d = i / (SHAPES_COUNT - 1) is the normalised position of shape i in
 * the SHAPES[] array (0 = easiest, 1 = hardest) and
 * t = min(score, DIFFICULTY_MAX_SCORE) / DIFFICULTY_MAX_SCORE.
 * @{
 */

/** \brief Score at which the shape picker reaches maximum difficulty. */
#define DIFFICULTY_MAX_SCORE 75000

/**
 * \brief Minimum probability weight assigned to any shape at any difficulty.
 *
 * Prevents any shape from having zero probability; both easy and hard shapes
 * are always reachable.  Value 0.01 means a 1% floor weight.
 */
#define MIN_DIFFICULTY_WEIGHT 0.01f

/** @} */

/**
 * \defgroup SCORING Scoring constants
 * \brief Point values awarded for placement and line-clearing events.
 * @{
 */

/** \brief Points awarded per occupied cell placed on the grid. */
#define SCORE_PER_PLACED_CELL 1

/** \brief Points awarded per cell cleared from the grid. */
#define SCORE_PER_CLEARED_CELL 10

/** \brief Bonus points awarded per complete line (row or column) cleared. */
#define SCORE_PER_LINE_BONUS 25

/** \brief Extra bonus points per additional line cleared beyond the first. */
#define SCORE_MULTI_LINE_BONUS 50

/**
 * \brief Multiplier increment per consecutive clearing move.
 *
 * Each move that clears at least one line increases the combo counter by 1,
 * raising the effective multiplier by this fraction.
 */
#define COMBO_STEP_MULT 0.25f

/** @} */

/**
 * \defgroup PARTIAL_FILL Partial-fill starting mode
 * \brief Range of random cells pre-filled in the "partially filled" start mode.
 * @{
 */

/** \brief Minimum number of cells pre-filled when starting with a partial grid.
 */
#define FILL_MIN 16

/** \brief Maximum number of cells pre-filled when starting with a partial grid.
 */
#define FILL_MAX 28

/** @} */

/**
 * \defgroup ANIMATION Animation timings
 * \brief Durations (seconds) for in-game animations.
 * @{
 */

/** \brief Duration (seconds) of the pop-scale animation when a piece is placed.
 */
#define PLACE_POP_TIME 0.18f

/** \brief Duration (seconds) of the flash animation before cleared cells
 * vanish. */
#define CLEAR_FLASH_TIME 0.22f

/** @} */

/**
 * \defgroup BAG Bag randomizer
 * \brief Constants for the weighted bag-randomizer used to draw shapes.
 * @{
 */

/**
 * \brief Number of shape draws in one bag cycle before the bag is reshuffled.
 *
 * Can be set to SHAPES_COUNT to draw each shape exactly once per cycle.
 */
#define BAG_SIZE 24

/** @} */

/**
 * \defgroup PARTICLES Particle system
 * \brief Constants governing the particle burst effects.
 * @{
 */

/** \brief Maximum number of particles alive at the same time. */
#define MAX_PARTICLES 1000

/** \brief Minimum lifetime (seconds) of a single particle. */
#define PARTICLE_LIFE_MIN 0.30f

/** \brief Maximum lifetime (seconds) of a single particle. */
#define PARTICLE_LIFE_MAX 0.60f

/** \brief Minimum launch speed (pixels/second) of a particle. */
#define PARTICLE_SPEED_MIN 90.0f

/** \brief Maximum launch speed (pixels/second) of a particle. */
#define PARTICLE_SPEED_MAX 220.0f

/** \brief Number of particles spawned per cleared cell during a line-clear
 * event. */
#define PARTICLES_PER_CLEARED_CELL 15

/**
 * \brief Maximum total particles spawned for a single line-clear event.
 *
 * Caps the burst so that large multi-line clears do not stall rendering.
 */
#define PARTICLES_CAP_PER_CLEAR 500

/** @} */

/**
 * \defgroup SCREEN_SHAKE Screen shake
 * \brief Constants governing the camera shake effect on line clears.
 * @{
 */

/** \brief Duration (seconds) of a screen-shake effect. */
#define SHAKE_TIME 0.22f

/** \brief Peak displacement (pixels) of the screen-shake camera offset. */
#define SHAKE_STRENGTH 7.0f

/**
 * \brief Multiplier applied to SHAKE_STRENGTH for multi-line clears.
 *
 * Each additional line beyond the first increases shake strength by this
 * factor multiplied by (lines - 2) * 0.35.
 */
#define SHAKE_MULTILINE_BOOST 1.6f

/** @} */

/**
 * \defgroup RETURN_ANIM Return-to-tray animation
 * \brief Constants for the animation that snaps a piece back to the tray on
 *        an invalid drop.
 * @{
 */

/** \brief Duration (seconds) of the return-to-tray animation. */
#define RETURN_TIME 0.22f

/** @} */

/**
 * \defgroup ASSET_PATHS Asset file names
 * \brief File names for fonts, audio samples, and the high-score record.
 * @{
 */

/** \brief Legacy file name for the single high-score record (pre-v2). */
#define HIGHSCORE_FILENAME "blockblaster_highscore.txt"

/** \brief File name for the top-5 high scores with player names. */
#define SCORES_FILENAME "blockblaster_scores.txt"

/** \brief File name for the persisted last player name. */
#define PLAYER_NAME_FILENAME "blockblaster_playername.txt"

/** \brief File name (inside DATA/) of the persisted sound-state record. */
#define SOUND_STATE_FILENAME "blockblaster_sound_state.txt"

/** \brief File name (inside DATA/) of the persisted game settings. */
#define SETTINGS_FILENAME "blockblaster_settings.txt"

/** \brief File name (inside DATA/) of the game font. */
#define FONT_FILENAME "game_sans_serif_7.ttf"

/** \brief Audio sample played when a piece is successfully placed. */
#define PLACE_SAMPLE "place.ogg"

/** \brief Audio sample played when a piece is selected from the tray. */
#define SELECT_SAMPLE "select.ogg"

/** \brief Audio sample played when a piece returns to the tray after an invalid
 * drop. */
#define SEND_TO_TRAY_SAMPLE "send_to_tray.ogg"

/** \brief Audio sample played when one or more lines are cleared. */
#define BREAK_LINES_SAMPLE "break_lines.ogg"

/** \brief Music track played on the main menu. */
#define MUSIC_INTRO "intro.ogg"

/** \brief Music track played on the game-over screen. */
#define MUSIC_END "intro.ogg"

/** \brief First in-game music track. */
#define MUSIC_1 "music1.ogg"

/** \brief Second in-game music track. */
#define MUSIC_2 "music2.ogg"

/** \brief Third in-game music track (alias of MUSIC_1). */
#define MUSIC_3 "music1.ogg"

/** @} */

/**
 * \defgroup SAVE_PATH Save directory
 * \brief Platform-dependent path prefix for save data.
 * @{
 */

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
/** \brief Save directory used under Emscripten (IDBFS mount point). */
#define SAVE_DIR "/save/"
#elif defined(__ANDROID__)
/** \brief Save directory used on Android; must be set from Java via JNI before
 * use. */
extern const char *android_internal_path;
/** \brief Save directory used on Android (runtime path ending with '/'). */
#define SAVE_DIR android_internal_path
#else
/** \brief Save directory used on desktop platforms. */
#define SAVE_DIR "./DATA/"
#endif

/** @} */

/**
 * \defgroup COMBO_LIMITS Combo multiplier limits
 * \brief Cap on the score multiplier accumulated through consecutive clears.
 * @{
 */

/** \brief Maximum score multiplier achievable through consecutive line clears.
 */
#define MAX_MULTIPLIER 20.0f

/** @} */

/**
 * \defgroup BONUS_POPUP Bonus score popup
 * \brief Constants governing the animated score-gain popup shown after clears.
 * @{
 */

/** \brief Maximum number of bonus-score popups alive simultaneously. */
#define MAX_BONUS_POPUPS 24

/** \brief Lifetime (seconds) of a bonus-score popup before it fades out. */
#define BONUS_LIFE 1.75f

/** \brief Upward drift speed (pixels/second) of a bonus-score popup. */
#define BONUS_RISE_SPEED 55.0f

/** \brief Number of particles spawned at the location of a bonus-score popup.
 */
#define BONUS_PARTICLES 50

/** @} */

/**
 * \defgroup COMBO_POPUP Combo multiplier popup
 * \brief Constants governing the large centred combo-multiplier popup.
 * @{
 */

/** \brief Lifetime (seconds) of the combo-multiplier popup. */
#define COMBO_POP_LIFE 1.75f

/** \brief Upward drift speed (pixels/second) of the combo popup. */
#define COMBO_POP_RISE 35.0f

/** \brief Baseline particle count for the combo burst effect. */
#define COMBO_POP_PARTICLES_BASE 50

/** @} */

/* ======================================================================== */
/* Structures                                                                */
/* ======================================================================== */

/**
 * \defgroup STRUCTS Game data structures
 * \brief All structures used to represent game state.
 * @{
 */

/** \brief Maximum number of high-score entries persisted. */
#define MAX_HIGH_SCORES 5

/** \brief Maximum length of a player name (not counting the null terminator).
 */
#define MAX_PLAYER_NAME_LEN 5

/**
 * \brief A single entry in the high-score table.
 */
typedef struct {
    int grid_w;                         /* Grid width for this game. */
    int grid_h;                         /* Grid height for this game. */
    int tray_count;                     /* Tray pieces count for this game. */
    long score;                         /* Score achieved. */
    int highest_combo;                  /* Highest combo reached. */
    char name[MAX_PLAYER_NAME_LEN + 1]; /* Player name (null-terminated). */
} HighScoreEntry;

/**
 * \brief A colour theme pairing a fill colour with a stroke colour.
 *
 * Used for cell backgrounds, piece tiles and UI overlays so that each piece
 * set (or individual piece) has a visually distinct appearance.
 */
typedef struct {
    ALLEGRO_COLOR fill; /* Interior fill colour of a cell or piece tile. */
    ALLEGRO_COLOR
    stroke;             /* Border / outline colour of a cell or piece tile. */
} Theme;

/**
 * \brief A single particle used for burst and sparkle visual effects.
 *
 * Particles are updated each frame: velocity is integrated, a simple gravity
 * force is applied, and alpha fades with remaining lifetime.
 */
typedef struct {
    float x;           /* Current horizontal position (virtual pixels). */
    float y;           /* Current vertical position (virtual pixels). */
    float vx;          /* Horizontal velocity (pixels/second). */
    float vy;          /* Vertical velocity (pixels/second). */
    float life;        /* Remaining lifetime (seconds). */
    float life0;       /* Initial lifetime used to compute the fade fraction. */
    float size;        /* Radius (pixels) of the rendered circle. */
    ALLEGRO_COLOR col; /* Base colour; alpha is modulated by life/life0. */
    bool alive; /* True while the particle should be updated and drawn. */
} Particle;

/**
 * \brief An animated "+N points" popup shown after a line-clear event.
 *
 * The popup floats upward and fades out over its lifetime.
 */
typedef struct {
    float x;     /* Horizontal centre of the popup (virtual pixels). */
    float y;     /* Current vertical position of the popup (virtual pixels). */
    float vy;    /* Vertical velocity; negative = moving upward. */
    float life;  /* Remaining lifetime (seconds). */
    float life0; /* Initial lifetime used to compute the fade fraction. */
    int points;  /* Point gain displayed by this popup. */
    float mult;  /* Multiplier displayed alongside the point gain. */
    Theme theme; /* Colour theme used to render the popup text. */
    bool alive;  /* True while the popup should be updated and drawn. */
} BonusPopup;

/**
 * \brief A large centred popup displayed when the combo multiplier increases.
 *
 * Grows from a small scale to full size via an ease-out animation, then fades.
 */
typedef struct {
    float x;       /* Horizontal centre of the popup (virtual pixels). */
    float y;       /* Current vertical position (virtual pixels). */
    float vx;      /* Horizontal velocity (virtual pixels/second). */
    float vy;      /* Vertical velocity (virtual pixels/second). */
    float life;    /* Remaining lifetime (seconds). */
    float life0;   /* Initial lifetime used to compute scale and fade. */
    float scale;   /* Current render scale (starts near 0, grows to 1). */
    char text[64]; /* Formatted combo string, e.g. "COMBO x3". */
    Theme theme;   /* Colour theme used to render the popup text. */
    float mult;    /* Multiplier value represented by this popup. */
    bool alive;    /* True while the popup should be updated and drawn. */
} ComboPopup;

/**
 * \brief Total number of named colour themes available in the theme table.
 *
 * The theme_table array inside GameContext is sized to this value and
 * populated at startup by init_themes().
 */
#define THEMES_COUNT 8

/* ======================================================================== */
/* Piece definitions                                                         */
/* ======================================================================== */

/**
 * \brief Maximum dimension (width or height) of any shape, in cells.
 *
 * All shapes are stored in a SHAPE_MAX x SHAPE_MAX boolean grid regardless
 * of their actual footprint.
 */
#define SHAPE_MAX 5

/**
 * \brief Immutable descriptor of a tetromino-like block shape.
 *
 * Shapes are defined as a boolean grid of up to SHAPE_MAX x SHAPE_MAX cells.
 * Only cells where cells[y][x] is true contribute to the shape's footprint.
 */
typedef struct {
    int w;                 /* Actual width of the shape in cells. */
    int h;                 /* Actual height of the shape in cells. */
    bool cells[SHAPE_MAX]
              [SHAPE_MAX]; /* Cell occupancy grid; cells[row][col]. */
    const char *name;      /* Short debug name identifying the shape. */
} Shape;

/**
 * \brief A single piece slot in the player's tray.
 *
 * Each slot holds one Shape and the colour theme applied to it when placed.
 * Once placed on the grid the slot is marked used and no longer shown.
 */
typedef struct {
    Shape shape; /* The block shape held by this tray slot. */
    bool used;   /* True once the player has placed this piece on the grid. */
    Theme theme; /* Colour theme used when drawing and placing the piece. */
} Piece;

/**
 * \brief The 10x10 play grid.
 *
 * Each cell stores an occupancy flag and the colour theme of the piece that
 * occupies it, so cleared cells can be drawn with the correct colour during
 * the flash animation.
 */
typedef struct {
    bool occ[GRID_H_MAX][GRID_W_MAX];         /* True for each occupied cell. */
    Theme cell_theme[GRID_H_MAX][GRID_W_MAX]; /* Per-cell colour theme. */
    bool has_theme[GRID_H_MAX]
                  [GRID_W_MAX]; /* True when cell_theme[y][x] is valid. */
} Grid;

/** @} */ /* end STRUCTS */

/* ======================================================================== */
/* Game state                                                                */
/* ======================================================================== */

/**
 * \brief Top-level game state machine states.
 */
typedef enum {
    STATE_MENU = 0,    /* The main menu is shown. */
    STATE_PLAY = 1,    /* A game session is active. */
    STATE_GAMEOVER = 2 /* The game-over overlay is displayed. */
} GAME_STATES;

/**
 * \brief All mutable state for a running game session.
 *
 * A single instance of this structure is allocated on the stack in main()
 * and passed by pointer to every subsystem.
 */
typedef struct {
    GAME_STATES state; /* Current state of the game state machine. */
    Grid grid;         /* The 10x10 play grid. */
    Piece tray[PIECES_PER_SET_MAX]; /* Piece slots offered each turn. */

    long score;      /* Player's score for the current session. */
    long high_score; /* All-time best score (derived from high_scores[0]). */
    int combo;       /* Number of consecutive moves that each cleared a line. */
    int highest_combo;    /** Highest combo for the current session. */
    float last_move_mult; /* Score multiplier applied on the previous move. */
    int combo_miss; /* Non-clearing placements since the last line clear. */

    /* ---- High-score table ---- */
    HighScoreEntry high_scores[MAX_HIGH_SCORES]; /* Top-5 high scores. */
    int high_score_count; /* Number of valid entries (0..MAX_HIGH_SCORES). */

    /* ---- Player name ---- */
    char player_name[MAX_PLAYER_NAME_LEN + 1]; /* Name for the current game. */
    char last_player_name[MAX_PLAYER_NAME_LEN + 1]; /* Persisted default. */
    bool editing_name; /* True while the player is typing their name. */
    int name_cursor;   /* Current cursor position within player_name. */

    /* ---- Drag state ---- */
    bool dragging;      /* True while the player is dragging a piece. */
    int dragging_index; /* Tray index (0..PIECES_PER_SET-1) of the dragged
                           piece. */
    float mouse_x; /* Current pointer horizontal position in virtual space. */
    float mouse_y; /* Current pointer vertical position in virtual space. */

    /* ---- Ghost snap / grab anchor ---- */
    int grab_sx; /* Column within the shape that was directly grabbed. */
    int grab_sy; /* Row within the shape that was directly grabbed. */

    /* ---- Drop preview ---- */
    bool can_drop_preview; /* True when the ghost preview is in a valid grid
                              position. */
    int preview_cell_x;    /* Grid column of the top-left corner of the ghost
                              preview. */
    int preview_cell_y;    /* Grid row of the top-left corner of the ghost
                              preview. */

    /* ---- Clear animation ---- */
    bool clearing; /* True while the clear-flash animation is running; input
                      is blocked. */
    float clear_t; /* Remaining time (seconds) of the clear animation. */
    bool pending_clear[GRID_H_MAX][GRID_W_MAX]; /* Cells flagged for removal at
                                                   the end of the clear
                                                   animation. */

    /* ---- Per-cell pop animation ---- */
    float pop_t[GRID_H_MAX][GRID_W_MAX]; /* Remaining pop-scale animation time
                                             for each cell. */

    /* ---- Game mode ---- */
    int start_mode; /* Start mode: 0 = empty grid, 1 = partially filled grid.
                     */

    /**
     * \brief Theme assignment mode.
     *
     * 0 = each piece receives its own random theme.
     * 1 = all pieces in a set share one randomly chosen theme.
     */
    int theme_mode;
    Theme set_theme; /* Shared theme for the current set (used when theme_mode
                        == 1). */

    /* ---- Bag randomizer ---- */
    int bag[BAG_SIZE]; /* Array of shape indices for the current bag cycle. */
    int bag_len;       /* Number of entries in the current bag. */
    int bag_pos;       /* Next draw position within the bag array. */

    /* ---- Screen shake ---- */
    float shake_t;        /* Remaining duration (seconds) of the current shake
                             effect. */
    float shake_strength; /* Peak displacement (pixels) of the current shake
                             effect. */
    float cam_x; /* Horizontal camera offset applied to the playfield each
                    frame. */
    float cam_y; /* Vertical camera offset applied to the playfield each
                    frame. */

    /* ---- Particles ---- */
    Particle particles[MAX_PARTICLES]; /* Pool of all particles; unused slots
                                          have alive = false. */
    Theme theme_table[THEMES_COUNT];   /* Runtime colour theme palette,
                                          populated by init_themes(). */

    /* ---- Bonus score popups ---- */
    BonusPopup bonus_popups[MAX_BONUS_POPUPS]; /* Pool of bonus-score popups. */

    /* ---- Combo popup ---- */
    ComboPopup combo_popup; /* Single centred combo-multiplier popup. */

    /* ---- Predicted clear highlight while dragging ---- */
    bool pred_full_row[GRID_H_MAX]; /* True for each row that would be cleared
                                      on the current drop. */
    bool pred_full_col[GRID_W_MAX]; /* True for each column that would be
                                       cleared on the current drop. */
    bool has_predicted_clear;       /* True when at least one pred_full_row/col
                                       entry is set. */

    /* ---- Return-to-tray animation ---- */
    bool returning;   /* True while a piece is animating back to its tray slot.
                       */
    int return_index; /* Tray index of the piece currently returning. */
    float return_t;   /* Remaining animation time (seconds). */
    float return_start_x; /* Starting horizontal position of the return
                             animation. */
    float return_start_y; /* Starting vertical position of the return
                             animation. */
    float return_end_x; /* Target horizontal position (centre of tray slot). */
    float return_end_y; /* Target vertical position (centre of tray slot). */

    /* ---- Display info ---- */
    int display_width;   /* Current physical display width in pixels. */
    int display_height;  /* Current physical display height in pixels. */
    float view_offset_x; /* Horizontal letterbox offset applied by the base
                            transform. */
    float view_offset_y; /* Vertical letterbox offset applied by the base
                            transform. */

    /* ---- Font ---- */
    ALLEGRO_FONT *font; /* Loaded game font handle. */

    /* ---- Display handle ---- */
    ALLEGRO_DISPLAY *display; /* The Allegro display created by main(). */
    int pending_w;            /* Desired width for a deferred display resize
                                 (Emscripten). */
    int pending_h;            /* Desired height for a deferred display resize
                                 (Emscripten). */
    bool pending_resize; /* True when a deferred display resize is scheduled.
                          */

    bool mouse_locked;   /* True when pointer lock is requested (Emscripten
                            mouse-look). */
    bool paused;         /* True while the game is paused. */
    bool is_fullscreen;  /* True while the display is in fullscreen mode. */
    bool confirm_exit;   /* True while the exit-confirmation dialog is visible.
                          */
    bool sound_on; /* True when audio playback (music and sfx) is enabled. */

    float scale;   /* Uniform display scale used to fit the virtual canvas onto
                      the screen. */

    /* ---- Settings (persisted) ---- */
    int setting_tray_count; /* Pieces per set chosen by the player (1..4). */
    int setting_grid_size;  /* Grid side length chosen by the player
                               (10, 15 or 20). */

} GameContext;

#include "blockblaster_shapes.h"

#ifdef __cplusplus
}
#endif

#endif /* __GAME_CONTEXT__ */
