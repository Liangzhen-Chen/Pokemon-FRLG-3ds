#!/bin/sh
set -eu
mode=${1:-}
repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
upstream="$repo_root/external/pokefirered"
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/frlg-flash.XXXXXX")
trap 'rm -rf "$test_dir"' EXIT HUP INT TERM
test "$(git -C "$upstream" rev-parse HEAD)" = c75f352304d529f6ba92d4f74b9cf8b5c3810788
test "$(shasum -a 256 "$upstream/src/save.c" | awk '{print $1}')" = 24f0b70d72cc83c50f7b2d6d73cb211c9b1272ab5fa40dc1d553ad48d1f4511c
save_source="$upstream/src/save.c"
if [ "$mode" != "--red-original" ]; then
    python3 "$repo_root/scripts/prepare-native-save.py" "$save_source" "$test_dir/save.c"
    save_source="$test_dir/save.c"
fi
extract_functions() {
source=$1
output=$2
shift 2
python3 - "$source" "$output" "$@" <<'PY'
from pathlib import Path
import re
import sys
source = Path(sys.argv[1]).read_text()
functions = []
for name in sys.argv[3:]:
    match = re.search(r"(?:static )?(?:u8|u16|void) " + name + r"\([^;]+\)\n\{", source)
    if match is None:
        raise SystemExit(f"missing original function: {name}")
    depth = 0
    for pos in range(source.index("{", match.start()), len(source)):
        if source[pos] == "{":
            depth += 1
        elif source[pos] == "}":
            depth -= 1
            if depth == 0:
                functions.append(source[match.start():pos + 1])
                break
    else:
        raise SystemExit(f"unclosed original function: {name}")
Path(sys.argv[2]).write_text("\n\n".join(functions) + "\n")
PY
}
extract_functions "$save_source" "$test_dir/save_functions.inc" \
    CopySaveSlotData GetSaveValidStatus TryLoadSaveSlot TryLoadSaveSector LoadGameSave
clang -std=gnu11 -O0 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    -I "$test_dir" "$repo_root/tests/upstream/native_flash_test.c" \
    -o "$test_dir/native-flash-test"
for fixture in empty bad-id valid bad-checksum; do
    "$test_dir/native-flash-test" "$fixture"
done
test "$("$test_dir/native-flash-test" empty)" = 'status=0 counter=0 first=165'

if [ "$mode" = "--red-original" ]; then
    exit 0
fi
mkdir "$test_dir/original"
extract_functions "$upstream/src/save.c" "$test_dir/original/save_functions.inc" \
    CopySaveSlotData GetSaveValidStatus TryLoadSaveSlot TryLoadSaveSector LoadGameSave
clang -std=gnu11 -O0 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    -I "$test_dir/original" "$repo_root/tests/upstream/native_flash_test.c" \
    -o "$test_dir/original-test"
for fixture in valid bad-checksum; do
    original=$("$test_dir/original-test" "$fixture" "$test_dir/original-$fixture.ram")
    generated=$("$test_dir/native-flash-test" "$fixture" "$test_dir/generated-$fixture.ram")
    test "$original" = "$generated"
    cmp "$test_dir/original-$fixture.ram" "$test_dir/generated-$fixture.ram"
done
python3 - "$repo_root/scripts/prepare-native-save.py" "$upstream/src/save.c" "$test_dir" <<'PY'
from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path
import subprocess
import sys
script, source_path, directory = sys.argv[1:]
spec = spec_from_file_location("prepare_native_save", script)
module = module_from_spec(spec)
spec.loader.exec_module(module)
source = Path(source_path).read_text()
for altered in (
    source.replace(module.COPY_ANCHOR, "", 1),
    source.replace(module.COPY_ANCHOR, module.COPY_ANCHOR * 2, 1),
    source.replace(module.STATUS_ANCHOR, "", 1),
    source.replace(module.STATUS_ANCHOR, module.STATUS_ANCHOR * 2, 1),
):
    try:
        module.transform(altered)
    except ValueError:
        pass
    else:
        raise AssertionError("changed anchor was accepted")
bad_source = Path(directory) / "changed-save.c"
bad_output = Path(directory) / "must-not-exist.c"
bad_source.write_text(source + "\n")
result = subprocess.run([sys.executable, script, str(bad_source), str(bad_output)],
                        capture_output=True, text=True)
assert result.returncode != 0 and not bad_output.exists(), result.stderr
PY
python3 - "$test_dir" <<'PY'
from pathlib import Path
import sys
root = Path(sys.argv[1])
(root / "erased").write_bytes(b"\xff" * 131072)
(root / "short").write_bytes(b"\xff" * 131071)
(root / "long").write_bytes(b"\xff" * 131073)
(root / "format").write_bytes(b"\xff" * 131071 + b"\x00")
(root / "loop").symlink_to("loop")
PY
set -- -std=gnu11 -DFIRERED -DENGLISH -DREVISION=0 -DMODERN=1 \
    -iquote "$repo_root/compat/upstream-full/include" \
    -iquote "$repo_root/compat/upstream-native/include" \
    -iquote "$repo_root/compat/include" -iquote "$upstream/include" \
    -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    -ffunction-sections -fdata-sections -fno-strict-aliasing
clang "$@" -include "$repo_root/compat/upstream-full/include/global.h" \
    -c "$upstream/src/agb_flash_1m.c" -o "$test_dir/agb_flash_1m.o"
clang "$@" -c "$repo_root/compat/upstream-full/src/frlg_native_flash.c" \
    -o "$test_dir/frlg_native_flash.o"
clang "$@" -DFRLG_NATIVE_SERVICE_TEST \
    -c "$repo_root/tests/upstream/native_flash_test.c" -o "$test_dir/service_test.o"
clang -fsanitize=address,undefined -Wl,-dead_strip "$test_dir/agb_flash_1m.o" \
    "$test_dir/frlg_native_flash.o" "$test_dir/service_test.o" \
    -o "$test_dir/service-test"
"$test_dir/service-test" "$test_dir/erased" "$test_dir/missing" "$test_dir/short" \
    "$test_dir/long" "$test_dir/format" "$test_dir/loop"
extract_functions "$upstream/src/load_save.c" "$test_dir/flash_check.inc" \
    CheckForFlashMemory
extract_functions "$upstream/src/save.c" "$test_dir/integration_flash_io.inc" \
    ReadFlashSector CalculateChecksum
extract_functions "$save_source" "$test_dir/integration_save_functions.inc" \
    CopySaveSlotData GetSaveValidStatus TryLoadSaveSlot TryLoadSaveSector LoadGameSave
clang "$@" -DFRLG_NATIVE_INTEGRATION_TEST -I "$test_dir" \
    -c "$repo_root/tests/upstream/native_flash_test.c" \
    -o "$test_dir/integration_test.o"
clang -fsanitize=address,undefined -Wl,-dead_strip "$test_dir/agb_flash_1m.o" \
    "$test_dir/frlg_native_flash.o" "$test_dir/integration_test.o" \
    -o "$test_dir/integration-test"
"$test_dir/integration-test" "$test_dir/erased"

if [ "$mode" = "--arm" ]; then
    arm_bin="${DEVKITARM:-/opt/devkitpro/devkitARM}/bin"
    set -- -std=gnu11 -DFIRERED -DENGLISH -DREVISION=0 -DMODERN=1 \
        -iquote "$repo_root/compat/upstream-full/include" \
        -iquote "$repo_root/compat/upstream-native/include" \
        -iquote "$repo_root/compat/include" -iquote "$upstream/include" \
        -include "$repo_root/compat/upstream-full/include/global.h" \
        -O2 -fno-strict-aliasing -ffunction-sections -fdata-sections \
        -march=armv6k -mtune=mpcore -mfloat-abi=hard
    for module in agb_flash_1m load_save save frlg_native_flash; do
        case "$module" in
            save) source="$save_source" ;;
            frlg_native_flash) source="$repo_root/compat/upstream-full/src/frlg_native_flash.c" ;;
            *) source="$upstream/src/$module.c" ;;
        esac
        "$arm_bin/arm-none-eabi-gcc" "$@" -c "$source" -o "$test_dir/$module.o"
    done
    "$arm_bin/arm-none-eabi-ld" -r --gc-sections -u IdentifyFlash -u LoadGameSave \
        "$test_dir/agb_flash_1m.o" "$test_dir/load_save.o" "$test_dir/save.o" \
        "$test_dir/frlg_native_flash.o" -o "$test_dir/flash-partial.o"
    "$arm_bin/arm-none-eabi-nm" -u "$test_dir/flash-partial.o" > "$test_dir/unresolved.txt"
    if "$arm_bin/arm-none-eabi-nm" --defined-only "$test_dir/flash-partial.o" \
        | grep -q WaitForFlashWrite_Common; then
        echo 'unsafe flash wait function survived section collection' >&2
        exit 1
    fi
    for symbol in ReadFlashId ReadFlash SetFlashTimerIntr MX29L010 LE26FV10N1TS \
        DefaultFlash gFlash ProgramFlashSectorAndVerify; do
        count=$("$arm_bin/arm-none-eabi-nm" -g --defined-only \
            "$test_dir/agb_flash_1m.o" "$test_dir/load_save.o" "$test_dir/save.o" \
            "$test_dir/frlg_native_flash.o" \
            | awk -v wanted="$symbol" '$NF == wanted { n++ } END { print n+0 }')
        test "$count" -eq 1
    done
    printf 'ARM four-object partial link: ok; remaining non-B symbols: %s\n' \
        "$(wc -l < "$test_dir/unresolved.txt" | tr -d ' ')"
fi
