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

    verify_macos_libraries(bundle, executable, "Vulkan")


def is_vulkan_library(path):
    name = Path(path).name.lower()
    return name.startswith(("libvulkan", "libmoltenvk"))


def verify_macos_metal(bundle, executable):
    for path in (bundle / "Frameworks").rglob("*.dylib"):
        if is_vulkan_library(path):
            raise RuntimeError(f"Unexpected Vulkan library in Metal package: {path}")
    manifest = bundle / "Resources/vulkan/icd.d/MoltenVK_icd.json"
    if manifest.exists() or manifest.is_symlink():
        raise RuntimeError(f"Unexpected Vulkan driver manifest in Metal package: {manifest}")
    verify_macos_libraries(bundle, executable, "Metal")


def verify_macos_libraries(bundle, executable, renderer):
    frameworks = bundle / "Frameworks"
    # Inspect every bundled dylib, including any ICD loaded via JSON.
    # System libraries may live in the dyld cache.
    images = {executable, *(p.resolve() for p in frameworks.rglob("*.dylib"))}
    for image in sorted(images):
        if not image.is_relative_to(bundle):
            raise RuntimeError(f"Library points outside the bundle: {image}")
        dependencies = subprocess.run(["otool", "-L", str(image)], text=True,
                                      capture_output=True, check=True).stdout
        libraries = [line.strip().split(" (", 1)[0]
                     for line in dependencies.splitlines() if line.startswith("\t")]
        if image == executable:
            if renderer == "Vulkan" and not any(Path(lib).name.startswith("libvulkan.") for lib in libraries):
                raise RuntimeError("macOS executable is not linked to the Vulkan loader")
            if renderer == "Metal" and not any(
                    lib.startswith("/System/Library/Frameworks/Metal.framework/") for lib in libraries):
                raise RuntimeError("macOS executable is not linked to Metal")
        for dependency in libraries:
            if renderer == "Metal" and is_vulkan_library(dependency):
                raise RuntimeError(f"Unexpected Vulkan dependency in {image}: {dependency}")
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
    macos_renderer = parser.add_mutually_exclusive_group()
    macos_renderer.add_argument("--require-macos-metal", action="store_true",
                                help="Require a self-contained macOS Metal package without a Vulkan runtime")
    macos_renderer.add_argument("--require-macos-vulkan", action="store_true",
                                help="Require a self-contained macOS Vulkan package and component licenses")
    parser.add_argument("--windows-opengl", action="store_true",
                        help="Verify an OpenGL Windows package instead of the default Vulkan package")
    args = parser.parse_args()
    root = args.directory.resolve()
    system = platform.system()
    if args.require_macos_vulkan and system != "Darwin":
        parser.error("--require-macos-vulkan requires macOS")
    if args.require_macos_metal and system != "Darwin":
        parser.error("--require-macos-metal requires macOS")
    if args.windows_opengl and system != "Windows":
        parser.error("--windows-opengl requires Windows")
    binary_dir = root / "bin"
    resources = binary_dir
    licenses = root / "share/licenses/nijiexpose"
    executable = binary_dir / "nijiexpose"
    required = []
    if system == "Windows":
        executable = executable.with_suffix(".exe")
        required += [binary_dir / name for name in (
            "SDL3.dll", "intl-8.dll", "iconv-2.dll", "charset-1.dll", "freetype.dll",
            "lua51.dll", "nijiexpose_embedded_tracker.dll", "vcruntime140.dll")]
        if args.windows_opengl:
            required.append(binary_dir / "SpoutLibrary.dll")
    elif system == "Darwin":
        bundle = root / "nijiexpose.app/Contents"
        executable = bundle / "MacOS/nijiexpose"
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
        elif args.require_macos_metal:
            verify_macos_metal(bundle, executable)
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
    if system == "Windows" and not args.windows_opengl:
        dependencies = subprocess.run(["dumpbin", "/DEPENDENTS", str(executable)], text=True,
                                      capture_output=True, check=True).stdout.lower()
        if "vulkan-1.dll" not in dependencies or (binary_dir / "SpoutLibrary.dll").exists():
            raise RuntimeError("Unexpected renderer or Spout runtime in the Windows Vulkan package")
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
    if args.require_macos_vulkan or args.require_macos_metal:
        runtime_env = {key: value for key, value in runtime_env.items()
                       if not key.startswith(("VK_", "VULKAN_", "DYLD_"))}
    version = (resources / "version.txt").read_text().strip()
    output = subprocess.run([str(executable), "--version"], check=True, cwd=executable.parent,
                            env=runtime_env, text=True, capture_output=True).stdout
    if not version or not output.startswith(f"nijiexpose {version} / "):
        raise RuntimeError(f"Packaged version differs: expected {version!r}, got {output!r}")
    if system == "Darwin" and info.get("CFBundleGetInfoString") != f"nijiexpose {version}":
        raise RuntimeError("macOS bundle version differs from the executable")
    print(output, end="")
    print(f"Verified {system} package: runtime, model hashes, gettext catalogs and licenses")


if __name__ == "__main__":
    main()
