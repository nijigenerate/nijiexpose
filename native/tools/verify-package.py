"""Check a staged or unpacked native package before publishing it."""
import argparse
import gettext
import hashlib
import json
from pathlib import Path
import platform
import plistlib
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    root = args.directory.resolve()
    system = platform.system()
    binary_dir = root / "bin"
    resources = binary_dir
    licenses = root / "share/licenses/nijiexpose"
    executable = binary_dir / "nijiexpose-native"
    required = []
    if system == "Windows":
        executable = executable.with_suffix(".exe")
        required += [binary_dir / name for name in (
            "SDL3.dll", "intl-8.dll", "iconv-2.dll", "charset-1.dll", "freetype.dll",
            "lua51.dll", "SpoutLibrary.dll", "nijiexpose_embedded_tracker.dll", "vcruntime140.dll")]
    elif system == "Darwin":
        bundle = root / "nijiexpose-native.app/Contents"
        executable = bundle / "MacOS/nijiexpose-native"
        resources = bundle / "Resources"
        licenses = resources / "licenses"
        required += [resources / "nijiexpose.icns", bundle / "Frameworks/libnijiexpose_embedded_tracker.dylib"]
        with (bundle / "Info.plist").open("rb") as stream:
            info = plistlib.load(stream)
        assert info["CFBundleExecutable"] == executable.name
        assert info["NSCameraUsageDescription"]
        assert info["LSMinimumSystemVersion"]
        subprocess.run(["codesign", "--verify", "--deep", "--strict", str(bundle.parent)], check=True)
    else:
        required += [root / "lib/libnijiexpose_embedded_tracker.so",
                     root / "share/applications/nijiexpose.desktop",
                     root / "share/metainfo/nijiexpose.appdata.xml",
                     root / "share/icons/hicolor/256x256/apps/nijiexpose.png"]
    required += [executable, licenses / "LICENSE", licenses / "nicxlive.txt",
                 licenses / "nijitrack/LICENSE.mediapipe", licenses / "nijitrack/LICENSE.halide"]
    for path in required:
        if not path.is_file() or not path.stat().st_size:
            raise RuntimeError(f"Missing package file: {path}")
    source_root = Path(__file__).resolve().parents[2]
    for name, metadata in json.loads((source_root / "vendor/nijitrack/models/models.lock.json").read_text()).items():
        model = resources / "models" / name
        assert hashlib.sha256(model.read_bytes()).hexdigest() == metadata["sha256"], model
    for catalog in (source_root / "tl").glob("*.po"):
        with (resources / "i18n" / (catalog.stem + ".mo")).open("rb") as stream:
            translations = gettext.GNUTranslations(stream)
        assert translations.gettext("LANG_NAME") not in ("", "LANG_NAME"), catalog
        if catalog.stem == "ja":
            assert translations.gettext("Tracking") == "トラッキング"
    subprocess.run([str(executable), "--version"], check=True, cwd=executable.parent)
    print(f"Verified {system} package: runtime, model hashes, gettext catalogs and licenses")


if __name__ == "__main__":
    main()
