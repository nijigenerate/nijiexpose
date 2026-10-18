# Match the D build's i2d-imgui -> cimgui -> imgui source chain exactly.
# Used only by the D implementation comparison tests.
function(nijiexpose_add_reference_imgui)
    set(BUILD_SHARED_LIBS OFF)
    set(SKIP_INSTALL_ALL ON)
    FetchContent_Declare(d_freetype
        URL https://codeload.github.com/freetype/freetype/tar.gz/d6857981239ea5f6e95cb4eb4402307f3527760a
        URL_HASH SHA256=6d47b210532abea9a39cee4286e9990bffcba2c5a392c8962c3758c7d6ca11dc
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(d_freetype)
    if(MSVC)
        target_compile_options(freetype PRIVATE /utf-8)
    endif()
    if(NOT BUILD_TESTING)
        return()
    endif()
    FetchContent_Declare(d_imgui
        URL https://codeload.github.com/Inochi2D/imgui/tar.gz/dd5b7c6847372016f45d5b5abda687bd5cd19224
        URL_HASH SHA256=2d048af300b5d0e43b767c74c1d473155dd45a6f623a15eaaF7ff2fbaee1a051
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(d_imgui)
    add_library(nijiexpose_reference_imgui STATIC
        ${d_imgui_SOURCE_DIR}/imgui.cpp
        ${d_imgui_SOURCE_DIR}/imgui_draw.cpp
        ${d_imgui_SOURCE_DIR}/imgui_tables.cpp
        ${d_imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${d_imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp)
    target_include_directories(nijiexpose_reference_imgui PUBLIC
        ${d_imgui_SOURCE_DIR} ${d_imgui_SOURCE_DIR}/backends)
    target_compile_definitions(nijiexpose_reference_imgui PUBLIC
        IMGUI_ENABLE_FREETYPE=1 IMGUI_ENABLE_STB_TRUETYPE=1 IMGUI_DISABLE_OBSOLETE_FUNCTIONS=1)
    target_link_libraries(nijiexpose_reference_imgui PUBLIC freetype PRIVATE ${CMAKE_DL_LIBS})
endfunction()
nijiexpose_add_reference_imgui()
