// Shader sources copied verbatim from nijimi/source/render/opengl/shaders.
#pragma once
#include "shader.hpp"
namespace nijiexpose::ogl {
inline const ShaderAsset MaskShaderSource{R"NIJIGLSL(/*
    Copyright © 2020, nijigenerate Project
    Distributed under the 2-Clause BSD License, see LICENSE file.
    
    Authors: Luna Nielsen
*/
#version 330
uniform mat4 mvp;
uniform vec2 offset;
layout(location = 0) in float vertX;
layout(location = 1) in float vertY;
layout(location = 2) in float deformX;
layout(location = 3) in float deformY;

out vec2 texUVs;

void main() {
    vec2 verts = vec2(vertX, vertY);
    vec2 deform = vec2(deformX, deformY);
    gl_Position = mvp * vec4(verts.x - offset.x + deform.x, verts.y - offset.y + deform.y, 0, 1);
}
)NIJIGLSL", R"NIJIGLSL(/*
    Copyright © 2020, nijigenerate Project
    Distributed under the 2-Clause BSD License, see LICENSE file.
    
    Authors: Luna Nielsen
*/
#version 330
out vec4 outColor;

void main() {
    outColor = vec4(0, 0, 0, 1);
})NIJIGLSL"};
}
