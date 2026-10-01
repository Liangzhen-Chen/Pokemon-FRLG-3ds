# Reference renderer shutdown patch

The optional root 3DS renderer uses source from [zelda-tmc-3ds](https://github.com/EstebanPdN/zelda-tmc-3ds) at `f9984989e3d96f2bcb4fcc60e69fe9756d74c3c6`. Apply `zelda-tmc-mode1-shutdown.patch` to that checkout before using the reference-renderer target. The patch avoids freeing a worker whose thread has not joined and preserves its state for a later shutdown attempt.

```sh
# Run inside the reference checkout, with the patch copied to a known path.
git apply --check /path/to/zelda-tmc-mode1-shutdown.patch
git apply /path/to/zelda-tmc-mode1-shutdown.patch
```

The existing optional target has a workspace-relative reference path in `platform/3ds/Makefile`; match that directory layout or explicitly supply the corresponding source/include paths in a separate build adaptation. A standalone Windows checkout of this optional target has not been verified. The default graphical demo does not use this reference renderer.

This patch modifies GPL-3.0-or-later source. The upstream license and third-party notices are included here and must be retained. No game assets or reference binaries are included.
