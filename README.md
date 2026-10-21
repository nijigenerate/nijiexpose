# nijiexpose

nijiexpose is a VTubing application using nicxlive, C++20, SDL3, ImGui,
Boost.JSON, and GNU gettext. The native tracker runs in worker threads in the
same process, with bundled task models and direct tracking-frame delivery.

- [Build and verification instructions for Windows, macOS, and Linux](native/README.md)
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

Full D/C++ feature parity is still being verified; consult the remaining tasks.
