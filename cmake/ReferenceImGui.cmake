# Match the D build's i2d-imgui -> cimgui -> imgui source chain exactly.
# Used only by the D implementation comparison tests.
function(nijiexpose_add_reference_imgui)
    nijiexpose_dependency(freetype)
    FetchContent_Declare(freetype_binary URL "${dependency_url}" URL_HASH "SHA256=${dependency_hash}" DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(freetype_binary)
    add_library(freetype SHARED IMPORTED GLOBAL)
    set_target_properties(freetype PROPERTIES
        IMPORTED_IMPLIB "${freetype_binary_SOURCE_DIR}/release dll/win64/freetype.lib"
        IMPORTED_LOCATION "${freetype_binary_SOURCE_DIR}/release dll/win64/freetype.dll"
        INTERFACE_INCLUDE_DIRECTORIES "${freetype_binary_SOURCE_DIR}/include")
    set(NIJIEXPOSE_FREETYPE_LICENSE "${freetype_binary_SOURCE_DIR}/FTL.TXT" PARENT_SCOPE)
    if(NOT BUILD_TESTING)
        return()
    endif()
    nijiexpose_dependency(imgui_reference)
    FetchContent_Declare(d_imgui
        URL "${dependency_url}"
        URL_HASH "SHA256=${dependency_hash}"
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
