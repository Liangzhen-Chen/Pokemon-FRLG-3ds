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
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/frlg-native-reset-state.XXXXXX")
trap 'rm -rf "$test_dir"' EXIT HUP INT TERM
cd "$repo_root"
set -- -std=gnu11 -Wall -Wextra -Werror -Wno-ignored-qualifiers -fno-strict-aliasing \
    -ffunction-sections -fdata-sections \
    -DFIRERED -DENGLISH -DREVISION=0 -DMODERN=1 \
    -iquote compat/upstream-full/include -iquote compat/upstream-native/include \
    -iquote compat/include -iquote external/pokefirered/include
case "${FRLG_NATIVE_SANITIZE:-0}" in
    0) ;;
    1) set -- "$@" -fsanitize=address,undefined -fno-sanitize-recover=all -g ;;
    *) printf '%s\n' 'FRLG_NATIVE_SANITIZE must be 0 or 1' >&2; exit 2 ;;
esac
cc "$@" -DClearSav2=UpstreamClearSav2Unused -c "$upstream/src/load_save.c" -o "$test_dir/load_save.o"
cc "$@" -c "$upstream/src/random.c" -o "$test_dir/random.o"
cc "$@" -c compat/upstream-full/src/frlg_native_save_state.c -o "$test_dir/save_state.o"
cc "$@" -c compat/upstream-full/src/frlg_native_quest_state.c -o "$test_dir/quest_state.o"
cc "$@" -c compat/upstream-full/src/frlg_native_reset_state.c -o "$test_dir/reset_state.o"
cc "$@" -c tests/upstream/native_reset_state_test.c -o "$test_dir/test.o"
cc "$@" -Wl,-dead_strip "$test_dir/load_save.o" "$test_dir/random.o" \
    "$test_dir/save_state.o" "$test_dir/quest_state.o" "$test_dir/reset_state.o" \
    "$test_dir/test.o" -o "$test_dir/test"
"$test_dir/test"
printf '%s\n' 'Native M1 reset state host scenarios passed.'
armcc="${DEVKITARM:?Set DEVKITARM}/bin/arm-none-eabi-gcc"
"$armcc" -std=gnu11 -O2 -Wall -Wextra -Werror -Wno-ignored-qualifiers -fno-strict-aliasing \
    -DFIRERED -DENGLISH -DREVISION=0 -DMODERN=1 \
    -march=armv6k -mtune=mpcore -mfloat-abi=hard \
    -iquote compat/upstream-full/include -iquote compat/upstream-native/include \
    -iquote compat/include -iquote external/pokefirered/include \
    -c compat/upstream-full/src/frlg_native_reset_state.c -o "$test_dir/reset_state_arm11.o"
printf '%s\n' 'Native M1 reset state ARM11 object compiled; full target not linked.'
