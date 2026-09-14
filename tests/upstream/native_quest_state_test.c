#include <assert.h>
#include <stddef.h>
#include "global.h"
#include "quest_log.h"
#include "constants/quest_log.h"
#include "frlg_native_quest_state.h"

struct SaveBlock1 *gSaveBlock1Ptr;

static void test_idle_relocation(void)
{
    union
    {
        max_align_t align;
        u8 bytes[sizeof(struct SaveBlock1) + 128];
    } save;
    void *oldSave = save.bytes;
    u16 *record = (u16 *)(save.bytes + 16);
    u16 *recording = (u16 *)(save.bytes + 32);

    frlg_native_quest_init();
    gSaveBlock1Ptr = (struct SaveBlock1 *)(save.bytes + 64);
    gQuestLogState = 0;
    gQuestLogDefeatedWildMonRecord = record;
    gQuestLogRecordingPointer = recording;
    QL_AddASLROffset(oldSave);
    assert(gQuestLogDefeatedWildMonRecord == (u16 *)(save.bytes + 80));
    assert(gQuestLogRecordingPointer == recording);
    assert(frlg_native_quest_status() == FRLG_NATIVE_QUEST_OK);

    gQuestLogDefeatedWildMonRecord = NULL;
    QL_AddASLROffset(oldSave);
    assert(gQuestLogDefeatedWildMonRecord == NULL);
    assert(frlg_native_quest_status() == FRLG_NATIVE_QUEST_OK);

    oldSave = save.bytes + 64;
    gSaveBlock1Ptr = (struct SaveBlock1 *)save.bytes;
    gQuestLogDefeatedWildMonRecord = (u16 *)(save.bytes + 80);
    recording = (u16 *)(save.bytes + 96);
    gQuestLogRecordingPointer = recording;
    QL_AddASLROffset(oldSave);
    assert(gQuestLogDefeatedWildMonRecord == (u16 *)(save.bytes + 16));
    assert(gQuestLogRecordingPointer == recording);
    assert(frlg_native_quest_status() == FRLG_NATIVE_QUEST_OK);
}

static void test_active_states_are_sticky(void)
{
    union
    {
        max_align_t align;
        u8 bytes[sizeof(struct SaveBlock1) + 128];
    } save;
    int state;

    gSaveBlock1Ptr = (struct SaveBlock1 *)(save.bytes + 64);
    for (state = QL_STATE_RECORDING; state <= QL_STATE_PLAYBACK_LAST; state++)
    {
        frlg_native_quest_init();
        gQuestLogState = state;
        gQuestLogDefeatedWildMonRecord = (u16 *)(save.bytes + 16);
        gQuestLogRecordingPointer = (u16 *)(save.bytes + 32);
        QL_AddASLROffset(save.bytes);
        assert(frlg_native_quest_status() == FRLG_NATIVE_QUEST_UNSUPPORTED_STATE);

        gQuestLogState = 0;
        QL_AddASLROffset(save.bytes);
        assert(frlg_native_quest_status() == FRLG_NATIVE_QUEST_UNSUPPORTED_STATE);
        frlg_native_quest_init();
        assert(frlg_native_quest_status() == FRLG_NATIVE_QUEST_OK);
    }
}

int main(void)
{
    test_idle_relocation();
    test_active_states_are_sticky();
    return 0;
}
