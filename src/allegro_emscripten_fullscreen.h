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
 * \file allegro_emscripten_fullscreen.h
 * \brief Emscripten fullscreen state-change callback declaration.
 *
 * Only compiled when __EMSCRIPTEN__ is defined.  The callback is registered
 * in main() via emscripten_set_fullscreenchange_callback() and translates
 * browser fullscreen events into pending display resize requests that the main
 * render loop processes on the next frame.
 *
 * \author Castagnier Mickael aka Gull Ra Driel
 * \version 1.0
 * \date 08/12/2025
 */

#ifndef __ALLEGRO_EMSCRIPTEN_FULLSCREEN__
#define __ALLEGRO_EMSCRIPTEN_FULLSCREEN__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __EMSCRIPTEN__

#include <allegro5/allegro.h>
#include <emscripten/html5.h>

/**
 * \brief Emscripten fullscreen change callback.
 *
 * Called by the browser whenever the document fullscreen state changes.
 * Updates GameContext::is_fullscreen, ::pending_w, ::pending_h and
 * ::pending_resize so the main render loop can resize the Allegro display
 * on the next frame.
 *
 * \param eventType  Emscripten event type identifier (EMSCRIPTEN_EVENT_*).
 * \param e          Pointer to the fullscreen change event data.
 * \param userData   Pointer to the GameContext cast from void *.
 * \return           EM_TRUE to indicate the event was handled.
 */
EM_BOOL on_fullscreen_change(int eventType,
                             const EmscriptenFullscreenChangeEvent *e,
                             void *userData);

/**
 * \brief Register the Page Visibility API callback for tab hide/show.
 *
 * Stores the Allegro timer and event-queue handles and registers an
 * internal visibilitychange listener.  When the browser tab is hidden the
 * timer is stopped; when it becomes visible again the event queue is
 * flushed and the timer is restarted.  This prevents a burst of
 * accumulated timer events from causing a freeze when the user returns to
 * the tab.
 *
 * Must be called once after the timer and event queue are created.
 *
 * \param timer  The Allegro timer that drives the game loop.
 * \param queue  The Allegro event queue used by the game loop.
 */
void web_init_tab_visibility(ALLEGRO_TIMER *timer, ALLEGRO_EVENT_QUEUE *queue);

/**
 * \brief Register a JavaScript keydown listener that captures layout-aware
 *        characters.
 *
 * The browser's keydown event provides event.key which respects the active
 * keyboard layout (e.g. AZERTY).  Allegro's Emscripten backend maps
 * event.code (physical key position) to ALLEGRO_KEY_* constants, which
 * always reflect a QWERTY layout.  This callback captures the
 * layout-correct character so that text input (e.g. high-score name entry)
 * matches what the user actually typed.
 *
 * Must be called once during initialisation.
 */
void web_init_key_char_capture(void);

/**
 * \brief Consume and return the next layout-aware character from the
 *        internal ring buffer, or '\\0' if the buffer is empty.
 *
 * Each call to this function removes one character from the buffer.
 * Designed to be called once per ALLEGRO_EVENT_KEY_DOWN to stay in sync
 * with the Allegro event queue.
 */
char web_consume_key_char(void);

/**
 * \brief Request HTML5 fullscreen on the canvas element.
 *
 * Uses emscripten_request_fullscreen_strategy() which must be called from
 * a user-gesture context (key press or mouse click).
 */
void web_request_fullscreen(void);

#endif /* __EMSCRIPTEN__ */

#ifdef __cplusplus
}
#endif

#endif /* __ALLEGRO_EMSCRIPTEN_FULLSCREEN__ */
