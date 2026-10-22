# Building nijiexpose

Build the native C++20 application from the repository root. CMake 3.24+,
Python 3 and Git are required. The commands below build the application without
the optional comparison tests. Do not build the archived D application.

## Windows x64

Use PowerShell 7 in a Visual Studio 2022 x64 developer environment with the C++
workload and Windows SDK installed. Install CMake, Ninja and GNU gettext tools
(`msgfmt`; also `xgettext` and `msgmerge` for translation maintenance) on PATH.
An OpenGL 3.3 driver is required to run the application.

```powershell
cmake -DBOOST_DESTINATION="$PWD/build/boost" -P cmake/FetchBoostHeaders.cmake
$boostRoot = (Get-Content build/boost/prefix.txt -Raw).Trim()
cmake --preset native-release -DBUILD_TESTING=OFF -DBOOST_ROOT="$boostRoot" -DBoost_NO_SYSTEM_PATHS=ON -DBoost_NO_BOOST_CMAKE=ON
cmake --build --preset native-release --parallel 3
cmake --install build/native-release --prefix out/native
./out/native/bin/nijiexpose-native.exe
```

CMake downloads verified Windows dependency binaries and the pinned tracker
toolchain. The application uses MSVC; the MediaPipe SDK uses clang-cl and is
built from source. For long checkout paths, configure
`-DNIJIEXPOSE_TRACKER_SOURCE_DIR=<short-absolute-path>` for the SDK workspace.
Keep the installed DLLs beside the executable and the rest of the install tree.

To create ZIP and MSI packages, make WiX 3 available first. The helper uses
an installed WiX 3.14; otherwise it requires Chocolatey to install it:

```powershell
./build-aux/windows/ensure-wix.ps1
cpack --config build/native-release/CPackConfig.cmake -C Release
```

## macOS arm64

Use a current Xcode toolchain with C++20 floating-point `std::from_chars`
support. CI uses the `macos-26` runner. Install Homebrew binary dependencies
and select JDK 21 for the tracker build:

```sh
brew install --force-bottle cmake ninja openjdk@21 boost sdl3 freetype gettext jq pkg-config
export JAVA_HOME="$(brew --prefix openjdk@21)/libexec/openjdk.jdk/Contents/Home"
export PATH="$(brew --prefix gettext)/bin:$PATH"
export CMAKE_PREFIX_PATH="$(brew --prefix gettext)${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"
cmake --preset native-release -DBUILD_TESTING=OFF -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build --preset native-release --parallel 3
cmake --install build/native-release --prefix out/native-macos
open out/native-macos/nijiexpose-native.app
cpack --config build/native-release/CPackConfig.cmake -C Release
```

This produces an app ZIP and DMG, bundles the required Homebrew dylibs and
their notices, fixes library paths and ad-hoc signs the app. Developer ID signing
and notarization are not configured. The pinned tracker recipe targets arm64;
these commands do not produce an Intel or universal package.

## Linux x86_64 (Ubuntu 24.04 / WSL2)

Install binary development dependencies:

```sh
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build clang lld git jq curl \
  pkg-config python3 openjdk-21-jdk-headless libboost-json-dev libfreetype-dev \
  gettext liblua5.1-0-dev libgl-dev libegl-dev libx11-dev libxi-dev libxtst-dev \
  libxrandr-dev libxcursor-dev libxfixes-dev libxinerama-dev libwayland-dev \
  wayland-protocols libxkbcommon-dev libasound2-dev libpulse-dev libudev-dev \
  libxss-dev libxext-dev libxrender-dev libdrm-dev libgbm-dev libgles2-mesa-dev \
  libdbus-1-dev libibus-1.0-dev libdecor-0-dev libpipewire-0.3-dev
cmake --preset native-release -DBUILD_TESTING=OFF -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build --preset native-release --parallel 3
cmake --install build/native-release --prefix out/native-linux
./out/native-linux/bin/nijiexpose-native
cpack --config build/native-release/CPackConfig.cmake -C Release
```

On WSL, keep the checkout and Linux build on the Linux filesystem and use WSLg
for display. Do not reuse a Windows build directory. Ubuntu 24.04 lacks SDL3,
so CMake builds the pinned official SDL3 source when no suitable installed SDL3
is available. Lua links statically from the distribution-provided `liblua5.1.a`; its shared
library is not required at runtime. MediaPipe also builds from source using the
pinned nijitrack recipe.
Linux packages require compatible system runtime libraries; see the
[package guide](build-aux/README.md). File dialogs need an XDG desktop portal
or Zenity. Global keyboard capture uses X11; Spout is unavailable.

To build and validate an AppImage after the Linux build:

```sh
python3 build-aux/linux/appimage.py build/native-release --version 0.1.0
```

The script downloads a checksum-pinned official linuxdeploy binary, includes
runtime libraries and their notices, and checks the final AppImage by extracting
and launching it. CI uses this same script. The baseline is Ubuntu 24.04;
AppImage does not remove the host glibc, graphics-driver or desktop-portal requirements.

## Translations

Install GNU gettext tools before configuring, then maintain catalogs with:

```sh
python3 native/tools/translations.py update
python3 native/tools/translations.py compile --output out/i18n
```

Use `python` instead of `python3` on Windows if appropriate. Add
`--gettext-bin <directory>` when the tools are not on PATH. Review new and fuzzy
entries in `tl/*.po` after updating. Normal builds compile changed PO files and
refresh the executable's catalogs without requiring a C++ relink. Hosts without
`msgfmt` must supply compiled catalogs in `out/i18n` or configure
`-DNIJIEXPOSE_TRANSLATIONS_DIR=<directory>`. See [tl/README.md](tl/README.md)
for extraction commands and installed catalog locations.

## Tests and package validation

To enable tests, reconfigure the same preset with `-DBUILD_TESTING=ON`, rebuild,
and run `ctest --preset native-release`. Windows D-reference comparisons use
LDC (`ldc2` on PATH); they do not require DMD or a D application build.
GPU and real-inference tests require additional inputs and a display as described
in [native/README.md](native/README.md#verification).

CPack writes packages to `build/native-release/packages`. Extract a generated
ZIP, then check its root directory:

```sh
python3 native/tools/verify-package.py <unpacked-package-root>
```

This checks runtime files, model hashes, gettext catalogs, licenses, the
executable's `--version` and, on macOS, its signature. It does not replace
interactive application or rendered-model tests.

CI builds the packages for all three platforms as listed in
[README.md](README.md#supported-packages), runs the Windows non-GPU test suite
and checks unpacked packages. C/C++ compilation uses sccache; MediaPipe uses
Bazel's disk action cache. Unix caches are saved before packaging so a packaging
failure does not discard compiled outputs. Preserve local build directories for
incremental builds; compiler, SDK or source changes can invalidate cache entries.

Dependency pins and update procedures are documented in
[cmake/DEPENDENCY_UPDATES.md](cmake/DEPENDENCY_UPDATES.md). Platform packaging
details, including MSI upgrade-version behavior, are in
[build-aux/README.md](build-aux/README.md).

## Nightly and tagged releases

The native workflow runs daily at 00:00 UTC once it is on the default branch.
Scheduled builds and the `nightly` tag publish ZIP files to the nightly prerelease.
Push `[v]MAJOR.MINOR.PATCH` to build and publish MSI (Windows), DMG (macOS) and
AppImage (Linux). A `-rcN` suffix marks a prerelease. The numeric tag version is
passed to CMake as `NIJIEXPOSE_VERSION`, including the MSI product version.
Tags are not created automatically for testing.

Branch/PR builds validate both ZIP and installer/AppImage packaging without
publishing releases. Manual runs offer `verify`, `nightly` and `release` modes;
manual runs only upload Actions artifacts. Release publishing waits for all
platform jobs and their package checks to succeed.
