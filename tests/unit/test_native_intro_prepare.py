from pathlib import Path
import importlib.util
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts/prepare-native-intro.py"
SOURCE = ROOT / "external/pokefirered/src/intro.c"
spec = importlib.util.spec_from_file_location("native_intro", SCRIPT)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class NativeIntroPrepareTests(unittest.TestCase):
    def test_locked_intro_keeps_offline_path_without_colosseum_resource(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "native_intro.c"
            result = subprocess.run(
                [sys.executable, str(SCRIPT), str(SOURCE), str(output)],
                capture_output=True,
                text=True,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            generated = output.read_text()
            self.assertNotIn("gMultiBootProgram_PokemonColosseum_Start", generated)
            self.assertNotIn("COLOSSEUM_GAME_CODE", generated)
            self.assertIn("GameCubeMultiBoot_ExecuteProgram(&sGcmb);", generated)
            self.assertIn("GameCubeMultiBoot_Quit();\n                SetSerialCallback(SerialCB);", generated)
            self.assertIn("case 142:\n        ResetSerial();", generated)

    def test_source_drift_is_rejected_without_replacing_output(self):
        with tempfile.TemporaryDirectory() as temporary:
            source = Path(temporary) / "intro.c"
            output = Path(temporary) / "native_intro.c"
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

    def test_missing_or_duplicate_ready_block_is_rejected(self):
        source = SOURCE.read_text()
        with self.assertRaises(ValueError):
            module.transform(source.replace(module.READY_BLOCK, ""))
        with self.assertRaises(ValueError):
            module.transform(source + module.READY_BLOCK)


if __name__ == "__main__":
    unittest.main()
