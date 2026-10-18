# Updating native build dependencies

`dependencies.lock.json` is the build input for SDL3, gettext/libiconv, FreeType,
Boost, ImGui, reference ImGui and nicxlive. CMake and the Boost download helper
read it directly. Do not duplicate version-specific URLs or hashes in CI.

Prefer upstream binaries with the required ABI. SDL provides its own MSVC SDK.
GNU gettext's [download page](https://www.gnu.org/software/gettext/) links to
Michele Locati's Windows distribution; FreeType's
[download page](https://freetype.org/download.html) links to
ubawurinna/freetype-windows-binaries. The latter two are upstream-recommended
distributors, not binaries built by this repository's developer. No locally
built dependency SDK is uploaded or used.

1. Select a release from the recorded upstream or official download page.
   Check the x64 MSVC ABI, DLL/import-library pair, headers, CRT and transitive
   DLL dependencies. Preserve the required features and application behavior.
2. Download the release, verify its upstream checksum/signature where available,
   and record its exact URL, version, SHA-256, ABI and license paths in the lock.
   Do not use a mutable `latest` URL. For a vendored source dependency, replace
   the archive and matching license notices and update `vendor/manifest.json`
   for those files. Keep the upstream source archive intact.
3. If a package changes its directory layout, update the corresponding CMake
   import/install paths. Stage its notices and every required runtime DLL.
   Update version statements in the README. Tracker SDK and task bundles keep
   their own `vendor/nijitrack/sdk.sha256` and `models/models.lock.json`; update
   their provenance and licenses together when changing those packages.
4. Configure a fresh build directory, compile the application, run comparisons
   and functional tests, then install into an empty staging directory and test
   the staged executable. Compare rendered output when changing graphics/font
   dependencies, and locale switching when changing gettext. Record any change
   in behavior; a dependency version bump does not authorize feature removal.
5. Commit and push the update, inspect the actual CI result, and fix failures.
   Re-run the same commit to verify cache reuse when changing cache/toolchain
   configuration. Build time and sccache statistics are job artifacts.

Binary downloads do not need compilation caches. sccache reuses C/C++ outputs
only when compiler, inputs and options match; affected files rebuild after an
update. Boost's download cache is keyed by the lock/helper hashes. Linking and
tests still run. ImGui has application-selected compile settings and backends;
the patched nicxlive core and application code are compiled with sccache.

Updates are explicit, reviewable commits. CI never silently selects a new
dependency version. A distributor's missing binary is not a reason to publish
an ad hoc local build or change the required version without recording it.
