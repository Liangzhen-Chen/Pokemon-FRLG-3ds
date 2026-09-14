#ifndef FRLG_NATIVE_QUEST_STATE_H
#define FRLG_NATIVE_QUEST_STATE_H

typedef enum
{
    FRLG_NATIVE_QUEST_OK,
    FRLG_NATIVE_QUEST_UNSUPPORTED_STATE,
} FrlgNativeQuestStatus;

void frlg_native_quest_init(void);
FrlgNativeQuestStatus frlg_native_quest_status(void);

#endif
