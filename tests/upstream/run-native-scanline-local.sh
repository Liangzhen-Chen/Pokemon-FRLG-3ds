#!/bin/sh
set -eu
if [ "${FRLG_ENABLE_UPSTREAM_LOCAL:-0}" != 1 ]; then
    printf '%s\n' 'Disabled: set FRLG_ENABLE_UPSTREAM_LOCAL=1 for native scanline tests.' >&2
    exit 2
fi
repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
upstream="$repo_root/external/pokefirered"
expected=$(awk -F '\t' '$1 == "external/pokefirered" {print $3}' "$repo_root/config/source-dependencies.tsv")
actual=$(git -C "$upstream" rev-parse HEAD)
if [ -z "$expected" ] || [ "$actual" != "$expected" ] ||
   [ -n "$(git -C "$upstream" status --porcelain --untracked-files=no)" ]; then
    printf '%s\n' 'Upstream must be at its locked commit with no tracked modifications.' >&2
    exit 1
fi
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/frlg-native-scanline.XXXXXX")
trap 'rm -rf "$test_dir"' EXIT HUP INT TERM
cd "$repo_root"
cc -std=c11 -Wall -Wextra -Werror -fno-strict-aliasing \
    -DFIRERED -DENGLISH -DREVISION=0 -DMODERN=1 \
    -iquote compat/upstream-full/include -iquote compat/upstream-native/include \
    -iquote compat/include \
    -iquote external/pokefirered/include \
    compat/upstream-native/src/frlg_native_io.c compat/src/frlg_gba_memory.c \
    compat/src/frlg_gba_lz77.c \
    compat/upstream-full/src/frlg_native_scanline.c tests/upstream/native_scanline_test.c \
    -o "$test_dir/native_scanline_test"
"$test_dir/native_scanline_test"
"$test_dir/native_scanline_test" dest
"$test_dir/native_scanline_test" control
"$test_dir/native_scanline_test" state
printf '%s\n' 'Native title scanline host tests passed.'
