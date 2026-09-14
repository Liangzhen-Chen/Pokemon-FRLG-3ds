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

# The native target is explicit and internal. Reject upstream inputs in the
# default Makefile path, including a reference under an unrelated condition.
if ! awk '
    /^[[:space:]]*(ifeq|ifneq|ifdef|ifndef)[[:space:]]/ {
        depth++
        if ($0 ~ /^[[:space:]]*ifeq[[:space:]]*\(\$\(FRLG_NATIVE_GF\),1\)/)
            native_depth = depth
    }
    /^[[:space:]]*else([[:space:]]|$)/ && depth == native_depth {
        print "Native target gate has an unchecked else branch" > "/dev/stderr"
        failed = 1
    }
    /^[[:space:]]*endif([[:space:]]|$)/ {
        if (depth == native_depth)
            native_depth = 0
        depth--
    }
    /external\/pokefirered|compat\/upstream-(full|native)|\/startup\/|build-native-gf/ && native_depth == 0 {
        print "Default 3DS Makefile references native or upstream input at line " NR > "/dev/stderr"
        failed = 1
    }
    END { exit failed }
' "$repo_root/platform/3ds/Makefile"; then
    exit 1
fi

if grep -Eq 'external/pokefirered|FRLG_NATIVE_GF|build-native-gf-3ds' \
    "$repo_root/.github/workflows/build-3ds.yml"; then
    printf '%s\n' 'Public 3DS CI enables or references the internal native target.' >&2
    exit 1
fi

if git -C "$repo_root" ls-files 'artifacts/**' | grep -E '/p2b/' | grep -q .; then
    printf '%s\n' "A P2b artifact is tracked before upstream distribution permission was confirmed." >&2
    exit 1
fi

printf '%s\n' "Upstream boundary check passed: default probe excludes upstream; native target remains internal."
