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
development harness. See [native build instructions](native/README.md) and the
[migration requirements](NATIVE_CPP_DESIGN.md). Windows implementation parity
with the D code is required; the harness is not an equivalent application port.

We have a quick start guide [BUILD.md](https://github.com/nijigenerate/nijigenerate/blob/main/BUILD.md) in the nijigenerate project. This file contains information on how to build nijiexpose.
