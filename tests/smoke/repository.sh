#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)

required_files="README.md Makefile .gitignore .gitmodules config/tool-assets.tsv config/source-dependencies.tsv config/build-environment.env scripts/bootstrap.sh scripts/download-test-tools.sh scripts/check-environment.sh scripts/check-upstream-boundary.sh platform/3ds/Makefile platform/3ds/source/main.c platform/3ds/source/frlg_input_3ds.c platform/3ds/include/frlg_input_3ds.h platform/3ds/source/frlg_video_3ds.c platform/3ds/include/frlg_video_3ds.h platform/3ds/README.md compat/include/frlg_keypad.h compat/src/frlg_keypad.c compat/include/frlg_gba_memory.h compat/src/frlg_gba_memory.c compat/include/frlg_gba_display.h compat/src/frlg_gba_display.c compat/include/frlg_gba_dma.h compat/src/frlg_gba_dma.c compat/include/frlg_gba_mode3.h compat/src/frlg_gba_mode3.c core/include/frlg_runtime.h core/src/frlg_runtime_probe.c compat/upstream-shim/include/global.h core/p2b/include/frlg_upstream_probe.h core/p2b/source/frlg_upstream_probe.c tests/upstream/run-local.sh tests/upstream/upstream_probe_test.c tests/unit/run.sh tests/unit/keypad_test.c tests/unit/gba_memory_test.c tests/unit/gba_io_dma_test.c tests/unit/gba_mode3_test.c tests/unit/runtime_probe_test.c"

for path in $required_files; do
    test -e "$repo_root/$path" || {
        printf '%s\n' "Missing required file: $path" >&2
        exit 1
    }
done

if git -C "$repo_root" ls-files '*.gba' '*.sav' '*.pk3' | grep -q .; then
    printf '%s\n' "ROM, save, or creature data is tracked by the main repository." >&2
    exit 1
fi


git -C "$repo_root" submodule status | awk '
    /^[+-]/ { bad = 1; print "Uninitialized or modified submodule: " $0 > "/dev/stderr" }
    END { exit bad }
'

while IFS="$(printf '\t')" read -r path repository expected_commit role; do
    case "$path" in ''|'#'*) continue ;; esac
    actual_commit=$(git -C "$repo_root/$path" rev-parse HEAD)
    if [ "$actual_commit" != "$expected_commit" ]; then
        printf '%s\n' "Submodule commit mismatch: $path" >&2
        exit 1
    fi
done < "$repo_root/config/source-dependencies.tsv"

printf '%s\n' "Repository smoke test passed."
