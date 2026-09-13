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
preproc="$upstream/tools/preproc/preproc"
if [ ! -x "$preproc" ]; then
    printf '%s\n' 'Build the locked upstream tools/preproc first.' >&2
    exit 2
fi
arm_bin="${DEVKITARM:?Set DEVKITARM}/bin"
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/frlg-startup-compile.XXXXXX")
trap 'rm -rf "$test_dir"' EXIT HUP INT TERM
set -- -std=gnu11 -DFIRERED -DENGLISH -DREVISION=0 -DMODERN=1 \
    -iquote "$repo_root/compat/upstream-full/include" \
    -iquote "$repo_root/compat/upstream-native/include" \
    -iquote "$repo_root/compat/include" -iquote "$upstream/include"
"$arm_bin/arm-none-eabi-cpp" "$@" "$upstream/src/graphics.c" -o "$test_dir/graphics.i"
python3 "$repo_root/scripts/select-title-assets.py" "$test_dir/graphics.i" > "$test_dir/title_assets.c"
for module in sprite intro title_screen new_menu_helpers title_assets; do
    source="$upstream/src/$module.c"
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
