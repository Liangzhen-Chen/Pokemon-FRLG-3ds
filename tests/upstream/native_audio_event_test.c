#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "global.h"
#include "main.h"
#include "m4a.h"
#include "sound.h"
#include "constants/quest_log.h"
#include "constants/songs.h"
#include "constants/species.h"
#include "frlg_native_audio.h"
#include "task.h"

struct Main gMain;
u8 gDisableMapMusicChangeOnMapLoad;
u8 gQuestLogState;
extern struct MusicPlayerInfo *gMPlay_PokemonCry;
static TaskFunc sDuckTask;

struct TestWave {
    u16 type;
    u16 status;
    u32 freq;
    u32 loopStart;
    u32 size;
    s8 data[4];
};
static struct TestWave sCryWave = {1, 0, 61440, 0, 4, {1, 2, 3, 4}};
const struct ToneData voicegroup000 = {0};
struct ToneData gCryTable[6] = {
    [5] = {0x20, 60, 0, 0, (struct WaveData *)&sCryWave, 255, 0, 255, 0}
};
struct ToneData gCryTable_Reverse[6];

u16 SpeciesToCryId(u16 species)
{
    assert(species == SPECIES_CHARIZARD - 1);
    return 5;
}

u8 CreateTask(TaskFunc func, u8 priority)
{
    (void)priority;
    assert(sDuckTask == NULL);
    sDuckTask = func;
    return 1;
}

bool8 FuncIsActiveTask(TaskFunc func)
{
    return sDuckTask == func;
}

void DestroyTask(u8 taskId)
{
    assert(taskId == 1);
    sDuckTask = NULL;
}

struct MusicPlayerTrack gMPlayTrack_BGM[10];
struct MusicPlayerTrack gMPlayTrack_SE1[3];
struct MusicPlayerTrack gMPlayTrack_SE2[9];
struct MusicPlayerTrack gMPlayTrack_SE3[1];

const struct MusicPlayer gMPlayTable[] = {
    {&gMPlayInfo_BGM, gMPlayTrack_BGM, 10, 0},
    {&gMPlayInfo_SE1, gMPlayTrack_SE1, 3, 1},
    {&gMPlayInfo_SE2, gMPlayTrack_SE2, 9, 1},
    {&gMPlayInfo_SE3, gMPlayTrack_SE3, 1, 0},
};

static u8 sTitleTrack[] = {0xB0, 0xB0, 0xB1};
static u8 sGameFreakTrack[] = {0x81, 0xB1};
static u8 sPatternTrack[] = {0xB3, 0, 0, 0, 0, 0x81, 0xB1, 0x81, 0xB4};
static u8 sGotoTrack[] = {0x81, 0xB2, 0, 0, 0, 0, 0xC9, 0x81, 0xB1};
static u8 sRepeatTrack[] = {0x81, 0xB5, 3, 0, 0, 0, 0, 0xB1};
static struct SongHeader sTitleHeader = {1, 0, 0, 0, NULL, {sTitleTrack}};
static struct SongHeader sGameFreakHeader = {1, 0, 0, 0, NULL, {sGameFreakTrack}};
const struct Song gSongTable[MUS_TEACHY_TV_MENU + 1] = {
    [MUS_TITLE] = {&sTitleHeader, 0, 0},
    [MUS_GAME_FREAK] = {&sGameFreakHeader, 0, 0},
};

static const FrlgNativeAudioState *reset_audio(void)
{
    memset(&gMain, 0, sizeof(gMain));
    gDisableMapMusicChangeOnMapLoad = 0;
    gQuestLogState = 0;
    sDuckTask = NULL;
    m4aSoundInit();
    const FrlgNativeAudioState *state = frlg_native_audio_state();
    assert(state->error == FRLG_NATIVE_AUDIO_ERROR_NONE);
    assert(state->request_count == 0);
    assert(strcmp(state->output_status, "audio_output_unsupported") == 0);
    assert(gSoundInfo.pcmDmaCounter == 0);
    return state;
}

static void set_relative_target(u8 *field, u8 *target)
{
    intptr_t displacement = target - field;
    assert(displacement >= INT32_MIN && displacement <= INT32_MAX);
    int32_t encoded = displacement;
    memcpy(field, &encoded, sizeof(encoded));
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
    m4aSongNumStart(MUS_TITLE);
    m4aSongNumStart(MUS_GAME_FREAK);
    assert(state->request_count == 3);
    assert(state->requests[1].song_id == MUS_TITLE);
    assert(state->requests[2].song_id == MUS_GAME_FREAK);
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
    PlayBGM(MUS_TITLE);
    m4aSoundMain();
    assert(!IsBGMStopped());
    FadeOutMapMusic(1);
    assert(!IsNotWaitingForBGMStop());
    for (unsigned i = 0; i < 16; ++i)
    {
        MapMusicMain();
        m4aSoundMain();
    }
    assert(IsBGMStopped());
    MapMusicMain();
    assert(IsNotWaitingForBGMStop());
    assert(state->error == FRLG_NATIVE_AUDIO_ERROR_NONE);

    state = reset_audio();
    PlayBGM(MUS_TITLE);
    m4aSoundMain();
    FadeOutBGMTemporarily(1);
    for (unsigned i = 0; i < 16; ++i)
        m4aSoundMain();
    assert(gMPlayInfo_BGM.status & MUSICPLAYER_STATUS_TRACK);
    assert(gMPlayInfo_BGM.status & MUSICPLAYER_STATUS_PAUSE);
    FadeInBGM(1);
    m4aSoundMain();
    assert(!(gMPlayInfo_BGM.status & MUSICPLAYER_STATUS_PAUSE));
    assert(state->error == FRLG_NATIVE_AUDIO_ERROR_NONE);

    state = reset_audio();
    PlayBGM(MUS_TITLE);
    m4aSoundMain();
    PlayCry_Normal(SPECIES_CHARIZARD, 0);
    assert(sDuckTask != NULL);
    assert(gMPlay_PokemonCry != NULL);
    m4aSoundMain();
    assert(IsPokemonCryPlaying(gMPlay_PokemonCry));
    for (unsigned i = 0; i < 5; ++i)
    {
        if (sDuckTask)
            sDuckTask(1);
        m4aSoundMain();
    }
    assert(!IsPokemonCryPlaying(gMPlay_PokemonCry));
    assert(sDuckTask == NULL);
    assert(gMPlayInfo_BGM.tracks[0].volX == 64);
    assert(state->error == FRLG_NATIVE_AUDIO_ERROR_NONE);

    state = reset_audio();
    PlayCry_Normal(SPECIES_CHARIZARD, 0);
    m4aSoundMain();
    assert(IsPokemonCryPlaying(gMPlay_PokemonCry));
    StopCry();
    assert(!IsPokemonCryPlaying(gMPlay_PokemonCry));
    m4aSoundMain();
    assert(!IsPokemonCryPlaying(gMPlay_PokemonCry));
    assert(state->error == FRLG_NATIVE_AUDIO_ERROR_NONE);

    state = reset_audio();
    PlayCry_Normal(SPECIES_CHARIZARD, 0);
    StopCry();
    m4aSoundMain();
    assert(!IsPokemonCryPlaying(gMPlay_PokemonCry));
    assert(state->error == FRLG_NATIVE_AUDIO_ERROR_NONE);

    sCryWave.freq = 81700;
    state = reset_audio();
    PlayCry_Normal(SPECIES_CHARIZARD, 0);
    m4aSoundMain();
    m4aSoundMain();
    assert(IsPokemonCryPlaying(gMPlay_PokemonCry));
    m4aSoundMain();
    assert(!IsPokemonCryPlaying(gMPlay_PokemonCry));
    assert(state->error == FRLG_NATIVE_AUDIO_ERROR_NONE);
    sCryWave.freq = 61440;

    state = reset_audio();
    m4aSongNumStart(MUS_GAME_FREAK);
    for (unsigned i = 0; i < 4; ++i)
        m4aSoundMain();
    assert(IsBGMStopped());
    assert(state->error == FRLG_NATIVE_AUDIO_ERROR_NONE);

    set_relative_target(&sPatternTrack[1], &sPatternTrack[7]);
    sTitleHeader.part[0] = sPatternTrack;
    state = reset_audio();
    PlayBGM(MUS_TITLE);
    for (unsigned i = 0; i < 5; ++i)
        m4aSoundMain();
    assert(IsBGMStopped());
    assert(state->error == FRLG_NATIVE_AUDIO_ERROR_NONE);
    sTitleHeader.part[0] = sTitleTrack;

    set_relative_target(&sGotoTrack[2], &sGotoTrack[7]);
    sTitleHeader.part[0] = sGotoTrack;
    state = reset_audio();
    PlayBGM(MUS_TITLE);
    for (unsigned i = 0; i < 5; ++i)
        m4aSoundMain();
    assert(IsBGMStopped());
    assert(state->error == FRLG_NATIVE_AUDIO_ERROR_NONE);
    sTitleHeader.part[0] = sTitleTrack;

    set_relative_target(&sRepeatTrack[3], &sRepeatTrack[0]);
    sTitleHeader.part[0] = sRepeatTrack;
    state = reset_audio();
    PlayBGM(MUS_TITLE);
    for (unsigned i = 0; i < 7; ++i)
        m4aSoundMain();
    assert(IsBGMStopped());
    assert(state->error == FRLG_NATIVE_AUDIO_ERROR_NONE);
    sTitleHeader.part[0] = sTitleTrack;

    state = reset_audio();
    for (unsigned i = 0; i < FRLG_NATIVE_AUDIO_MAX_REQUESTS; ++i)
        m4aSongNumStart(MUS_GAME_FREAK);
    assert(state->request_count == FRLG_NATIVE_AUDIO_MAX_REQUESTS);
    m4aSongNumStart(777);
    assert(state->error == FRLG_NATIVE_AUDIO_ERROR_EVENT_OVERFLOW);
    assert(state->request_count == FRLG_NATIVE_AUDIO_MAX_REQUESTS);
    assert(state->requests[0].song_id == MUS_GAME_FREAK);
    assert(state->requests[FRLG_NATIVE_AUDIO_MAX_REQUESTS - 1].song_id == MUS_GAME_FREAK);
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
