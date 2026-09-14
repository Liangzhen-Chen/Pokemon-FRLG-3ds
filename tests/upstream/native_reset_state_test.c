#include <assert.h>
#include <string.h>
#include "global.h"
#include "item.h"
#include "load_save.h"
#include "new_game.h"
#include "pokemon.h"
#include "quest_log.h"
#include "random.h"
#include "constants/items.h"
#include "constants/quest_log.h"
#include "frlg_native_quest_state.h"

static struct SaveBlock1 oldSave;
static struct SaveBlock1 currentSave;
static const u8 emptyQuestLog[sizeof(currentSave.questLog)];

static void fill_parties(void)
{
    memset(gPlayerParty, 0xa5, sizeof(gPlayerParty));
    memset(gEnemyParty, 0x5a, sizeof(gEnemyParty));
    gPlayerPartyCount = 4;
    gEnemyPartyCount = 5;
}

static void assert_reset_party(const struct Pokemon *party)
{
    static const struct BoxPokemon emptyBox;

    for (size_t i = 0; i < PARTY_SIZE; i++)
    {
        const struct Pokemon *mon = &party[i];
        assert(memcmp(&mon->box, &emptyBox, sizeof(emptyBox)) == 0);
        assert(mon->status == 0 && mon->level == 0);
        assert(mon->hp == 0 && mon->maxHP == 0);
        assert(mon->attack == 0 && mon->defense == 0 && mon->speed == 0);
        assert(mon->spAttack == 0 && mon->spDefense == 0);
        assert(mon->mail == MAIL_NONE);
    }
}

static void assert_party_counts(void)
{
    assert(gPlayerPartyCount == 4);
    assert(gEnemyPartyCount == 5);
}

static void assert_pockets_bound_to(const struct SaveBlock1 *save)
{
    assert(gBagPockets[POCKET_ITEMS - 1].itemSlots == save->bagPocket_Items);
    assert(gBagPockets[POCKET_KEY_ITEMS - 1].itemSlots == save->bagPocket_KeyItems);
    assert(gBagPockets[POCKET_POKE_BALLS - 1].itemSlots == save->bagPocket_PokeBalls);
    assert(gBagPockets[POCKET_TM_CASE - 1].itemSlots == save->bagPocket_TMHM);
    assert(gBagPockets[POCKET_BERRY_POUCH - 1].itemSlots == save->bagPocket_Berries);
}

static void test_first_copyright_reset(void)
{
    frlg_native_quest_init();
    gSaveBlock1Ptr = &gSaveBlock1;
    memset(gSaveBlock1.questLog, 0xa5, sizeof(gSaveBlock1.questLog));
    fill_parties();
    SetBagPocketsPointers();
    gQuestLogState = 0;
    gQuestLogDefeatedWildMonRecord = (u16 *)gSaveBlock1.questLog;
    gQuestLogRecordingPointer = (u16 *)gSaveBlock1.questLog;
    gRngValue = 0x12345678;

    ResetMenuAndMonGlobals();

    assert_reset_party(gPlayerParty);
    assert_reset_party(gEnemyParty);
    assert_party_counts();
    assert(memcmp(gSaveBlock1.questLog, emptyQuestLog, sizeof(emptyQuestLog)) == 0);
    assert(gQuestLogState == 0);
    assert(gQuestLogDefeatedWildMonRecord == NULL);
    assert(gQuestLogRecordingPointer == NULL);
    assert(frlg_native_quest_status() == FRLG_NATIVE_QUEST_OK);
    assert_pockets_bound_to(&gSaveBlock1);
    assert(gRngValue == ISO_RANDOMIZE1(0x12345678u));
}

static void test_title_start_reset(void)
{
    memset(oldSave.questLog, 0x5a, sizeof(oldSave.questLog));
    memset(currentSave.questLog, 0xa5, sizeof(currentSave.questLog));
    gSaveBlock1Ptr = &currentSave;
    SetBagPocketsPointers();
    currentSave.bagPocket_Items[0].itemId = ITEM_POTION;
    fill_parties();
    gQuestLogState = 0;
    gQuestLogDefeatedWildMonRecord = (u16 *)oldSave.questLog;
    gQuestLogRecordingPointer = (u16 *)oldSave.questLog;
    QL_AddASLROffset(&oldSave);
    assert(gQuestLogDefeatedWildMonRecord == (u16 *)currentSave.questLog);
    gRngValue = 0x87654321;

    ResetMenuAndMonGlobals();

    assert_reset_party(gPlayerParty);
    assert_reset_party(gEnemyParty);
    assert_party_counts();
    assert(memcmp(currentSave.questLog, emptyQuestLog, sizeof(emptyQuestLog)) == 0);
    for (size_t i = 0; i < sizeof(oldSave.questLog); i++)
        assert(((const u8 *)oldSave.questLog)[i] == 0x5a);
    assert(gQuestLogState == 0);
    assert(gQuestLogDefeatedWildMonRecord == NULL);
    assert(gQuestLogRecordingPointer == NULL);
    assert(frlg_native_quest_status() == FRLG_NATIVE_QUEST_OK);
    assert_pockets_bound_to(&currentSave);
    assert(currentSave.bagPocket_Items[0].itemId == ITEM_POTION);
    assert(gRngValue == ISO_RANDOMIZE1(0x87654321u));
}

static void test_title_start_unsupported_quest(void)
{
    memset(currentSave.questLog, 0xa5, sizeof(currentSave.questLog));
    gSaveBlock1Ptr = &currentSave;
    gQuestLogState = QL_STATE_PLAYBACK;
    gQuestLogDefeatedWildMonRecord = (u16 *)oldSave.questLog;
    gQuestLogRecordingPointer = (u16 *)oldSave.questLog;
    frlg_native_quest_init();
    QL_AddASLROffset(&oldSave);
    assert(frlg_native_quest_status() == FRLG_NATIVE_QUEST_UNSUPPORTED_STATE);
    gRngValue = 0x2468ace0;

    ResetMenuAndMonGlobals();

    assert(memcmp(currentSave.questLog, emptyQuestLog, sizeof(emptyQuestLog)) == 0);
    for (size_t i = 0; i < sizeof(oldSave.questLog); i++)
        assert(((const u8 *)oldSave.questLog)[i] == 0x5a);
    assert(gQuestLogState == 0);
    assert(gQuestLogDefeatedWildMonRecord == NULL);
    assert(gQuestLogRecordingPointer == NULL);
    assert(frlg_native_quest_status() == FRLG_NATIVE_QUEST_UNSUPPORTED_STATE);
    assert(gRngValue == ISO_RANDOMIZE1(0x2468ace0u));
}

int main(void)
{
    test_first_copyright_reset();
    test_title_start_reset();
    test_title_start_unsupported_quest();
    return 0;
}
