#!/usr/bin/env python3
"""Select title declarations from compiler-preprocessed locked graphics.c."""
from pathlib import Path
import re
import sys

SYMBOLS = (
    "gGraphics_TitleScreen_GameTitleLogoPals", "gGraphics_TitleScreen_GameTitleLogoTiles",
    "gGraphics_TitleScreen_GameTitleLogoMap", "gGraphics_TitleScreen_BoxArtMonPals",
    "gGraphics_TitleScreen_BoxArtMonTiles", "gGraphics_TitleScreen_BoxArtMonMap",
    "gGraphics_TitleScreen_BackgroundPals", "gGraphics_TitleScreen_CopyrightPressStartTiles",
    "gGraphics_TitleScreen_CopyrightPressStartMap", "gTitleScreen_Slash_Pal",
    "gTitleScreen_BlankSprite_Tiles",
)


def select(source):
    declarations = []
    compressed = []
    for symbol in SYMBOLS:
        pattern = rf'const\s+(u8|u16|u32)\s+{symbol}\s*\[\s*\]\s*=\s*INCBIN_U(8|16|32)\("([^"]+)"\)\s*;'
        matches = list(re.finditer(pattern, source))
        if len(matches) != 1:
            raise ValueError(f"expected one preprocessed definition for {symbol}")
        match = matches[0]
        path = Path(match.group(3))
        if (match.group(1) != "u" + match.group(2) or ".." in path.parts
                or path.as_posix() != match.group(3) or "leafgreen" in path.parts
                or not match.group(3).startswith("graphics/title_screen/")):
            raise ValueError(f"unexpected title definition: {symbol}")
        declarations.append(match.group(0))
        if path.suffix == ".lz":
            compressed.append(f"    {{(const uint8_t *){symbol}, sizeof({symbol})}},")
    return ('#include "global.h"\n#include "frlg_native_title_assets.h"\n'
            + "\n".join(declarations)
            + "\nconst FrlgNativeLzResource gFrlgTitleLzResources[] = {\n"
            + "\n".join(compressed) + "\n};\n"
            + "const size_t gFrlgTitleLzResourceCount = sizeof(gFrlgTitleLzResources) / sizeof(gFrlgTitleLzResources[0]);\n")


if __name__ == "__main__":
    print(select(Path(sys.argv[1]).read_text()), end="")
