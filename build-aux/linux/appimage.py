"""Build and verify an x86_64 AppImage from a CMake install tree on Ubuntu."""
import argparse
import hashlib
import os
import re
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


# Dated official release: unlike the rolling continuous release, its assets are
# not routinely replaced. Keep a fixed digest even if the upstream URL changes.
LINUXDEPLOY_URL = (
    "https://github.com/linuxdeploy/linuxdeploy/releases/download/"
    "1-alpha-20251107-1/linuxdeploy-x86_64.AppImage"
)
LINUXDEPLOY_SHA256 = "c20cd71e3a4e3b80c3483cef793cda3f4e990aca14014d23c544ca3ce1270b4d"


def run(*args, **kwargs):
    return subprocess.run(args, check=True, **kwargs)


def verify_host_dialogs(appdir):
    """Dialogs belong to the host desktop, not to this AppImage."""
    forbidden = []
    for path in appdir.rglob("*"):
        if not (path.is_file() or path.is_symlink()):
            continue
        name = path.name
        if (name == "zenity" or name.startswith("xdg-desktop-portal") or
                name.startswith(("libgtk-", "libgtk-x11-", "libgdk-",
                                 "libgdk-x11-", "libadwaita-"))):
            forbidden.append(str(path.relative_to(appdir)))
    if forbidden:
        raise RuntimeError("Host dialog components must not be bundled: " + ", ".join(forbidden))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    args = parser.parse_args()
    build = args.build.resolve()
    version = (build / "nijiexpose-version.txt").read_text().strip()
    version = re.sub(r"[^A-Za-z0-9._+-]", "_", version)
    source = Path(__file__).resolve().parents[2]
    tools = build / "packaging-tools"
    tools.mkdir(exist_ok=True)
    deploy = tools / "linuxdeploy-x86_64.AppImage"
    if not deploy.exists():
        run("curl", "--fail", "--location", "--retry", "3", "--header",
            "Accept: application/octet-stream", "--output", str(deploy),
            LINUXDEPLOY_URL)
    if hashlib.sha256(deploy.read_bytes()).hexdigest() != LINUXDEPLOY_SHA256:
        raise RuntimeError("linuxdeploy checksum mismatch")
    deploy.chmod(0o755)
    output = build / "packages" / f"nijiexpose-{version}-Linux-x86_64.AppImage"
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
            str(prefix / "bin/nijiexpose"), "--desktop-file",
            str(prefix / "share/applications/nijiexpose.desktop"), "--icon-file",
            str(source / "build-aux/osx/nijiexpose.iconset/icon_256x256.png"), "--icon-filename", "nijiexpose",
            env=env)
        # Keep SDL's host portal selection; do not add a GTK deployment plugin
        # or bundle Zenity. Fail rather than silently strip linked dependencies.
        verify_host_dialogs(appdir)
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
        verify_host_dialogs(extracted)
        run(sys.executable, str(source / "native/tools/verify-package.py"), str(extracted / "usr"))
        run(str(extracted / "AppRun"), "--version", cwd=work)
        run(str(output), "--version", env=env, cwd=work)
    print(f"Verified AppImage: {output}")


if __name__ == "__main__":
    main()
