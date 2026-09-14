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
python3 scripts/prepare-native-audio.py --upstream "$upstream" --output-dir "$test_dir/assets"
python3 - "$test_dir/assets/songs/mus_intro_fight.o" "$test_dir" <<'PY'
from pathlib import Path
import re
import struct
import subprocess
import sys

song_object = Path(sys.argv[1])
test_dir = Path(sys.argv[2])
symbols = subprocess.check_output(["arm-none-eabi-nm", "-a", str(song_object)], text=True)
match = re.search(r"^([0-9a-fA-F]+)\s+[Rr]\s+mus_intro_fight_8$", symbols, re.MULTILINE)
if match is None:
    raise SystemExit("Locked MUS_INTRO_FIGHT track 8 is missing")
section = test_dir / "intro_fight_rodata.bin"
subprocess.run(["arm-none-eabi-objcopy", "--dump-section", f".rodata={section}", str(song_object)], check=True)
start = int(match.group(1), 16)
prefix = section.read_bytes()[start:start + 21]
if prefix != bytes.fromhex("bc00bd7fc22ccd08120810c10cbf40be44d24f7f83"):
    raise SystemExit("Locked MUS_INTRO_FIGHT track 8 prefix changed")
(test_dir / "intro_fight_track8.inc").write_text("".join(f"0x{byte:02x}, " for byte in prefix))
wave = song_object.parent.parent / "sound/direct_sound_samples/cries/nidorino.bin"
wave_data = wave.read_bytes()
type_, status, freq, loop_start, size = struct.unpack_from("<HHIII", wave_data)
if (type_, status, freq, loop_start, size) != (1, 0, 0x00A44000, 0, 5960):
    raise SystemExit("Locked Nidorino cry wave header changed")
(test_dir / "nidorino_wave.inc").write_text(
    ", ".join(str(value) for value in (type_, status, freq, loop_start, size))
    + ", {" + ", ".join(str(value) for value in wave_data[16:20]) + "}"
)
sound_object = song_object.parent.parent / "sound_data.o"
sound_symbols = subprocess.check_output(["arm-none-eabi-nm", str(sound_object)], text=True)
cry_table = re.search(r"^([0-9a-fA-F]+)\s+[Rr]\s+gCryTable$", sound_symbols, re.MULTILINE)
if cry_table is None:
    raise SystemExit("Locked cry table is missing")
subprocess.run(["arm-none-eabi-objcopy", "--dump-section", f".rodata={section}", str(sound_object)], check=True)
tone_offset = int(cry_table.group(1), 16) + 32 * 12
if section.read_bytes()[tone_offset:tone_offset + 12] != bytes.fromhex("203c000000000000ff00ff00"):
    raise SystemExit("Locked Nidorino cry tone changed")
relocations = subprocess.check_output(["arm-none-eabi-readelf", "-Wr", str(sound_object)], text=True)
if re.search(rf"^{tone_offset + 4:08x}\s+.*\bCry_Nidorino$", relocations, re.MULTILINE) is None:
    raise SystemExit("Locked Nidorino cry tone does not reference its wave")
PY
set -- -std=gnu11 -O2 -fno-strict-aliasing -ffunction-sections -fdata-sections \
    -DFIRERED -DENGLISH -DREVISION=0 -DMODERN=1 \
    -iquote compat/upstream-full/include -iquote compat/upstream-native/include \
    -iquote compat/include -iquote external/pokefirered/include -iquote "$test_dir"
cc "$@" -c "$upstream/src/sound.c" -o "$test_dir/sound.o"
if ! nm "$test_dir/sound.o" | grep -Eq '[[:space:]][Tt][[:space:]]_?PlaySE$'; then
    printf '%s\n' 'Original sound.o does not define PlaySE.' >&2
    exit 1
fi
cc "$@" -Wall -Wextra -Werror -c tests/upstream/native_audio_event_test.c -o "$test_dir/test.o"
cc "$@" -Wall -Wextra -Werror -c compat/upstream-full/src/frlg_native_audio.c -o "$test_dir/audio.o"
cc "$@" -Wall -Wextra -Werror -c compat/upstream-full/src/frlg_native_music_player.c -o "$test_dir/music_player.o"
cc "$@" -c "$upstream/src/m4a_tables.c" -o "$test_dir/m4a_tables.o"
case "$(uname -s)" in
    Darwin) set -- "$@" -Wl,-dead_strip ;;
    *) set -- "$@" -Wl,--gc-sections ;;
esac
cc "$@" "$test_dir/sound.o" "$test_dir/test.o" "$test_dir/audio.o" \
    "$test_dir/music_player.o" "$test_dir/m4a_tables.o" -o "$test_dir/native_audio_event_test"
"$test_dir/native_audio_event_test"

test -s "$test_dir/assets/objects.txt"
grep -Fxq 'sound_data.o' "$test_dir/assets/objects.txt"
arm-none-eabi-nm "$test_dir/assets/sound_data.o" | grep -Eq '[[:space:]][Rr][[:space:]]gSongTable$'
arm-none-eabi-nm "$test_dir/assets/sound_data.o" | grep -Eq '[[:space:]][Rr][[:space:]]gMPlayTable$'
arm-none-eabi-nm "$test_dir/assets/sound_data.o" | grep -Eq '[[:space:]][Rr][[:space:]]gCryTable$'
arm-none-eabi-gcc -std=gnu11 -O2 -Wall -Wextra -Werror \
    -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft \
    -DFIRERED -DENGLISH -DREVISION=0 -DMODERN=1 \
    -iquote compat/upstream-full/include -iquote compat/upstream-native/include \
    -iquote compat/include -iquote external/pokefirered/include \
    -c compat/upstream-full/src/frlg_native_music_player.c -o "$test_dir/assets/music_player.o"
python3 - "$test_dir/assets" <<'PY'
from pathlib import Path
import re
import subprocess
import sys

assets = Path(sys.argv[1])
objects = [assets / name for name in (assets / "objects.txt").read_text().splitlines()]
bad = []
for obj in objects:
    relocations = subprocess.check_output(["arm-none-eabi-readelf", "-Wr", str(obj)], text=True)
    for line in relocations.splitlines():
        match = re.match(r"\s*([0-9a-fA-F]+)\s+[0-9a-fA-F]+\s+(R_ARM_\S+)", line)
        if match and (match.group(2) != "R_ARM_ABS32" or int(match.group(1), 16) % 4):
            bad.append(f"{obj.relative_to(assets)}:{match.group(1)}:{match.group(2)}")
if bad:
    raise SystemExit(f"{len(bad)} unsupported ARM relocations; first: {bad[0]}")
subprocess.run(["arm-none-eabi-ld", "-r", "-o", str(assets / "audio-closure.o"),
                str(assets / "music_player.o"), *map(str, objects)], check=True)
PY
