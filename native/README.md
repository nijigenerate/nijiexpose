# Native C++ / SDL3 build

The Windows C++ migration uses nicxlive, SDL 3.2.28, and official Dear ImGui
1.93.0-docking (commit 4e02e84cbb85d48881388e7090c2dfa1708c02a1). Source archives
are fixed by commit and SHA256. SDL3/OpenGL3 backends come from the same
ImGui release; FreeType support is enabled. Translation uses GNU gettext
libintl 0.22.5, and JSON uses Boost.JSON from Boost 1.90.0. These backend
differences are explicitly authorized; their internals are not reimplemented.

The executable includes the ported application loop, scene, tracking, Lua,
product UI, model rendering, animation, INP loading and Spout. Full implementation
parity and functional verification remain incomplete; see the current
[remaining tasks and acceptance criteria](../NATIVE_CPP_TASKS.md).
Application branches, calculations and update order must be retained during
migration; see [the requirements](../NATIVE_CPP_DESIGN.md).

## Build

Use a Visual Studio 2022 developer shell, CMake 3.24+, Ninja, Python,
Boost 1.90.0 headers, and an OpenGL 3.3 driver. Selected dependencies are bundled
in [vendor](../vendor/README.md). Unmodified Boost, gettext/libiconv and SDL3
remain external dependencies.
Existing LDC is used for original
D comparison executables. No DMD installation is required.

```powershell
vcpkg install --triplet=x64-windows-native --overlay-triplets=cmake/vcpkg/triplets --x-manifest-root=cmake/vcpkg --x-install-root=out/vcpkg --x-buildtrees-root=build/vcpkg --x-packages-root=out/vcpkg-packages
cmake --preset native-release
cmake --build --preset native-release --parallel 3
ctest --preset native-release
cmake --install build/native-release --prefix out/native
```

Use the existing vcpkg executable. An existing gettext prefix can be supplied
with `-DNIJIEXPOSE_GETTEXT_PREFIX=/path/to/prefix`. Gettext and libiconv DLLs
and license texts are staged automatically. Boost.JSON is compiled from its
`src.hpp`; no separately built Boost.JSON library is needed.
Compiled catalogs in `out/i18n` are installed beside the executable. CI
compiles the existing `tl/*.po` files using GNU msgfmt.

Pass `-DBOOST_ROOT=/path/to/boost` when needed. An optional nicxlive source
override is `-DNIJIEXPOSE_NICXLIVE_SOURCE_DIR=/path/to/nicxlive`.

The staged Windows executable is `out/native/bin/nijiexpose-native.exe`.
Keep all staged DLLs beside it and distribute the staged licenses. `--version`
reports the linked ImGui version. `--smoke-test` checks four hidden-window
frames, framebuffer readback, resizing and shutdown without saving a layout.
Enable its CTest registration using `-DNIJIEXPOSE_GPU_TESTS=ON`.

## Verification

Windows CI uses sccache with the GitHub Actions cache backend for C/C++
compilation, alongside the existing vcpkg binary cache. Each run records build
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
nijitrack inference/camera/preview code; `vendor/nijitrack` contains the copied
face/hand task bundles and checksummed Windows static SDK archive.
The SDK uses /MT, so a same-process DLL
with a C ABI isolates its CRT from the application's /MD runtime.

Settings are passed as a structure. Inference publishes complete face/pose/gaze
packets to a bounded latest-frame queue. The main thread drains it into the
Embedded nijitrack input adaptor. Key events are delivered directly as callbacks.
Neither a tracker subprocess nor OSC transport is used. SDL event dispatch and
preview rendering remain on the host thread; workers are cancelled and joined
before the tracker is destroyed. Python and executable-path settings are ignored
when loading older tracker configurations.

Tracker download and automatic installation code is not needed and has been
removed. The application uses the bundled native tracker and task files.

To enable the real-inference integration test, set NIJIEXPOSE_TRACKER_TEST_IMAGE
to a local MediaPipe portrait.jpg fixture. No test image is downloaded by builds.
