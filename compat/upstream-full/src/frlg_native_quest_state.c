#include "../include/global.h"
#include "frlg_native_quest_state.h"
#include "quest_log.h"

u8 gQuestLogState;
u16 *gQuestLogDefeatedWildMonRecord;
u16 *gQuestLogRecordingPointer;
static FrlgNativeQuestStatus sQuestStatus;

void frlg_native_quest_init(void)
{
    sQuestStatus = FRLG_NATIVE_QUEST_OK;
}

FrlgNativeQuestStatus frlg_native_quest_status(void)
{
    return sQuestStatus;
}

void QL_AddASLROffset(void *oldSaveBlockPtr)
{
    if (gQuestLogState != 0)
    {
        sQuestStatus = FRLG_NATIVE_QUEST_UNSUPPORTED_STATE;
        return;
    }

    if (gQuestLogDefeatedWildMonRecord != NULL)
    {
        gQuestLogDefeatedWildMonRecord = (u16 *)((uintptr_t)gQuestLogDefeatedWildMonRecord
            + (uintptr_t)gSaveBlock1Ptr - (uintptr_t)oldSaveBlockPtr);
    }
}
