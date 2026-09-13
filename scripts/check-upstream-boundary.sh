#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
upstream="$repo_root/external/pokefirered"

for path in src/random.c src/math_util.c include/random.h include/math_util.h; do
    test -f "$upstream/$path" || {
        printf '%s\n' "Missing selected upstream probe file: $path" >&2
        exit 1
    }
done

if find "$upstream" -maxdepth 1 -type f \
    \( -iname 'LICENSE' -o -iname 'LICENSE.*' -o -iname 'COPYING' -o -iname 'COPYING.*' \) \
    | grep -q .; then
    printf '%s\n' "A project-level upstream license appeared; review the distribution policy before changing the build." >&2
    exit 1
fi

for path in "$repo_root/platform/3ds/Makefile" "$repo_root/.github/workflows/build-3ds.yml"; do
    if grep -q 'external/pokefirered' "$path"; then
        printf '%s\n' "Public 3DS build links pokefirered unexpectedly: ${path#$repo_root/}" >&2
        exit 1
    fi
done

if git -C "$repo_root" ls-files 'artifacts/**' | grep -E '/p2b/' | grep -q .; then
    printf '%s\n' "A P2b artifact is tracked before upstream distribution permission was confirmed." >&2
    exit 1
fi

printf '%s\n' "Upstream boundary check passed: local probe only; no public linked artifact."
