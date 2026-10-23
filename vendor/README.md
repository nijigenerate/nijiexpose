# Native Windows build dependencies

This directory contains the dependencies selected for vendoring. Archives are
extracted under the build directory and verified against `manifest.json`.
Original license files remain inside archives and are copied into `licenses/`;
tracker SDK notices are also available under `nijitrack/licenses/`.

Unmodified **Boost, gettext (including libiconv), and SDL3 are not vendored**, as
requested. Their pinned versions are resolved by CMake / CI from
`cmake/dependencies.lock.json`. FreeType also uses a downloaded binary from
the distributor linked by its official download page.
Their required notices are still installed with application distributions.

| Dependency | Version / source | Contents |
| --- | --- | --- |
| nicxlive | nijigenerate/nicxlive, `6b96e03c8db4b1400252ce2dbcbe8f5a356bf656` | Source snapshot including the nicxlive `resource-lifetime` branch changes, hierarchy-rescan render refresh (PR #19), and upstream license; resource management is implemented in nicxlive itself, not in the application overlay |
| ImGui | ocornut/imgui, `4e02e84cbb85d48881388e7090c2dfa1708c02a1` | Original docking source archive, MIT |
| Reference ImGui | Inochi2D/imgui, `dd5b7c6847372016f45d5b5abda687bd5cd19224` | Original source archive, MIT; comparison tests only |
| nijitrack | Source pin in `native/tracker/NijitrackSource.cmake` | Both Windows and macOS build the SDK from source; no prebuilt SDK archive is vendored. Model bundles and license notices remain here. |
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
