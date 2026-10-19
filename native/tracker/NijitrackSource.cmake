# Build the pinned nijitrack SDK using its own verified patches and scripts.
include(ExternalProject)
set(tracker_source_url "https://codeload.github.com/nijigenerate/nijitrack/tar.gz/ac76c322aedd62740eee01c3ea1c528f7de51f6c")
set(tracker_source_hash bb5f1f25f8d1849f9bee94d9858fdc96bbe84249bcc8d1c7b06762824cea916d)
set(tracker_source "${CMAKE_BINARY_DIR}/_deps/nijitrack-main-src")
if(WIN32)
    set(NIJIEXPOSE_TRACKER_SOURCE_DIR "${CMAKE_BINARY_DIR}/nt" CACHE PATH
        "Short SDK source/build path (Windows generators require short paths)")
    set(tracker_source "${NIJIEXPOSE_TRACKER_SOURCE_DIR}")
endif()
set(tracker_native "${tracker_source}/native")
set(tracker_sdk "${tracker_native}/vendor/sdk-halide")
if(WIN32)
    include("${CMAKE_CURRENT_LIST_DIR}/WindowsSource.cmake")
    return()
endif()
find_program(NIJIEXPOSE_BASH bash REQUIRED)
foreach(tool cmake ninja clang clang++ git jq curl tar pkg-config)
    find_program(tracker_tool_${tool} NAMES ${tool} REQUIRED)
endforeach()
set(NIJIEXPOSE_JAVA_HOME "$ENV{JAVA_HOME}" CACHE PATH "JDK used by nijitrack's Bazel repository configuration")
if(NOT NIJIEXPOSE_JAVA_HOME)
    execute_process(COMMAND /usr/libexec/java_home
        OUTPUT_VARIABLE tracker_java_home OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    if(NOT tracker_java_home AND EXISTS "/opt/homebrew/opt/openjdk/libexec/openjdk.jdk/Contents/Home/bin/java")
        set(tracker_java_home "/opt/homebrew/opt/openjdk/libexec/openjdk.jdk/Contents/Home")
    endif()
    set(NIJIEXPOSE_JAVA_HOME "${tracker_java_home}" CACHE PATH "JDK used by nijitrack's Bazel repository configuration" FORCE)
endif()
if(NOT EXISTS "${NIJIEXPOSE_JAVA_HOME}/bin/javac")
    message(FATAL_ERROR "A JDK is required: install openjdk and set NIJIEXPOSE_JAVA_HOME to its Contents/Home directory")
endif()
execute_process(COMMAND "${NIJIEXPOSE_JAVA_HOME}/bin/java" -version
    RESULT_VARIABLE tracker_java_result OUTPUT_QUIET ERROR_QUIET)
if(NOT tracker_java_result EQUAL 0)
    message(FATAL_ERROR "The configured nijitrack JDK cannot run: ${NIJIEXPOSE_JAVA_HOME}")
endif()
if(NOT CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64" OR
   (CMAKE_OSX_ARCHITECTURES AND NOT CMAKE_OSX_ARCHITECTURES STREQUAL "arm64"))
    message(FATAL_ERROR "The pinned nijitrack macOS source recipe supports arm64")
endif()
execute_process(COMMAND xcrun --sdk macosx --show-sdk-version
    OUTPUT_VARIABLE tracker_sdk_version OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY)
# With Command Line Tools, nijitrack's Bazel target defaults its deployment
# version to the selected SDK. Record that requirement consistently at link
# time and in the app's metadata; objects built for older macOS remain usable.
set(NIJIEXPOSE_MACOS_MINIMUM_VERSION "${tracker_sdk_version}" PARENT_SCOPE)
target_link_options(nijiexpose_native PRIVATE "-mmacosx-version-min=${tracker_sdk_version}")
# The upstream script records paths in its recipe and Bazel actions. Keep the
# source, compiler search path and all caches in this persistent build tree.
set(tracker_environment "${CMAKE_COMMAND}" -E env
    "JAVA_HOME=${NIJIEXPOSE_JAVA_HOME}"
    "PATH=/usr/bin:/bin:/usr/sbin:/sbin:/opt/homebrew/bin")
ExternalProject_Add(nijitrack_source_sdk
    URL "${tracker_source_url}"
    URL_HASH "SHA256=${tracker_source_hash}"
    SOURCE_DIR "${tracker_source}"
    UPDATE_COMMAND ""
    PATCH_COMMAND "${CMAKE_COMMAND}" -DNIJITRACK_SOURCE=<SOURCE_DIR>
        -P "${CMAKE_CURRENT_LIST_DIR}/patches/apply-nijitrack.cmake"
    CONFIGURE_COMMAND ${tracker_environment} "${NIJIEXPOSE_BASH}" <SOURCE_DIR>/native/build.sh setup
    BUILD_COMMAND ${tracker_environment} "${NIJIEXPOSE_BASH}" <SOURCE_DIR>/native/build.sh sdk
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS "${tracker_sdk}/libmediapipe_tracking.a"
        "${tracker_sdk}/libnijitrack_warp.a"
        "${tracker_sdk}/mediapipe_tracking_linkopts.txt")
ExternalProject_Add_Step(nijitrack_source_sdk prepare
    COMMAND ${tracker_environment} "${NIJIEXPOSE_BASH}" <SOURCE_DIR>/native/build.sh prepare
    DEPENDEES configure DEPENDERS build)
ExternalProject_Add_StepTargets(nijitrack_source_sdk configure prepare)
add_custom_target(nijitrack_sdk_preflight
    COMMAND ${tracker_environment} "NIJITRACK_ANALYZE_ONLY=ON"
        "${NIJIEXPOSE_BASH}" "${tracker_native}/build.sh" sdk
    DEPENDS nijitrack_source_sdk-prepare
    USES_TERMINAL VERBATIM)

add_library(nijitrack_mediapipe STATIC IMPORTED)
set_target_properties(nijitrack_mediapipe PROPERTIES
    IMPORTED_LOCATION "${tracker_sdk}/libmediapipe_tracking.a")
add_library(nijitrack_halide STATIC IMPORTED)
set_target_properties(nijitrack_halide PROPERTIES
    IMPORTED_LOCATION "${tracker_sdk}/libnijitrack_warp.a")
