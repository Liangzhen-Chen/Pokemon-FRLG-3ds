#include "../include/global.h"
#include "new_game.h"
#include "pokemon.h"
#include "quest_log.h"
#include "random.h"

void ResetMenuAndMonGlobals(void)
{
    ZeroPlayerPartyMons();
    ZeroEnemyPartyMons();
    ResetQuestLog();
    /* Keep the shared RNG step; wild encounter state is outside M1. */
    (void)Random();
}
