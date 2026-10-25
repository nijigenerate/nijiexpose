# D comparison tests

The optional local Windows style, launchpath, subprocess and log tests compare against
`origin/dlang-impl`, not snapshots of the deleted archive. Fetch the remote branch, then explicitly enable the tests in the local build:

```sh
git fetch origin dlang-impl:refs/remotes/origin/dlang-impl
cmake -S . -B build/native -DNIJIEXPOSE_D_REFERENCE_TESTS=ON
```

Install LDC (`ldc2` on PATH; the local `C:/opt/ldc-1.41/bin` is also searched).
CMake resolves the branch to one commit and extracts only the four application
modules needed by these tests into the build directory's
`native/dlang-reference/source`. The exact commit is printed at configure time
and saved in `native/dlang-reference/revision.txt`. Fetch and reconfigure to
compare against an updated branch. Missing branch/compiler is an error, not a
fallback to old snapshots. The option defaults to OFF. CI does not fetch the D branch, install LDC, or run these comparisons.

`dlang/reference_*.d` are test harnesses. LDC compiles them with the unmodified
branch modules; it does not build the D application. Style assignments are read
directly from that same branch revision and compared bit-for-bit. All generated
files and binaries stay in the build directory. Nothing generates D application
source in the C++ tree.

Coverage: 21,898 path/UTF-8/splitLines/file cases, five subprocess scenarios,
five logging scenarios, and all literal style assignments. Existing recorded
fixtures for other migration comparisons are also registered only when the
local comparison option is enabled; they are not live branch comparisons.
Normal builds and CI register only functional tests. Settings functionality
checks defaults, typed values, save/reload and unknown-field preservation
without consulting D results.
