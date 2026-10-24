# OpenGL-only model renderer and sprite shader assets.
add_library(nijiexpose_gl_resources STATIC
    port/nijilive/opengl/api.cpp port/nijimi/opengl/texture.cpp
    port/nijimi/opengl/shader.cpp port/nijimi/opengl/blend.cpp
    port/nijimi/opengl/drawable.cpp port/nijimi/opengl/runtime.cpp
    port/nijilive/opengl/debug.cpp port/nijimi/opengl/part.cpp
    port/nijimi/opengl/blending.cpp port/nijimi/opengl/mask.cpp
    port/nijimi/opengl/dynamic_composite.cpp port/nijilive/opengl/draw_texture.cpp
    port/nijimi/opengl/opengl_thumb.cpp port/nijimi/opengl/probes.cpp port/nijimi/opengl/present.cpp port/nijimi/opengl/backend.cpp port/nijilive/opengl/difference.cpp)
target_include_directories(nijiexpose_gl_resources PUBLIC port src "${nicxlive_SOURCE_DIR}")
target_include_directories(nijiexpose_gl_resources PRIVATE port/nijilive/opengl)
target_link_libraries(nijiexpose_gl_resources PUBLIC nicxlive::nicxlive SDL3::SDL3 OpenGL::GL)
if(MSVC)
    target_compile_options(nijiexpose_gl_resources PRIVATE /utf-8 /W4 /fp:strict)
else()
    target_compile_options(nijiexpose_gl_resources PRIVATE -Wall -Wextra -Wpedantic)
endif()
file(READ "${PROJECT_SOURCE_DIR}/native/res/shaders/batch.vert" native_batch_vertex)
file(READ "${PROJECT_SOURCE_DIR}/native/res/shaders/batch.frag" native_batch_fragment)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/spritebatch_assets.hpp"
    "#pragma once\n#include \"nijilive/opengl/shader.hpp\"\nnamespace nijiexpose { inline const ogl::ShaderAsset SpriteBatchShaderSource{R\"NIJIGLSL(${native_batch_vertex})NIJIGLSL\",R\"NIJIGLSL(${native_batch_fragment})NIJIGLSL\"}; }\n")
