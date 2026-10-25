# Extract only the application modules used by the comparison tests. Never
# overlay the D branch onto the C++ source tree or build the retired application.
find_program(NIJIEXPOSE_LDC_EXECUTABLE NAMES ldc2 HINTS C:/opt/ldc-1.41/bin REQUIRED)
execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${PROJECT_SOURCE_DIR}"
    rev-parse --verify refs/remotes/origin/dlang-impl
    OUTPUT_VARIABLE dlang_reference_commit OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE dlang_reference_result ERROR_QUIET)
if(NOT dlang_reference_result EQUAL 0)
    message(FATAL_ERROR "D comparisons require origin/dlang-impl. Run: git fetch origin dlang-impl:refs/remotes/origin/dlang-impl")
endif()
message(STATUS "D comparison reference: origin/dlang-impl (${dlang_reference_commit})")
execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${PROJECT_SOURCE_DIR}"
    rev-parse --git-path refs/remotes/origin/dlang-impl
    OUTPUT_VARIABLE dlang_reference_ref OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY)
get_filename_component(dlang_reference_ref "${dlang_reference_ref}" ABSOLUTE BASE_DIR "${PROJECT_SOURCE_DIR}")
if(EXISTS "${dlang_reference_ref}")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${dlang_reference_ref}")
endif()
set(dlang_reference_root "${CMAKE_CURRENT_BINARY_DIR}/dlang-reference")
file(MAKE_DIRECTORY "${dlang_reference_root}")
file(WRITE "${dlang_reference_root}/revision.txt" "${dlang_reference_commit}\n")
foreach(module IN ITEMS log.d utils/launchpath.d utils/subprocess.d windows/utils.d)
    set(reference_path "source/nijiexpose/${module}")
    get_filename_component(reference_directory "${dlang_reference_root}/${reference_path}" DIRECTORY)
    file(MAKE_DIRECTORY "${reference_directory}")
    execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${PROJECT_SOURCE_DIR}"
        show "${dlang_reference_commit}:${reference_path}"
        OUTPUT_FILE "${dlang_reference_root}/${reference_path}"
        COMMAND_ERROR_IS_FATAL ANY)
endforeach()
