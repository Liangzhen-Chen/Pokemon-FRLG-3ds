#!/usr/bin/env python3
"""Generate a native-only copy of the locked FireRed title screen source."""

import hashlib
import os
from pathlib import Path
import sys
import tempfile


SOURCE_SHA256 = "c2521f842e2f7385f429465330c50c663a88b24da7d419e6dbe77362643f56d5"
NATIVE_LZ_TABLE = """
#include "frlg_native_title_assets.h"
const FrlgNativeLzResource gFrlgTitleLocalLzResources[] = {
    {(const uint8_t *)sBorderBgTiles, sizeof(sBorderBgTiles)},
    {(const uint8_t *)sBorderBgMap, sizeof(sBorderBgMap)},
    {(const uint8_t *)sFlames_Gfx, sizeof(sFlames_Gfx)},
    {(const uint8_t *)sBlankFlames_Gfx, sizeof(sBlankFlames_Gfx)},
    {(const uint8_t *)sSlash_Gfx, sizeof(sSlash_Gfx)},
};
const size_t gFrlgTitleLocalLzResourceCount = sizeof(gFrlgTitleLocalLzResources) / sizeof(gFrlgTitleLocalLzResources[0]);
"""


def transform(source: str) -> str:
    return source + NATIVE_LZ_TABLE


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: prepare-native-title-screen.py SOURCE OUTPUT", file=sys.stderr)
        return 2
    source_path, output_path = map(Path, sys.argv[1:])
    source = source_path.read_bytes()
    if hashlib.sha256(source).hexdigest() != SOURCE_SHA256:
        print("locked title_screen SHA-256 mismatch", file=sys.stderr)
        return 1

    descriptor, temporary = tempfile.mkstemp(prefix=".native-title-", dir=output_path.parent)
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8", newline="") as stream:
            stream.write(transform(source.decode("utf-8")))
        os.replace(temporary, output_path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
