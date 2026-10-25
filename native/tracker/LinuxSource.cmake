if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
    message(FATAL_ERROR "The pinned nijitrack Linux source recipe supports x86_64")
endif()
find_program(NIJIEXPOSE_BASH bash REQUIRED)
foreach(tool ninja clang clang++ git jq curl tar pkg-config)
    find_program(tracker_tool_${tool} NAMES ${tool} REQUIRED)
endforeach()
find_package(Java REQUIRED COMPONENTS Development)
get_filename_component(tracker_javac "${Java_JAVAC_EXECUTABLE}" REALPATH)
get_filename_component(tracker_java_bin "${tracker_javac}" DIRECTORY)
get_filename_component(tracker_java_home "${tracker_java_bin}" DIRECTORY)
set(NIJIEXPOSE_BAZEL_CACHE "${CMAKE_BINARY_DIR}/tracker-cache" CACHE PATH "Tracker compiler action cache")
set(tracker_environment "${CMAKE_COMMAND}" -E env
    "JAVA_HOME=${tracker_java_home}"
    "NIJIEXPOSE_BAZEL_CACHE=${NIJIEXPOSE_BAZEL_CACHE}")
ExternalProject_Add(nijitrack_source_sdk
    URL "${tracker_source_url}" URL_HASH "SHA256=${tracker_source_hash}"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_DIR "${tracker_source}" UPDATE_COMMAND ""
    PATCH_COMMAND "${CMAKE_COMMAND}" -DNIJITRACK_SOURCE=<SOURCE_DIR>
        -P "${CMAKE_CURRENT_LIST_DIR}/patches/linux-cache.cmake"
    CONFIGURE_COMMAND ${tracker_environment} "${NIJIEXPOSE_BASH}" <SOURCE_DIR>/native/build.sh setup
    BUILD_COMMAND ${tracker_environment} "${NIJIEXPOSE_BASH}" <SOURCE_DIR>/native/build.sh sdk
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS "${tracker_sdk}/libmediapipe_tracking.a"
        "${tracker_sdk}/libnijitrack_warp.a" "${tracker_sdk}/mediapipe_tracking_linkopts.txt")
ExternalProject_Add_StepDependencies(nijitrack_source_sdk patch
    "${CMAKE_CURRENT_LIST_DIR}/patches/linux-cache.cmake")
ExternalProject_Add_Step(nijitrack_source_sdk prepare
    COMMAND ${tracker_environment} "${NIJIEXPOSE_BASH}" <SOURCE_DIR>/native/build.sh prepare
    DEPENDEES configure DEPENDERS build)
ExternalProject_Add_StepTargets(nijitrack_source_sdk configure prepare)
add_library(nijitrack_mediapipe STATIC IMPORTED)
set_target_properties(nijitrack_mediapipe PROPERTIES IMPORTED_LOCATION "${tracker_sdk}/libmediapipe_tracking.a")
add_library(nijitrack_halide STATIC IMPORTED)
set_target_properties(nijitrack_halide PROPERTIES IMPORTED_LOCATION "${tracker_sdk}/libnijitrack_warp.a")
