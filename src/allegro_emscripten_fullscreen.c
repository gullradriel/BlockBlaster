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
 * \file allegro_emscripten_fullscreen.c
 * \brief Emscripten fullscreen and tab-visibility callbacks.
 *
 * Compiled only when __EMSCRIPTEN__ is defined.
 *
 * on_fullscreen_change() stores the new fullscreen state and the element
 * dimensions into the GameContext so that the main render loop can resize the
 * Allegro display on the next frame without performing OpenGL operations from
 * an asynchronous context.
 *
 * web_init_tab_visibility() registers a Page Visibility API callback that
 * stops the Allegro timer when the browser tab is hidden and restarts it
 * (after flushing stale events) when the tab becomes visible again.  This
 * mirrors the Android HALT_DRAWING / RESUME_DRAWING handling and prevents
 * two related problems:
 *   - A burst of accumulated timer events processed all at once when the
 *     user returns to the tab, causing a momentary freeze.
 *   - The game loop spinning without rendering while the tab is hidden,
 *     wasting CPU and potentially mis-advancing animation timers.
 *
 * \author Castagnier Mickael aka Gull Ra Driel
 * \version 1.0
 * \date 18/12/2025
 */

#ifdef __EMSCRIPTEN__

#include "allegro_emscripten_fullscreen.h"

#include "blockblaster_context.h"
#include "nilorea/n_log.h"

#include <string.h>

/**
 * \brief Emscripten fullscreen change callback.
 *
 * See allegro_emscripten_fullscreen.h for full documentation.
 */
EM_BOOL on_fullscreen_change(int eventType,
                             const EmscriptenFullscreenChangeEvent *e,
                             void *userData)
{
    GameContext *ctx = (GameContext *) userData;
    ctx->is_fullscreen = (bool) e->isFullscreen;
    ctx->pending_w = e->elementWidth;
    ctx->pending_h = e->elementHeight;
    ctx->pending_resize = true;
    n_log(LOG_INFO, "fullscreen=%d element=%dx%d", (int) e->isFullscreen,
          e->elementWidth, e->elementHeight);
    return EM_TRUE;
}

/* Timer and event-queue handles stored by web_init_tab_visibility(). */
static ALLEGRO_TIMER *g_tab_timer = NULL;
static ALLEGRO_EVENT_QUEUE *g_tab_queue = NULL;

/**
 * \brief Page Visibility API callback.
 *
 * Called by the browser when the tab is hidden or shown.  When hidden the
 * Allegro timer is stopped so that no timer events accumulate in the queue
 * while the game is not being drawn.  When the tab becomes visible the
 * event queue is flushed (discarding any stale events that arrived before
 * the timer was fully stopped) and the timer is restarted so rendering
 * resumes normally on the next tick.
 */
static EM_BOOL on_visibility_change(int eventType,
                                    const EmscriptenVisibilityChangeEvent *e,
                                    void *userData)
{
    (void) eventType;
    (void) userData;
    if (e->hidden) {
        /* Tab went to background: stop the game timer so no events pile up. */
        if (g_tab_timer)
            al_stop_timer(g_tab_timer);
        n_log(LOG_INFO, "tab hidden: timer stopped");
    } else {
        /* Tab is visible again: discard any stale queued events, then
         * restart the timer so the game loop resumes from a clean state. */
        if (g_tab_queue)
            al_flush_event_queue(g_tab_queue);
        if (g_tab_timer)
            al_start_timer(g_tab_timer);
        n_log(LOG_INFO, "tab visible: timer restarted");
    }
    return EM_FALSE;
}

/** \brief Register the Page Visibility API callback.  See header. */
void web_init_tab_visibility(ALLEGRO_TIMER *timer, ALLEGRO_EVENT_QUEUE *queue)
{
    g_tab_timer = timer;
    g_tab_queue = queue;
    emscripten_set_visibilitychange_callback(NULL, EM_FALSE,
                                             on_visibility_change);
}

/* ======================================================================== */
/* Layout-aware key character capture                                        */
/* ======================================================================== */

/** Ring buffer size for captured key characters. */
#define KEY_CHAR_BUF_SIZE 16

static char g_key_char_buf[KEY_CHAR_BUF_SIZE];
static int g_key_char_head = 0;
static int g_key_char_tail = 0;

/**
 * \brief JavaScript keydown callback that captures layout-aware characters.
 *
 * The EmscriptenKeyboardEvent::key field reflects the active keyboard layout
 * (e.g. pressing 'A' on AZERTY yields "a", not "q").  We store single-byte
 * characters in a small ring buffer; multi-byte keys (Backspace, Enter, …)
 * and UTF-8 sequences longer than one byte are silently ignored because the
 * game only needs ASCII A-Z for player names.
 *
 * Returns EM_FALSE so Allegro's own keyboard handler still receives the event.
 */
static EM_BOOL on_keydown_for_char(int eventType,
                                   const EmscriptenKeyboardEvent *e,
                                   void *userData)
{
    (void) eventType;
    (void) userData;
    if (e->key[0] != '\0' && e->key[1] == '\0') {
        int next = (g_key_char_head + 1) % KEY_CHAR_BUF_SIZE;
        if (next != g_key_char_tail) {
            g_key_char_buf[g_key_char_head] = e->key[0];
            g_key_char_head = next;
        }
    }
    return EM_FALSE;
}

/** \brief Consume the next layout-aware character.  See header. */
char web_consume_key_char(void)
{
    if (g_key_char_tail == g_key_char_head)
        return '\0';
    char c = g_key_char_buf[g_key_char_tail];
    g_key_char_tail = (g_key_char_tail + 1) % KEY_CHAR_BUF_SIZE;
    return c;
}

/** \brief Register the JavaScript keydown listener for layout-aware chars.  See
 * header. */
void web_init_key_char_capture(void)
{
    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, NULL,
                                    EM_TRUE, on_keydown_for_char);
}

/* ======================================================================== */
/* Programmatic fullscreen request                                           */
/* ======================================================================== */

/** \brief Request HTML5 fullscreen on the canvas.  See header. */
void web_request_fullscreen(void)
{
    EmscriptenFullscreenStrategy strategy;
    memset(&strategy, 0, sizeof(strategy));
    strategy.scaleMode = EMSCRIPTEN_FULLSCREEN_SCALE_STRETCH;
    strategy.canvasResolutionScaleMode =
        EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_HIDEF;
    strategy.filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_DEFAULT;
    EMSCRIPTEN_RESULT r =
        emscripten_request_fullscreen_strategy("#canvas", EM_TRUE, &strategy);
    n_log(LOG_INFO, "emscripten_request_fullscreen_strategy result=%d", r);
}

#endif
