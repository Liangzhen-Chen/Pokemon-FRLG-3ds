#ifndef FRLG_NATIVE_AUDIO_H
#define FRLG_NATIVE_AUDIO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FRLG_NATIVE_AUDIO_MAX_REQUESTS 64u

typedef enum FrlgNativeAudioError {
    FRLG_NATIVE_AUDIO_ERROR_NONE,
    FRLG_NATIVE_AUDIO_ERROR_NOT_INITIALIZED,
    FRLG_NATIVE_AUDIO_ERROR_EVENT_OVERFLOW
} FrlgNativeAudioError;

typedef struct FrlgNativeAudioRequest {
    uint16_t song_id;
    uint32_t completed_vblanks;
} FrlgNativeAudioRequest;

typedef struct FrlgNativeAudioState {
    const char *output_status;
    FrlgNativeAudioError error;
    bool initialized;
    size_t request_count;
    FrlgNativeAudioRequest requests[FRLG_NATIVE_AUDIO_MAX_REQUESTS];
    bool stereo_set;
    uint32_t stereo_value;
    uint32_t stereo_completed_vblanks;
    uint32_t sound_main_calls;
    uint32_t vsync_calls;
} FrlgNativeAudioState;

/* Snapshot remains owned by the silent service. Error is sticky until the
 * next m4aSoundInit; callers must stop the frame on a non-NONE error. */
const FrlgNativeAudioState *frlg_native_audio_state(void);

#endif
