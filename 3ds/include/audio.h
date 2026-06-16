#pragma once
#include <3ds.h>
#include <stddef.h>
#include <stdbool.h>
bool audio_init(void);
void audio_play(const unsigned char *pcm, size_t bytes, int rate); // blocks until done
void audio_exit(void);
