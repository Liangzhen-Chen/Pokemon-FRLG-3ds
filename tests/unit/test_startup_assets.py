#!/usr/bin/env python3
import importlib.util
import pathlib
import tempfile
import unittest
import os
from unittest.mock import patch

ROOT = pathlib.Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("startup_assets", ROOT / "scripts" / "build-startup-assets.py")
MOD = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MOD)


class StartupAssetTests(unittest.TestCase):
    def test_source_aliases_rejected_before_any_conversion(self):
        for alias in ("source_symlink", "destination_hardlink", "destination_in_source"):
            with self.subTest(alias=alias), tempfile.TemporaryDirectory() as tmp:
                root = pathlib.Path(tmp)
                out = root / "out"
                source = out / "graphics" if alias == "destination_in_source" else root / "src"
                (source / "graphics").mkdir(parents=True)
                (out / "graphics").mkdir(parents=True, exist_ok=True)
                original = source / "graphics/a.png"
                original.write_bytes(b"original")
                if alias == "source_symlink":
                    original.unlink()
                    (out / "input.png").write_bytes(b"original")
                    original.symlink_to(out / "input.png")
                elif alias == "destination_hardlink":
                    os.link(original, out / "graphics/a.4bpp")
                manifest = root / "m.tsv"
                manifest.write_text("graphics/a.4bpp.lz\t4bpp_lz\n")
                with patch.object(MOD.subprocess, "run") as run:
                    with self.assertRaises(ValueError):
                        MOD.build(source, out, pathlib.Path("/usr/bin/true"), manifest)
                    run.assert_not_called()
                self.assertEqual(original.read_bytes(), b"original")

    def test_manifest_has_exactly_79_safe_outputs(self):
        rows = MOD.load_manifest(ROOT / "config" / "startup-assets.tsv")
        self.assertEqual(len(rows), 79)
        for output, _kind, _options in rows:
            self.assertEqual(pathlib.PurePosixPath(output).as_posix(), output)
            self.assertNotIn("..", pathlib.PurePosixPath(output).parts)

    def test_late_invalid_destination_prevents_earlier_conversion(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp)
            source, out = root / "src", root / "out"
            (source / "graphics").mkdir(parents=True)
            (out / "graphics").mkdir(parents=True)
            for name in ("a", "b"):
                (source / f"graphics/{name}.png").touch()
            (out / "graphics/b.4bpp").symlink_to("missing")
            manifest = root / "m.tsv"
            manifest.write_text("graphics/a.4bpp.lz\t4bpp_lz\ngraphics/b.4bpp.lz\t4bpp_lz\n")
            with patch.object(MOD.subprocess, "run") as run:
                with self.assertRaises(ValueError):
                    MOD.build(source, out, pathlib.Path("/usr/bin/true"), manifest)
                run.assert_not_called()

    def test_png_cannot_be_compressed_directly_to_lz(self):
        with self.assertRaises(ValueError):
            MOD.conversion_steps("graphics/a.png", "graphics/a.lz")

    def test_raw_4bpp_menu_resource_conversion(self):
        self.assertEqual(MOD.source_for("graphics/text_window/unused.4bpp", "4bpp", ROOT),
                         pathlib.Path("graphics/text_window/unused.png"))
        self.assertEqual(MOD.conversion_steps("a.png", "a.4bpp"),
                         [(pathlib.Path("a.png"), pathlib.Path("a.4bpp"))])

    def test_8bpp_title_conversion(self):
        self.assertEqual(MOD.source_for("graphics/logo.8bpp.lz", "8bpp_lz", ROOT), pathlib.Path("graphics/logo.png"))
        self.assertEqual(MOD.conversion_steps("logo.png", "logo.8bpp.lz"),
                         [(pathlib.Path("logo.png"), pathlib.Path("logo.8bpp")),
                          (pathlib.Path("logo.8bpp"), pathlib.Path("logo.8bpp.lz"))])

    def test_missing_input_is_reported_before_tool_execution(self):
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(FileNotFoundError):
                MOD.validate_inputs(pathlib.Path(tmp), [("graphics/x.4bpp.lz", "4bpp_lz", "")])

    def test_output_root_cannot_be_source_root(self):
        with self.assertRaises(ValueError):
            MOD.build(pathlib.Path("/src"), pathlib.Path("/src"), pathlib.Path("tool"), ROOT / "config/startup-assets.tsv")

    def test_special_tile_counts_are_explicit(self):
        rows = dict((output, options) for output, _kind, options in MOD.load_manifest(ROOT / "config/startup-assets.tsv"))
        self.assertEqual(rows["graphics/intro/scene_1/grass.4bpp.lz"], "-num_tiles 397 -Wnum_tiles")
        self.assertEqual(rows["graphics/intro/scene_2/plants.4bpp.lz"], "-num_tiles 17 -Wnum_tiles")
        self.assertEqual(rows["graphics/intro/scene_2/nidorino_close.4bpp.lz"], "-num_tiles 170 -Wnum_tiles")
        self.assertEqual(rows["graphics/intro/scene_2/gengar_close.4bpp.lz"], "-num_tiles 114 -Wnum_tiles")
        self.assertEqual(rows["graphics/intro/scene_3/gengar_anim.4bpp.lz"], "-num_tiles 348 -Wnum_tiles")
        self.assertEqual(rows["graphics/title_screen/firered/box_art_mon.4bpp.lz"], "-num_tiles 135 -Wnum_tiles")

    def test_duplicate_and_non_graphics_outputs_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            pathlib.Path(tmp, "graphics").mkdir()
            pathlib.Path(tmp, "graphics/a.png").touch()
            with self.assertRaises(ValueError):
                MOD.validate_inputs(pathlib.Path(tmp), [("graphics/a.4bpp.lz", "4bpp_lz", ""), ("graphics/a.4bpp.lz", "4bpp_lz", "")])
        with self.assertRaises(ValueError):
            MOD.validate_inputs(pathlib.Path("/src"), [("other/a.4bpp.lz", "4bpp_lz", "")])

    def test_kind_suffix_and_options_are_validated(self):
        with self.assertRaises(ValueError):
            MOD.validate_inputs(pathlib.Path("/src"), [("graphics/a.bin.lz", "4bpp_lz", "")])
        with self.assertRaises(ValueError):
            MOD.validate_inputs(pathlib.Path("/src"), [("graphics/a.4bpp.lz", "4bpp_lz", "-bad")])

    def test_missing_tool_does_not_create_output(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = pathlib.Path(tmp, "src"); source.mkdir(); (source / "graphics").mkdir()
            (source / "graphics/a.png").touch()
            output = pathlib.Path(tmp, "out")
            manifest = pathlib.Path(tmp, "m.tsv"); manifest.write_text("graphics/a.4bpp.lz\t4bpp_lz\t\n")
            with self.assertRaises(FileNotFoundError):
                MOD.build(source, output, pathlib.Path(tmp, "missing"), manifest)
            self.assertFalse(output.exists())

    def test_dangling_symlink_is_rejected_preflight(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp); source = root / "src"; (source / "graphics").mkdir(parents=True); (source / "graphics/a.png").touch()
            out = root / "out"; (out / "graphics").mkdir(parents=True); (out / "graphics/a.4bpp").symlink_to("missing")
            manifest = root / "m.tsv"; manifest.write_text("graphics/a.4bpp.lz\t4bpp_lz\t\n")
            with self.assertRaises(ValueError):
                MOD.build(source, out, pathlib.Path("/usr/bin/true"), manifest)


if __name__ == "__main__":
    unittest.main()
