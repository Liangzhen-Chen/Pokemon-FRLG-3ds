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


def transform(source: str) -> str:
    if (source.count(READY_BLOCK) != 1
            or source.count(RESOURCE_DEFINE) != 1
            or source.count(RESOURCE_DECLARATION) != 1):
        raise ValueError("locked intro Colosseum anchors changed")
    return (source.replace(READY_BLOCK, NATIVE_READY_BLOCK)
            .replace(RESOURCE_DEFINE, "")
            .replace(RESOURCE_DECLARATION, ""))


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
