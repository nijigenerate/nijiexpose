# Use the same pinned source recipe as macOS; never consume a packaged SDK.
find_program(NIJIEXPOSE_PWSH pwsh REQUIRED)
set(NIJIEXPOSE_BAZEL_CACHE "${CMAKE_BINARY_DIR}/tracker-cache" CACHE PATH "Bazel compilation cache")
set(tracker_environment "${CMAKE_COMMAND}" -E env
    "NIJIEXPOSE_BAZEL_CACHE=${NIJIEXPOSE_BAZEL_CACHE}")
ExternalProject_Add(nijitrack_source_sdk
    URL "${tracker_source_url}" URL_HASH "SHA256=${tracker_source_hash}"
    SOURCE_DIR "${tracker_source}"
    UPDATE_COMMAND ""
    PATCH_COMMAND "${NIJIEXPOSE_PWSH}" -NoProfile -File
        "${CMAKE_CURRENT_LIST_DIR}/patches/enable-windows-cache.ps1" -Source <SOURCE_DIR>
    CONFIGURE_COMMAND "${NIJIEXPOSE_PWSH}" -NoProfile -File
        <SOURCE_DIR>/native/scripts/setup_tracking_toolchain.ps1
    BUILD_COMMAND ${tracker_environment} "${NIJIEXPOSE_PWSH}" -NoProfile -File
        <SOURCE_DIR>/native/scripts/build_mediapipe_halide.ps1
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS "${tracker_sdk}/mediapipe_tracking.lib"
        "${tracker_sdk}/nijitrack_warp.lib"
        "${tracker_sdk}/mediapipe_tracking_linkopts.txt")
ExternalProject_Add_Step(nijitrack_source_sdk prepare
    COMMAND "${NIJIEXPOSE_PWSH}" -NoProfile -File
        <SOURCE_DIR>/native/scripts/prepare_mediapipe_halide.ps1
    DEPENDEES configure DEPENDERS build)
ExternalProject_Add_StepTargets(nijitrack_source_sdk configure prepare)
