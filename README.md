# nijiexpose

nijiexpose is a VTubing application using nicxlive, C++20, SDL3, ImGui,
Boost.JSON, and GNU gettext. The native tracker runs in worker threads in the
same process, with bundled task models and direct tracking-frame delivery.

- [Build, test and package instructions](BUILD.md)
- [Detailed native implementation and verification notes](native/README.md)
- [Native packages and installers](build-aux/README.md)
- [Translation maintenance](tl/README.md)
- [Remaining migration tasks and feature differences](NATIVE_CPP_TASKS.md)
- [Native implementation design](NATIVE_CPP_DESIGN.md)
- [Archived D implementation and build assets](dlang_archive/README.md)

The active application is in `native`, dependencies in `vendor` and `cmake`,
and shared translation catalogs in `tl`. C++ runtime assets are in `native/res`.
Active packaging assets are in `build-aux`. Legacy D source, D-only packaging,
workflows, and D comparison programs are in
`dlang_archive`. C++ parity checks continue to use the archived D references.

## Supported packages

| Platform | Architecture | Formats |
| --- | --- | --- |
| Windows | x64 | ZIP, MSI |
| macOS | arm64 | ZIP containing the app bundle, DMG |
| Linux (Ubuntu 24.04 or compatible) | x86_64 | ZIP, tar.gz |

The [native CI workflow](.github/workflows/native-cpp.yml) builds and packages
all three platforms. Download packages from its GitHub Actions artifacts.
Packages include tracker task models, gettext catalogs and dependency notices.
Keep the extracted directory structure intact. macOS packages are ad-hoc
signed; they are not Developer ID signed or notarized. Spout is Windows-only.

## Development

Start with [BUILD.md](BUILD.md) for prerequisites and commands. MediaPipe is
built from pinned nijitrack source and statically linked into the embedded
tracker library. Official dependency binaries are used where available;
unmodified Boost, gettext and SDL3 are not vendored. CI retains compilation
caches for incremental builds.

Translations are maintained in `tl/*.po`, extracted from `native/port`, and
compiled to gettext catalogs during the build. See [translation maintenance](tl/README.md).
The archived D application is not built; optional parity tests use its reference
programs. Full D/C++ feature parity is still being verified; consult the
[remaining tasks](NATIVE_CPP_TASKS.md).
