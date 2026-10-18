# Official stable release; source and backends are updated together.
FetchContent_Declare(imgui
    URL https://codeload.github.com/ocornut/imgui/tar.gz/4e02e84cbb85d48881388e7090c2dfa1708c02a1
    URL_HASH SHA256=4737967e4a571038fc8310b355a79e2b3b86e30008f06db3dce812837789c9e9
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
