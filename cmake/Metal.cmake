# Embed the ported MSL so the Command Line Tools are sufficient to build the app.
set(metal_dir "${CMAKE_CURRENT_SOURCE_DIR}/src/render/metal")
set(metal_shader "${metal_dir}/shaders/render.metal")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${metal_shader}")
file(READ "${metal_shader}" metal_source)
configure_file("${PROJECT_SOURCE_DIR}/cmake/metal_shaders.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/metal_shaders.hpp" @ONLY)

add_library(nijiexpose_metal STATIC "${metal_dir}/context.mm" "${metal_dir}/backend.mm")
target_include_directories(nijiexpose_metal PUBLIC src "${nicxlive_SOURCE_DIR}"
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_options(nijiexpose_metal PRIVATE -fobjc-arc)
target_link_libraries(nijiexpose_metal PUBLIC nicxlive::nicxlive nijiexpose_imgui SDL3::SDL3
    "-framework Metal" "-framework QuartzCore" "-framework Cocoa")
