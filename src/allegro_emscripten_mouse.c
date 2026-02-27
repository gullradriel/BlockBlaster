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
 * \file allegro_emscripten_mouse.c
 * \brief Emscripten pointer-lock and mouse input implementation.
 *
 * Implements the browser Pointer Lock integration declared in
 * allegro_emscripten_mouse.h.  See the header for the full rationale and
 * usage notes.
 *
 * Compiled only when __EMSCRIPTEN__ is defined.
 *
 * \author Castagnier Mickael aka Gull Ra Driel
 * \version 1.0
 * \date 18/12/2025
 */

#ifdef __EMSCRIPTEN__

#include "allegro_emscripten_mouse.h"

#include <emscripten.h>
#include <emscripten/html5.h>
#include <stdbool.h>

/*
  Browser Pointer Lock integration for Allegro5 mouse-look.

  Why this exists:
  - On the Web (WASM), you cannot reliably "warp" the OS cursor
  (al_set_mouse_xy).
  - Without Pointer Lock, the cursor can leave the canvas and the browser stops
  sending mouse move deltas.
  - With Pointer Lock active, the browser provides relative deltas
  (movementX/movementY) indefinitely.

  IMPORTANT BROWSER RULE:
  Pointer Lock can only be *entered* from a user gesture (mouse click / key
  press). So "lock on startup" must be implemented as:
    - ctx->mouse_locked = true (game wants it)
    - request_pointerlock is performed on the first user gesture (or on unpause
  key).
*/

/*
 * Accumulated relative mouse deltas.  These are written by on_mousemove()
 * while pointer lock is active.  Currently unused because the game does
 * not call web_init_pointer_lock(), but kept for future use.
 */
static float pending_mdx = 0.0f;
static float pending_mdy = 0.0f;

/** \brief True when the browser has granted pointer lock. Updated by
 * on_pl_change(). */
static bool g_pl_active = false;

/** \brief True when the game has requested pointer lock via
 * web_request_pointer_lock(). */
static bool g_pl_wanted = false;

/** \brief Focus the canvas using JS (portable across Emscripten versions). */
void web_focus_canvas(void)
{
    EM_ASM({
        if (Module && Module['canvas'])
            Module['canvas'].focus();
    });
}

/** \brief Request pointer lock on the canvas (must be called during a user
 * gesture). */
void web_request_pointer_lock(void)
{
    g_pl_wanted = true;
    web_focus_canvas();
    emscripten_request_pointerlock("#canvas", /*unadjustedMovement=*/1);
}

/** \brief Exit pointer lock (safe even if not active). */
void web_exit_pointer_lock(void)
{
    g_pl_wanted = false;
    emscripten_exit_pointerlock();
}

/*
  Small helper: are we truly allowed to consume infinite mouse deltas?

  We require both:
    - the game wants lock (ctx->mouse_locked, playing, not paused)
    - the browser actually granted pointer lock (g_pl_active)
*/
/**
 * \brief Check if mouse capture (pointer lock) is fully active.
 *
 * \param ctx  Game context to query.
 * \return     true if the game wants lock, is playing/unpaused, and the
 *             browser has granted lock.
 */
bool mouse_capture_active(const GameContext *ctx)
{
    return (ctx->mouse_locked && !ctx->paused && ctx->state == STATE_PLAY &&
            g_pl_active);
}

/* Pointer lock change callback (browser grants/revokes lock). */
static EM_BOOL on_pl_change(int eventType,
                            const EmscriptenPointerlockChangeEvent *e,
                            void *userData)
{
    (void) eventType;
    (void) userData;
    g_pl_active = e->isActive;
    return EM_TRUE;
}

/*
  Pointer lock error callback.

  Note: many Emscripten versions do NOT define a dedicated "error event" struct.
  The callback type is: EM_BOOL (*)(int, const void*, void*)
*/
static EM_BOOL on_pl_error(int eventType, const void *reserved, void *userData)
{
    (void) eventType;
    (void) reserved;
    (void) userData;
    g_pl_active = false;
    return EM_TRUE;
}

/*
  Mouse move callback:
  Use movementX/movementY to feed your existing pending_mdx/pending_mdy.
  This is the most reliable way to get deltas in browsers under pointer lock.
*/
static EM_BOOL on_mousemove(int eventType, const EmscriptenMouseEvent *e,
                            void *userData)
{
    (void) eventType;
    GameContext *ctx = (GameContext *) userData;

    if (!mouse_capture_active(ctx))
        return EM_TRUE;

    pending_mdx += (float) e->movementX;
    pending_mdy += (float) e->movementY;
    return EM_TRUE;
}

/*
  Click callback:
  If gameplay wants mouse lock, clicking the canvas will acquire pointer lock.
*/
static EM_BOOL on_canvas_click(int eventType, const EmscriptenMouseEvent *e,
                               void *userData)
{
    (void) eventType;
    (void) e;
    GameContext *ctx = (GameContext *) userData;

    /* Only request if gameplay *wants* lock and we don't have it yet. */
    if (ctx->mouse_locked && !g_pl_active) {
        web_request_pointer_lock();
    }
    return EM_TRUE;
}

/**
 * \brief Install all pointer-lock and mouse callbacks once after ctx is
 * created.
 *
 * \param ctx  Game context passed as userData to the registered callbacks.
 */
void web_init_pointer_lock(GameContext *ctx)
{
    emscripten_set_pointerlockchange_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT,
                                              ctx, 1, on_pl_change);
    emscripten_set_pointerlockerror_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT,
                                             ctx, 1, on_pl_error);

    emscripten_set_mousemove_callback("#canvas", ctx, 1, on_mousemove);
    emscripten_set_click_callback("#canvas", ctx, 1, on_canvas_click);
}

#endif /* __EMSCRIPTEN__ */
