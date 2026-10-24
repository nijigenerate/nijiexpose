set(vulkan_shader_headers)
foreach(shader part.vert part.frag part_stage1.frag part_stage2.frag part_mask.frag mask.vert mask.frag overlay_blur.frag emission_combine.frag opaque_alpha.vert opaque_alpha.frag)
    string(REPLACE "." "_" symbol "${shader}")
    set(spv "${CMAKE_CURRENT_BINARY_DIR}/vulkan/${shader}.spv")
    set(header "${CMAKE_CURRENT_BINARY_DIR}/vulkan/${symbol}.hpp")
    add_custom_command(OUTPUT "${header}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/vulkan"
        COMMAND Vulkan::glslc "${CMAKE_CURRENT_SOURCE_DIR}/src/render/vulkan/shaders/${shader}" -o "${spv}"
        COMMAND ${CMAKE_COMMAND} "-DINPUT=${spv}" "-DOUTPUT=${header}" "-DSYMBOL=${symbol}" -P "${PROJECT_SOURCE_DIR}/cmake/EmbedSpirv.cmake"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/render/vulkan/shaders/${shader}" "${PROJECT_SOURCE_DIR}/cmake/EmbedSpirv.cmake" VERBATIM)
    list(APPEND vulkan_shader_headers "${header}")
endforeach()
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/vulkan_shaders.hpp" "#pragma once\n#include <cstdint>\n#include \"vulkan/part_vert.hpp\"\n#include \"vulkan/part_frag.hpp\"\n#include \"vulkan/part_stage1_frag.hpp\"\n#include \"vulkan/part_stage2_frag.hpp\"\n#include \"vulkan/part_mask_frag.hpp\"\n#include \"vulkan/mask_vert.hpp\"\n#include \"vulkan/mask_frag.hpp\"\n#include \"vulkan/overlay_blur_frag.hpp\"\n#include \"vulkan/emission_combine_frag.hpp\"\n")
add_library(nijiexpose_vulkan STATIC src/render/vulkan/context.cpp src/render/vulkan/backend.cpp ${vulkan_shader_headers})
target_include_directories(nijiexpose_vulkan PUBLIC src "${nicxlive_SOURCE_DIR}" PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(nijiexpose_vulkan PUBLIC nicxlive::nicxlive nijiexpose_imgui SDL3::SDL3 Vulkan::Vulkan)
if(MSVC)
    target_compile_options(nijiexpose_vulkan PRIVATE /utf-8 /W4 /fp:strict)
endif()
