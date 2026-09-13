import importlib.util
from pathlib import Path
import struct
import unittest
import json
import os
import tempfile
import time

spec = importlib.util.spec_from_file_location("dev_input", Path(__file__).resolve().parents[2] / "scripts/dev-input.py")
dev = importlib.util.module_from_spec(spec)
spec.loader.exec_module(dev)

class PacketTests(unittest.TestCase):
    def test_exact_layout(self):
        self.assertEqual(dev.packet(7, 1, "A+RIGHT", 2), struct.pack("<4sQIHHI", b"FRI1",7,1,17,2,0))

    def test_release(self):
        self.assertEqual(dev.packet(7,2,"NONE",0),struct.pack("<4sQIHHI",b"FRI1",7,2,0,0,0))

    def test_rejects_invalid(self):
        for args in [(0,1,"A",1),(7,0,"A",1),(7,2**32,"A",1),
                     (7,1,"X",1),(7,1,"A",0),(7,1,"A",121),(7,1,"NONE",1)]:
            with self.subTest(args=args), self.assertRaises(ValueError):
                dev.packet(*args)

    def test_atomic_publish_and_sequence(self):
        with tempfile.TemporaryDirectory() as name:
            directory = Path(name)
            status = directory / "status.json"
            status.write_text(json.dumps({"session":"7","sequence":0}))
            self.assertEqual(dev.publish(directory,"A",2),(7,1))
            self.assertEqual((directory / "command.bin").read_bytes(),dev.packet(7,1,"A",2))
            self.assertEqual(dev.publish(directory,"NONE",0),(7,2))
            self.assertFalse(list(directory.glob("command-*")))
            # A new boot may ignore the previous session's large sequence.
            status.write_text(json.dumps({"session":"8","sequence":0}))
            self.assertEqual(dev.publish(directory,"A",1),(8,1))

    def test_stale_and_invalid_publish_do_not_replace(self):
        with tempfile.TemporaryDirectory() as name:
            directory = Path(name)
            status = directory / "status.json"
            status.write_text(json.dumps({"session":"7","sequence":0}))
            dev.publish(directory,"A",1)
            before = (directory / "command.bin").read_bytes()
            with self.assertRaises(ValueError): dev.publish(directory,"INVALID",1)
            self.assertEqual((directory / "command.bin").read_bytes(),before)
            old = time.time() - 10
            os.utime(status,(old,old))
            with self.assertRaises(ValueError): dev.publish(directory,"A",1)
            self.assertEqual((directory / "command.bin").read_bytes(),before)

if __name__ == "__main__":
    unittest.main()
