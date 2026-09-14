#!/usr/bin/env python3
"""Generate a native-only copy of the locked FireRed intro source."""

import hashlib
import os
from pathlib import Path
import sys
import tempfile


SOURCE_SHA256 = "b34890d753fe07cb471084c6935ef029c300cbd48b3139943dd6d124c8b7e8c2"
READY_BLOCK = (
    "                if (sGcmb.gcmb_field_2 == 2)\n"
    "                {\n"
    "                    if (*(u32 *)(EWRAM_START + 0xAC) == COLOSSEUM_GAME_CODE)\n"
    "                    {\n"
    "                        CpuCopy16(gMultiBootProgram_PokemonColosseum_Start, (void *)EWRAM_START, 0x28000);\n"
    "                        *(u32 *)(EWRAM_START + 0xAC) = COLOSSEUM_GAME_CODE;\n"
    "                    }\n"
    "                    GameCubeMultiBoot_ExecuteProgram(&sGcmb);\n"
    "                }"
)
NATIVE_READY_BLOCK = (
    "                if (sGcmb.gcmb_field_2 == 2)\n"
    "                {\n"
    "                    GameCubeMultiBoot_ExecuteProgram(&sGcmb);\n"
    "                }"
)
RESOURCE_DEFINE = '#define COLOSSEUM_GAME_CODE 0x65366347 // "Gc6e" in ASCII\n'
RESOURCE_DECLARATION = "extern const u32 gMultiBootProgram_PokemonColosseum_Start[];\n"
NATIVE_LZ_TABLE = """
#include "frlg_native_title_assets.h"
const FrlgNativeLzResource gFrlgIntroLzResources[] = {
    {(const uint8_t *)sCopyright_Gfx, sizeof(sCopyright_Gfx)},
    {(const uint8_t *)sCopyright_Map, sizeof(sCopyright_Map)},
    {(const uint8_t *)sGameFreakText_Gfx, sizeof(sGameFreakText_Gfx)},
    {(const uint8_t *)sGameFreakLogo_Gfx, sizeof(sGameFreakLogo_Gfx)},
    {(const uint8_t *)sGameFreakBg_Gfx, sizeof(sGameFreakBg_Gfx)},
    {(const uint8_t *)sGameFreakBg_Map, sizeof(sGameFreakBg_Map)},
    {(const uint8_t *)sScene1_Bg_Gfx, sizeof(sScene1_Bg_Gfx)},
    {(const uint8_t *)sScene1_Bg_Map, sizeof(sScene1_Bg_Map)},
    {(const uint8_t *)sScene1_Grass_Gfx, sizeof(sScene1_Grass_Gfx)},
    {(const uint8_t *)sScene1_Grass_Map, sizeof(sScene1_Grass_Map)},
    {(const uint8_t *)sScene2_Bg_Gfx, sizeof(sScene2_Bg_Gfx)},
    {(const uint8_t *)sScene2_Bg_Map, sizeof(sScene2_Bg_Map)},
    {(const uint8_t *)sScene2_Plants_Gfx, sizeof(sScene2_Plants_Gfx)},
    {(const uint8_t *)sScene2_Plants_Map, sizeof(sScene2_Plants_Map)},
    {(const uint8_t *)sScene2_NidorinoClose_Gfx, sizeof(sScene2_NidorinoClose_Gfx)},
    {(const uint8_t *)sScene2_NidorinoClose_Map, sizeof(sScene2_NidorinoClose_Map)},
    {(const uint8_t *)sScene2_GengarClose_Gfx, sizeof(sScene2_GengarClose_Gfx)},
    {(const uint8_t *)sScene2_GengarClose_Map, sizeof(sScene2_GengarClose_Map)},
    {(const uint8_t *)sScene3_Bg_Gfx, sizeof(sScene3_Bg_Gfx)},
    {(const uint8_t *)sScene3_Bg_Map, sizeof(sScene3_Bg_Map)},
    {(const uint8_t *)sScene3_GengarAnim_Gfx, sizeof(sScene3_GengarAnim_Gfx)},
    {(const uint8_t *)sScene3_GengarAnim_Map, sizeof(sScene3_GengarAnim_Map)},
    {(const uint8_t *)sStar_Gfx, sizeof(sStar_Gfx)},
    {(const uint8_t *)sSparklesSmall_Gfx, sizeof(sSparklesSmall_Gfx)},
    {(const uint8_t *)sSparklesBig_Gfx, sizeof(sSparklesBig_Gfx)},
    {(const uint8_t *)sPresents_Gfx, sizeof(sPresents_Gfx)},
    {(const uint8_t *)sScene2_Gengar_Gfx, sizeof(sScene2_Gengar_Gfx)},
    {(const uint8_t *)sScene2_Nidorino_Gfx, sizeof(sScene2_Nidorino_Gfx)},
    {(const uint8_t *)sScene3_Nidorino_Gfx, sizeof(sScene3_Nidorino_Gfx)},
    {(const uint8_t *)sScene3_Grass_Gfx, sizeof(sScene3_Grass_Gfx)},
    {(const uint8_t *)sScene3_GengarStatic_Gfx, sizeof(sScene3_GengarStatic_Gfx)},
    {(const uint8_t *)sScene3_Swipe_Gfx, sizeof(sScene3_Swipe_Gfx)},
    {(const uint8_t *)sScene3_RecoilDust_Gfx, sizeof(sScene3_RecoilDust_Gfx)},
};
const size_t gFrlgIntroLzResourceCount = sizeof(gFrlgIntroLzResources) / sizeof(gFrlgIntroLzResources[0]);
"""


def transform(source: str) -> str:
    if (source.count(READY_BLOCK) != 1
            or source.count(RESOURCE_DEFINE) != 1
            or source.count(RESOURCE_DECLARATION) != 1):
        raise ValueError("locked intro Colosseum anchors changed")
    return (source.replace(READY_BLOCK, NATIVE_READY_BLOCK)
            .replace(RESOURCE_DEFINE, "")
            .replace(RESOURCE_DECLARATION, "") + NATIVE_LZ_TABLE)


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: prepare-native-intro.py SOURCE OUTPUT", file=sys.stderr)
        return 2
    source_path, output_path = map(Path, sys.argv[1:])
    source = source_path.read_bytes()
    if hashlib.sha256(source).hexdigest() != SOURCE_SHA256:
        print("locked intro SHA-256 mismatch", file=sys.stderr)
        return 1
    try:
        generated = transform(source.decode("utf-8"))
    except ValueError as exc:
        print(exc, file=sys.stderr)
        return 1

    descriptor, temporary = tempfile.mkstemp(prefix=".native-intro-", dir=output_path.parent)
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8", newline="") as stream:
            stream.write(generated)
        os.replace(temporary, output_path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
