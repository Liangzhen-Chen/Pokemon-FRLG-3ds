#include <string.h>

#include "global.h"
#include "main.h"
#include "m4a.h"
#include "frlg_native_audio.h"

struct SoundInfo gSoundInfo;

static const char sOutputStatus[] = "audio_output_unsupported";
static FrlgNativeAudioState sState = {
    .output_status = sOutputStatus
};

static bool audio_ready(void)
{
    if (sState.error != FRLG_NATIVE_AUDIO_ERROR_NONE)
        return false;
    if (!sState.initialized)
    {
        sState.error = FRLG_NATIVE_AUDIO_ERROR_NOT_INITIALIZED;
        return false;
    }
    return true;
}

const FrlgNativeAudioState *frlg_native_audio_state(void)
{
    return &sState;
}

void m4aSoundInit(void)
{
    memset(&gSoundInfo, 0, sizeof(gSoundInfo));
    memset(&sState, 0, sizeof(sState));
    sState.output_status = sOutputStatus;
    sState.initialized = true;
}

void m4aSoundMain(void)
{
    if (audio_ready())
        sState.sound_main_calls++;
}

void m4aSoundVSync(void)
{
    if (audio_ready())
        sState.vsync_calls++;
}

void m4aSongNumStart(u16 songNum)
{
    if (!audio_ready())
        return;
    if (sState.request_count == FRLG_NATIVE_AUDIO_MAX_REQUESTS)
    {
        sState.error = FRLG_NATIVE_AUDIO_ERROR_EVENT_OVERFLOW;
        return;
    }
    FrlgNativeAudioRequest *request = &sState.requests[sState.request_count++];
    request->song_id = songNum;
    request->completed_vblanks = gMain.vblankCounter2;
}

void SetPokemonCryStereo(u32 value)
{
    if (!audio_ready())
        return;
    sState.stereo_set = true;
    sState.stereo_value = value;
    sState.stereo_completed_vblanks = gMain.vblankCounter2;
}
