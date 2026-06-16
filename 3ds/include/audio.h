#pragma once
#include <3ds.h>
#include <stddef.h>
#include <stdbool.h>
bool audio_init(void);
void audio_sfx_shutter(void);     // play shutter click once (channel 1)
void audio_proc_start(void);      // start looping processing sound (channel 1)
void audio_proc_stop(void);       // stop processing sound
void audio_play_voice(const unsigned char *pcm, size_t bytes, int rate); // channel 0, non-blocking
bool audio_voice_playing(void);   // true while the voice waveform is still playing
void audio_exit(void);
