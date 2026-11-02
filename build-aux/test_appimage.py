"""Network-free tests for the pinned AppImage tool acquisition boundary."""
import hashlib
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

SOURCE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("appimage", SOURCE / "linux/appimage.py")
appimage = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(appimage)


class ReachedInstall(Exception):
    """Stop before any native build, install or tool execution."""


class AcquisitionTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.build = Path(self.temporary.name)
        (self.build / "nijiexpose-version.txt").write_text("v1.0.0-beta4\n")
        self.tools = self.build / "packaging-tools"
        self.tools.mkdir()
        self.deploy = self.tools / "linuxdeploy-x86_64.AppImage"
        self.payload = b"fixture-only-not-executable"
        self.digest = hashlib.sha256(self.payload).hexdigest()
        self.calls = []

    def fake_run(self, *args, **kwargs):
        self.calls.append(args)
        if args[0] == "curl":
            self.assertEqual(args[-1], appimage.LINUXDEPLOY_URL)
            self.assertIn("--fail", args)
            self.assertIn("--location", args)
            self.assertIn("--retry", args)
            self.deploy.write_bytes(self.payload)
        elif args[0] == "cmake":
            raise ReachedInstall()
        else:
            self.fail(f"Unexpected native tool execution: {args}")

    def invoke(self):
        with patch("sys.argv", ["appimage.py", str(self.build)]), \
                patch.object(appimage, "run", side_effect=self.fake_run):
            appimage.main()

    def test_dated_official_release_and_digest(self):
        self.assertEqual(appimage.LINUXDEPLOY_URL,
                         "https://github.com/linuxdeploy/linuxdeploy/releases/download/"
                         "1-alpha-20251107-1/linuxdeploy-x86_64.AppImage")
        self.assertEqual(appimage.LINUXDEPLOY_SHA256,
                         "c20cd71e3a4e3b80c3483cef793cda3f4e990aca14014d23c544ca3ce1270b4d")

    def test_new_valid_download_reaches_install(self):
        with patch.object(appimage, "LINUXDEPLOY_SHA256", self.digest):
            with self.assertRaises(ReachedInstall):
                self.invoke()
        self.assertEqual([args[0] for args in self.calls], ["curl", "cmake"])
        self.assertEqual(self.deploy.stat().st_mode & 0o777, 0o755)

    def test_cached_valid_file_is_checked_without_download(self):
        self.deploy.write_bytes(self.payload)
        with patch.object(appimage, "LINUXDEPLOY_SHA256", self.digest):
            with self.assertRaises(ReachedInstall):
                self.invoke()
        self.assertEqual([args[0] for args in self.calls], ["cmake"])

    def test_download_with_wrong_digest_fails_before_install(self):
        with self.assertRaisesRegex(RuntimeError, "checksum mismatch"):
            self.invoke()
        self.assertEqual([args[0] for args in self.calls], ["curl"])
        self.assertEqual(self.deploy.stat().st_mode & 0o111, 0)

    def test_cached_corruption_has_no_download_or_unverified_fallback(self):
        self.deploy.write_bytes(b"corrupt cached image")
        with self.assertRaisesRegex(RuntimeError, "checksum mismatch"):
            self.invoke()
        self.assertEqual(self.calls, [])
        self.assertEqual(self.deploy.stat().st_mode & 0o111, 0)

    def test_empty_cached_file_fails(self):
        self.deploy.touch()
        with self.assertRaisesRegex(RuntimeError, "checksum mismatch"):
            self.invoke()
        self.assertEqual(self.calls, [])

    def test_download_error_propagates_before_execution(self):
        with patch("sys.argv", ["appimage.py", str(self.build)]), \
                patch.object(appimage, "run", side_effect=subprocess.CalledProcessError(22, "curl")) as run:
            with self.assertRaises(subprocess.CalledProcessError):
                appimage.main()
        self.assertEqual(run.call_count, 1)
        self.assertFalse(self.deploy.exists())

    def test_host_dialog_guard_remains_fail_closed(self):
        forbidden = self.build / "libgtk-3.so"
        forbidden.touch()
        with self.assertRaisesRegex(RuntimeError, "Host dialog components must not be bundled"):
            appimage.verify_host_dialogs(self.build)


if __name__ == "__main__":
    unittest.main()
