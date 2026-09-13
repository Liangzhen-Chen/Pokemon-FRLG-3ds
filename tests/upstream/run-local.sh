#!/bin/sh
set -eu

if [ "${FRLG_ENABLE_UPSTREAM_LOCAL:-0}" != "1" ]; then
    printf '%s\n' "Local upstream probe is disabled. Run: FRLG_ENABLE_UPSTREAM_LOCAL=1 make test-upstream-local" >&2
    exit 2
fi

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
test_dir=${TMPDIR:-/tmp}/frlg-upstream-tests.$$
trap 'rm -rf "$test_dir"' EXIT HUP INT TERM
mkdir -p "$test_dir"

expected_commit=$(awk -F '\t' '$1 == "external/pokefirered" { print $3 }' "$repo_root/config/source-dependencies.tsv")
actual_commit=$(git -C "$repo_root/external/pokefirered" rev-parse HEAD)
if [ -z "$expected_commit" ] || [ "$actual_commit" != "$expected_commit" ]; then
    printf '%s\n' "pokefirered submodule is missing or at an unexpected commit." >&2
    exit 1
fi

cc -std=c11 -Wall -Wextra -Werror \
    -I"$repo_root/compat/upstream-shim/include" \
    -I"$repo_root/core/p2b/include" \
    -I"$repo_root/external/pokefirered/include" \
    "$repo_root/external/pokefirered/src/random.c" \
    "$repo_root/external/pokefirered/src/math_util.c" \
    "$repo_root/core/p2b/source/frlg_upstream_probe.c" \
    "$repo_root/tests/upstream/upstream_probe_test.c" \
    -o "$test_dir/frlg_upstream_probe"

"$test_dir/frlg_upstream_probe"
printf '%s\n' "Local upstream random/math probe passed."
