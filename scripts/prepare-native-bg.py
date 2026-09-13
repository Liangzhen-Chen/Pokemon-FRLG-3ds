#!/usr/bin/env python3
"""Emit the locked background source with defined unsigned DMA bit masks.

Callers verify the upstream revision first. Never modifies the source file.
"""
from pathlib import Path
import sys


def prepare(source):
    replacements = {"(1 << (cursor % 0x20))": 3, "(1 << mod)": 2}
    for expression, count in replacements.items():
        if source.count(expression) != count:
            raise ValueError(f"upstream DMA mask pattern changed: {expression}")
    for expression in replacements:
        source = source.replace(expression, expression.replace("1 <<", "1u <<"))
    return source


if __name__ == "__main__":
    print(prepare(Path(sys.argv[1]).read_text()), end="")
