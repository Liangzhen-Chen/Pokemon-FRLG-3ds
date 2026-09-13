#!/usr/bin/env python3
"""Report unresolved live relocations after GC; never claim a runnable game."""
import argparse
import json
from pathlib import Path
import re
import subprocess


def unresolved_references(symbols, relocations):
    undefined = set(re.findall(r"^\s*U\s+(\S+)\s*$", symbols, re.MULTILINE))
    missing = {}
    section = None
    for line in relocations.splitlines():
        header = re.fullmatch(r"RELOCATION RECORDS FOR \[(.+)\]:", line)
        if header:
            section = header.group(1)
            continue
        fields = line.split()
        if not section or section.startswith((".debug", ".zdebug")) or len(fields) != 3:
            continue
        if not re.fullmatch(r"[0-9a-fA-F]+", fields[0]) or not fields[1].startswith("R_ARM_"):
            continue
        symbol = re.sub(r"[+-]0x[0-9a-fA-F]+$", "", fields[2])
        if symbol in undefined:
            missing.setdefault(symbol, set()).add(section)
    return {symbol: sorted(sections) for symbol, sections in sorted(missing.items())}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--arm-bin", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("objects", nargs="+", type=Path)
    args = parser.parse_args()
    subprocess.run([str(args.arm_bin / "arm-none-eabi-ld"), "-r", "--gc-sections",
                    "-u", "CB2_InitCopyrightScreenAfterBootup", *map(str, args.objects),
                    "-o", str(args.output)], check=True)
    def inspect(tool, *flags):
        return subprocess.check_output([str(args.arm_bin / tool), *flags, str(args.output)], text=True)
    defined = inspect("arm-none-eabi-nm", "--defined-only")
    if not re.search(r"^\S+\s+[Tt]\s+CB2_InitCopyrightScreenAfterBootup$", defined, re.MULTILINE):
        raise RuntimeError("The requested copyright entrypoint is absent from the linked closure")
    missing = unresolved_references(inspect("arm-none-eabi-nm", "-u"),
                                    inspect("arm-none-eabi-objdump", "-r"))
    print(json.dumps({"root": "CB2_InitCopyrightScreenAfterBootup", "runnable": False,
                      "unresolved_count": len(missing), "unresolved_references": missing}, indent=2))
    return 1 if missing else 0


if __name__ == "__main__":
    raise SystemExit(main())
