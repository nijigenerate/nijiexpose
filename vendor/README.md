# Native Windows build dependencies

This directory contains the dependencies selected for vendoring. Archives are
extracted under the build directory and verified against `manifest.json`.
Original license files remain inside archives and are copied into `licenses/`;
tracker SDK notices are also available under `nijitrack/licenses/`.

Unmodified **Boost, gettext (including libiconv), and SDL3 are not vendored**, as
requested. Their pinned versions are resolved by the existing CMake / CI setup.
Their required notices are still installed with application distributions.

| Dependency | Version / source | Contents |
| --- | --- | --- |
| nicxlive | nijigenerate/nicxlive, `6ff544470b0f851ab998d8a12220aa750dac0d6b` | Original source archive and upstream license; application overlay remains in `native/tools` |
| ImGui | ocornut/imgui, `4e02e84cbb85d48881388e7090c2dfa1708c02a1` | Original docking source archive, MIT |
| Reference ImGui | Inochi2D/imgui, `dd5b7c6847372016f45d5b5abda687bd5cd19224` | Original source archive, MIT; comparison tests only |
| FreeType | freetype/freetype, `d6857981239ea5f6e95cb4eb4402307f3527760a` | Original source archive, upstream licenses including bundled zlib |
| nijitrack SDK | Hash in `nijitrack/sdk.sha256` | MediaPipe / Halide native SDK archive, component licenses and source provenance |
| MediaPipe task bundles | Versions, original URLs and hashes in `nijitrack/models/models.lock.json` | Face and hand models, Apache-2.0 license |
| Lua / Spout | Existing pinned application binaries, unchanged | Moved from `native/res/bin` to `runtime`; licenses in `licenses` |

Already checked-in adapted source and application resources (fonts, shaders,
icons, translations) stay in their existing source/resource directories with
their notices. They are not downloaded during the build.

Build tools are prerequisites: MSVC with Windows SDK, CMake, Python, and Ninja
when using the Ninja preset. LDC enables the original-D comparison tests. They
are not application dependencies and are not redistributed here. CI's gettext
catalog compiler and optional test portrait acquisition are separate from
application dependency resolution. The application contains no tracker download
or installer code.
