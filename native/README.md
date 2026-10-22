# Native C++ / SDL3 build

For the current platform setup, packaging and translation commands, start with
[BUILD.md](../BUILD.md). This page retains detailed implementation and test notes.

The C++ application uses nicxlive, SDL 3, and official Dear ImGui
1.93.0-docking (commit 4e02e84cbb85d48881388e7090c2dfa1708c02a1). Source archives
are fixed by commit and SHA256. SDL3/OpenGL3 backends come from the same
ImGui release; FreeType support is enabled. Translation uses GNU gettext
libintl 1.0, and JSON uses Boost.JSON from Boost 1.90.0. These backend
differences are explicitly authorized; their internals are not reimplemented.

The executable includes the ported application loop, scene, tracking, Lua,
product UI, model rendering, animation, INP loading and Spout. Full implementation
parity and functional verification remain incomplete; see the current
[remaining tasks and acceptance criteria](../NATIVE_CPP_TASKS.md).
Application branches, calculations and update order must be retained during
migration; see [the requirements](../NATIVE_CPP_DESIGN.md).

## Build

### Linux (x86_64, Ubuntu 24.04 / WSL2)

Install distribution binaries for the development dependencies:

```sh
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build clang lld git jq curl \
  pkg-config python3 openjdk-21-jdk-headless libboost-json-dev libfreetype-dev \
  gettext liblua5.1-0-dev libgl-dev libegl-dev libx11-dev libxi-dev libxtst-dev \
  libxrandr-dev libxcursor-dev libxfixes-dev libxinerama-dev libwayland-dev \
  libxkbcommon-dev libasound2-dev libpulse-dev libudev-dev libxss-dev libxext-dev \
  libxrender-dev libdrm-dev libgbm-dev libgles2-mesa-dev libdbus-1-dev \
  libibus-1.0-dev libdecor-0-dev libpipewire-0.3-dev wayland-protocols
cmake -S . -B "$HOME/build-nijiexpose-linux" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build "$HOME/build-nijiexpose-linux" --parallel 6
ctest --test-dir "$HOME/build-nijiexpose-linux" --output-on-failure
cmake --install "$HOME/build-nijiexpose-linux" --prefix "$HOME/nijiexpose-linux"
"$HOME/nijiexpose-linux/bin/nijiexpose-native"
```

Keep Linux build outputs on the WSL Linux filesystem, separate from Windows
build directories. CMake uses an installed SDL3 >=3.4 if available; otherwise
it builds the SHA256-verified official SDL 3.4.18 source release (Ubuntu 24.04
does not provide SDL3). Boost, gettext, FreeType and Lua use system packages; Lua is linked from the
static `liblua5.1.a` archive.
The application uses the CMake-selected C++ compiler; the pinned nijitrack
SDK recipe uses Clang, official Bazel/Halide binaries and MediaPipe source.
The SDK is embedded in the same process through a shared library. Its Bazel
action cache persists in `<build>/tracker-cache`.

Fonts/icons install in `share/nijiexpose`, tracker models in `bin/models`,
and shared libraries in `lib`. Linux settings default to
`$XDG_CONFIG_HOME/nijiexpose` or `$HOME/.config/nijiexpose`, preferring an
existing `.nijiexpose` directory at the same location just like the D version.
`IN_CONFIG_PATH` still overrides this location. Enable `NIJIEXPOSE_GPU_TESTS` for desktop
OpenGL tests and set `NIJIEXPOSE_TRACKER_TEST_IMAGE` to a local portrait to
enable the real MediaPipe inference test. WSLg supplies the test display.
For headless desktop tests install `xvfb xauth` and run
`LIBGL_ALWAYS_SOFTWARE=1 xvfb-run -a -s "-screen 0 1280x1024x24" ctest --test-dir "$HOME/build-nijiexpose-linux" --output-on-failure`.
The Linux application test captures the actual framebuffer just before swap;
this avoids relying on front-buffer readback after presentation while preserving
the same pixel and interaction assertions as the Windows test.
The upstream global keyboard backend uses X11 RECORD; global Wayland keyboard
capture is not supported. Spout output is Windows-only.

### Windows

On Windows, use a Visual Studio 2022 developer shell, PowerShell 7, Git, CMake 3.24+, Ninja, Python,
Boost headers and an OpenGL 3.3 driver. LDC is used for original D comparison
executables; no DMD installation is required.

Windows builds the MediaPipe/Halide SDK from the same pinned nijitrack source
as macOS. CMake runs upstream `setup_tracking_toolchain.ps1`,
`prepare_mediapipe_halide.ps1`, and `build_mediapipe_halide.ps1` in that order.
Setup downloads and verifies the upstream LLVM 19.1.7, Bazel and Halide tools.
The SDK uses clang-cl and `/MT`; the application remains MSVC with `/MD`.
No prebuilt tracker SDK is vendored or downloaded. Keep the build directory
for incremental builds. CI preserves Bazel's compiler action cache in
`build/native-release/tracker-cache`; compiler inputs and options determine reuse.
The SDK workspace defaults to `<build>/nt`. For a deeply nested checkout, use
`-DNIJIEXPOSE_TRACKER_SOURCE_DIR=<short-absolute-path>`: upstream Windows
generators still have path length limits. Use a separate path per concurrent build.

```powershell
cmake -DBOOST_DESTINATION="$PWD/build/boost" -P cmake/FetchBoostHeaders.cmake
$boostRoot = Get-Content build/boost/prefix.txt -Raw
cmake --preset native-release -DBOOST_ROOT="$boostRoot" -DBoost_NO_SYSTEM_PATHS=ON -DBoost_NO_BOOST_CMAKE=ON
cmake --build --preset native-release --parallel 3
ctest --preset native-release
cmake --install build/native-release --prefix out/native
```

SDL3 uses the official MSVC development binary. GNU gettext/libiconv and FreeType
use Windows binaries from the distributors linked by their official download
pages. These dependencies are downloaded with SHA-256 verification, not compiled.
ImGui and the patched nicxlive core are built from pinned source with the app.
Boost.JSON is compiled from `src.hpp`; no separately built Boost library is needed.

Versions, URLs, hashes, ABI requirements and license paths are recorded in
[dependencies.lock.json](../cmake/dependencies.lock.json). See
[dependency updates](../cmake/DEPENDENCY_UPDATES.md) for the update procedure.
Libraries selected for vendoring and their notices are in [vendor](../vendor/README.md).
Unmodified Boost, gettext/libiconv and SDL3 are not committed to the repository.
Changed PO catalogs are compiled during the build with GNU msgfmt. Hosts without
msgfmt can use precompiled catalogs in `out/i18n`; CI supplies them from its
translation job. All required runtime DLLs and notices are staged. See
[translation maintenance](../tl/README.md) and [package generation](../build-aux/README.md).

The staged Windows executable is `out/native/bin/nijiexpose-native.exe`.
Keep all staged DLLs beside it and distribute the staged licenses. `--version`
reports the linked ImGui version. `--smoke-test` checks four hidden-window
frames, framebuffer readback, resizing and shutdown without saving a layout.
Enable its CTest registration using `-DNIJIEXPOSE_GPU_TESTS=ON`.

## macOS build

Install the platform dependencies with Homebrew:

```sh
brew install --force-bottle cmake ninja openjdk@21 boost sdl3 freetype gettext jq pkg-config
export JAVA_HOME="$(brew --prefix openjdk@21)/libexec/openjdk.jdk/Contents/Home"
export PATH="$(brew --prefix gettext)/bin:$PATH"
export CMAKE_PREFIX_PATH="$(brew --prefix gettext)${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"
cmake -S . -B build/native-macos-source -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build/native-macos-source --parallel 4
cmake --install build/native-macos-source --prefix out/native-macos
```

CMake fetches nijitrack at the commit and archive hash in
[`tracker/NijitrackSource.cmake`](tracker/NijitrackSource.cmake). It invokes
nijitrack's own `native/build.sh setup`, `prepare` and `sdk` stages. These verify
pinned tools, MediaPipe sources, C API headers, patches and the existing Halide
arbitrary-angle ROI implementation. The source-built static face/hand SDK and
Halide AOT archive are linked into the embedded tracker with calculator
registration objects preserved. No additional MediaPipe implementation is
maintained here. A small patch to nijitrack’s build script detects the installed
macOS SDK when using Command Line Tools and supports an analysis-only preflight.
The emitted SDK link options are consumed directly. CMake checks that a JDK is
available and passes its `JAVA_HOME` to Bazel; `NIJIEXPOSE_JAVA_HOME` selects a
nonstandard installation.

Keep `build/native-macos-source`: nijitrack's sources, tools, prepared workspace,
Bazel cache and static archives all remain beneath its `_deps/nijitrack-main-src`
directory. CMake records each completed SDK stage; application changes do not
rerun SDK preparation. Changing the pinned nijitrack recipe requires rebuilding
the affected SDK outputs. A successful configure is not an inference check. Before SDK compilation, run
`cmake --build build/native-macos-source --target nijitrack_sdk_preflight`
to analyze the same target and arguments without compiling the SDK.

The application bundle includes models, fonts and dialog artwork. Spout frame
sharing is disabled on macOS. Camera and keyboard access require macOS camera
and Input Monitoring permissions respectively.

## Verification

Windows CI uses sccache with the GitHub Actions cache backend for C/C++
compilation. Boost headers are cached separately; SDL3, gettext/libiconv and
FreeType are downloaded as binaries. Each run records build
seconds and cache hit/miss statistics in its job summary and test artifacts.
Source/compiler/options changes invalidate the affected compilation entries;
linking and tests still run normally.
MediaPipe uses Bazel's disk action cache, saved separately even after a failed
build. The job records Bazel's actual cache-hit summary and fails if an exact
cache key was restored but no compiler results were reused. Runner image,
MSVC/Windows SDK identity and recipe changes are represented in cache keys;
Bazel additionally checks each action's inputs before reuse.

The application links ImGui 1.93.0. Original ImGui 1.88 WIP is compiled
only when BUILD_TESTING is enabled, for D comparison executables, and is
not linked into the application. The same ported widget sources are built
against both versions. New API names map the original tab style colors;
the original widget logic is retained.

The local Windows Release suite covers modern ImGui toggle/text editing,
categories and panel/tool-window state, D comparisons for category drawing,
style, path utilities, subprocesses and logging, runtime clock, command-line
checks, and SDL3/OpenGL rendering. Settings are checked against 16 original
D results, permitting Boost.JSON formatting and object-order differences.
Gettext integration tests cover Japanese/German switching, metadata,
unknown languages, clearing and C-locale launch environments. Updated ImGui drawing
is not claimed to be byte-identical to the older renderer. Full application
parity and interactive IME/HiDPI checks remain outstanding. The native CI runs
the Windows non-GPU suite and builds and validates packages on Windows, macOS
and Linux.

The GPU suite also exercises native Puppet rendering, masks, dynamic composites,
difference aggregation, texture operations, SpriteBatch, image import, INP
loading/extensions, actual Spout publication, and the ported application-window
lifecycle. Original-D comparisons include texture readbacks, rendered pixels,
SpriteBatch vertex bits and 950 animation interpolation/playback records. GPU
hash comparisons are local-driver checks; hosted CI runs the non-GPU suite.
Embedded fonts and dialog artwork are copied from the original nijiui resources.
The bundled x64 Spout DLL is the unchanged bindbc-spout2 0.1.1 distribution.

## Embedded nijitrack

The native tracker runs in this process. `native/tracker` contains the adapted
nijitrack inference/camera/preview code. On Windows, CMake builds the pinned
nijitrack source recipe and links the generated static SDK into a same-process
DLL with a C ABI because that SDK uses `/MT`. On macOS, CMake runs the pinned nijitrack source build recipe and links its
static SDK and Halide AOT archive into the adapter `.dylib`.

Settings are passed as a structure. Inference publishes complete face/pose/gaze
packets to a bounded latest-frame queue. The main thread drains it into the
Embedded nijitrack input adaptor. Key events are delivered directly as callbacks.
Neither a tracker subprocess nor OSC transport is used. SDL event dispatch and
preview rendering remain on the host thread; workers are cancelled and joined
before the tracker is destroyed. Python and executable-path settings are ignored
when loading older tracker configurations.

Tracker download and automatic installation code is not needed and has been
removed. The application uses the native tracker and bundled task files.

To enable the real-inference integration test, set NIJIEXPOSE_TRACKER_TEST_IMAGE
to a local MediaPipe portrait.jpg fixture. No test image is downloaded by builds.
On macOS this builds the existing `nijiexpose_embedded_tracker_test` target
without enabling the Windows comparison suite. Run it with the model path,
portrait path and output directory, using `SDL_VIDEODRIVER=dummy` and
`SDL_RENDER_DRIVER=software`. It checks real face inference, pose/blendshape
packets, preview, restart, cancellation and error propagation.

Linux file-dialog verification uses `nijiexpose_file_dialog_test` for cross-thread
completion, cancellation, errors, immediate completion, late completion after
quit, filter lifetime, and preservation of queued application events. The
`nijiexpose_file_dialog_manual <expected-path>` executable opens the actual SDL
file chooser; pass an empty expected path to check cancellation. Linux needs
an available XDG desktop portal or Zenity. MediaPipe itself is statically linked
into the embedded tracker shared library (the latter remains a shared library).

Local verification on 2026-10-20: Ubuntu 24.04 / WSL2 x86_64 Release build,
installation, all 14 CTest cases, real portrait inference, and Kurone INP
rendering passed. The actual Zenity chooser selected Kurone.inp and canceled
successfully under Xvfb. AddressSanitizer passed the dialog lifetime tests.
MediaPipe symbols are defined in the tracker library, with no dynamic MediaPipe
dependency. macOS runtime behavior was not tested on this host.
The Windows Release build succeeded; 36 of 37 tests passed. The application
actions test failed at the animation Loop checkbox, including on an isolated
rerun; this remains unresolved. Linux GitHub CI has not been added or verified.

The file chooser now uses SDL_ShowFileDialogWithProperties on all platforms,
including Windows. Titles, an explicit SDL parent window (or keyboard-focus
fallback), extension filters, and an All Files filter share the same code.
Windows GetOpenFileNameW-specific application code has been removed.
After this unification, Windows Release passed 37/37 tests and Linux Release
passed 14/14. Actual Windows and Linux (X11/Zenity) dialogs both selected
Kurone.inp and canceled successfully using the shared function with a parent
window. AddressSanitizer passed the updated lifetime/property tests. The earlier
Windows Loop checkbox failure did not reproduce in this run; its intermittent
cause has not been established. macOS and Linux portal runtime checks remain
outstanding.

The Linux real-inference test now resolves its bundled model through the
application's Tracker::modelPath and validates the configuration before starting
inference. It runs beside the application with its copied models directory,
instead of supplying a model path from the source tree. Linux uses models next
to the executable; only macOS uses the bundle Resources/models directory.

## D archive layout

The old D application, dub files, packaging assets, legacy workflows and D
reference programs are preserved in `../dlang_archive`. Active native builds
use `native/res`; shared gettext catalogs remain in the root `tl` directory.
The reference manifest and CMake parity targets now use `dlang_archive/source`
and `dlang_archive/reference/tests`. Relocation does not change the D source
hashes or disable C++ parity checks. Archived workflows are not active GitHub
Actions workflows. No D application build was run for this reorganization.

### Shared render resource lifetime

nicxlive owns Drawable retirement and CPU shared-atlas lifetime. Normal updates and DLL unload/failure handling process deferred releases internally, including empty shared arrays when the last model is deleted. Hosts do not call a shared-buffer collection API.

RenderBackend receives individual `disposeDrawableBuffers(id)` notifications on the rendering thread. The OpenGL backend releases an empty index buffer immediately and batches survivor compaction internally before its next upload/draw. There is no separate collection hook in RenderBackend. Tests use ordinary updates, unloads and drawing to verify automatic cleanup.

The vendored nicxlive source snapshot contains the same changes as the local `resource-lifetime` branch (see dependencies.lock.json). It is a source archive with its license, not a prebuilt library. Publication of a new upstream revision can replace this snapshot with a normal immutable source pin.

The OpenGL host consumes the standard nicxlive command queue and uses the existing UnityResourceCallbacks create/update/release contract, following nijimi. The separate NativeBackend interface and custom emitter factory have been removed. nicxlive changes address resource lifetime, texture channel handling, bounds and initialization defects; GPU-specific implementation remains in the host. The application build override retains only its existing Welding exclusion.

KHR advanced blending is selected by the OpenGL host from its actual capabilities and blend mode, ignoring the core multistage hint. The host implements all 11 KHR equations, coherent enable/disable, and non-coherent barriers. The Windows model-render test checks all 11 equations against framebuffer values in both coherent modes and checks forced fallback.
