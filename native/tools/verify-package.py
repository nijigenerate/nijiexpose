"""Check a staged or unpacked native package before publishing it."""
import argparse
import gettext
import hashlib
import json
import os
from pathlib import Path
import platform
import plistlib
import subprocess


def verify_macos_vulkan(bundle, executable, licenses):
    frameworks = bundle / "Frameworks"
    manifest = bundle / "Resources/vulkan/icd.d/MoltenVK_icd.json"
    required = [frameworks / "libMoltenVK.dylib", frameworks / "libvulkan.1.dylib",
                manifest, licenses / "vulkan-sdk/LICENSE.txt"]
    for path in required:
        if not path.is_file() or not path.stat().st_size:
            raise RuntimeError(f"Missing Vulkan package file: {path}")
        if not path.resolve().is_relative_to(bundle):
            raise RuntimeError(f"Vulkan file points outside the bundle: {path}")
    driver = json.loads(manifest.read_text())["ICD"]
    if driver["library_path"] != "../../../Frameworks/libMoltenVK.dylib":
        raise RuntimeError(f"Unexpected MoltenVK driver path: {driver['library_path']}")
    if not driver.get("is_portability_driver"):
        raise RuntimeError("MoltenVK manifest is missing portability metadata")

    notice_root = licenses / "vulkan-sdk"
    source_notices = Path(__file__).resolve().parents[1] / "res/licenses/vulkan-sdk"
    provenance = json.loads((source_notices / "provenance.json").read_text())
    if (notice_root / "provenance.json").read_bytes() != (source_notices / "provenance.json").read_bytes():
        raise RuntimeError("Vulkan license provenance differs from the source")
    for notice in provenance["files"]:
        path = notice_root / notice["file"]
        if hashlib.sha256(path.read_bytes()).hexdigest() != notice["sha256"]:
            raise RuntimeError(f"Incorrect Vulkan license: {path}")

    # Inspect every bundled dylib, including the ICD loaded via JSON rather than
    # linked to the application. System libraries may live in the dyld cache.
    images = {executable, *(p.resolve() for p in frameworks.rglob("*.dylib"))}
    for image in sorted(images):
        if not image.is_relative_to(bundle):
            raise RuntimeError(f"Library points outside the bundle: {image}")
        dependencies = subprocess.run(["otool", "-L", str(image)], text=True,
                                      capture_output=True, check=True).stdout
        if image == executable and "libvulkan." not in dependencies:
            raise RuntimeError("macOS executable is not linked to the Vulkan loader")
        for line in dependencies.splitlines():
            if not line.startswith("\t"):
                continue
            dependency = line.strip().split(" (", 1)[0]
            if dependency.startswith(("/System/Library/", "/usr/lib/")):
                continue
            if dependency.startswith("@executable_path/"):
                target = executable.parent / dependency.removeprefix("@executable_path/")
            elif dependency.startswith("@loader_path/"):
                target = image.parent / dependency.removeprefix("@loader_path/")
            elif dependency.startswith("@rpath/"):
                target = frameworks / dependency.removeprefix("@rpath/")
            else:
                raise RuntimeError(f"External library dependency in {image}: {dependency}")
            if not target.is_file() or not target.resolve().is_relative_to(bundle):
                raise RuntimeError(f"Unbundled library dependency in {image}: {dependency}")
        commands = subprocess.run(["otool", "-l", str(image)], text=True,
                                  capture_output=True, check=True).stdout.splitlines()
        for index, line in enumerate(commands):
            if line.strip() == "cmd LC_RPATH":
                rpath = commands[index + 2].strip().removeprefix("path ").split(" (offset ", 1)[0]
                if not rpath.startswith(("@loader_path/", "@executable_path/")):
                    raise RuntimeError(f"External runtime search path in {image}: {rpath}")
                anchor = image.parent if rpath.startswith("@loader_path/") else executable.parent
                if not (anchor / rpath.split("/", 1)[1]).resolve().is_relative_to(bundle):
                    raise RuntimeError(f"Runtime search path escapes the bundle: {rpath}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    parser.add_argument("--require-macos-vulkan", action="store_true",
                        help="Require a self-contained macOS Vulkan package and component licenses")
    args = parser.parse_args()
    root = args.directory.resolve()
    system = platform.system()
    if args.require_macos_vulkan and system != "Darwin":
        parser.error("--require-macos-vulkan requires macOS")
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
        if args.require_macos_vulkan:
            verify_macos_vulkan(bundle, executable, licenses)
    else:
        required += [root / "lib/libnijiexpose_embedded_tracker.so",
                     root / "share/applications/nijiexpose.desktop",
                     root / "share/metainfo/io.github.nijigenerate.nijiexpose.metainfo.xml",
                     root / "share/icons/hicolor/256x256/apps/nijiexpose.png"]
    required += [executable, licenses / "LICENSE", licenses / "nicxlive.txt",
                 licenses / "native/facetrack-d.txt", licenses / "native/vmc-d.txt",
                 licenses / "native/NotoSansCJK-OFL.txt",
                 licenses / "nijitrack/LICENSE.mediapipe", licenses / "nijitrack/LICENSE.halide"]
    for path in required:
        if not path.is_file() or not path.stat().st_size:
            raise RuntimeError(f"Missing package file: {path}")
    if system == "Linux":
        dependencies = subprocess.run(["ldd", str(executable)], text=True,
                                      capture_output=True, check=True).stdout
        if "liblua" in dependencies or "not found" in dependencies:
            raise RuntimeError(f"Unexpected runtime dependencies: {dependencies}")
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
            for source, target in (("Roll", "ロール"), ("Pitch", "ピッチ"), ("Yaw", "ヨー"), ("Key", "キー")):
                assert translations.gettext(f"%s ({source})") == f"%s ({target})"
    runtime_env = os.environ.copy()
    if args.require_macos_vulkan:
        runtime_env = {key: value for key, value in runtime_env.items()
                       if not key.startswith(("VK_", "VULKAN_", "DYLD_"))}
    subprocess.run([str(executable), "--version"], check=True, cwd=executable.parent, env=runtime_env)
    print(f"Verified {system} package: runtime, model hashes, gettext catalogs and licenses")


if __name__ == "__main__":
    main()
