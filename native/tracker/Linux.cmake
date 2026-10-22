include("${CMAKE_CURRENT_LIST_DIR}/NijitrackSource.cmake")
find_package(X11 REQUIRED COMPONENTS Xtst)
add_library(nijiexpose_embedded_tracker SHARED embedded.cpp keyboard/common.cpp keyboard/x11.cpp)
target_compile_features(nijiexpose_embedded_tracker PRIVATE cxx_std_20)
target_compile_definitions(nijiexpose_embedded_tracker PRIVATE NIJITRACK_EMBEDDED_EXPORTS)
target_include_directories(nijiexpose_embedded_tracker PRIVATE upstream include
    "${PROJECT_SOURCE_DIR}/native/tracker")
target_include_directories(nijiexpose_embedded_tracker INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(nijiexpose_embedded_tracker PRIVATE
    "$<LINK_LIBRARY:WHOLE_ARCHIVE,nijitrack_mediapipe>" nijitrack_halide
    SDL3::SDL3 X11::X11 X11::Xtst)
# CMake places link options before object files. Preserve libraries emitted by
# the SDK response file until its static archive has supplied the references.
target_link_options(nijiexpose_embedded_tracker PRIVATE
    "LINKER:--push-state,--no-as-needed" "@${tracker_sdk}/mediapipe_tracking_linkopts.txt"
    "LINKER:--pop-state" "LINKER:-z,defs")
add_dependencies(nijiexpose_embedded_tracker nijitrack_source_sdk)
set_target_properties(nijiexpose_embedded_tracker PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/native" INSTALL_RPATH "$ORIGIN")
add_custom_target(nijiexpose_tracker_models ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${tracker_vendor}/models" "$<TARGET_FILE_DIR:nijiexpose_native>/models")
add_dependencies(nijiexpose_native nijiexpose_tracker_models)
install(TARGETS nijiexpose_embedded_tracker LIBRARY DESTINATION lib)
install(DIRECTORY "${tracker_vendor}/models/" DESTINATION bin/models)
install(FILES LICENSE.nijitrack "${tracker_sdk}/LICENSE.mediapipe"
    "${tracker_sdk}/LICENSE.halide" "${tracker_sdk}/SOURCE.txt"
    DESTINATION ${NIJIEXPOSE_LICENSE_DESTINATION}/nijitrack)
install(DIRECTORY "${tracker_sdk}/licenses/" DESTINATION ${NIJIEXPOSE_LICENSE_DESTINATION}/nijitrack/licenses)
if(BUILD_TESTING)
    add_test(NAME native_tracker_halide_roi COMMAND "${tracker_native}/build-mediapipe/halide/warp-tests")
    set(NIJIEXPOSE_TRACKER_TEST_IMAGE "" CACHE FILEPATH "Portrait fixture for real inference tests")
    if(NIJIEXPOSE_TRACKER_TEST_IMAGE)
        add_executable(nijiexpose_embedded_tracker_test ../tests/embedded_tracker_test.cpp)
        target_compile_features(nijiexpose_embedded_tracker_test PRIVATE cxx_std_20)
        target_include_directories(nijiexpose_embedded_tracker_test PRIVATE
            "${PROJECT_SOURCE_DIR}/native/port" "${PROJECT_SOURCE_DIR}/native/src" "${nicxlive_SOURCE_DIR}")
        target_link_libraries(nijiexpose_embedded_tracker_test PRIVATE nijiexpose_tracker SDL3::SDL3)
        target_compile_definitions(nijiexpose_embedded_tracker_test PRIVATE NIJIEXPOSE_TEST_BUNDLED_MODEL)
        set_target_properties(nijiexpose_embedded_tracker_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/native")
        add_dependencies(nijiexpose_embedded_tracker_test nijiexpose_tracker_models)
        add_test(NAME native_embedded_tracker COMMAND nijiexpose_embedded_tracker_test
            --bundled "${NIJIEXPOSE_TRACKER_TEST_IMAGE}" "${CMAKE_BINARY_DIR}/tracker-results")
        set_tests_properties(native_embedded_tracker PROPERTIES TIMEOUT 60 ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_RENDER_DRIVER=software")
    endif()
endif()
