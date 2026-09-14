from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts/prepare-native-title-screen.py"
SOURCE = ROOT / "external/pokefirered/src/title_screen.c"


class NativeTitlePrepareTests(unittest.TestCase):
    def test_title_local_lz_sources_are_registered(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "native_title_screen.c"
            result = subprocess.run(
                [sys.executable, str(SCRIPT), str(SOURCE), str(output)],
                capture_output=True,
                text=True,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            source = SOURCE.read_text()
            background = set(re.findall(
                r"DecompressAndCopyTileDataToVram\([^,]+,\s*(s\w+),", source
            ))
            fire_red_sheets = re.search(
                r"#if defined\(FIRERED\)\s+static const struct CompressedSpriteSheet "
                r"sSpriteSheets\[\] = \{(.*?)\};", source, re.DOTALL,
            )
            self.assertIsNotNone(fire_red_sheets)
            sprites = set(re.findall(r"\{\s*(s\w+),", fire_red_sheets.group(1)))
            registered = set(re.findall(
                r"\{\(const uint8_t \*\)(s\w+), sizeof\(\1\)\}",
                output.read_text(),
            ))
            self.assertEqual((len(background), len(sprites)), (2, 3))
            self.assertEqual(registered, background | sprites)
            self.assertTrue(output.read_text().startswith(source))

    def test_source_drift_does_not_replace_output(self):
        with tempfile.TemporaryDirectory() as temporary:
            source = Path(temporary) / "title_screen.c"
            output = Path(temporary) / "native_title_screen.c"
            source.write_bytes(SOURCE.read_bytes() + b"\n")
            output.write_text("previous output")
            result = subprocess.run(
                [sys.executable, str(SCRIPT), str(source), str(output)],
                capture_output=True,
                text=True,
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("SHA-256 mismatch", result.stderr)
            self.assertEqual(output.read_text(), "previous output")


if __name__ == "__main__":
    unittest.main()
