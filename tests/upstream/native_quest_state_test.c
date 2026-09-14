#include <assert.h>
#include <stddef.h>
#include <string.h>
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

static void test_reset_current_quest_log(void)
{
    static struct SaveBlock1 previousSave;
    static struct SaveBlock1 currentSave;
    static const u8 emptyQuestLog[sizeof(currentSave.questLog)];

    memset(previousSave.questLog, 0x5a, sizeof(previousSave.questLog));
    memset(currentSave.questLog, 0xa5, sizeof(currentSave.questLog));
    gSaveBlock1Ptr = &currentSave;
    gQuestLogState = 0;
    gQuestLogDefeatedWildMonRecord = (u16 *)previousSave.questLog;
    gQuestLogRecordingPointer = (u16 *)previousSave.questLog;
    frlg_native_quest_init();
    ResetQuestLog();
    assert(memcmp(currentSave.questLog, emptyQuestLog, sizeof(emptyQuestLog)) == 0);
    assert(gQuestLogState == 0);
    assert(gQuestLogDefeatedWildMonRecord == NULL);
    assert(gQuestLogRecordingPointer == NULL);
    assert(frlg_native_quest_status() == FRLG_NATIVE_QUEST_OK);

    memset(currentSave.questLog, 0xa5, sizeof(currentSave.questLog));
    gQuestLogState = QL_STATE_PLAYBACK;
    gQuestLogDefeatedWildMonRecord = (u16 *)previousSave.questLog;
    gQuestLogRecordingPointer = (u16 *)previousSave.questLog;
    frlg_native_quest_init();
    QL_AddASLROffset(&previousSave);
    ResetQuestLog();
    assert(memcmp(currentSave.questLog, emptyQuestLog, sizeof(emptyQuestLog)) == 0);
    for (size_t i = 0; i < sizeof(previousSave.questLog); i++)
        assert(((const u8 *)previousSave.questLog)[i] == 0x5a);
    assert(gQuestLogState == 0);
    assert(gQuestLogDefeatedWildMonRecord == NULL);
    assert(gQuestLogRecordingPointer == NULL);
    assert(frlg_native_quest_status() == FRLG_NATIVE_QUEST_UNSUPPORTED_STATE);

    frlg_native_quest_init();
    gQuestLogState = QL_STATE_RECORDING;
    ResetQuestLog();
    assert(frlg_native_quest_status() == FRLG_NATIVE_QUEST_UNSUPPORTED_STATE);
}

int main(void)
{
    test_idle_relocation();
    test_active_states_are_sticky();
    test_reset_current_quest_log();
    return 0;
}
