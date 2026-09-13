#!/bin/sh
set -eu
if [ "${FRLG_ENABLE_UPSTREAM_LOCAL:-0}" != 1 ]; then
    printf '%s\n' 'Disabled: set FRLG_ENABLE_UPSTREAM_LOCAL=1' >&2
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
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/frlg-native-multiboot.XXXXXX")
trap 'rm -rf "$test_dir"' EXIT HUP INT TERM
cd "$repo_root"
set -- -std=gnu11 -Wall -Wextra -Werror -fno-strict-aliasing \
    -DFIRERED -DENGLISH -DREVISION=0 -DMODERN=1 \
    -iquote compat/upstream-full/include -iquote compat/upstream-native/include \
    -iquote compat/include -iquote external/pokefirered/include
case "${FRLG_NATIVE_SANITIZE:-0}" in
    0) ;;
    1) set -- "$@" -fsanitize=address,undefined -fno-sanitize-recover=all -g ;;
    *) printf '%s\n' 'FRLG_NATIVE_SANITIZE must be 0 or 1' >&2; exit 2 ;;
esac
cc "$@" compat/upstream-full/src/frlg_native_multiboot.c \
    compat/upstream-native/src/frlg_native_io.c compat/src/frlg_gba_memory.c \
    compat/src/frlg_gba_lz77.c tests/upstream/native_multiboot_test.c \
    -o "$test_dir/native_multiboot_test"
for scenario in normal serial execute transfer; do
    "$test_dir/native_multiboot_test" "$scenario"
done
printf '%s\n' 'Native multiboot local host scenarios passed.'
armcc="${DEVKITARM:?Set DEVKITARM}/bin/arm-none-eabi-gcc"
"$armcc" -std=gnu11 -O2 -Wall -Wextra -Werror -fno-strict-aliasing \
    -DFIRERED -DENGLISH -DREVISION=0 -DMODERN=1 \
    -march=armv6k -mtune=mpcore -mfloat-abi=hard \
    -iquote compat/upstream-full/include -iquote compat/upstream-native/include \
    -iquote compat/include -iquote external/pokefirered/include \
    -c compat/upstream-full/src/frlg_native_multiboot.c -o "$test_dir/native_multiboot_arm11.o"
printf '%s\n' 'Native multiboot ARM11 object compiled; full target not linked.'
