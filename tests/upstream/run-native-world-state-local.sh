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
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/frlg-native-world-state.XXXXXX")
trap 'rm -rf "$test_dir"' EXIT HUP INT TERM
cd "$repo_root"
set -- -std=gnu11 -Wall -Wextra -Werror -Wno-ignored-qualifiers -fno-strict-aliasing \
    -DFIRERED -DENGLISH -DREVISION=0 -DMODERN=1 \
    -iquote compat/upstream-full/include -iquote compat/upstream-native/include \
    -iquote compat/include -iquote external/pokefirered/include
case "${FRLG_NATIVE_SANITIZE:-0}" in
    0) ;;
    1) set -- "$@" -fsanitize=address,undefined -fno-sanitize-recover=all -g ;;
    *) printf '%s\n' 'FRLG_NATIVE_SANITIZE must be 0 or 1' >&2; exit 2 ;;
esac
cc "$@" compat/upstream-full/src/frlg_native_world_state.c \
    tests/upstream/native_world_state_test.c -o "$test_dir/test"
"$test_dir/test"
printf '%s\n' 'Native world state host scenarios passed.'
armcc="${DEVKITARM:?Set DEVKITARM}/bin/arm-none-eabi-gcc"
"$armcc" -std=gnu11 -O2 -Wall -Wextra -Werror -Wno-ignored-qualifiers -fno-strict-aliasing \
    -DFIRERED -DENGLISH -DREVISION=0 -DMODERN=1 \
    -march=armv6k -mtune=mpcore -mfloat-abi=hard \
    -iquote compat/upstream-full/include -iquote compat/upstream-native/include \
    -iquote compat/include -iquote external/pokefirered/include \
    -c compat/upstream-full/src/frlg_native_world_state.c -o "$test_dir/world_state_arm11.o"
printf '%s\n' 'Native world state ARM11 object compiled; full target not linked.'
