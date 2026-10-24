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
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_include_directories(nijiexpose_imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_compile_definitions(nijiexpose_imgui PUBLIC IMGUI_ENABLE_FREETYPE=1 IMGUI_DISABLE_OBSOLETE_FUNCTIONS=1)
target_link_libraries(nijiexpose_imgui PUBLIC freetype SDL3::SDL3 PRIVATE ${CMAKE_DL_LIBS})
if(NIJIEXPOSE_RENDERER STREQUAL "VULKAN")
    target_sources(nijiexpose_imgui PRIVATE ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp)
    target_link_libraries(nijiexpose_imgui PUBLIC Vulkan::Vulkan)
endif()
