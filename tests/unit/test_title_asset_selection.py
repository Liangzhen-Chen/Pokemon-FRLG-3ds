import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location("title_assets", Path(__file__).resolve().parents[2] / "scripts/select-title-assets.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class TitleAssetSelectionTests(unittest.TestCase):
    def source(self):
        return "\n".join(f'const u8 {s}[] = INCBIN_U8("graphics/title_screen/firered/{i}.bin.lz");'
                         for i, s in enumerate(module.SYMBOLS))

    def test_selected_declarations_and_lengths_only(self):
        result = module.select(self.source() + '\nconst u8 unrelated[] = {1};')
        self.assertNotIn("unrelated", result)
        for symbol in module.SYMBOLS:
            self.assertIn(f"sizeof({symbol})", result)
        self.assertIn("gFrlgTitleLzResources", result)

    def test_missing_or_duplicate_definition_rejected(self):
        with self.assertRaises(ValueError):
            module.select("")
        with self.assertRaises(ValueError):
            module.select(self.source() + "\n" + self.source())

    def test_other_variant_or_unsafe_path_rejected(self):
        for bad in ("leafgreen/", "../"):
            with self.assertRaises(ValueError):
                module.select(self.source().replace("firered/", bad))


if __name__ == "__main__":
    unittest.main()
