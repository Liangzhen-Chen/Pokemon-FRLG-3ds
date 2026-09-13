#!/usr/bin/env python3
"""Generate a bounded native-only copy of the locked FireRed save parser."""

import hashlib
import os
from pathlib import Path
import sys
import tempfile

SOURCE_SHA256 = "24f0b70d72cc83c50f7b2d6d73cb211c9b1272ab5fa40dc1d553ad48d1f4511c"
COPY_ANCHOR = "        id = gSaveDataBufferPtr->id;\n        if (id == 0)"
STATUS_ANCHOR = (
    "            signatureValid = TRUE;\n"
    "            checksum = CalculateChecksum(gSaveDataBufferPtr->data, "
    "locations[gSaveDataBufferPtr->id].size);"
)


def transform(source: str) -> str:
    if source.count(COPY_ANCHOR) != 1 or source.count(STATUS_ANCHOR) != 2:
        raise ValueError("locked save parser anchors changed")
    source = source.replace(
        COPY_ANCHOR,
        "        id = gSaveDataBufferPtr->id;\n"
        "        if (gSaveDataBufferPtr->signature != SECTOR_SIGNATURE "
        "|| id >= NUM_SECTORS_PER_SLOT)\n"
        "            continue;\n"
        "        if (id == 0)",
    )
    return source.replace(
        STATUS_ANCHOR,
        "            signatureValid = TRUE;\n"
        "            if (gSaveDataBufferPtr->id >= NUM_SECTORS_PER_SLOT)\n"
        "                continue;\n"
        "            checksum = CalculateChecksum(gSaveDataBufferPtr->data, "
        "locations[gSaveDataBufferPtr->id].size);",
    )


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: prepare-native-save.py SOURCE OUTPUT", file=sys.stderr)
        return 2
    source_path, output_path = map(Path, sys.argv[1:])
    if output_path.exists():
        print("output already exists", file=sys.stderr)
        return 1
    source = source_path.read_bytes()
    if hashlib.sha256(source).hexdigest() != SOURCE_SHA256:
        print("locked save parser SHA-256 mismatch", file=sys.stderr)
        return 1
    try:
        generated = transform(source.decode("utf-8"))
    except ValueError as exc:
        print(exc, file=sys.stderr)
        return 1

    descriptor, temporary = tempfile.mkstemp(prefix=".native-save-", dir=output_path.parent)
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
