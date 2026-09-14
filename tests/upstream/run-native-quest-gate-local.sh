#!/bin/sh
set -eu
if [ "${FRLG_ENABLE_UPSTREAM_LOCAL:-0}" != 1 ]; then
    printf '%s\n' 'Disabled: set FRLG_ENABLE_UPSTREAM_LOCAL=1' >&2
    exit 2
fi
repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
upstream="$repo_root/external/pokefirered"
expected=$(awk -F '\t' '$1 == "external/pokefirered" {print $3}' "$repo_root/config/source-dependencies.tsv")
if [ -z "$expected" ] || [ "$(git -C "$upstream" rev-parse HEAD)" != "$expected" ] ||
   [ -n "$(git -C "$upstream" status --porcelain --untracked-files=no)" ]; then
    printf '%s\n' 'Upstream must match its locked, unmodified revision.' >&2
    exit 1
fi
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/frlg-native-quest-gate.XXXXXX")
trap 'rm -rf "$test_dir"' EXIT HUP INT TERM
cd "$repo_root"
set -- -std=gnu11 -O0 -Wall -Wextra -Werror -Wno-ignored-qualifiers \
    -fno-strict-aliasing -ffunction-sections -fdata-sections \
    -DFIRERED -DENGLISH -DREVISION=0 -DMODERN=0 \
    -iquote compat/upstream-full/include -iquote compat/upstream-native/include \
    -iquote compat/include -iquote external/pokefirered/include
case "${FRLG_NATIVE_SANITIZE:-0}" in
    0) ;;
    1) set -- "$@" -fsanitize=address,undefined -fno-sanitize-recover=all -g ;;
    *) printf '%s\n' 'FRLG_NATIVE_SANITIZE must be 0 or 1' >&2; exit 2 ;;
esac
if [ "$(uname)" = Darwin ]; then
    set -- "$@" -Wl,-dead_strip
else
    set -- "$@" -Wl,--gc-sections
fi
cc "$@" compat/upstream-full/src/frlg_native_main.c \
    compat/upstream-full/src/frlg_native_quest_state.c \
    tests/upstream/native_quest_gate_test.c -o "$test_dir/test"
"$test_dir/test"
printf '%s\n' 'Native quest gate host scenario passed.'
