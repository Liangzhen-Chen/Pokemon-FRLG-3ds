#!/bin/sh
set -eu

if [ "${FRLG_ENABLE_UPSTREAM_LOCAL:-0}" != 1 ]; then
    printf '%s\n' 'Disabled: set FRLG_ENABLE_UPSTREAM_LOCAL=1.' >&2
    exit 2
fi

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
cd "$repo_root"
upstream=external/pokefirered
expected=$(awk -F '\t' '$1 == "external/pokefirered" {print $3}' config/source-dependencies.tsv)
if [ -z "$expected" ] || [ "$(git -C "$upstream" rev-parse HEAD)" != "$expected" ] ||
   [ -n "$(git -C "$upstream" status --porcelain --untracked-files=no)" ]; then
    printf '%s\n' 'Upstream must match its locked, unmodified revision.' >&2
    exit 1
fi

test_dir=$(mktemp -d "${TMPDIR:-/tmp}/frlg-audio-test.XXXXXX")
trap 'rm -rf "$test_dir"' EXIT HUP INT TERM
set -- -std=gnu11 -O2 -fno-strict-aliasing -ffunction-sections -fdata-sections \
    -DFIRERED -DENGLISH -DREVISION=0 -DMODERN=1 \
    -iquote compat/upstream-full/include -iquote compat/upstream-native/include \
    -iquote compat/include -iquote external/pokefirered/include
cc "$@" -c "$upstream/src/sound.c" -o "$test_dir/sound.o"
if ! nm "$test_dir/sound.o" | grep -Eq '[[:space:]][Tt][[:space:]]_?PlaySE$'; then
    printf '%s\n' 'Original sound.o does not define PlaySE.' >&2
    exit 1
fi
cc "$@" -Wall -Wextra -Werror -c tests/upstream/native_audio_event_test.c -o "$test_dir/test.o"
cc "$@" -Wall -Wextra -Werror -c compat/upstream-full/src/frlg_native_audio.c -o "$test_dir/audio.o"
case "$(uname -s)" in
    Darwin) set -- "$@" -Wl,-dead_strip ;;
    *) set -- "$@" -Wl,--gc-sections ;;
esac
cc "$@" "$test_dir/sound.o" "$test_dir/test.o" "$test_dir/audio.o" -o "$test_dir/native_audio_event_test"
"$test_dir/native_audio_event_test"
