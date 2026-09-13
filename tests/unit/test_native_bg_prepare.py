import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location("native_bg", Path(__file__).resolve().parents[2] / "scripts/prepare-native-bg.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class NativeBgPrepareTests(unittest.TestCase):
    def test_only_dma_masks_become_unsigned(self):
        source = "(1 << (cursor % 0x20));" * 3 + "(1 << mod);" * 2 + "1 << (i % 8)"
        result = module.prepare(source)
        self.assertEqual(result.count("1u <<"), 5)
        self.assertIn("1 << (i % 8)", result)

    def test_source_drift_is_rejected(self):
        with self.assertRaises(ValueError):
            module.prepare("(1 << mod)")


if __name__ == "__main__":
    unittest.main()
