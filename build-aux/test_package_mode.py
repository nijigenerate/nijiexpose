"""Regression checks for tag, schedule and manual package selection."""
import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location("package_mode", Path(__file__).with_name("package-mode.py"))
mode = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mode)


class PackageModeTests(unittest.TestCase):
    def test_release_tags(self):
        for tag in ("1.2.3", "v1.2.3", "v1.2.3-rc1"):
            self.assertEqual(mode.select("push", f"refs/tags/{tag}"), ("release", "1.2.3"))

    def test_nightly(self):
        self.assertEqual(mode.select("push", "refs/tags/nightly"), ("nightly", "0.1.0"))
        self.assertEqual(mode.select("schedule", "refs/heads/main"), ("nightly", "0.1.0"))

    def test_manual_tag_obeys_requested_mode(self):
        for requested in ("verify", "nightly", "release"):
            self.assertEqual(mode.select("workflow_dispatch", "refs/tags/v1.2.3", requested),
                             (requested, "1.2.3"))
        self.assertEqual(mode.select("workflow_dispatch", "refs/tags/nightly", "verify"),
                         ("verify", "0.1.0"))

    def test_branch_does_not_release(self):
        self.assertEqual(mode.select("push", "refs/heads/native-cpp"), ("verify", "0.1.0"))

    def test_invalid_tag(self):
        with self.assertRaises(ValueError):
            mode.select("push", "refs/tags/v1.2.3-invalid")


if __name__ == "__main__":
    unittest.main()
