#!/bin/sh
set -eu
if [ "${FRLG_ENABLE_UPSTREAM_LOCAL:-0}" != 1 ]; then
    printf '%s\n' 'Disabled: use FRLG_ENABLE_UPSTREAM_LOCAL=1 make test-native-gpu-local' >&2
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
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/frlg-native-gpu.XXXXXX")
trap 'rm -rf "$test_dir"' EXIT HUP INT TERM
cd "$repo_root"
# Upstream aliases byte buffers as halfwords; preserve its intended aliasing model.
set -- -std=c11 -Wall -Wextra -Werror -fno-strict-aliasing \
    compat/src/frlg_gba_lz77.c \
    -iquote compat/upstream-native/include -iquote compat/include \
    -iquote external/pokefirered/include
case "${FRLG_NATIVE_SANITIZE:-0}" in
    0) ;;
    1) set -- "$@" -fsanitize=address,undefined -fno-sanitize-recover=all -g ;;
    *) printf '%s\n' 'FRLG_NATIVE_SANITIZE must be 0 or 1' >&2; exit 2 ;;
esac
cc "$@" compat/upstream-native/src/frlg_native_io.c compat/src/frlg_gba_memory.c \
    tests/upstream/native_transfer_test.c -o "$test_dir/native_transfer_test"
"$test_dir/native_transfer_test"
cc "$@" compat/upstream-native/src/frlg_native_io.c compat/src/frlg_gba_memory.c \
    tests/upstream/native_bios_test.c -o "$test_dir/native_bios_test"
"$test_dir/native_bios_test"
for test_name in native_lz77_test native_lz_binding_test; do
    cc "$@" compat/upstream-native/src/frlg_native_io.c compat/src/frlg_gba_memory.c \
        "tests/upstream/$test_name.c" -o "$test_dir/$test_name"
    "$test_dir/$test_name"
done
cc "$@" compat/upstream-native/src/frlg_native_io.c \
    external/pokefirered/src/gpu_regs.c external/pokefirered/src/dma3_manager.c \
    compat/src/frlg_gba_memory.c \
    compat/src/frlg_gba_display.c compat/src/frlg_gba_mode3.c \
    compat/src/frlg_gba_mode0.c tests/upstream/native_gpu_test.c \
    -o "$test_dir/native_gpu_test"
"$test_dir/native_gpu_test"
printf '%s\n' 'Real upstream GPU writes reached the software renderer on the host.'
cc "$@" compat/upstream-native/src/frlg_native_io.c compat/src/frlg_gba_memory.c \
    external/pokefirered/src/dma3_manager.c tests/upstream/native_dma_test.c \
    -o "$test_dir/native_dma_test"
"$test_dir/native_dma_test"
printf '%s\n' 'Real upstream DMA queue copied/filled native and mapped memory.'
# Cross-compile only. Do not execute a game entrypoint or publish derived binaries.
armcc="${DEVKITARM:?Set DEVKITARM}/bin/arm-none-eabi-gcc"
"$armcc" -std=gnu11 -Wall -Wextra -Werror -O2 -march=armv6k -mfloat-abi=hard \
    -iquote compat/upstream-native/include -iquote compat/include \
    -c compat/upstream-native/src/frlg_native_io.c -o "$test_dir/native_io_arm11.o"
"$armcc" -std=gnu11 -O2 -fno-strict-aliasing -march=armv6k -mfloat-abi=hard \
    -iquote compat/upstream-native/include -iquote compat/include \
    -iquote external/pokefirered/include -c external/pokefirered/src/gpu_regs.c \
    -o "$test_dir/gpu_regs_arm11.o"
"$DEVKITARM/bin/arm-none-eabi-nm" -u "$test_dir/gpu_regs_arm11.o" | grep -q frlg_native_io_base
printf '%s\n' 'ARM11 GPU object compiled and references the native IO binding; not a runnable game.'
"$armcc" -std=gnu11 -O2 -fno-strict-aliasing -march=armv6k -mfloat-abi=hard \
    -iquote compat/upstream-native/include -iquote compat/include \
    -iquote external/pokefirered/include -c external/pokefirered/src/dma3_manager.c \
    -o "$test_dir/dma3_manager_arm11.o"
"$DEVKITARM/bin/arm-none-eabi-nm" -u "$test_dir/dma3_manager_arm11.o" | grep -q frlg_native_dma_copy
printf '%s\n' 'ARM11 DMA manager compiled with native transfer hooks.'
"$armcc" -std=gnu11 -Wall -Wextra -Werror -O2 -march=armv6k -mfloat-abi=hard \
    -iquote compat/include -c compat/src/frlg_gba_lz77.c -o "$test_dir/lz77_arm11.o"

if [ "${FRLG_NATIVE_ASSETS:-0}" = 1 ]; then
    gfx="$upstream/tools/gbagfx/gbagfx"
    if [ ! -x "$gfx" ]; then
        printf '%s\n' 'Build the locked upstream tools/gbagfx first.' >&2
        exit 2
    fi
    "$gfx" "$upstream/graphics/intro/copyright.png" "$test_dir/copyright.4bpp"
    "$gfx" "$test_dir/copyright.4bpp" "$test_dir/copyright.4bpp.lz"
    "$gfx" "$upstream/graphics/intro/copyright.bin" "$test_dir/copyright.bin.lz"
    cc "$@" compat/upstream-native/src/frlg_native_io.c compat/src/frlg_gba_memory.c \
        tests/upstream/native_lz_asset_test.c -o "$test_dir/native_lz_asset_test"
    "$test_dir/native_lz_asset_test" "$test_dir/copyright.4bpp.lz" "$test_dir/copyright.4bpp" \
        "$test_dir/copyright.bin.lz" "$upstream/graphics/intro/copyright.bin"
fi

# Full original declarations: no IDE zero-resource or raw-string substitutes.
set -- -std=gnu11 -O2 -fno-strict-aliasing -ffunction-sections -fdata-sections \
    -DFIRERED -DENGLISH -DREVISION=0 -DMODERN=1 \
    -iquote compat/upstream-full/include -iquote compat/upstream-native/include \
    -iquote compat/include -iquote external/pokefirered/include
for module in palette blend_palette gpu_regs malloc menu2 task; do
    "$armcc" "$@" -march=armv6k -mfloat-abi=hard -c "$upstream/src/$module.c" \
        -o "$test_dir/full_${module}_arm11.o"
done
"$armcc" "$@" -march=armv6k -mfloat-abi=hard \
    -c compat/upstream-full/src/frlg_native_main.c -o "$test_dir/native_main_arm11.o"
case "$(uname -s)" in
    Darwin) set -- "$@" -Wl,-dead_strip ;;
    *) set -- "$@" -Wl,--gc-sections ;;
esac
if [ "${FRLG_NATIVE_SANITIZE:-0}" = 1 ]; then
    set -- "$@" -fsanitize=address,undefined -fno-sanitize-recover=all -g
fi
cc "$@" compat/upstream-native/src/frlg_native_io.c compat/src/frlg_gba_memory.c \
    compat/src/frlg_gba_lz77.c tests/upstream/native_full_header_test.c \
    -o "$test_dir/native_full_header_test"
"$test_dir/native_full_header_test"
# Unused palette services are discarded, not replaced with fake game services.
cc "$@" compat/upstream-native/src/frlg_native_io.c compat/src/frlg_gba_memory.c \
    compat/src/frlg_gba_lz77.c compat/src/frlg_gba_display.c compat/src/frlg_gba_mode0.c \
    compat/src/frlg_gba_mode3.c "$upstream/src/palette.c" "$upstream/src/blend_palette.c" \
    "$upstream/src/gpu_regs.c" tests/upstream/native_palette_test.c \
    -o "$test_dir/native_palette_test"
"$test_dir/native_palette_test"
printf '%s\n' 'Original palette fade reached the renderer through full game declarations; ARM11 objects compiled.'
cc "$@" compat/upstream-native/src/frlg_native_io.c compat/src/frlg_gba_memory.c \
    compat/src/frlg_gba_lz77.c "$upstream/src/malloc.c" tests/upstream/native_heap_test.c \
    -o "$test_dir/native_heap_test"
"$test_dir/native_heap_test"
printf '%s\n' 'Original heap allocated, zeroed, coalesced and reset in mapped EWRAM; host aligned cases only.'
# Host-only MODERN=0 excludes ARM register-reset assembly in unused AgbMain;
# the original ReadKeys path is unchanged. ARM11 above uses MODERN=1.
cc "$@" -UMODERN -DMODERN=0 compat/upstream-native/src/frlg_native_io.c \
    compat/src/frlg_gba_memory.c compat/src/frlg_gba_lz77.c compat/upstream-full/src/frlg_native_main.c \
    tests/upstream/native_main_input_test.c -o "$test_dir/native_main_input_test"
"$test_dir/native_main_input_test"
printf '%s\n' 'Original input reader passed edges, repeat and L=A tests; native main ARM11 object compiled.'
cc "$@" compat/upstream-native/src/frlg_native_io.c compat/src/frlg_gba_memory.c \
    compat/src/frlg_gba_lz77.c compat/src/frlg_gba_display.c compat/src/frlg_gba_mode0.c \
    compat/src/frlg_gba_mode3.c "$upstream/src/task.c" "$upstream/src/gpu_regs.c" \
    "$upstream/src/menu2.c" tests/upstream/native_blend_task_test.c -o "$test_dir/native_blend_task_test"
"$test_dir/native_blend_task_test"
printf '%s\n' 'Original scheduler advanced blend coefficients in priority order and removed completed tasks.'

if [ "${FRLG_NATIVE_ASSETS:-0}" = 1 ]; then
    python3 scripts/build-startup-assets.py --output-root "$test_dir/startup-assets"
    cc -std=gnu11 -DFIRERED -DENGLISH -DREVISION=0 -DMODERN=1 \
        -iquote compat/upstream-full/include -iquote compat/upstream-native/include \
        -iquote compat/include -iquote external/pokefirered/include \
        -E "$upstream/src/new_menu_helpers.c" -o "$test_dir/menu.i"
    (cd "$test_dir/startup-assets" && "$upstream/tools/preproc/preproc" \
        -i "$upstream/src/new_menu_helpers.c" "$upstream/charmap.txt" \
        < "$test_dir/menu.i" > "$test_dir/menu.c")
    python3 scripts/prepare-native-bg.py "$upstream/src/bg.c" > "$test_dir/native_bg.c"
    cc "$@" compat/upstream-native/src/frlg_native_io.c compat/src/frlg_gba_memory.c \
        compat/src/frlg_gba_lz77.c "$upstream/src/malloc.c" "$test_dir/native_bg.c" \
        "$upstream/src/gpu_regs.c" "$upstream/src/dma3_manager.c" "$test_dir/menu.c" \
        tests/upstream/native_menu_transfer_test.c -o "$test_dir/native_menu_transfer_test"
    "$test_dir/native_menu_transfer_test"
    printf '%s\n' 'Original menu decompression, BG DMA upload and deferred heap release passed on host.'
    cc -std=gnu11 -DFIRERED -DENGLISH -DREVISION=0 -DMODERN=1 \
        -iquote compat/upstream-full/include -iquote compat/upstream-native/include \
        -iquote compat/include -iquote external/pokefirered/include \
        -E "$upstream/src/graphics.c" -o "$test_dir/graphics.i"
    python3 scripts/select-title-assets.py "$test_dir/graphics.i" > "$test_dir/title_assets.c"
    cc -std=gnu11 -DFIRERED -DENGLISH -DREVISION=0 -DMODERN=1 \
        -iquote compat/upstream-full/include -iquote compat/upstream-native/include \
        -iquote compat/include -iquote external/pokefirered/include \
        -E "$test_dir/title_assets.c" -o "$test_dir/title_assets.i"
    (cd "$test_dir/startup-assets" && "$upstream/tools/preproc/preproc" \
        -i "$test_dir/title_assets.c" "$upstream/charmap.txt" \
        < "$test_dir/title_assets.i" > "$test_dir/title_assets.pre")
    cc "$@" compat/upstream-native/src/frlg_native_io.c compat/src/frlg_gba_memory.c \
        compat/src/frlg_gba_lz77.c tests/upstream/native_title_asset_test.c \
        -x c "$test_dir/title_assets.pre" -o "$test_dir/native_title_asset_test"
    "$test_dir/native_title_asset_test" \
        "$test_dir/startup-assets/graphics/title_screen/firered/game_title_logo.8bpp" \
        "$upstream/graphics/title_screen/firered/game_title_logo.bin" \
        "$test_dir/startup-assets/graphics/title_screen/firered/box_art_mon.4bpp" \
        "$upstream/graphics/title_screen/firered/box_art_mon.bin" \
        "$test_dir/startup-assets/graphics/title_screen/copyright_press_start.4bpp" \
        "$upstream/graphics/title_screen/copyright_press_start.bin" \
        "$test_dir/startup-assets/graphics/title_screen/blank_sprite.4bpp"
fi
