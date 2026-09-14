#include <assert.h>
#include <stdalign.h>
#include <string.h>
#include "global.h"
#include "item.h"
#include "load_save.h"
#include "new_game.h"
#include "pokemon.h"
#include "constants/items.h"

static alignas(8) unsigned char save1Area[sizeof(struct SaveBlock1) + 128];
static alignas(8) unsigned char save2Area[sizeof(struct SaveBlock2) + 128];
static unsigned clearSav2Calls;

void ClearSav2(void)
{
    clearSav2Calls++;
    memset(save2Area, 0, sizeof(save2Area));
}

static void test_bag_pockets(void)
{
    static const u8 capacities[NUM_BAG_POCKETS] = {
        BAG_ITEMS_COUNT, BAG_KEYITEMS_COUNT, BAG_POKEBALLS_COUNT,
        BAG_TMHM_COUNT, BAG_BERRIES_COUNT
    };
    for (size_t offset = 0; offset <= 120; offset += 120)
    {
        gSaveBlock1Ptr = (struct SaveBlock1 *)(save1Area + offset);
        SetBagPocketsPointers();
        struct ItemSlot *slots[NUM_BAG_POCKETS] = {
            gSaveBlock1Ptr->bagPocket_Items,
            gSaveBlock1Ptr->bagPocket_KeyItems,
            gSaveBlock1Ptr->bagPocket_PokeBalls,
            gSaveBlock1Ptr->bagPocket_TMHM,
            gSaveBlock1Ptr->bagPocket_Berries
        };
        for (size_t i = 0; i < NUM_BAG_POCKETS; ++i)
        {
            assert(gBagPockets[i].itemSlots == slots[i]);
            assert(gBagPockets[i].capacity == capacities[i]);
        }
    }
}

static void test_party_and_objects(void)
{
    gSaveBlock1Ptr = (struct SaveBlock1 *)(save1Area + 120);
    memset(gSaveBlock1Ptr->playerParty, 0x36, sizeof(gSaveBlock1Ptr->playerParty));
    memset(gSaveBlock1Ptr->objectEvents, 0x91, sizeof(gSaveBlock1Ptr->objectEvents));
    gSaveBlock1Ptr->playerPartyCount = 4;
    memset(gPlayerParty, 0xa5, sizeof(gPlayerParty));
    memset(gObjectEvents, 0xa5, sizeof(gObjectEvents));
    gPlayerPartyCount = 0;
    LoadSerializedGame();
    assert(gPlayerPartyCount == 4);
    assert(memcmp(gPlayerParty, gSaveBlock1Ptr->playerParty, sizeof(gPlayerParty)) == 0);
    assert(memcmp(gObjectEvents, gSaveBlock1Ptr->objectEvents, sizeof(gObjectEvents)) == 0);
    gPlayerPartyCount = 2;
    memset(gPlayerParty, 0x47, sizeof(gPlayerParty));
    memset(gObjectEvents, 0x82, sizeof(gObjectEvents));
    SaveSerializedGame();
    assert(gSaveBlock1Ptr->playerPartyCount == 2);
    assert(memcmp(gPlayerParty, gSaveBlock1Ptr->playerParty, sizeof(gPlayerParty)) == 0);
    assert(memcmp(gObjectEvents, gSaveBlock1Ptr->objectEvents, sizeof(gObjectEvents)) == 0);
}

static void test_save2_defaults(void)
{
    memset(save2Area, 0xa5, sizeof(save2Area));
    gSaveBlock2Ptr = (struct SaveBlock2 *)(save2Area + 120);
    Sav2_ClearSetDefault();
    assert(clearSav2Calls == 1);
    assert(gSaveBlock2Ptr->optionsTextSpeed == OPTIONS_TEXT_SPEED_MID);
    assert(gSaveBlock2Ptr->optionsWindowFrameType == 0);
    assert(gSaveBlock2Ptr->optionsSound == OPTIONS_SOUND_MONO);
    assert(gSaveBlock2Ptr->optionsBattleStyle == OPTIONS_BATTLE_STYLE_SHIFT);
    assert(gSaveBlock2Ptr->optionsBattleSceneOff == FALSE);
    assert(gSaveBlock2Ptr->regionMapZoom == FALSE);
    assert(gSaveBlock2Ptr->optionsButtonMode == OPTIONS_BUTTON_MODE_HELP);
    assert(gSaveBlock2Ptr->playerName[0] == 0);
    assert(gSaveBlock2Ptr->encryptionKey == 0);
    assert(save2Area[0] == 0 && save2Area[sizeof(save2Area) - 1] == 0);
}

static void assert_zero_mon(const struct Pokemon *mon)
{
    static const struct BoxPokemon emptyBox;

    assert(memcmp(&mon->box, &emptyBox, sizeof(emptyBox)) == 0);
    assert(mon->status == 0);
    assert(mon->level == 0);
    assert(mon->hp == 0 && mon->maxHP == 0);
    assert(mon->attack == 0 && mon->defense == 0 && mon->speed == 0);
    assert(mon->spAttack == 0 && mon->spDefense == 0);
    assert(mon->mail == MAIL_NONE);
}

static void test_party_reset(void)
{
    struct Pokemon mon;

    memset(&mon, 0xa5, sizeof(mon));
    ZeroMonData(&mon);
    assert_zero_mon(&mon);

    memset(gPlayerParty, 0xa5, sizeof(gPlayerParty));
    memset(gEnemyParty, 0x5a, sizeof(gEnemyParty));
    gPlayerPartyCount = 4;
    gEnemyPartyCount = 5;
    ZeroPlayerPartyMons();
    ZeroEnemyPartyMons();
    for (size_t i = 0; i < PARTY_SIZE; ++i)
    {
        assert_zero_mon(&gPlayerParty[i]);
        assert_zero_mon(&gEnemyParty[i]);
    }
    assert(gPlayerPartyCount == 4);
    assert(gEnemyPartyCount == 5);
}

int main(void)
{
    _Static_assert(ARRAY_COUNT(gObjectEvents) == OBJECT_EVENTS_COUNT, "object count");
    _Static_assert(ARRAY_COUNT(gPlayerParty) == PARTY_SIZE, "party size");
    _Static_assert(ARRAY_COUNT(gEnemyParty) == PARTY_SIZE, "enemy party size");
    _Static_assert(sizeof(gPlayerPartyCount) == sizeof(u8), "party count type");
    _Static_assert(sizeof(gEnemyPartyCount) == sizeof(u8), "enemy party count type");
    test_bag_pockets();
    test_party_and_objects();
    test_save2_defaults();
    test_party_reset();
    return 0;
}
