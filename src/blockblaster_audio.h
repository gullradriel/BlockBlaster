/* Copyright (C) 2026 Nilorea Studio — GPL-3.0-or-later (see COPYING). */

/**
 * \file blockblaster_audio.h
 * \brief Audio management: loading, playback and cleanup.
 */

#ifndef __BLOCKBLASTER_AUDIO__
#define __BLOCKBLASTER_AUDIO__

#ifdef __cplusplus
extern "C" {
#endif

#include "blockblaster_context.h"

/** \brief Sound effect played when a piece is placed on the grid. */
extern ALLEGRO_SAMPLE *sfx_place;
/** \brief Sound effect played when a piece is selected from the tray. */
extern ALLEGRO_SAMPLE *sfx_select;
/** \brief Sound effect played when a piece returns to the tray. */
extern ALLEGRO_SAMPLE *sfx_send_to_tray;
/** \brief Sound effect played when one or more lines are cleared. */
extern ALLEGRO_SAMPLE *sfx_break_lines;
/** \brief Array of loaded music tracks (intro, end, 3 in-game). */
extern ALLEGRO_SAMPLE *sfx_music[5];
/** \brief Active music sample instance (only one track plays at a time). */
extern ALLEGRO_SAMPLE_INSTANCE *music_instance;
/** \brief Index into sfx_music[] of the currently playing track, or -1. */
extern int music_current_track;
/** \brief True when the audio subsystem was initialised successfully. */
extern bool audio_ok;

/** \brief Play a one-shot sound effect if audio is available and not muted. */
void blockblaster_play_sfx(ALLEGRO_SAMPLE *sample, GameContext *gm);

/** \brief Stop and destroy the active music instance, resetting the pointer. */
void blockblaster_stop_music(void);

/** \brief Load an audio sample from the platform data directory. */
bool blockblaster_load_audio_sample(ALLEGRO_SAMPLE **sample,
                                    const char *filename);

/** \brief Load all audio samples (sfx + music). */
void blockblaster_load_all_audio(void);

/** \brief Destroy all loaded audio samples and the music instance. */
void blockblaster_destroy_all_audio(void);

/**
 * \brief Switch to the given music track if not already playing it.
 * \param track  Index into sfx_music[] (0 = intro, 1 = end, 2-4 = in-game).
 * \param gm     Game context (checked for sound_on).
 */
void blockblaster_play_music_track(int track, GameContext *gm);

#ifdef __cplusplus
}
#endif

#endif /* __BLOCKBLASTER_AUDIO__ */
