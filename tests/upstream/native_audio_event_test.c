#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "global.h"
#include "main.h"
#include "m4a.h"
#include "sound.h"
#include "constants/quest_log.h"
#include "constants/songs.h"
#include "frlg_native_audio.h"

struct Main gMain;
u8 gDisableMapMusicChangeOnMapLoad;
u8 gQuestLogState;

static const FrlgNativeAudioState *reset_audio(void)
{
    memset(&gMain, 0, sizeof(gMain));
    gDisableMapMusicChangeOnMapLoad = 0;
    gQuestLogState = 0;
    m4aSoundInit();
    const FrlgNativeAudioState *state = frlg_native_audio_state();
    assert(state->error == FRLG_NATIVE_AUDIO_ERROR_NONE);
    assert(state->request_count == 0);
    assert(strcmp(state->output_status, "audio_output_unsupported") == 0);
    assert(gSoundInfo.pcmDmaCounter == 0);
    return state;
}

int main(void)
{
    m4aSongNumStart(9);
    assert(frlg_native_audio_state()->error == FRLG_NATIVE_AUDIO_ERROR_NOT_INITIALIZED);

    const FrlgNativeAudioState *state = reset_audio();
    gMain.vblankCounter2 = 8;
    PlaySE(MUS_GAME_FREAK);
    assert(state->request_count == 1);
    assert(state->requests[0].song_id == MUS_GAME_FREAK);
    assert(state->requests[0].completed_vblanks == 8);

    gDisableMapMusicChangeOnMapLoad = 1;
    PlaySE(MUS_GAME_FREAK);
    assert(state->request_count == 1);
    gDisableMapMusicChangeOnMapLoad = 0;
    gQuestLogState = QL_STATE_PLAYBACK;
    PlaySE(MUS_GAME_FREAK);
    assert(state->request_count == 1);

    gQuestLogState = 0;
    gMain.vblankCounter2 = 9;
    m4aSongNumStart(22);
    m4aSongNumStart(23);
    assert(state->request_count == 3);
    assert(state->requests[1].song_id == 22);
    assert(state->requests[2].song_id == 23);
    assert(state->requests[1].completed_vblanks == 9);
    assert(state->requests[2].completed_vblanks == 9);

    SetPokemonCryStereo(0);
    assert(state->stereo_set);
    assert(state->stereo_value == 0);
    assert(state->stereo_completed_vblanks == 9);
    gMain.vblankCounter2 = 10;
    SetPokemonCryStereo(1);
    assert(state->stereo_value == 1);
    assert(state->stereo_completed_vblanks == 10);

    m4aSoundVSync();
    m4aSoundMain();
    assert(state->vsync_calls == 1);
    assert(state->sound_main_calls == 1);
    assert(gSoundInfo.pcmDmaCounter == 0);

    state = reset_audio();
    for (unsigned i = 0; i < FRLG_NATIVE_AUDIO_MAX_REQUESTS; ++i)
        m4aSongNumStart((u16)i);
    assert(state->request_count == FRLG_NATIVE_AUDIO_MAX_REQUESTS);
    m4aSongNumStart(777);
    assert(state->error == FRLG_NATIVE_AUDIO_ERROR_EVENT_OVERFLOW);
    assert(state->request_count == FRLG_NATIVE_AUDIO_MAX_REQUESTS);
    assert(state->requests[0].song_id == 0);
    assert(state->requests[FRLG_NATIVE_AUDIO_MAX_REQUESTS - 1].song_id == FRLG_NATIVE_AUDIO_MAX_REQUESTS - 1);
    m4aSongNumStart(888);
    assert(state->request_count == FRLG_NATIVE_AUDIO_MAX_REQUESTS);
    m4aSoundMain();
    m4aSoundVSync();
    SetPokemonCryStereo(1);
    assert(state->sound_main_calls == 0);
    assert(state->vsync_calls == 0);
    assert(!state->stereo_set);
    assert(strcmp(state->output_status, "audio_output_unsupported") == 0);

    puts("Original PlaySE gating and native silent request state passed.");
    return 0;
}
