#include <stdint.h>
#include <string.h>

#include "global.h"
#include "m4a.h"
#include "constants/songs.h"
#include "frlg_native_music_player.h"

extern const u8 gClockTable[];
extern const struct PokemonCrySong gPokemonCrySongTemplate;

struct MusicPlayerInfo gMPlayInfo_BGM;
struct MusicPlayerInfo gMPlayInfo_SE1;
struct MusicPlayerInfo gMPlayInfo_SE2;
struct MusicPlayerInfo gMPlayInfo_SE3;
extern struct MusicPlayerTrack gMPlayTrack_BGM[];
extern struct MusicPlayerTrack gMPlayTrack_SE1[];
extern struct MusicPlayerTrack gMPlayTrack_SE2[];
extern struct MusicPlayerTrack gMPlayTrack_SE3[];
struct PokemonCrySong gPokemonCrySong;
struct PokemonCrySong gPokemonCrySongs[MAX_POKEMON_CRIES];
struct MusicPlayerInfo gPokemonCryMusicPlayers[MAX_POKEMON_CRIES];
struct MusicPlayerTrack gPokemonCryTracks[MAX_POKEMON_CRIES * 2];

struct VirtualCryChannel {
    struct SoundChannel channel;
    const struct WaveData *wave;
    uint64_t sample_phase;
    bool pending;
};

static struct VirtualCryChannel sCryChannels[MAX_POKEMON_CRIES];
static s16 sCryPitch;

static struct MusicPlayerInfo *const sPlayers[] = {
    &gMPlayInfo_BGM, &gMPlayInfo_SE1, &gMPlayInfo_SE2, &gMPlayInfo_SE3
};

static uintptr_t read_target(u8 **cursor)
{
    uintptr_t target;
    memcpy(&target, *cursor, sizeof(target));
    *cursor += sizeof(target);
    return target;
}

static void fail_sequence(void)
{
    frlg_native_audio_set_error(FRLG_NATIVE_AUDIO_ERROR_UNSUPPORTED_SEQUENCE);
}

void frlg_native_music_init(void)
{
    gPokemonCrySong = gPokemonCrySongTemplate;
    memset(gPokemonCrySongs, 0, sizeof(gPokemonCrySongs));
    memset(gPokemonCryMusicPlayers, 0, sizeof(gPokemonCryMusicPlayers));
    memset(gPokemonCryTracks, 0, sizeof(gPokemonCryTracks));
    memset(sCryChannels, 0, sizeof(sCryChannels));
    sCryPitch = 0;
    for (unsigned i = 0; i < MAX_POKEMON_CRIES; ++i)
    {
        struct MusicPlayerInfo *info = &gPokemonCryMusicPlayers[i];
        info->tracks = &gPokemonCryTracks[i * 2];
        info->trackCount = 2;
        info->ident = ID_NUMBER;
        info->status = MUSICPLAYER_STATUS_PAUSE;
        info->tempoU = 0x100;
    }
    for (unsigned i = 0; i < ARRAY_COUNT(sPlayers); ++i)
    {
        const struct MusicPlayer *entry = &gMPlayTable[i];
        struct MusicPlayerInfo *info = sPlayers[i];
        if (entry->info != info || entry->unk_8 == 0 || entry->unk_8 > MAX_MUSICPLAYER_TRACKS)
        {
            fail_sequence();
            return;
        }
        memset(info, 0, sizeof(*info));
        memset(entry->track, 0, sizeof(*entry->track) * entry->unk_8);
        info->tracks = entry->track;
        info->trackCount = entry->unk_8;
        info->unk_B = entry->unk_A;
        info->ident = ID_NUMBER;
        info->status = MUSICPLAYER_STATUS_PAUSE;
        info->tempoU = 0x100;
    }
}

void frlg_native_music_start(u16 songNum)
{
    if (songNum > MUS_TEACHY_TV_MENU || gSongTable[songNum].header == NULL)
    {
        frlg_native_audio_set_error(FRLG_NATIVE_AUDIO_ERROR_INVALID_SONG);
        return;
    }
    const struct Song *song = &gSongTable[songNum];
    if (song->ms >= ARRAY_COUNT(sPlayers))
    {
        frlg_native_audio_set_error(FRLG_NATIVE_AUDIO_ERROR_INVALID_SONG);
        return;
    }
    struct MusicPlayerInfo *info = gMPlayTable[song->ms].info;
    struct SongHeader *header = song->header;
    if (header->trackCount == 0 || header->trackCount > info->trackCount)
    {
        frlg_native_audio_set_error(FRLG_NATIVE_AUDIO_ERROR_INVALID_SONG);
        return;
    }
    if (info->unk_B && info->songHeader &&
        (info->status & MUSICPLAYER_STATUS_TRACK) &&
        !(info->status & MUSICPLAYER_STATUS_PAUSE) &&
        info->priority > header->priority)
        return;

    info->status = 0;
    info->songHeader = header;
    info->tone = header->tone;
    info->priority = header->priority;
    info->clock = 0;
    info->tempoD = 150;
    info->tempoI = 150;
    info->tempoC = 0;
    info->fadeOI = 0;
    for (unsigned i = 0; i < info->trackCount; ++i)
    {
        struct MusicPlayerTrack *track = &info->tracks[i];
        memset(track, 0, sizeof(*track));
        if (i < header->trackCount)
        {
            track->flags = MPT_FLG_EXIST | MPT_FLG_START;
            track->cmdPtr = header->part[i];
        }
    }
}

static void fade_step(struct MusicPlayerInfo *info)
{
    if (info->fadeOI == 0 || --info->fadeOC != 0)
        return;
    info->fadeOC = info->fadeOI;
    if (info->fadeOV & FADE_IN)
    {
        info->fadeOV += 4 << FADE_VOL_SHIFT;
        if (info->fadeOV >= 64 << FADE_VOL_SHIFT)
        {
            info->fadeOV = 64 << FADE_VOL_SHIFT;
            info->fadeOI = 0;
        }
        return;
    }
    if ((info->fadeOV & ~TEMPORARY_FADE) <= 4 << FADE_VOL_SHIFT)
    {
        for (unsigned i = 0; i < info->trackCount; ++i)
            if (!(info->fadeOV & TEMPORARY_FADE))
                info->tracks[i].flags = 0;
        if (info->fadeOV & TEMPORARY_FADE)
            info->status |= MUSICPLAYER_STATUS_PAUSE;
        else
            info->status = MUSICPLAYER_STATUS_PAUSE;
        info->fadeOV = 0;
        info->fadeOI = 0;
        return;
    }
    info->fadeOV -= 4 << FADE_VOL_SHIFT;
}

static void track_event(struct MusicPlayerInfo *info, struct MusicPlayerTrack *track)
{
    u8 *cursor = track->cmdPtr;
    u8 command = *cursor;
    if (command >= 0x80)
    {
        cursor++;
        if (command >= 0xBD)
            track->runningStatus = command;
    }
    else
    {
        command = track->runningStatus;
        if (command < 0xBD)
        {
            fail_sequence();
            return;
        }
    }

    if (command <= 0xB0)
    {
        track->wait = gClockTable[command - 0x80];
    }
    else if (command >= 0xCF)
    {
        track->gateTime = gClockTable[command - 0xCF];
        if (*cursor < 0x80)
        {
            track->key = *cursor++;
            if (*cursor < 0x80)
            {
                track->velocity = *cursor++;
                if (*cursor < 0x80)
                    track->gateTime += *cursor++;
            }
        }
    }
    else
    {
        switch (command)
        {
        case 0xB1: /* FINE */
            track->flags = 0;
            break;
        case 0xB2: /* GOTO */
            cursor = (u8 *)read_target(&cursor);
            break;
        case 0xB3: /* PATT */
            if (track->patternLevel == ARRAY_COUNT(track->patternStack))
            {
                fail_sequence();
                return;
            }
            track->patternStack[track->patternLevel++] = cursor + sizeof(uintptr_t);
            cursor = (u8 *)read_target(&cursor);
            break;
        case 0xB4: /* PEND */
            if (track->patternLevel)
                cursor = track->patternStack[--track->patternLevel];
            break;
        case 0xB5: /* REPT */
            if (*cursor == 0 || ++track->repN < *cursor)
            {
                cursor++;
                cursor = (u8 *)read_target(&cursor);
            }
            else
            {
                track->repN = 0;
                cursor += 1 + sizeof(uintptr_t);
            }
            break;
        case 0xBA: /* PRIO */
            track->priority = *cursor++;
            break;
        case 0xBB: /* TEMPO */
            info->tempoD = *cursor++ * 2;
            info->tempoI = (info->tempoD * info->tempoU) >> 8;
            break;
        case 0xBC: /* KEYSH */
            track->keyShift = *cursor++;
            break;
        case 0xBD: /* VOICE */
            track->tone = info->tone[*cursor++];
            break;
        case 0xBE: /* VOL */
            track->vol = *cursor++;
            break;
        case 0xBF: /* PAN */
            track->pan = *cursor++ - C_V;
            break;
        case 0xC0: /* BEND */
            track->bend = *cursor++ - C_V;
            break;
        case 0xC1: /* BENDR */
            track->bendRange = *cursor++;
            break;
        case 0xC2: /* LFOS */
            track->lfoSpeed = *cursor++;
            break;
        case 0xC3: /* LFODL */
            track->lfoDelay = *cursor++;
            break;
        case 0xC4: /* MOD */
            track->mod = *cursor++;
            break;
        case 0xC5: /* MODT */
            track->modT = *cursor++;
            break;
        case 0xC8: /* TUNE */
            track->tune = *cursor++ - C_V;
            break;
        case 0xCE: /* EOT */
            if (*cursor < 0x80)
                cursor++;
            break;
        default:
            fail_sequence();
            return;
        }
    }
    track->cmdPtr = cursor;
}

static void player_step(struct MusicPlayerInfo *info)
{
    if (info->status & MUSICPLAYER_STATUS_PAUSE)
        return;
    fade_step(info);
    if (info->status & MUSICPLAYER_STATUS_PAUSE)
        return;
    info->tempoC += info->tempoI;
    while (info->tempoC >= 150)
    {
        u32 active = 0;
        for (unsigned i = 0; i < info->trackCount; ++i)
        {
            struct MusicPlayerTrack *track = &info->tracks[i];
            if (!(track->flags & MPT_FLG_EXIST))
                continue;
            active |= 1u << i;
            if (track->flags & MPT_FLG_START)
            {
                track->flags = MPT_FLG_EXIST;
                track->bendRange = 2;
                track->volX = 64;
                track->lfoSpeed = 22;
            }
            unsigned events = 0;
            while (track->wait == 0 && (track->flags & MPT_FLG_EXIST))
            {
                if (++events > 1024)
                {
                    fail_sequence();
                    return;
                }
                track_event(info, track);
                if (frlg_native_audio_state()->error != FRLG_NATIVE_AUDIO_ERROR_NONE)
                    return;
            }
            if (track->wait)
                track->wait--;
        }
        info->clock++;
        if (!active)
        {
            info->status = MUSICPLAYER_STATUS_PAUSE;
            return;
        }
        info->status = active;
        info->tempoC -= 150;
    }
}

void frlg_native_music_main(void)
{
    for (unsigned i = 0; i < ARRAY_COUNT(sPlayers); ++i)
    {
        player_step(sPlayers[i]);
        if (frlg_native_audio_state()->error != FRLG_NATIVE_AUDIO_ERROR_NONE)
            return;
    }
    for (unsigned i = 0; i < MAX_POKEMON_CRIES; ++i)
    {
        struct VirtualCryChannel *virtual = &sCryChannels[i];
        struct MusicPlayerInfo *info = &gPokemonCryMusicPlayers[i];
        struct MusicPlayerTrack *track = info->tracks;
        if (virtual->pending)
        {
            virtual->pending = false;
            virtual->channel.track = track;
            virtual->channel.statusFlags = SOUND_CHANNEL_SF_START;
            track->chan = &virtual->channel;
            track->flags = MPT_FLG_EXIST;
            info->status = 1;
        }
        if (track->chan == &virtual->channel)
        {
            virtual->sample_phase += virtual->wave->freq;
            info->clock++;
            if (virtual->sample_phase >= (uint64_t)virtual->wave->size * 1024u * 60u)
            {
                virtual->channel.track = NULL;
                virtual->channel.statusFlags = 0;
                track->chan = NULL;
                track->flags = 0;
                info->status = MUSICPLAYER_STATUS_PAUSE;
            }
        }
    }
}

struct MusicPlayerInfo *SetPokemonCryTone(struct ToneData *tone)
{
    if (tone == NULL || tone->type != 0x20 || tone->key != 60 || tone->wav == NULL ||
        tone->wav->size == 0 || tone->wav->freq == 0 || tone->wav->status != 0 ||
        sCryPitch != 15360 || gPokemonCrySong.trackCount != 1 ||
        gPokemonCrySong.releaseValue != 0 || gPokemonCrySong.unkCmd0DParam != 0 ||
        (uint64_t)gPokemonCrySong.length * tone->wav->freq <
            (uint64_t)tone->wav->size * 1024u * 60u)
    {
        fail_sequence();
        return NULL;
    }

    unsigned chosen = 0;
    for (unsigned i = 0; i < MAX_POKEMON_CRIES; ++i)
    {
        if (gPokemonCryMusicPlayers[i].tracks[0].chan == NULL)
        {
            chosen = i;
            break;
        }
        if (gPokemonCryMusicPlayers[i].clock > gPokemonCryMusicPlayers[chosen].clock)
            chosen = i;
    }
    struct MusicPlayerInfo *info = &gPokemonCryMusicPlayers[chosen];
    struct MusicPlayerTrack *track = info->tracks;
    struct VirtualCryChannel *virtual = &sCryChannels[chosen];
    virtual->channel.track = NULL;
    virtual->channel.statusFlags = 0;
    virtual->wave = tone->wav;
    virtual->sample_phase = 0;
    virtual->pending = true;
    memset(track, 0, sizeof(*track) * 2);
    track->flags = MPT_FLG_EXIST | MPT_FLG_START;
    track->tone = *tone;
    gPokemonCrySongs[chosen] = gPokemonCrySong;
    gPokemonCrySongs[chosen].tone = tone;
    info->songHeader = (struct SongHeader *)&gPokemonCrySongs[chosen];
    info->tone = tone;
    info->status = 0;
    info->clock = 0;
    return info;
}

void SetPokemonCryVolume(u8 value)
{
    gPokemonCrySong.volumeValue = value & 0x7F;
}

void SetPokemonCryPanpot(s8 value)
{
    gPokemonCrySong.panValue = (value + C_V) & 0x7F;
}

void SetPokemonCryPitch(s16 value)
{
    s16 adjusted = value + 0x80;
    u8 delta = gPokemonCrySong.tuneValue2 - gPokemonCrySong.tuneValue;
    gPokemonCrySong.tieKeyValue = (adjusted >> 8) & 0x7F;
    gPokemonCrySong.tuneValue = (adjusted >> 1) & 0x7F;
    gPokemonCrySong.tuneValue2 = (delta + gPokemonCrySong.tuneValue) & 0x7F;
    sCryPitch = value;
}

void SetPokemonCryLength(u16 value)
{
    gPokemonCrySong.length = value;
}

void SetPokemonCryRelease(u8 value)
{
    gPokemonCrySong.releaseValue = value;
}

void SetPokemonCryProgress(u32 value)
{
    gPokemonCrySong.unkCmd0DParam = value;
}

void SetPokemonCryChorus(s8 value)
{
    if (value)
        fail_sequence();
    else
        gPokemonCrySong.trackCount = 1;
}

void SetPokemonCryPriority(u8 value)
{
    gPokemonCrySong.priority = value;
}

bool32 IsPokemonCryPlaying(struct MusicPlayerInfo *info)
{
    if (info == NULL)
    {
        fail_sequence();
        return FALSE;
    }
    struct MusicPlayerTrack *track = info->tracks;
    return track->chan != NULL && track->chan->track == track;
}

void m4aMPlayFadeOut(struct MusicPlayerInfo *info, u16 speed)
{
    if (info->ident == ID_NUMBER && speed)
    {
        info->fadeOC = speed;
        info->fadeOI = speed;
        info->fadeOV = 64 << FADE_VOL_SHIFT;
    }
}

void m4aMPlayFadeOutTemporarily(struct MusicPlayerInfo *info, u16 speed)
{
    if (info->ident == ID_NUMBER && speed)
    {
        info->fadeOC = speed;
        info->fadeOI = speed;
        info->fadeOV = (64 << FADE_VOL_SHIFT) | TEMPORARY_FADE;
    }
}

void m4aMPlayVolumeControl(struct MusicPlayerInfo *info, u16 bits, u16 volume)
{
    if (info->ident != ID_NUMBER)
        return;
    for (unsigned i = 0; i < info->trackCount; ++i)
        if ((bits & (1u << i)) && (info->tracks[i].flags & MPT_FLG_EXIST))
        {
            info->tracks[i].volX = volume / 4;
            info->tracks[i].flags |= MPT_FLG_VOLCHG;
        }
}

void m4aMPlayPanpotControl(struct MusicPlayerInfo *info, u16 bits, s8 pan)
{
    if (info->ident != ID_NUMBER)
        return;
    for (unsigned i = 0; i < info->trackCount; ++i)
        if ((bits & (1u << i)) && (info->tracks[i].flags & MPT_FLG_EXIST))
        {
            info->tracks[i].panX = pan;
            info->tracks[i].flags |= MPT_FLG_VOLCHG;
        }
}

void m4aMPlayStop(struct MusicPlayerInfo *info)
{
    if (info->ident == ID_NUMBER)
    {
        for (unsigned i = 0; i < MAX_POKEMON_CRIES; ++i)
            if (info == &gPokemonCryMusicPlayers[i])
                sCryChannels[i].pending = false;
        info->status |= MUSICPLAYER_STATUS_PAUSE;
        for (unsigned i = 0; i < info->trackCount; ++i)
        {
            if (info->tracks[i].chan != NULL)
            {
                info->tracks[i].chan->track = NULL;
                info->tracks[i].chan->statusFlags = 0;
                info->tracks[i].chan = NULL;
            }
            info->tracks[i].flags = 0;
        }
    }
}

void m4aMPlayAllStop(void)
{
    for (unsigned i = 0; i < ARRAY_COUNT(sPlayers); ++i)
        m4aMPlayStop(sPlayers[i]);
    for (unsigned i = 0; i < MAX_POKEMON_CRIES; ++i)
        m4aMPlayStop(&gPokemonCryMusicPlayers[i]);
}

void m4aMPlayAllContinue(void)
{
    for (unsigned i = 0; i < ARRAY_COUNT(sPlayers); ++i)
        m4aMPlayContinue(sPlayers[i]);
    for (unsigned i = 0; i < MAX_POKEMON_CRIES; ++i)
        m4aMPlayContinue(&gPokemonCryMusicPlayers[i]);
}

void m4aMPlayContinue(struct MusicPlayerInfo *info)
{
    if (info->ident == ID_NUMBER)
        info->status &= ~MUSICPLAYER_STATUS_PAUSE;
}

void m4aMPlayFadeIn(struct MusicPlayerInfo *info, u16 speed)
{
    if (info->ident == ID_NUMBER && speed)
    {
        info->fadeOC = speed;
        info->fadeOI = speed;
        info->fadeOV = FADE_IN;
        info->status &= ~MUSICPLAYER_STATUS_PAUSE;
    }
}

void m4aMPlayImmInit(struct MusicPlayerInfo *info)
{
    for (unsigned i = 0; i < info->trackCount; ++i)
    {
        struct MusicPlayerTrack *track = &info->tracks[i];
        if ((track->flags & (MPT_FLG_EXIST | MPT_FLG_START)) ==
            (MPT_FLG_EXIST | MPT_FLG_START))
        {
            track->flags = MPT_FLG_EXIST;
            track->bendRange = 2;
            track->volX = 64;
            track->lfoSpeed = 22;
        }
    }
}

void m4aSongNumStop(u16 songNum)
{
    if (songNum > MUS_TEACHY_TV_MENU || gSongTable[songNum].header == NULL)
    {
        frlg_native_audio_set_error(FRLG_NATIVE_AUDIO_ERROR_INVALID_SONG);
        return;
    }
    const struct Song *song = &gSongTable[songNum];
    if (song->ms >= ARRAY_COUNT(sPlayers))
    {
        frlg_native_audio_set_error(FRLG_NATIVE_AUDIO_ERROR_INVALID_SONG);
        return;
    }
    struct MusicPlayerInfo *info = gMPlayTable[song->ms].info;
    if (info->songHeader == song->header)
        m4aMPlayStop(info);
}
