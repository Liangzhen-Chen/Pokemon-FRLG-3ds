#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
failed=0
allow_missing_toolchain=0

if [ "${1:-}" = "--allow-missing-toolchain" ]; then
    allow_missing_toolchain=1
fi

check_command() {
    if command -v "$1" >/dev/null 2>&1; then
        printf 'ok      %s\n' "$1"
    else
        printf 'missing %s\n' "$1"
        failed=1
    fi
}

printf '%s\n' "Host tools"
check_command git
check_command make
check_command curl
check_command shasum

printf '%s\n' "3DS toolchain"
if [ -x /opt/devkitpro/devkitARM/bin/arm-none-eabi-gcc ]; then
    printf '%s\n' "ok      /opt/devkitpro/devkitARM/bin/arm-none-eabi-gcc"
else
    printf '%s\n' "missing devkitARM (install instructions: README.md)"
    if [ "$allow_missing_toolchain" -ne 1 ]; then
        failed=1
    fi
fi

printf '%s\n' "Pinned sources"
git -C "$repo_root" submodule status || failed=1

if [ "$failed" -ne 0 ]; then
    printf '%s\n' "Environment is incomplete; see README.md." >&2
    exit 1
fi

printf '%s\n' "Environment check passed."
