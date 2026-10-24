# Official stable release; source and backends are updated together.
nijiexpose_dependency(imgui)
FetchContent_Declare(imgui
    URL "${dependency_url}"
    URL_HASH "SHA256=${dependency_hash}"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(imgui)
add_library(nijiexpose_imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp)
target_include_directories(nijiexpose_imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_compile_definitions(nijiexpose_imgui PUBLIC IMGUI_ENABLE_FREETYPE=1 IMGUI_DISABLE_OBSOLETE_FUNCTIONS=1)
target_link_libraries(nijiexpose_imgui PUBLIC freetype SDL3::SDL3 PRIVATE ${CMAKE_DL_LIBS})
if(NIJIEXPOSE_RENDERER STREQUAL "VULKAN")
    # The upstream example helper only tries OPAQUE/INHERIT. Apply nijimi's
    # alpha selection from the very first swapchain. Starting OPAQUE can pin
    # later capability queries to OPAQUE on Windows, preventing mode changes.
    # Preserve the pinned upstream source.
    file(READ "${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp" vulkan_backend)
    set(alpha_condition "if (cap.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)")
    string(FIND "${vulkan_backend}" "${alpha_condition}" alpha_position)
    if(alpha_position EQUAL -1)
        message(FATAL_ERROR "ImGui swapchain alpha selection changed; review the transparency adaptation")
    endif()
    string(REPLACE "${alpha_condition}"
        "if (cap.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)\n            info.compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;\n        else if (cap.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)\n            info.compositeAlpha = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;\n        else if (cap.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)\n            info.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;\n        else ${alpha_condition}"
        vulkan_backend "${vulkan_backend}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/imgui_impl_vulkan.cpp" "${vulkan_backend}")
    target_sources(nijiexpose_imgui PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/imgui_impl_vulkan.cpp")
    target_link_libraries(nijiexpose_imgui PUBLIC Vulkan::Vulkan)
else()
    target_sources(nijiexpose_imgui PRIVATE ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
endif()
