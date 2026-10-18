#pragma once
#include "shader.hpp"
namespace nijiexpose::ogl {
inline const ShaderAsset SceneShaderSource{R"NIJIGLSL(/*
    Copyright © 2020, nijigenerate Project
    Distributed under the 2-Clause BSD License, see LICENSE file.
    
    Authors: Luna Nielsen
*/
#version 330
uniform mat4 mvp;
layout(location = 0) in vec2 verts;
layout(location = 1) in vec2 uvs;

out vec2 texUVs;

void main() {
    gl_Position = mvp * vec4(verts.x, verts.y, 0, 1);
    texUVs = uvs;
})NIJIGLSL", R"NIJIGLSL(/*
    Copyright © 2020, nijigenerate Project
    Distributed under the 2-Clause BSD License, see LICENSE file.
    
    Authors: Luna Nielsen
*/
#version 330
in vec2 texUVs;
out vec4 outColor;

uniform sampler2D fbo;

void main() {
    // Set color to the corrosponding pixel in the FBO
    vec4 color = texture(fbo, texUVs);
    outColor = vec4(color.r, color.g, color.b, color.a);
})NIJIGLSL"};
inline const ShaderAsset LightingShaderSource{R"NIJIGLSL(/*
    Copyright © 2020, nijigenerate Project
    Distributed under the 2-Clause BSD License, see LICENSE file.
    
    Authors: Luna Nielsen
*/
#version 330
uniform mat4 mvp;
layout(location = 0) in vec2 verts;
layout(location = 1) in vec2 uvs;

out vec2 texUVs;

void main() {
    gl_Position = mvp * vec4(verts.x, verts.y, 0, 1);
    texUVs = uvs;
})NIJIGLSL", R"NIJIGLSL(/*
    Copyright © 2020, nijigenerate Project
    Distributed under the 2-Clause BSD License, see LICENSE file.
    
    Authors: Luna Nielsen
*/
#version 330
in vec2 texUVs;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outEmissive;
layout(location = 2) out vec4 outBump;

uniform vec3 ambientLight;
uniform vec2 fbSize;

uniform sampler2D albedo;
uniform sampler2D emissive;
uniform sampler2D bumpmap;
uniform int LOD = 2;
uniform int samples = 25;

// Gaussian
float gaussian(vec2 i, float sigma) {
    return exp(-0.5*dot(i /= sigma, i)) / (6.28*sigma*sigma);
}

// Bloom texture by blurring it
vec4 bloom(sampler2D sp, vec2 uv, vec2 scale) {
    float sigma = float(samples) * 0.25;
    vec4 out_ = vec4(0);
    int sLOD = 1 << LOD;
    int s = samples/sLOD;
    
    for ( int i = 0; i < s*s; i++ ) {
        vec2 d = vec2(i%s, i/s)*float(sLOD) - float(samples)/2.0;
        out_ += gaussian(d, sigma) * textureLod( sp, uv + scale * d, LOD);
    }
    
    return out_ / out_.a;
}

void main() {

    // Bloom
    outEmissive = texture(emissive, texUVs)+bloom(emissive, texUVs, 1.0/fbSize);

    // Set color to the corrosponding pixel in the FBO
    vec4 light = vec4(ambientLight, 1) + outEmissive;

    outAlbedo = (texture(albedo, texUVs)*light);
    outBump = texture(bumpmap, texUVs);
})NIJIGLSL"};
inline const ShaderAsset LineShaderSource{R"NIJIGLSL(/*
    Copyright © 2020, nijigenerate Project
    Distributed under the 2-Clause BSD License, see LICENSE file.
    
    Authors: Luna Nielsen
*/
#version 330
uniform mat4 mvp;
layout(location = 0) in float vertX;
layout(location = 1) in float vertY;
layout(location = 2) in float vertZ;

out vec2 texUVs;

void main() {
    vec3 verts = vec3(vertX, vertY, vertZ);
    gl_Position = mvp * vec4(verts, 1.0);
}
)NIJIGLSL", R"NIJIGLSL(/*
    Copyright © 2020, nijigenerate Project
    Distributed under the 2-Clause BSD License, see LICENSE file.
    
    Authors: Luna Nielsen
*/
#version 330
layout(location = 0) out vec4 outColor;

uniform vec4 color;

void main() {
    outColor = color;
})NIJIGLSL"};
inline const ShaderAsset PointShaderSource{R"NIJIGLSL(/*
    Copyright © 2020, nijigenerate Project
    Distributed under the 2-Clause BSD License, see LICENSE file.
    
    Authors: Luna Nielsen
*/
#version 330
uniform mat4 mvp;
layout(location = 0) in float vertX;
layout(location = 1) in float vertY;
layout(location = 2) in float vertZ;

out vec2 texUVs;

void main() {
    vec3 verts = vec3(vertX, vertY, vertZ);
    gl_Position = mvp * vec4(verts, 1.0);
}
)NIJIGLSL", R"NIJIGLSL(/*
    Copyright © 2020, nijigenerate Project
    Distributed under the 2-Clause BSD License, see LICENSE file.
    
    Authors: Luna Nielsen
*/
#version 330
layout(location = 0) out vec4 outColor;

uniform vec4 color;

void main() {
    float r = 0.0; // radius
    float alpha = 1.0; // alpha

    // r = point in circle compared against circle raidus
    vec2 cxy = 2.0 * gl_PointCoord - 1.0;
    r = dot(cxy, cxy);

    // epsilon width
    float epsilon = fwidth(r)*0.5;

    // apply delta
    alpha = 1.0 - smoothstep(1.0 - epsilon, 1.0 + epsilon, r);
    outColor = color * alpha;
})NIJIGLSL"};
}
