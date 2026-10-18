find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(native_nicxlive_dir "${CMAKE_BINARY_DIR}/native-nicxlive")
execute_process(COMMAND "${Python3_EXECUTABLE}"
    "${PROJECT_SOURCE_DIR}/native/tools/prepare-nicxlive-native.py"
    --source "${nicxlive_SOURCE_DIR}" --output "${native_nicxlive_dir}"
    COMMAND_ERROR_IS_FATAL ANY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/native/tools/prepare-nicxlive-native.py")
get_target_property(native_nicxlive_sources nicxlive SOURCES)
list(FILTER native_nicxlive_sources EXCLUDE REGEX "(^|/)core/(texture|puppet|nodes/part)\\.cpp$")
set(native_nicxlive_overlay_sources)
foreach(source IN LISTS native_nicxlive_sources)
    if(source MATCHES "^core/")
        list(APPEND native_nicxlive_overlay_sources "${native_nicxlive_dir}/${source}")
    else()
        list(APPEND native_nicxlive_overlay_sources "${source}")
    endif()
endforeach()
set_property(TARGET nicxlive PROPERTY SOURCES "${native_nicxlive_overlay_sources}")
include_directories(BEFORE "${native_nicxlive_dir}" "${native_nicxlive_dir}/core")
target_include_directories(nicxlive BEFORE PUBLIC "${native_nicxlive_dir}" "${native_nicxlive_dir}/core"
    PRIVATE "${native_nicxlive_dir}/core/nodes")
target_sources(nicxlive PRIVATE "${native_nicxlive_dir}/texture.cpp" "${native_nicxlive_dir}/part.cpp" "${native_nicxlive_dir}/puppet.cpp")
target_include_directories(nicxlive PRIVATE "${PROJECT_SOURCE_DIR}/native/port"
    "${nicxlive_SOURCE_DIR}" "${nicxlive_SOURCE_DIR}/core/nodes")
