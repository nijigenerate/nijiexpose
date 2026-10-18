# Native C++ / SDL3 build

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

On Windows, use a Visual Studio 2022 developer shell, CMake 3.24+, Ninja, Python,
Boost headers and an OpenGL 3.3 driver. LDC is used for original D comparison
executables; no DMD installation is required.

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
Compiled catalogs in `out/i18n` are installed beside the executable; CI generates
them using GNU msgfmt. All required runtime DLLs and notices are staged.

The staged Windows executable is `out/native/bin/nijiexpose-native.exe`.
Keep all staged DLLs beside it and distribute the staged licenses. `--version`
reports the linked ImGui version. `--smoke-test` checks four hidden-window
frames, framebuffer readback, resizing and shutdown without saving a layout.
Enable its CTest registration using `-DNIJIEXPOSE_GPU_TESTS=ON`.

## macOS build

Install the platform dependencies with Homebrew:

```sh
brew install cmake ninja openjdk boost sdl3 freetype gettext jq pkg-config
cmake -S . -B build/native-macos-source -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
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
parity and interactive IME/HiDPI checks remain outstanding. CI is configured
for these non-GPU tests but has not yet run remotely.

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
nijitrack inference/camera/preview code. On Windows, `vendor/nijitrack` provides
the checksummed static SDK archive, isolated behind a same-process DLL with a C
ABI because that SDK uses `/MT`. On macOS, CMake runs the pinned nijitrack source build recipe and links its
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
