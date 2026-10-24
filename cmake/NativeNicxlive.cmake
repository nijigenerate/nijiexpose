find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(native_nicxlive_dir "${CMAKE_BINARY_DIR}/native-nicxlive")
execute_process(COMMAND "${Python3_EXECUTABLE}"
    "${PROJECT_SOURCE_DIR}/native/tools/prepare-nicxlive-native.py"
    --source "${nicxlive_SOURCE_DIR}" --output "${native_nicxlive_dir}"
    COMMAND_ERROR_IS_FATAL ANY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/native/tools/prepare-nicxlive-native.py")
get_target_property(native_nicxlive_sources nicxlive SOURCES)
list(FILTER native_nicxlive_sources EXCLUDE REGEX "(^|/)core/nodes/drawable\\.cpp$")
set_property(TARGET nicxlive PROPERTY SOURCES "${native_nicxlive_sources}")
target_sources(nicxlive PRIVATE "${native_nicxlive_dir}/drawable.cpp")
target_include_directories(nicxlive PUBLIC "${nicxlive_SOURCE_DIR}" "${nicxlive_SOURCE_DIR}/core"
    PRIVATE "${nicxlive_SOURCE_DIR}/core/nodes")
