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
 * \file allegro_emscripten_mouse.h
 * \brief Emscripten pointer-lock and mouse input declarations.
 *
 * Provides browser Pointer Lock integration for the Emscripten build so that
 * relative mouse deltas remain available even when the cursor is at a canvas
 * edge.  Pointer Lock can only be entered from a user gesture; the
 * mouse_locked flag in GameContext signals intent, and
 * web_request_pointer_lock() performs the actual browser request on the next
 * suitable gesture.
 *
 * Only compiled when __EMSCRIPTEN__ is defined.
 *
 * \author Castagnier Mickael aka Gull Ra Driel
 * \version 1.0
 * \date 08/12/2025
 */

#ifndef __ALLEGRO_EMSCRIPTEN_MOUSE__
#define __ALLEGRO_EMSCRIPTEN_MOUSE__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __EMSCRIPTEN__

#include "blockblaster_context.h"

#include <emscripten.h>
#include <emscripten/html5.h>
#include <stdbool.h>

/**
 * \brief Give keyboard focus to the Allegro canvas element.
 *
 * Calls Module['canvas'].focus() via inline JavaScript.  Must be called
 * before requesting pointer lock because browsers require the canvas to have
 * focus before granting it.
 */
void web_focus_canvas(void);

/**
 * \brief Request browser pointer lock on the canvas.
 *
 * Sets the internal g_pl_wanted flag and calls
 * emscripten_request_pointerlock() with unadjusted movement enabled.
 * The lock is granted asynchronously; poll mouse_capture_active() to check
 * whether the browser has granted it.
 *
 * Must be called from within a user-gesture handler (mouse click or key
 * press); otherwise the browser will silently deny the request.
 */
void web_request_pointer_lock(void);

/**
 * \brief Release browser pointer lock.
 *
 * Clears g_pl_wanted and calls emscripten_exit_pointerlock().  Safe to call
 * even when pointer lock is not currently active.
 */
void web_exit_pointer_lock(void);

/**
 * \brief Test whether the game may consume relative mouse movement deltas.
 *
 * Returns true only when all of the following conditions hold:
 *  - ctx->mouse_locked is true (game wants pointer lock)
 *  - ctx->paused is false
 *  - ctx->state == STATE_PLAY
 *  - g_pl_active is true (browser has granted pointer lock)
 *
 * \param ctx  Game context to query.
 * \return     true when relative mouse deltas should be consumed.
 */
bool mouse_capture_active(const GameContext *ctx);

/*
 * Note: the internal callbacks (on_pl_change, on_pl_error, on_mousemove,
 * on_canvas_click) are static to allegro_emscripten_mouse.c and intentionally
 * not declared here.  Only the public API is exposed.
 */

/**
 * \brief Register all pointer-lock and mouse callbacks.
 *
 * Must be called once after the GameContext is created.  Registers
 * on_pl_change, on_pl_error, on_mousemove and on_canvas_click.
 *
 * \param ctx  Game context passed as userData to the registered callbacks.
 */
void web_init_pointer_lock(GameContext *ctx);

#endif /* __EMSCRIPTEN__ */

#ifdef __cplusplus
}
#endif

#endif /* __ALLEGRO_EMSCRIPTEN_MOUSE__ */
