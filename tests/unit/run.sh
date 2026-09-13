#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
test_dir=${TMPDIR:-/tmp}/frlg-3ds-tests.$$
trap 'rm -rf "$test_dir"' EXIT HUP INT TERM
mkdir -p "$test_dir"

cc -std=c11 -Wall -Wextra -Werror \
    -I"$repo_root/compat/include" \
    -I"$repo_root/core/include" \
    "$repo_root/compat/src/frlg_gba_memory.c" \
    "$repo_root/compat/src/frlg_gba_display.c" \
    "$repo_root/compat/src/frlg_gba_dma.c" \
    "$repo_root/compat/src/frlg_gba_mode3.c" \
    "$repo_root/compat/src/frlg_gba_mode0.c" \
    "$repo_root/compat/src/frlg_keypad.c" \
    "$repo_root/core/src/frlg_runtime_probe.c" \
    "$repo_root/tests/unit/keypad_test.c" \
    "$repo_root/tests/unit/gba_memory_test.c" \
    "$repo_root/tests/unit/gba_io_dma_test.c" \
    "$repo_root/tests/unit/gba_mode3_test.c" \
    "$repo_root/tests/unit/gba_mode0_test.c" \
    "$repo_root/tests/unit/runtime_probe_test.c" \
    -o "$test_dir/frlg_unit_tests"
"$test_dir/frlg_unit_tests"

cc -std=c11 -Wall -Wextra -Werror -I"$repo_root/tests/support" \
    "$repo_root/tests/support/frlg_test_input.c" \
    "$repo_root/tests/unit/test_input_test.c" -o "$test_dir/test_input_tests"
"$test_dir/test_input_tests"

cc -std=c11 -Wall -Wextra -Werror \
    -I"$repo_root/compat/include" -I"$repo_root/compat/upstream-native/include" \
    "$repo_root/compat/upstream-native/src/frlg_native_io.c" \
    "$repo_root/compat/src/frlg_gba_memory.c" \
    "$repo_root/compat/src/frlg_gba_lz77.c" \
    "$repo_root/tests/upstream/native_io_test.c" -o "$test_dir/native_io_test"
"$test_dir/native_io_test"

cc -std=c11 -Wall -Wextra -Werror \
    -I"$repo_root/compat/include" -I"$repo_root/compat/upstream-native/include" \
    "$repo_root/compat/upstream-native/src/frlg_native_io.c" \
    "$repo_root/compat/src/frlg_gba_memory.c" \
    "$repo_root/compat/src/frlg_gba_lz77.c" \
    "$repo_root/tests/upstream/native_transfer_test.c" -o "$test_dir/native_transfer_test"
"$test_dir/native_transfer_test"

cc -std=c11 -Wall -Wextra -Werror \
    -I"$repo_root/compat/include" -I"$repo_root/compat/upstream-native/include" \
    "$repo_root/compat/upstream-native/src/frlg_native_io.c" \
    "$repo_root/compat/src/frlg_gba_memory.c" \
    "$repo_root/compat/src/frlg_gba_lz77.c" \
    "$repo_root/tests/upstream/native_bios_test.c" -o "$test_dir/native_bios_test"
"$test_dir/native_bios_test"

for test_name in native_lz77_test native_lz_binding_test; do
    cc -std=c11 -Wall -Wextra -Werror \
        -I"$repo_root/compat/include" -I"$repo_root/compat/upstream-native/include" \
        "$repo_root/compat/upstream-native/src/frlg_native_io.c" \
        "$repo_root/compat/src/frlg_gba_memory.c" "$repo_root/compat/src/frlg_gba_lz77.c" \
        "$repo_root/tests/upstream/$test_name.c" -o "$test_dir/$test_name"
    "$test_dir/$test_name"
done

printf '%s\n' "Host unit tests passed."
python3 "$repo_root/tests/unit/test_startup_link_audit.py"
python3 "$repo_root/tests/unit/test_startup_assets.py"
python3 "$repo_root/tests/unit/test_native_bg_prepare.py"
python3 "$repo_root/tests/unit/test_title_asset_selection.py"
