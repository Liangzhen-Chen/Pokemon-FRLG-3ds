import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location("link_audit", Path(__file__).resolve().parents[1] / "upstream/startup-link-audit.py")
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class LinkAuditTests(unittest.TestCase):
    def test_reports_only_undefined_live_references(self):
        symbols = "         U LiveService\n         U DeadService\n         U SomeData\n"
        relocations = """RELOCATION RECORDS FOR [.text.Callback]:
OFFSET TYPE VALUE
00000004 R_ARM_CALL LiveService
00000008 R_ARM_ABS32 SomeData+0x00000004
0000000c R_ARM_CALL AlreadyDefined

RELOCATION RECORDS FOR [.text.Other]:
00000000 R_ARM_CALL LiveService
RELOCATION RECORDS FOR [.debug_info]:
00000000 R_ARM_ABS32 DeadService
"""
        self.assertEqual(audit.unresolved_references(symbols, relocations), {
            "LiveService": [".text.Callback", ".text.Other"], "SomeData": [".text.Callback"]})

    def test_no_live_reference_means_no_missing_dependency(self):
        self.assertEqual(audit.unresolved_references(" U Discarded\n", ""), {})


if __name__ == "__main__":
    unittest.main()
