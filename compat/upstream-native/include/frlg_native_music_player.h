#ifndef FRLG_NATIVE_MUSIC_PLAYER_H
#define FRLG_NATIVE_MUSIC_PLAYER_H

#include <stdint.h>
#include "frlg_native_audio.h"

void frlg_native_music_init(void);
void frlg_native_music_main(void);
void frlg_native_music_start(uint16_t songNum);
void frlg_native_audio_set_error(FrlgNativeAudioError error);

#endif
