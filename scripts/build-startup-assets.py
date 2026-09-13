#!/usr/bin/env python3
"""Generate the exact FIRERED intro/title INCBIN resources locally."""
from pathlib import Path
import argparse
import subprocess
import shlex
import os
import re


def load_manifest(path):
    rows = []
    for line in Path(path).read_text().splitlines():
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        fields = line.split("\t")
        if len(fields) not in (2, 3):
            raise ValueError(f"invalid manifest row: {line}")
        rows.append((fields[0], fields[1], fields[2] if len(fields) == 3 else ""))
    return rows


def source_for(output, kind, source_root):
    p = Path(output)
    if kind == "4bpp":
        return p.with_suffix(".png")
    if kind in ("4bpp_lz", "8bpp_lz"):
        return p.with_suffix("").with_suffix(".png")
    if kind == "bin_lz":
        return p.with_suffix("")
    if kind == "palette":
        base = p.with_suffix("")
        pal = base.with_suffix(".pal")
        return pal if (source_root / pal).is_file() else base.with_suffix(".png")
    raise ValueError(f"unsupported conversion kind: {kind}")


def conversion_steps(source, output):
    source, output = Path(source), Path(output)
    if output.suffix == ".4bpp" and source.suffix == ".png":
        return [(source, output)]
    if output.suffixes[-2:] in ([".4bpp", ".lz"], [".8bpp", ".lz"]):
        if source.suffix != ".png":
            raise ValueError("tile input must be PNG")
        raw = output.with_suffix("")
        return [(source, raw), (raw, output)]
    if output.suffix == ".lz":
        if source.suffix == ".png":
            raise ValueError("PNG cannot be compressed directly to LZ")
        return [(source, output)]
    if output.suffix == ".gbapal":
        return [(source, output)]
    raise ValueError(f"unsupported output: {output}")


def validate_inputs(source_root, rows):
    seen = set()
    for output, kind, _options in rows:
        out = Path(output)
        if output in seen or out.is_absolute() or ".." in out.parts or out.as_posix() != output or not output.startswith("graphics/"):
            raise ValueError(f"unsafe output path: {output}")
        expected = {"4bpp": ".4bpp", "4bpp_lz": ".4bpp.lz", "8bpp_lz": ".8bpp.lz", "bin_lz": ".bin.lz", "palette": ".gbapal"}.get(kind)
        if expected is None or not output.endswith(expected):
            raise ValueError(f"kind/suffix mismatch: {output} ({kind})")
        if _options and (kind != "4bpp_lz" or not re.fullmatch(r"-num_tiles [1-9][0-9]* -Wnum_tiles", _options)):
            raise ValueError(f"unsupported options: {_options}")
        seen.add(output)
        source = source_for(output, kind, source_root)
        if not (source_root / source).is_file():
            raise FileNotFoundError(source_root / source)


def build(source_root, output_root, tool, manifest):
    source_root = source_root.resolve()
    output_root = output_root.resolve()
    if output_root == source_root or source_root in output_root.parents:
        raise ValueError("output root must be outside source root")
    if not tool.is_file() or not tool.stat().st_mode & 0o111:
        raise FileNotFoundError(tool)
    rows = load_manifest(manifest)
    validate_inputs(source_root, rows)
    input_ids = set()
    for output, kind, _options in rows:
        source = (source_root / source_for(output, kind, source_root)).resolve()
        if source_root not in source.parents:
            raise ValueError(f"source escapes source root: {source}")
        info = source.stat()
        input_ids.add((info.st_dev, info.st_ino))
    plan = []
    for output, kind, options in rows:
        source = source_root / source_for(output, kind, source_root)
        target = output_root / output
        for src, dst in conversion_steps(source, target):
            src = src.resolve()
            if source_root != src and source_root not in src.parents and output_root not in src.parents:
                raise ValueError(f"source escapes source root: {src}")
            if dst.is_symlink():
                raise ValueError(f"symlink destination: {dst}")
            resolved_parent = dst.parent.resolve()
            if output_root != resolved_parent and output_root not in resolved_parent.parents:
                raise ValueError(f"destination escapes output root: {dst}")
            if resolved_parent == source_root or source_root in resolved_parent.parents:
                raise ValueError(f"destination enters source root: {dst}")
            if dst.exists():
                info = dst.stat()
                if (info.st_dev, info.st_ino) in input_ids:
                    raise ValueError(f"destination aliases an input: {dst}")
            plan.append((src, dst, options))
    output_root.mkdir(parents=True, exist_ok=True)
    for src, dst, options in plan:
        dst.parent.mkdir(parents=True, exist_ok=True)
        args = [str(tool), str(src), str(dst)]
        if dst.suffix == ".4bpp" and options:
            args.extend(shlex.split(options))
        subprocess.run(args, check=True)
    return len(rows)


def main():
    root = Path(__file__).resolve().parents[1]
    if os.environ.get("FRLG_ENABLE_UPSTREAM_LOCAL") != "1":
        raise SystemExit("set FRLG_ENABLE_UPSTREAM_LOCAL=1 to generate upstream assets")
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-root", type=Path, default=root / "external/pokefirered")
    parser.add_argument("--output-root", type=Path, default=root / ".tools/frlg-startup-assets")
    parser.add_argument("--tool", type=Path, default=root / "external/pokefirered/tools/gbagfx/gbagfx")
    parser.add_argument("--manifest", type=Path, default=root / "config/startup-assets.tsv")
    args = parser.parse_args()
    expected = next(line.split("\t")[2] for line in (root / "config/source-dependencies.tsv").read_text().splitlines() if line.startswith("external/pokefirered\t"))
    actual = subprocess.check_output(["git", "-C", str(args.source_root), "rev-parse", "HEAD"], text=True).strip()
    if actual != expected:
        raise SystemExit(f"source HEAD {actual} does not match locked {expected}")
    dirty = subprocess.check_output(["git", "-C", str(args.source_root), "status", "--porcelain"], text=True)
    if dirty.strip():
        raise SystemExit("source tree has tracked or untracked modifications")
    print(f"generated {build(args.source_root, args.output_root, args.tool, args.manifest)} startup assets")


if __name__ == "__main__":
    main()
