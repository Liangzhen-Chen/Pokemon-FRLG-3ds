#include "../include/global.h"
#include "frlg_native_save_state.h"
#include "load_save.h"
#include "constants/items.h"

struct ObjectEvent gObjectEvents[OBJECT_EVENTS_COUNT] = {};
struct Pokemon gPlayerParty[PARTY_SIZE] = {};
u8 gPlayerPartyCount = 0;
struct Pokemon gEnemyParty[PARTY_SIZE] = {};
u8 gEnemyPartyCount = 0;
struct BagPocket gBagPockets[NUM_BAG_POCKETS] = {};

void ZeroMonData(struct Pokemon *mon)
{
    memset(&mon->box, 0, sizeof(mon->box));
    mon->status = 0;
    mon->level = 0;
    mon->hp = 0;
    mon->maxHP = 0;
    mon->attack = 0;
    mon->defense = 0;
    mon->speed = 0;
    mon->spAttack = 0;
    mon->spDefense = 0;
    mon->mail = MAIL_NONE;
}

void ZeroPlayerPartyMons(void)
{
    for (s32 i = 0; i < PARTY_SIZE; i++)
        ZeroMonData(&gPlayerParty[i]);
}

void ZeroEnemyPartyMons(void)
{
    for (s32 i = 0; i < PARTY_SIZE; i++)
        ZeroMonData(&gEnemyParty[i]);
}

void SetBagPocketsPointers(void)
{
    gBagPockets[POCKET_ITEMS - 1].itemSlots = gSaveBlock1Ptr->bagPocket_Items;
    gBagPockets[POCKET_ITEMS - 1].capacity = BAG_ITEMS_COUNT;
    gBagPockets[POCKET_KEY_ITEMS - 1].itemSlots = gSaveBlock1Ptr->bagPocket_KeyItems;
    gBagPockets[POCKET_KEY_ITEMS - 1].capacity = BAG_KEYITEMS_COUNT;
    gBagPockets[POCKET_POKE_BALLS - 1].itemSlots = gSaveBlock1Ptr->bagPocket_PokeBalls;
    gBagPockets[POCKET_POKE_BALLS - 1].capacity = BAG_POKEBALLS_COUNT;
    gBagPockets[POCKET_TM_CASE - 1].itemSlots = gSaveBlock1Ptr->bagPocket_TMHM;
    gBagPockets[POCKET_TM_CASE - 1].capacity = BAG_TMHM_COUNT;
    gBagPockets[POCKET_BERRY_POUCH - 1].itemSlots = gSaveBlock1Ptr->bagPocket_Berries;
    gBagPockets[POCKET_BERRY_POUCH - 1].capacity = BAG_BERRIES_COUNT;
}

void Sav2_ClearSetDefault(void)
{
    ClearSav2();
    gSaveBlock2Ptr->optionsTextSpeed = OPTIONS_TEXT_SPEED_MID;
    gSaveBlock2Ptr->optionsWindowFrameType = 0;
    gSaveBlock2Ptr->optionsSound = OPTIONS_SOUND_MONO;
    gSaveBlock2Ptr->optionsBattleStyle = OPTIONS_BATTLE_STYLE_SHIFT;
    gSaveBlock2Ptr->optionsBattleSceneOff = FALSE;
    gSaveBlock2Ptr->regionMapZoom = FALSE;
    gSaveBlock2Ptr->optionsButtonMode = OPTIONS_BUTTON_MODE_HELP;
}
