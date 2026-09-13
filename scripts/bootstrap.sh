#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repo_root"

git submodule sync
git submodule update --init --jobs 4
"$repo_root/scripts/download-test-tools.sh"
"$repo_root/scripts/check-environment.sh" --allow-missing-toolchain

printf '%s\n' "Setup complete. Run make check after installing devkitPro."
