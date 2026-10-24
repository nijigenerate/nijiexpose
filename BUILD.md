# Building nijiexpose

Build the native C++20 application from the repository root. CMake 3.24+,
Python 3 and Git are required. The commands below build the application without
the optional comparison tests. Do not build the archived D application.

## Vulkan renderer (`native-vulkan`)

The model renderer is a C++ port of `nijimi/source/render/vulkan/vulkan_backend.d`.
The model shaders originate from nijimi, with the part-opacity correction documented alongside original source hashes in
`native/nijimi-vulkan-provenance.json`. SDL3 owns the surface and the official
Dear ImGui Vulkan backend renders the existing UI. No nicxlive API extension is
required.

Install the Vulkan SDK (including `glslc`) and set `VULKAN_SDK`, then add
`-DNIJIEXPOSE_RENDERER=VULKAN` to the configure command below. The executable name
and install layout stay the same. `-DNIJIEXPOSE_RENDERER=OPENGL` selects the existing
renderer and remains the default preset. macOS CI explicitly selects Vulkan;
Windows and Linux CI select OpenGL.

Use `native-opengl-release` or `native-vulkan-release` in place of
`native-release` in the configure/build/test commands below to keep separate
build directories. Install and package from the corresponding
`build/native-opengl-release` or `build/native-vulkan-release` directory.
The original `native-release` preset remains available and selects OpenGL.

Each configuration compiles only its selected model renderer and ImGui renderer.
OpenGL builds do not require the Vulkan SDK; Vulkan builds do not find or link
OpenGL or compile the OpenGL ImGui backend. On Windows, only OpenGL application
packages include `SpoutLibrary.dll`; Vulkan uses the static Spout registry and
D3D11 sharing implementation. The separate Vulkan integration test still uses
that DLL for its independent receiver. OpenGL-specific GPU tests are built only
in the OpenGL configuration; the Vulkan model test is an explicit build target.

Windows rendering has been checked with the Vulkan validation layer using Kurone
and Midori models: model movement, resize, removal, switching, and ImGui rendering.
The manual integration test accepts two actual model paths and an output prefix:

```powershell
cmake --build build/native --config Release --target nijiexpose_vulkan_model_test
$env:VK_INSTANCE_LAYERS="VK_LAYER_KHRONOS_validation"
./build/native/native/Release/nijiexpose_vulkan_model_test.exe first.inp second.inp out/vulkan-model
```

On Windows, Spout publishes a D3D11 shared texture imported into Vulkan with
`VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_KMT_BIT`. The sender uses the
vendored official Spout sender registry, access mutex, and frame counter. It
renders directly into the published image: there is no send-side GPU copy,
extra send submission, hidden OpenGL context, or CPU image readback. The
D3D11 adapter is matched to Vulkan by LUID; the driver must support importing
the shared RGBA8 render target. The named Spout mutex covers rendering and GPU
completion before the frame counter is advanced. A CPU fence wait remains for
this synchronization; zero-copy does not mean synchronization-free delivery.
The previous D3D11 allocation stays alive until replacement metadata is published.
The integration test uses the existing Spout DLL as an independent
receiver registry to verify that the published handle is the render target,
and D3D11 to compare all RGBA bytes before and after movement,
resize, removal, and model switching.

Vulkan retains albedo and emission in separate color planes, including dynamic
composites. Ambient light modulates albedo; emission is added afterwards without
changing coverage alpha. Each plane has its own stencil and uses single-color
render passes so advanced blending does not require multiple color attachments.
Postprocessing disabled uses unmodulated albedo plus emission. Bloom and full
OpenGL lighting parity are not implemented. Depth/stencil attachment formats are
selected from the device's supported formats, including MoltenVK on macOS.

The `nijiexpose_vulkan_regression_test` target checks opacity, black/tinted ambient
light, the postprocessing toggle, emission occlusion/orientation, nested composite
opacity and normal/dodge stencil masks. These pixel checks run locally on macOS
with MoltenVK and `VK_LAYER_KHRONOS_validation`. Linux Vulkan runtime validation
and real noncoherent advanced-blend hardware coverage remain outstanding.

Advanced blending enables coherent operations when the device supports them.
Otherwise, advanced-blended geometry uses explicit color-attachment dependencies
between triangles to preserve ordering even within overlapping meshes. This
slower path is limited to devices without coherent advanced blending.

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
./out/native/bin/nijiexpose.exe
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
open out/native-macos/nijiexpose.app
cpack --config build/native-release/CPackConfig.cmake -C Release
```

This produces an app ZIP and DMG, bundles the required Homebrew dylibs and
their notices, fixes library paths and ad-hoc signs the app. Developer ID signing
and notarization are not configured. The pinned tracker recipe targets arm64;
these commands do not produce an Intel or universal package.

For Vulkan, install the macOS Vulkan SDK and select `native-vulkan-release`
with `VULKAN_SDK` pointing to its `macOS` directory. The installed app bundles
the Vulkan loader, MoltenVK, its driver manifest and component license notices.
The manifest uses a bundle-relative library path, so the installed app does not
require the SDK environment variables to locate MoltenVK.

The macOS CI job uses Vulkan SDK **1.4.328.1**, with the official archive SHA-256
checked before installation. It caches the SDK separately by version, digest,
OS, architecture and installer-script revision. Installation uses `copy_only=1`
and the core component, without copying runtime libraries to system directories.
CI overrides `native-release` with `-DNIJIEXPOSE_RENDERER=VULKAN` and explicit SDK
paths, retaining `build/native-release` and the existing MediaPipe cache paths.
Linux and Windows continue to use OpenGL.

macOS ZIP and DMG verification passes `--require-macos-vulkan` to
`native/tools/verify-package.py`. This checks the loader, MoltenVK, the relative
ICD manifest, bundled library dependencies, runtime search paths, license hashes
and app signature. The `--version` launch clears Vulkan and dyld environment
overrides. This is packaging verification, not a GPU rendering test.
When updating the SDK, update its workflow version/digest and review the pinned
component notices in `native/res/licenses/vulkan-sdk/provenance.json` together.

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
./out/native-linux/bin/nijiexpose
cpack --config build/native-release/CPackConfig.cmake -C Release
```

On WSL, keep the checkout and Linux build on the Linux filesystem and use WSLg
for display. Do not reuse a Windows build directory. Ubuntu 24.04 lacks SDL3,
so CMake builds the pinned official SDL3 source when no suitable installed SDL3
is available. Lua links statically from the distribution-provided `liblua5.1.a`; its shared
library is not required at runtime. MediaPipe also builds from source using the
pinned nijitrack recipe.
For the Vulkan configuration, also install the Ubuntu binary packages
`libvulkan-dev` and `glslc`, then select `native-vulkan-release` instead of
`native-release`. The OpenGL configuration does not require these packages.
Linux packages require compatible system runtime libraries; see the
[package guide](build-aux/README.md). File dialogs need an XDG desktop portal
or Zenity. Global keyboard capture uses X11; Spout is unavailable.
On WSLg, install `zenity` (`sudo apt-get install zenity`). The application
defaults to SDL's Zenity file dialog driver when `/usr/bin/zenity` or
`/bin/zenity` is executable, avoiding the unresponsive GTK portal modal observed
with an X11 parent. An explicit `SDL_FILE_DIALOG_DRIVER` setting takes priority;
other Linux environments retain SDL's default driver selection.
AppImage builds also use the host's portal and do not bundle Zenity or GTK.
The WSLg Zenity preference is a workaround, not a fix for the portal input issue.

To build and validate an AppImage after the Linux build:

```sh
python3 build-aux/linux/appimage.py build/native-release
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
Scheduled builds and the `nightly` tag publish ZIP files for all platforms and
a macOS DMG to the nightly prerelease.
Push `[v]MAJOR.MINOR.PATCH` to build and publish MSI (Windows), DMG (macOS) and
AppImage (Linux). A `-betaN` or `-rcN` suffix marks a prerelease. Application versions are derived from Git:
an exact tag uses its name; later commits use `<nearest-tag>-g<12-digit-commit>`;
repositories without tags use the 12-digit commit ID. CI fetches history and tags.
The same version appears in `--version`, the window title and distribution filenames.
Existing builds reconfigure when Git HEAD or refs change. Numeric-only metadata
(MSI, Windows fixed version fields and macOS bundle build numbers) uses the tag's
`MAJOR.MINOR.PATCH`, or `0.0.0` if the tag has no numeric version.
Tags are not created automatically for testing.

Branch/PR builds validate both ZIP and installer/AppImage packaging without
publishing releases. Manual runs offer `verify`, `nightly` and `release` modes;
`verify` only uploads Actions artifacts, while `nightly` and `release` publish
once all platform jobs and package checks succeed. Manual `release` runs require
an existing version tag: supply `release_tag` when running from a branch, or
select the tag as the workflow ref. All jobs build that tag's pinned commit,
including translations. Missing tags fail before the platform builds start.
Manual `nightly` builds the selected ref and publishes to `nightly`.
The `release_tag` input does not create or move Git tags. Example commands:

```sh
gh workflow run native-cpp.yml --ref main -f mode=nightly
gh workflow run native-cpp.yml --ref main -f mode=release -f release_tag=v1.2.3-beta1
gh workflow run native-cpp.yml --ref main -f mode=verify
```
