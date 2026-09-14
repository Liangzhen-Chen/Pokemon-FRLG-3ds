#include "../include/global.h"
#include "frlg_native_world_state.h"
#include "constants/hoenn_cries.h"

#include "../../../external/pokefirered/src/data/pokemon/cry_ids.h"

bool8 gDisableMapMusicChangeOnMapLoad = MUSIC_DISABLE_OFF;

u16 SpeciesToCryId(u16 species)
{
    if (species < SPECIES_OLD_UNOWN_B - 1)
        return species;

    if (species <= SPECIES_OLD_UNOWN_Z - 1)
        return SPECIES_UNOWN - 1;

    return sHoennSpeciesIdToCryId[species - ((SPECIES_OLD_UNOWN_Z + 1) - 1)];
}
