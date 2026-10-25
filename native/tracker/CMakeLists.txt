set(tracker_vendor "${PROJECT_SOURCE_DIR}/vendor/nijitrack")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include("${CMAKE_CURRENT_LIST_DIR}/Linux.cmake")
    return()
endif()
if(APPLE)
    include("${CMAKE_CURRENT_SOURCE_DIR}/NijitrackSource.cmake")
    add_library(nijiexpose_embedded_tracker SHARED
        embedded.cpp keyboard/common.cpp keyboard/macos.cpp)
    set_target_properties(nijiexpose_embedded_tracker PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/native"
        INSTALL_RPATH "@loader_path")
    target_compile_features(nijiexpose_embedded_tracker PRIVATE cxx_std_20)
    target_compile_definitions(nijiexpose_embedded_tracker PRIVATE NIJITRACK_EMBEDDED_EXPORTS)
    target_include_directories(nijiexpose_embedded_tracker PRIVATE
        upstream include "${PROJECT_SOURCE_DIR}/native/tracker")
    target_include_directories(nijiexpose_embedded_tracker INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(nijiexpose_embedded_tracker PRIVATE
        "$<LINK_LIBRARY:WHOLE_ARCHIVE,nijitrack_mediapipe>"
        nijitrack_halide SDL3::SDL3 "-framework ApplicationServices")
    # Upstream emits one linker argument per line. A compiler response file
    # preserves framework/name pairs and works before the SDK has been built.
    target_link_options(nijiexpose_embedded_tracker PRIVATE
        "-mmacosx-version-min=${tracker_sdk_version}"
        "@${tracker_sdk}/mediapipe_tracking_linkopts.txt")
    add_dependencies(nijiexpose_embedded_tracker nijitrack_source_sdk)
    install(TARGETS nijiexpose_embedded_tracker LIBRARY
        DESTINATION nijiexpose.app/Contents/Frameworks)
    install(FILES LICENSE.nijitrack "${tracker_sdk}/LICENSE.mediapipe"
        "${tracker_sdk}/LICENSE.halide" "${tracker_sdk}/SOURCE.txt"
        DESTINATION ${NIJIEXPOSE_LICENSE_DESTINATION}/nijitrack)
    install(DIRECTORY "${tracker_sdk}/licenses/"
        DESTINATION ${NIJIEXPOSE_LICENSE_DESTINATION}/nijitrack/licenses)
    set(NIJIEXPOSE_TRACKER_TEST_IMAGE "" CACHE FILEPATH
        "Optional local portrait fixture for the existing real-inference test")
    if(NIJIEXPOSE_TRACKER_TEST_IMAGE)
        add_executable(nijiexpose_embedded_tracker_test ../tests/embedded_tracker_test.cpp)
        set_target_properties(nijiexpose_embedded_tracker_test PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/native")
        target_compile_features(nijiexpose_embedded_tracker_test PRIVATE cxx_std_20)
        target_include_directories(nijiexpose_embedded_tracker_test PRIVATE
            "${PROJECT_SOURCE_DIR}/native/port" "${PROJECT_SOURCE_DIR}/native/src" "${nicxlive_SOURCE_DIR}")
        target_link_libraries(nijiexpose_embedded_tracker_test PRIVATE
            nijiexpose_embedded_tracker SDL3::SDL3)
    endif()
    return()
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/NijitrackSource.cmake")
set(NIJIEXPOSE_TRACKING_SDK "${tracker_sdk}")
file(READ "${tracker_vendor}/models/models.lock.json" model_lock)
foreach(name face_landmarker.task hand_landmarker.task)
    string(JSON expected GET "${model_lock}" "${name}" sha256)
    file(SHA256 "${tracker_vendor}/models/${name}" actual)
    if(NOT actual STREQUAL expected)
        message(FATAL_ERROR "Tracker task checksum mismatch: ${name}")
    endif()
endforeach()
add_library(nijiexpose_mediapipe STATIC IMPORTED)
set_target_properties(nijiexpose_mediapipe PROPERTIES IMPORTED_LOCATION "${NIJIEXPOSE_TRACKING_SDK}/mediapipe_tracking.lib")
add_library(nijiexpose_embedded_tracker SHARED embedded.cpp keyboard/common.cpp keyboard/windows.cpp)
add_dependencies(nijiexpose_embedded_tracker nijitrack_source_sdk)
set_target_properties(nijiexpose_embedded_tracker PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/native")
target_compile_features(nijiexpose_embedded_tracker PRIVATE cxx_std_20)
target_compile_definitions(nijiexpose_embedded_tracker PRIVATE NIJITRACK_EMBEDDED_EXPORTS NOMINMAX WIN32_LEAN_AND_MEAN)
target_compile_options(nijiexpose_embedded_tracker PRIVATE /utf-8 /W4)
target_include_directories(nijiexpose_embedded_tracker PRIVATE upstream include)
target_include_directories(nijiexpose_embedded_tracker INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(nijiexpose_embedded_tracker PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,nijiexpose_mediapipe>"
    "${NIJIEXPOSE_TRACKING_SDK}/nijitrack_warp.lib" SDL3::SDL3 bcrypt ws2_32 shlwapi shell32 dbghelp advapi32 user32)
add_custom_target(nijiexpose_tracker_models ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${tracker_vendor}/models" "$<TARGET_FILE_DIR:nijiexpose_native>/models")
add_dependencies(nijiexpose_native nijiexpose_tracker_models)
install(TARGETS nijiexpose_embedded_tracker RUNTIME DESTINATION bin)
install(DIRECTORY "${tracker_vendor}/models/" DESTINATION bin/models)
install(FILES LICENSE.nijitrack "${NIJIEXPOSE_TRACKING_SDK}/LICENSE.mediapipe"
    "${NIJIEXPOSE_TRACKING_SDK}/LICENSE.halide" "${NIJIEXPOSE_TRACKING_SDK}/SOURCE.txt" DESTINATION ${NIJIEXPOSE_LICENSE_DESTINATION}/nijitrack)
install(DIRECTORY "${NIJIEXPOSE_TRACKING_SDK}/licenses/" DESTINATION ${NIJIEXPOSE_LICENSE_DESTINATION}/nijitrack)
if(BUILD_TESTING)
    add_test(NAME native_tracker_halide_roi
        COMMAND "${tracker_native}/build-mediapipe/halide/warp-tests.exe")
    add_executable(nijiexpose_embedded_tracker_test ../tests/embedded_tracker_test.cpp)
    set_target_properties(nijiexpose_embedded_tracker_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/native")
    target_compile_features(nijiexpose_embedded_tracker_test PRIVATE cxx_std_20)
    target_include_directories(nijiexpose_embedded_tracker_test PRIVATE "${PROJECT_SOURCE_DIR}/native/port" "${PROJECT_SOURCE_DIR}/native/src" "${nicxlive_SOURCE_DIR}")
    target_link_libraries(nijiexpose_embedded_tracker_test PRIVATE nijiexpose_embedded_tracker SDL3::SDL3)
    set(NIJIEXPOSE_TRACKER_TEST_IMAGE "" CACHE FILEPATH "Optional local MediaPipe portrait fixture for real inference tests")
    if(NIJIEXPOSE_TRACKER_TEST_IMAGE)
        add_test(NAME native_embedded_tracker COMMAND nijiexpose_embedded_tracker_test
            "${tracker_vendor}/models/face_landmarker.task" "${NIJIEXPOSE_TRACKER_TEST_IMAGE}" "${CMAKE_BINARY_DIR}/tracker-results")
        set_tests_properties(native_embedded_tracker PROPERTIES TIMEOUT 60 ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_RENDER_DRIVER=software")
    endif()
endif()
