#!/bin/sh
set -eu
if [ "${FRLG_ENABLE_UPSTREAM_LOCAL:-0}" != 1 ]; then
    printf '%s\n' 'Disabled: set FRLG_ENABLE_UPSTREAM_LOCAL=1.' >&2
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
: "${FRLG_NATIVE_ASSET_ROOT:?Set the local generated asset root containing graphics/}"
asset_root=$(CDPATH= cd -- "$FRLG_NATIVE_ASSET_ROOT" && pwd)
preproc="${FRLG_NATIVE_PREPROC:-$upstream/tools/preproc/preproc}"
if [ ! -x "$preproc" ]; then
    printf '%s\n' 'Build the locked upstream tools/preproc first.' >&2
    exit 2
fi
arm_bin="${DEVKITARM:?Set DEVKITARM}/bin"
if [ -n "${FRLG_NATIVE_OUTPUT_DIR:-}" ]; then
    mkdir -p "$FRLG_NATIVE_OUTPUT_DIR"
    test_dir=$(CDPATH= cd -- "$FRLG_NATIVE_OUTPUT_DIR" && pwd)
else
    test_dir=$(mktemp -d "${TMPDIR:-/tmp}/frlg-startup-compile.XXXXXX")
    trap 'rm -rf "$test_dir"' EXIT HUP INT TERM
fi
set -- -std=gnu11 -DFIRERED -DENGLISH -DREVISION=0 -DMODERN=1 -DNDEBUG \
    -iquote "$repo_root/compat/upstream-full/include" \
    -iquote "$repo_root/compat/upstream-native/include" \
    -iquote "$repo_root/compat/include" -iquote "$upstream/include"
if [ -n "${FRLG_NATIVE_OUTPUT_DIR:-}" ]; then
    : "${FRLG_NATIVE_GBAGFX:?Set the local locked gbagfx executable for font assets}"
    mkdir -p "$asset_root/graphics/fonts"
    for font in down_arrows.4bpp down_arrow_3.4bpp down_arrow_4.4bpp keypad_icons.4bpp \
        latin_small.hwlatfont latin_normal.fwlatfont latin_male.fwlatfont latin_female.fwlatfont \
        japanese_small.fwjpnfont japanese_tall.fwjpnfont japanese_normal.fwjpnfont \
        japanese_male.fwjpnfont japanese_female.fwjpnfont japanese_bold.fwjpnfont braille.fwjpnfont; do
        stem=${font%.*}
        "$FRLG_NATIVE_GBAGFX" "$upstream/graphics/fonts/$stem.png" "$asset_root/graphics/fonts/$font"
    done
fi
"$arm_bin/arm-none-eabi-cpp" "$@" "$upstream/src/graphics.c" -o "$test_dir/graphics.i"
python3 "$repo_root/scripts/select-title-assets.py" "$test_dir/graphics.i" > "$test_dir/title_assets.c"
python3 "$repo_root/scripts/prepare-native-intro.py" "$upstream/src/intro.c" "$test_dir/native_intro.c"
modules='sprite intro title_screen new_menu_helpers title_assets'
if [ -n "${FRLG_NATIVE_OUTPUT_DIR:-}" ]; then modules="$modules text braille_text"; fi
for module in $modules; do
    source="$upstream/src/$module.c"
    if [ "$module" = intro ]; then source="$test_dir/native_intro.c"; fi
    if [ "$module" = title_assets ]; then source="$test_dir/title_assets.c"; fi
    "$arm_bin/arm-none-eabi-cpp" "$@" -Wno-trigraphs "$source" \
        -o "$test_dir/$module.i"
    (cd "$asset_root" && "$preproc" -i "$source" "$upstream/charmap.txt" \
        < "$test_dir/$module.i" > "$test_dir/$module.pre")
    "$arm_bin/arm-none-eabi-gcc" "$@" -x c -O2 -fno-strict-aliasing \
        -ffunction-sections -fdata-sections -march=armv6k -mtune=mpcore -mfloat-abi=hard \
        -c "$test_dir/$module.pre" -o "$test_dir/$module.o"
    if "$arm_bin/arm-none-eabi-nm" -u "$test_dir/$module.o" | grep -Eq '[[:space:]]gHeap$'; then
        printf '%s\n' "$module still references the unmapped linker-script heap." >&2
        exit 1
    fi
    printf '%s\n' "$module: real text/assets preprocessed and ARM11 hard-float object compiled; not linked."
done

if [ -n "${FRLG_NATIVE_OUTPUT_DIR:-}" ]; then
    python3 "$repo_root/scripts/prepare-native-bg.py" "$upstream/src/bg.c" > "$test_dir/native_bg.c"
    "$arm_bin/arm-none-eabi-gcc" "$@" -O2 -fno-strict-aliasing \
        -ffunction-sections -fdata-sections -march=armv6k -mtune=mpcore -mfloat-abi=hard \
        -c "$test_dir/native_bg.c" -o "$test_dir/native_bg.o"
    rm -f "$test_dir/native_save.c"
    python3 "$repo_root/scripts/prepare-native-save.py" "$upstream/src/save.c" "$test_dir/native_save.c"
    "$arm_bin/arm-none-eabi-gcc" "$@" -O2 -fno-strict-aliasing \
        -ffunction-sections -fdata-sections -march=armv6k -mtune=mpcore -mfloat-abi=hard \
        -c "$test_dir/native_save.c" -o "$test_dir/native_save.o"
fi

if [ "${FRLG_STARTUP_LINK_AUDIT:-0}" = 1 ]; then
    python3 "$repo_root/scripts/prepare-native-bg.py" "$upstream/src/bg.c" > "$test_dir/bg.c"
    for module in main bg palette blend_palette gpu_regs dma3_manager task malloc menu2 window blit decompress trig random math_util; do
        source="$upstream/src/$module.c"
        if [ "$module" = main ]; then source="$repo_root/compat/upstream-full/src/frlg_native_main.c"; fi
        if [ "$module" = bg ]; then source="$test_dir/bg.c"; fi
        "$arm_bin/arm-none-eabi-gcc" "$@" -O2 -fno-strict-aliasing \
            -ffunction-sections -fdata-sections -march=armv6k -mtune=mpcore -mfloat-abi=hard \
            -c "$source" -o "$test_dir/$module.o"
    done
    for source in "$repo_root/compat/upstream-native/src/frlg_native_io.c" \
        "$repo_root/compat/src/frlg_gba_memory.c" "$repo_root/compat/src/frlg_gba_lz77.c"; do
        "$arm_bin/arm-none-eabi-gcc" "$@" -O2 -ffunction-sections -fdata-sections \
            -march=armv6k -mtune=mpcore -mfloat-abi=hard -c "$source" \
            -o "$test_dir/$(basename "$source" .c).o"
    done
    # Exit nonzero while live dependencies are missing. This is not an executable link.
    python3 "$repo_root/tests/upstream/startup-link-audit.py" --arm-bin "$arm_bin" \
        --output "$test_dir/startup-closure.o" "$test_dir"/*.o
fi
