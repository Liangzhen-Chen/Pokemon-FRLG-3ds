#include <assert.h>
#include "global.h"
#include "pokemon.h"
#include "overworld.h"
#include "constants/hoenn_cries.h"

static void test_cry_ids(void)
{
    assert(SpeciesToCryId(SPECIES_NONE) == SPECIES_NONE);
    assert(SpeciesToCryId(SPECIES_VENUSAUR) == SPECIES_VENUSAUR);
    assert(SpeciesToCryId(SPECIES_CHARIZARD) == SPECIES_CHARIZARD);
    assert(SpeciesToCryId(SPECIES_NIDORINO) == SPECIES_NIDORINO);
    assert(SpeciesToCryId(SPECIES_OLD_UNOWN_B - 2) == SPECIES_OLD_UNOWN_B - 2);
    assert(SpeciesToCryId(SPECIES_OLD_UNOWN_B - 1) == SPECIES_UNOWN - 1);
    assert(SpeciesToCryId(SPECIES_OLD_UNOWN_Z - 1) == SPECIES_UNOWN - 1);
    assert(SpeciesToCryId(SPECIES_OLD_UNOWN_Z) == CRY_TREECKO);
    assert(SpeciesToCryId(SPECIES_TREECKO) == CRY_GROVYLE);
    assert(SpeciesToCryId(SPECIES_CHIMECHO - 1) == CRY_CHIMECHO);
    /* The locked formula indexes beyond its table for SPECIES_CHIMECHO. */
}

static void test_music_state(void)
{
    _Static_assert(sizeof(gDisableMapMusicChangeOnMapLoad) == sizeof(bool8), "music state type");
    assert(gDisableMapMusicChangeOnMapLoad == MUSIC_DISABLE_OFF);
    gDisableMapMusicChangeOnMapLoad = MUSIC_DISABLE_STOP;
    assert(gDisableMapMusicChangeOnMapLoad == MUSIC_DISABLE_STOP);
    gDisableMapMusicChangeOnMapLoad = MUSIC_DISABLE_KEEP;
    assert(gDisableMapMusicChangeOnMapLoad == MUSIC_DISABLE_KEEP);
    gDisableMapMusicChangeOnMapLoad = MUSIC_DISABLE_OFF;
}

int main(void)
{
    test_cry_ids();
    test_music_state();
    return 0;
}
