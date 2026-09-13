#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
manifest="$repo_root/config/tool-assets.tsv"
tools_dir="$repo_root/.tools/downloads"
mkdir -p "$tools_dir"

os_name=$(uname -s)
arch_name=$(uname -m)

if [ "$os_name" != "Darwin" ]; then
    printf '%s\n' "Automatic test-tool download currently supports macOS."
    printf '%s\n' "Use the pinned versions in config/tool-assets.tsv on other systems."
    exit 0
fi

case "$arch_name" in
    x86_64) platform="macos-x86_64" ;;
    arm64) platform="macos-universal" ;;
    *) printf '%s\n' "Unsupported macOS architecture: $arch_name" >&2; exit 1 ;;
esac

download_asset() {
    asset_name=$1
    asset_platform=$2
    asset_url=$3
    asset_sha=$4
    output="$tools_dir/$(basename "$asset_url")"

    if [ "$asset_sha" = "REQUIRES_VERIFICATION" ]; then
        printf '%s\n' "Skipping unverified asset: $asset_name ($asset_platform)" >&2
        return
    fi

    if [ -f "$output" ]; then
        actual=$(shasum -a 256 "$output" | awk '{print $1}')
        if [ "$actual" = "$asset_sha" ]; then
            printf '%s\n' "Already verified: $(basename "$output")"
            return
        fi
        printf '%s\n' "Checksum mismatch for existing file: $output" >&2
        exit 1
    fi

    printf '%s\n' "Downloading $asset_name for $asset_platform..."
    curl --fail --location --output "$output.part" "$asset_url"
    actual=$(shasum -a 256 "$output.part" | awk '{print $1}')
    if [ "$actual" != "$asset_sha" ]; then
        printf '%s\n' "Checksum mismatch for downloaded file: $asset_name" >&2
        exit 1
    fi
    mv "$output.part" "$output"
    printf '%s\n' "Verified: $output"
}

while IFS="$(printf '\t')" read -r name target url sha; do
    case "$name" in ''|'#'*) continue ;; esac
    if [ "$target" = "$platform" ] || [ "$name" = "devkitpro-pacman-6.0.2" ] || [ "$name" = "mGBA-0.10.5" ]; then
        download_asset "$name" "$target" "$url" "$sha"
    fi
done < "$manifest"

printf '%s\n' "Test tools and the devkitPro installer were downloaded and verified."
printf '%s\n' "The devkitPro installer is not run automatically."
