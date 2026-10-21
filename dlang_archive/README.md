# Archived D implementation

The legacy D application and D-only build assets live here. Shared packaging
assets have moved to `../build-aux` for the native application. The old packaging
scripts are retained as historical references and are not active build entry
points. The application source is unchanged.
The gettext catalogs remain in `../tl`; the root `gentl.sh` and `genpot.sh` now
use the native translation tooling.

`reference/tests` and `reference/tools` contain the D comparison programs and
reference-generation helpers formerly under `native`. The C++ test suite still
uses these references. Helpers resolve the repository root independently of cwd.

`.github/workflows` preserves the old D workflow definitions for reference.
They are archived, not active workflows; they require checkout/working-directory
and artifact-path adjustments before any future reactivation. The active C++
workflow remains at the repository root.

See [current native build instructions](../native/README.md). The original
README below describes the archived application's behavior and distribution.

---

# <div align="center"><img src="https://github.com/nijigenerate/nijiexpose/blob/main/res/logo.png" width="384" alt="nijigenerate"></div>

<img width="2560" height="1392" alt="image" src="https://github.com/user-attachments/assets/739561a1-d785-4290-a2f0-c763b3fcde0b" />

model by [seagetch](https://twitter.com/seagetch)_

*nijiexpose is the official tool for using nijilive puppets for VTubing.*

This application is currently under heavy development and isn't ready for production just yet.

## Downloads
Download release packages from [Releases](https://github.com/nijigenerate/nijiexpose/releases).
On Windows, run `nijiexpose-win32-x86_64.msi` to install the application and
shortcuts. The ZIP package remains available for portable use.

Windows builds run without a console window. Diagnostic output, including Lua
`print` and dependency output, is saved to
`%LOCALAPPDATA%\nijiexpose\logs\nijiexpose.log`. At startup, logs of 5 MiB or
larger are moved to `nijiexpose.log.previous`. If the log directory is unavailable,
output is discarded so logging does not prevent startup.

### Experimental Builds

We have a nightly build binary and a weekly build Flatpak available, but we do not guarantee stability. These versions may crash unexpectedly, and you are likely to encounter bugs. Be sure to save and back up your work frequently!

[![Nightly Builds](https://img.shields.io/github/actions/workflow/status/nijigenerate/nijiexpose/nightly.yml?label=Nightly&style=for-the-badge)](https://github.com/nijigenerate/nijiexpose/releases/tag/nightly)  

## Tracker launch settings

In tracking settings, choose a tracker launch mode:

- **Python**: set the tracker script directory containing `nijitrack.py`.
  Optionally specify the Python executable; leave it empty to use the directory's
  virtual environment or automatically detect Python. Automatic tracker installation
  is available in this mode.
- **Executable**: specify the tracker executable file directly (for example,
  `nijitrack.exe`). Python is not required in this mode. The executable must support
  the nijitrack command-line options, including `--list-devices`.

Both modes use the same camera, OSC host/port, flip, and preview settings. Existing
tracker settings continue to use Python until the launch mode is changed.
Paths copied with surrounding double quotes (Windows "Copy as path") are accepted.

## Building

The `native-cpp` branch also contains an experimental C++20 / nicxlive / SDL3
application with Windows and macOS build configurations. See [native build
instructions](native/README.md) and the [migration requirements](NATIVE_CPP_DESIGN.md).
Windows implementation parity with the D code is required; the native build is
not yet an equivalent application port.

We have a quick start guide [BUILD.md](https://github.com/nijigenerate/nijigenerate/blob/main/BUILD.md) in the nijigenerate project. This file contains information on how to build nijiexpose.

The C++ application now embeds nijitrack native in worker threads. Camera settings
are passed directly and tracking frames enter the application through an in-process
queue; no tracker subprocess or OSC connection is used. Bundled face/hand task files
and the Windows SDK are verified during configuration.
