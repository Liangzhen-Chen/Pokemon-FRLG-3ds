# FireRed dual-screen 3DS adaptation

This directory contains the 3DS source changes for the native FireRed dual-screen implementation. It is separate from the renderer and compatibility implementation at this repository's root. The patch includes the target CMake configuration, SDL input and storage adaptation, frame-thread yielding, and save-block layout fixes. It does not contain the upstream game, graphics, sound, ROMs, saves, or an executable.

The current CXI reaches the FireRed title screen in Azahar. The 3DS lower-screen interface is not implemented. New-game traversal, battles, in-game saving and Continue, audible sound, sustained speed, and physical-console operation are not validated. Low-level save-file write/read checks do not establish that in-game saving works.

## Fixed inputs

| Input | Source | Revision |
| --- | --- | --- |
| FireRed dual-screen | https://github.com/venom13/pokefirered-dualscreen | `cd551d2c6ae5536a2599114fed5ed823a9c9678c` |
| SDL2 | https://github.com/libsdl-org/SDL | `9519b9916cd29a14587af0507292f2bd31dd5752` (`release-2.30.7`) |
| makerom | https://github.com/3DSGuy/Project_CTR | `e8f5f529c54ff9b22a2491a480ffa69206bf7b19` |

The target uses devkitARM 16.1.0, libctru, the devkitPro 3DS CMake toolchain, C99, and FireRed English Rev1 data. Do not mix Rev0 assets into this build. Source changes are provided as a patch against the exact upstream revision; retain upstream notices. `UPSTREAM-LICENSE.txt` covers the upstream port modifications, not the underlying game assets.

## Applying and building

These commands use a Bash environment with Git, CMake, Make, Python 3, host C/C++ compilers, `/usr/bin/clang`, `awk`, `sed`, `pkg-config`, and host libpng/zlib. They reflect the macOS build setup; native Windows and WSL builds have not been validated. Install devkitPro separately and use its actual toolchain path if different from `/opt/devkitpro/cmake/3DS.cmake`.

Create a working directory with the pinned game checkout named `upstream`, the pinned SDL checkout named `sdl2-source`, and the pinned Project_CTR checkout named `project-ctr-source`. Obtain upstream inputs separately under their applicable terms. Initialize the game's SDL submodule as required by its resource preparation script.

Copy `firered-3ds.patch` and `firered-b-test.rsf` from this directory into that working directory, then run:

```sh
git -C upstream rev-parse HEAD
# Must equal cd551d2c6ae5536a2599114fed5ed823a9c9678c.
git -C upstream apply --check ../firered-3ds.patch
git -C upstream apply ../firered-3ds.patch

cmake -S sdl2-source -B sdl2-build \
  -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/3DS.cmake \
  -DCMAKE_BUILD_TYPE=Release -DSDL_SHARED=OFF -DSDL_STATIC=ON \
  -DCMAKE_INSTALL_PREFIX="$PWD/prefix"
cmake --build sdl2-build -j 4
cmake --install sdl2-build

(cd upstream && python3 tools/dualscreen/prepare_android.py --jobs 4)

cmake -S upstream/platform/3ds -B game-build \
  -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/3DS.cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build game-build -j 4

# Build makerom following Project_CTR's own instructions first.
project-ctr-source/makerom/bin/makerom -f ncch -rsf firered-b-test.rsf \
  -desc app:4 -elf game-build/firered_3ds.elf -o firered_3ds.cxi
```

The host `pkg-config` must resolve the host libpng/zlib during resource preparation. The patch expects the SDL installation at `prefix/` beside `upstream/`. The current target builds ELF plus a separately packaged CXI; it does not produce a working 3DSX because the packed game data contain unaligned absolute relocations. CIA packaging and installation on a physical console are not verified.

The save and settings directory is `sdmc:/3ds/pokefirered-dualscreen-3ds/`. Use isolated test storage; malformed existing saves stop startup. Sound requires the console's DSP firmware, which is not included.

To remove only these changes from an otherwise unchanged patched checkout, run `git apply --reverse --check ../firered-3ds.patch` and then `git apply --reverse ../firered-3ds.patch`. Do not reset unrelated work.
