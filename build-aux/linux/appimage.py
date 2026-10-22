"""Build and verify an x86_64 AppImage from a CMake install tree on Ubuntu."""
import argparse
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def run(*args, **kwargs):
    return subprocess.run(args, check=True, **kwargs)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    parser.add_argument("--version", default="0.1.0")
    args = parser.parse_args()
    build = args.build.resolve()
    source = Path(__file__).resolve().parents[2]
    tools = build / "packaging-tools"
    tools.mkdir(exist_ok=True)
    deploy = tools / "linuxdeploy-x86_64.AppImage"
    digest = "8aea8da0f7f7039d2a2cecb14657d752a222a5e1d3825caeef186c82f751cdd1"
    if not deploy.exists():
        # Immutable official release asset ID; continuous tag itself can move.
        run("curl", "--fail", "--location", "--retry", "3", "--header",
            "Accept: application/octet-stream", "--output", str(deploy),
            "https://api.github.com/repos/linuxdeploy/linuxdeploy/releases/assets/602438599")
    if hashlib.sha256(deploy.read_bytes()).hexdigest() != digest:
        raise RuntimeError("linuxdeploy checksum mismatch")
    deploy.chmod(0o755)
    output = build / "packages" / f"nijiexpose-{args.version}-Linux-x86_64.AppImage"
    output.parent.mkdir(exist_ok=True)
    env = dict(os.environ, APPIMAGE_EXTRACT_AND_RUN="1", ARCH="x86_64", LDAI_OUTPUT=str(output))
    with tempfile.TemporaryDirectory(prefix="appimage-", dir=build) as temporary:
        work = Path(temporary)
        appdir = work / "AppDir"
        prefix = appdir / "usr"
        run("cmake", "--install", str(build), "--prefix", str(prefix))
        desktop = prefix / "share/applications/nijiexpose.desktop"
        # Checkouts shared with Windows can have CRLF; linuxdeploy reads Icon literally.
        desktop.write_text(desktop.read_text(), encoding="utf-8", newline="\n")
        run(str(deploy), "--appdir", str(appdir), "--executable",
            str(prefix / "bin/nijiexpose-native"), "--desktop-file",
            str(prefix / "share/applications/nijiexpose.desktop"), "--icon-file",
            str(source / "build-aux/osx/nijiexpose.iconset/icon_256x256.png"), "--icon-filename", "nijiexpose",
            env=env)
        notices = prefix / "share/licenses/nijiexpose/system"
        # linuxdeploy bundles distro libraries; retain the corresponding package notices.
        for library in (prefix / "lib").rglob("*.so*"):
            if library.name.startswith(("libSDL3", "libnijiexpose_embedded_tracker")):
                continue  # Their notices are installed by CMake.
            owners = subprocess.run(["dpkg-query", "-S", f"*/{library.name}"],
                                    text=True, capture_output=True, check=True).stdout
            copied = False
            for line in owners.splitlines():
                package = line.split(": ", 1)[0].split(":", 1)[0]
                copyright_file = Path("/usr/share/doc") / package / "copyright"
                if copyright_file.is_file():
                    (notices / package).mkdir(parents=True, exist_ok=True)
                    shutil.copy2(copyright_file, notices / package / "copyright")
                    copied = True
            if not copied:
                raise RuntimeError(f"Missing license for {library.name}")
        run(str(deploy), "--appdir", str(appdir), "--output", "appimage", env=env)
        # Extract the final artifact, then test its actual packaged content and launcher.
        run(str(output), "--appimage-extract", cwd=work, stdout=subprocess.DEVNULL)
        extracted = work / "squashfs-root"
        run(sys.executable, str(source / "native/tools/verify-package.py"), str(extracted / "usr"))
        run(str(extracted / "AppRun"), "--version", cwd=work)
        run(str(output), "--version", env=env, cwd=work)
    print(f"Verified AppImage: {output}")


if __name__ == "__main__":
    main()
