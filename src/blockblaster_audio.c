/* Copyright (C) 2026 Nilorea Studio — GPL-3.0-or-later (see COPYING). */

/**
 * \file blockblaster_audio.c
 * \brief Audio management implementation.
 */

#include "blockblaster_audio.h"

#include "blockblaster_game.h"
#include "nilorea/n_log.h"

/* Global audio resources.  Loaded once in blockblaster_load_all_audio(). */
ALLEGRO_SAMPLE *sfx_place = NULL;
ALLEGRO_SAMPLE *sfx_select = NULL;
ALLEGRO_SAMPLE *sfx_send_to_tray = NULL;
ALLEGRO_SAMPLE *sfx_break_lines = NULL;
ALLEGRO_SAMPLE *sfx_music[5] = {NULL, NULL, NULL, NULL, NULL};
ALLEGRO_SAMPLE_INSTANCE *music_instance = NULL;
int music_current_track = -1;
bool audio_ok = false;

/**
 * \brief Play a one-shot sound effect if audio is available and enabled.
 *
 * \param sample  Allegro sample to play (may be NULL).
 * \param gm      Game context (provides the sound_on flag).
 */
void blockblaster_play_sfx(ALLEGRO_SAMPLE *sample, GameContext *gm)
{
    if (audio_ok && gm->sound_on && sample) {
        al_play_sample(sample, 1.0f, 0.0f, 1.0f, ALLEGRO_PLAYMODE_ONCE, NULL);
    }
}

/**
 * \brief Stop and destroy the currently playing music instance (if any).
 */
void blockblaster_stop_music(void)
{
    if (music_instance) {
        al_stop_sample_instance(music_instance);
        al_destroy_sample_instance(music_instance);
        music_instance = NULL;
    }
}

/**
 * \brief Load a single audio sample from the DATA directory.
 *
 * \param sample    Pointer to the ALLEGRO_SAMPLE* to receive the loaded
 *                  sample.
 * \param filename  Base filename (e.g. "place.ogg").
 * \return          true on success, false on failure (logged).
 */
bool blockblaster_load_audio_sample(ALLEGRO_SAMPLE **sample,
                                    const char *filename)
{
    char path[512];
    blockblaster_get_data_path(filename, path, sizeof(path));
    *sample = al_load_sample(path);
    if (!*sample) {
        n_log(LOG_ERR, "could not load %s, %s", path, strerror(al_get_errno()));
        return false;
    }
    return true;
}

/**
 * \brief Load every sound effect and music track used by the game.
 *
 * Must be called after al_install_audio() and al_reserve_samples().
 * If the audio subsystem was not initialised, the function returns early.
 */
void blockblaster_load_all_audio(void)
{
    if (!audio_ok) {
        n_log(LOG_ERR, "not loading audio: subsystem not initialised");
        return;
    }
    blockblaster_load_audio_sample(&sfx_place, PLACE_SAMPLE);
    blockblaster_load_audio_sample(&sfx_select, SELECT_SAMPLE);
    blockblaster_load_audio_sample(&sfx_send_to_tray, SEND_TO_TRAY_SAMPLE);
    blockblaster_load_audio_sample(&sfx_break_lines, BREAK_LINES_SAMPLE);
    blockblaster_load_audio_sample(&sfx_music[0], MUSIC_INTRO);
    blockblaster_load_audio_sample(&sfx_music[1], MUSIC_END);
    blockblaster_load_audio_sample(&sfx_music[2], MUSIC_1);
    blockblaster_load_audio_sample(&sfx_music[3], MUSIC_2);
    blockblaster_load_audio_sample(&sfx_music[4], MUSIC_3);
}

/**
 * \brief Destroy all loaded audio samples and the music instance.
 *
 * Safe to call even if some samples failed to load (NULL pointers are
 * skipped).
 */
void blockblaster_destroy_all_audio(void)
{
    blockblaster_stop_music();
    if (sfx_place) {
        al_destroy_sample(sfx_place);
        sfx_place = NULL;
    }
    if (sfx_select) {
        al_destroy_sample(sfx_select);
        sfx_select = NULL;
    }
    if (sfx_send_to_tray) {
        al_destroy_sample(sfx_send_to_tray);
        sfx_send_to_tray = NULL;
    }
    if (sfx_break_lines) {
        al_destroy_sample(sfx_break_lines);
        sfx_break_lines = NULL;
    }
    for (int i = 0; i < 5; i++) {
        if (sfx_music[i]) {
            al_destroy_sample(sfx_music[i]);
            sfx_music[i] = NULL;
        }
    }
}

/**
 * \brief Start looping a music track, stopping the previous one if different.
 *
 * Does nothing if audio is disabled, the track index is out of range, or
 * the requested track is already playing.
 *
 * \param track  Index into the sfx_music[] array (0 = intro, 1 = end,
 *               2-4 = gameplay music).
 * \param gm     Game context (provides the sound_on flag).
 */
void blockblaster_play_music_track(int track, GameContext *gm)
{
    if (!audio_ok || !gm->sound_on)
        return;
    if (track < 0 || track >= 5 || !sfx_music[track])
        return;
    if (music_current_track == track)
        return;

    blockblaster_stop_music();
    music_current_track = track;
    music_instance = al_create_sample_instance(sfx_music[track]);
    if (music_instance) {
        al_set_sample_instance_playmode(music_instance, ALLEGRO_PLAYMODE_LOOP);
        al_attach_sample_instance_to_mixer(music_instance,
                                           al_get_default_mixer());
        al_play_sample_instance(music_instance);
    }
}
