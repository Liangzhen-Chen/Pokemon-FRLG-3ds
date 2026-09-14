#!/usr/bin/env python3
"""Build locked FireRed sound data into an ignored ARM11 object directory."""

import argparse
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
UPSTREAM_KEY = "external/pokefirered"
MIDI_SOURCE = Path("sound/songs/midi")
TRACK_TARGET = re.compile(r"^\s*\.byte\s+(?:GOTO|PATT|REPT)\b")
TARGET_WORD = re.compile(r"^\s*\.word\s+([A-Za-z_][A-Za-z_0-9]*)\s*(?:@.*)?$")


def run(args, *, cwd=None):
    subprocess.run([str(arg) for arg in args], cwd=cwd, check=True)


def locked_upstream(path):
    if not path.is_dir():
        raise ValueError(f"upstream directory does not exist: {path}")
    rows = (ROOT / "config/source-dependencies.tsv").read_text().splitlines()
    expected = next((row.split("\t")[2] for row in rows if row.split("\t")[0] == UPSTREAM_KEY), None)
    if expected is None:
        raise ValueError("locked FireRed revision is missing")
    actual = subprocess.check_output(["git", "-C", str(path), "rev-parse", "HEAD"], text=True).strip()
    dirty = subprocess.check_output(
        ["git", "-C", str(path), "status", "--porcelain", "--untracked-files=no"], text=True
    ).strip()
    if actual != expected or dirty:
        raise ValueError("FireRed upstream must match its locked, unmodified revision")


def rewrite_song_targets(path):
    """Encode local sequence jumps as four relative bytes without ELF relocations."""
    lines = path.read_text().splitlines(keepends=True)
    converted = []
    index = 0
    while index < len(lines):
        line = lines[index]
        converted.append(line)
        if TRACK_TARGET.match(line):
            if index + 1 >= len(lines) or (match := TARGET_WORD.match(lines[index + 1])) is None:
                raise ValueError(f"sequence jump without local target in {path}:{index + 1}")
            target = match.group(1)
            converted.append("1:\n")
            for shift in (0, 8, 16, 24):
                converted.append(f"\t.byte (({target} - 1b) >> {shift}) & 0xff\n")
            index += 1
        index += 1
    path.write_text("".join(converted))


def prepare(upstream, output):
    locked_upstream(upstream)
    if upstream == output or upstream in output.parents:
        raise ValueError("output directory cannot be inside the read-only upstream")
    if ROOT == output or ROOT in output.parents:
        ignored = subprocess.run(["git", "-C", str(ROOT), "check-ignore", "-q", str(output)])
        if ignored.returncode != 0:
            raise ValueError("output directory inside repository must be Git-ignored")

    output.mkdir(parents=True, exist_ok=True)
    manifest = output / "objects.txt"
    manifest.unlink(missing_ok=True)
    shutil.copytree(upstream / "sound", output / "sound", dirs_exist_ok=True)
    shutil.copytree(upstream / "asm/macros", output / "asm/macros", dirs_exist_ok=True)

    tools_dir = output / "tools"
    tools_dir.mkdir(exist_ok=True)
    mid_tool = tools_dir / "mid2agb"
    wav_tool = tools_dir / "wav2agb"
    preproc_tool = tools_dir / "preproc"
    mid_sources = [upstream / "tools/mid2agb" / name for name in
                   ("agb.cpp", "error.cpp", "main.cpp", "midi.cpp", "tables.cpp")]
    wav_sources = [upstream / "tools/wav2agb" / name for name in
                   ("converter.cpp", "wav2agb.cpp", "wav_file.cpp")]
    preproc_sources = [upstream / "tools/preproc" / name for name in
                       ("asm_file.cpp", "c_file.cpp", "charmap.cpp", "preproc.cpp",
                        "string_parser.cpp", "utf8.cpp", "io.cpp")]
    run(["c++", "-std=c++11", "-O2", "-Wall", "-Wno-switch", "-Werror", *mid_sources, "-o", mid_tool])
    run(["c++", "-std=c++17", "-O2", "-Wall", "-Werror", *wav_sources, "-o", wav_tool])
    run(["c++", "-std=c++11", "-O2", "-Wall", "-Wno-switch", "-Werror", *preproc_sources, "-o", preproc_tool])

    for wave in sorted((output / "sound").rglob("*.wav")):
        result = wave.with_suffix(".bin")
        args = [wav_tool, "-b"]
        if "sound/direct_sound_samples/cries" in wave.as_posix():
            args += ["-c", "-l", "1", "--no-pad"]
        run([*args, wave, result])

    config = output / MIDI_SOURCE / "midi.cfg"
    options = {}
    for line in config.read_text().splitlines():
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        name, flags = line.split(":", 1)
        options[name.strip()] = shlex.split(flags)
    songs = sorted((output / MIDI_SOURCE).glob("*.mid"))
    if not songs:
        raise ValueError("locked FireRed MIDI files are missing")
    for song in songs:
        if song.name not in options:
            raise ValueError(f"MIDI conversion flags missing for {song.name}")
        run([mid_tool, song, song.with_suffix(".s"), *options[song.name]])
        rewrite_song_targets(song.with_suffix(".s"))

    assembler = "arm-none-eabi-as"
    include_args = ["-mcpu=mpcore", "-I", output, "-I", output / "sound", "-I", upstream]
    objects = ["sound_data.o", "m4a_tables.o"]
    source = upstream / "data/sound_data.s"
    charmap = upstream / "charmap.txt"
    first = subprocess.run([str(preproc_tool), str(source), str(charmap)],
                           cwd=output, check=True, stdout=subprocess.PIPE).stdout
    middle = subprocess.run(["arm-none-eabi-gcc", "-E", "-x", "assembler-with-cpp",
                             "-I", str(output), "-I", str(upstream), "-I", str(upstream / "include"),
                             "-DFIRERED=1", "-DENGLISH=1", "-DREVISION=0", "-DMODERN=1", "-"],
                            input=first, cwd=output, check=True, stdout=subprocess.PIPE).stdout
    final = subprocess.run([str(preproc_tool), "-ie", str(source), str(charmap)],
                           input=middle, cwd=output, check=True, stdout=subprocess.PIPE).stdout
    subprocess.run([assembler, *map(str, include_args), "-o", str(output / objects[0]), "-"],
                   input=final, cwd=output, check=True)
    run(["arm-none-eabi-gcc", "-std=gnu11", "-O2", "-ffunction-sections", "-fdata-sections",
         "-march=armv6k", "-mtune=mpcore", "-mfloat-abi=hard", "-mtp=soft",
         "-DFIRERED", "-DENGLISH", "-DREVISION=0", "-DMODERN=1",
         "-iquote", ROOT / "compat/upstream-full/include",
         "-iquote", ROOT / "compat/upstream-native/include",
         "-iquote", ROOT / "compat/include", "-iquote", upstream / "include",
         "-c", upstream / "src/m4a_tables.c", "-o", output / objects[1]])
    song_objects = output / "songs"
    song_objects.mkdir(exist_ok=True)
    for song in songs:
        relative = f"songs/{song.stem}.o"
        run([assembler, *include_args, "-o", output / relative, song.with_suffix(".s")], cwd=output)
        objects.append(relative)
    manifest.write_text("".join(f"{name}\n" for name in objects))
    print(f"Prepared {len(objects)} ARM11 sound objects in {output}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--upstream", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    try:
        prepare(args.upstream.resolve(), args.output_dir.resolve())
    except (OSError, ValueError, subprocess.CalledProcessError) as exc:
        print(f"Native audio asset preparation failed: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
