#ifndef FRLG_NATIVE_TITLE_ASSETS_H
#define FRLG_NATIVE_TITLE_ASSETS_H
#include "frlg_native_io.h"
/* Defined only by local preprocessed title assets, never by public builds.
 * This subset must later be merged with the complete game's resource table. */
extern const FrlgNativeLzResource gFrlgTitleLzResources[];
extern const size_t gFrlgTitleLzResourceCount;
extern const FrlgNativeLzResource gFrlgIntroLzResources[];
extern const size_t gFrlgIntroLzResourceCount;
extern const FrlgNativeLzResource gFrlgTitleLocalLzResources[];
extern const size_t gFrlgTitleLocalLzResourceCount;
#endif
