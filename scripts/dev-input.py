"""Project-only development input; never sends desktop keyboard events."""
import argparse
import json
import os
from pathlib import Path
import struct
import tempfile
import time

KEYS = {name: 1 << i for i, name in enumerate(
    ("A", "B", "SELECT", "START", "RIGHT", "LEFT", "UP", "DOWN", "R", "L"))}

def packet(session, sequence, keys, frames):
    if not 0 < session < 2**64 or not 0 < sequence < 2**32:
        raise ValueError("invalid session or sequence")
    mask = 0
    if keys != "NONE":
        for key in keys.split("+"):
            if key not in KEYS:
                raise ValueError("unknown key: " + key)
            mask |= KEYS[key]
    if not 0 <= frames <= 120 or (mask == 0) != (frames == 0):
        raise ValueError("use 1..120 frames for keys, or NONE with 0 frames")
    return struct.pack("<4sQIHHI", b"FRI1", session, sequence, mask, frames, 0)

def read_status(directory):
    path = directory / "status.json"
    with path.open() as stream:
        status = json.load(stream)
        age = time.time() - os.fstat(stream.fileno()).st_mtime
    if not -5 <= age <= 5:
        raise ValueError("stale status; launch the development probe first")
    return status

def publish(directory, keys, frames):
    status = read_status(directory)
    session, sequence = int(status["session"]), int(status["sequence"])
    previous = directory / "command.bin"
    if previous.exists():
        data = previous.read_bytes()
        if len(data) == 24:
            magic, old_session, old_sequence, _, _, _ = struct.unpack("<4sQIHHI", data)
            if magic == b"FRI1" and old_session == session:
                sequence = max(sequence, old_sequence)
    sequence += 1
    data = packet(session, sequence, keys, frames)
    # Same-directory atomic replacement avoids partial packets. One controller only.
    with tempfile.NamedTemporaryFile(dir=directory, prefix="command-", delete=False) as stream:
        temporary = Path(stream.name)
        try:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        except BaseException:
            temporary.unlink(missing_ok=True)
            raise
    try:
        os.replace(temporary, previous)
    finally:
        temporary.unlink(missing_ok=True)
    return session, sequence

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sd-root", required=True, type=Path)
    parser.add_argument("keys", help="A, RIGHT, A+RIGHT, or NONE")
    parser.add_argument("--frames", type=int, default=1)
    args = parser.parse_args()
    directory = args.sd_root.resolve(strict=True) / "frlg-dev-control"
    session, sequence = publish(directory, args.keys.upper(), args.frames)
    deadline = time.monotonic() + 3
    while time.monotonic() < deadline:
        status = read_status(directory)
        if int(status["session"]) != session:
            raise ValueError("probe restarted during command")
        if int(status["sequence"]) > sequence:
            raise ValueError("another controller replaced the command")
        if int(status["sequence"]) == sequence:
            print(json.dumps(status))
            return
        time.sleep(0.05)
    raise TimeoutError("no acknowledgement; command has not been verified")

if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, TimeoutError) as error:
        raise SystemExit(str(error))
