#version 450
layout(set = 0, binding = 0) uniform sampler2D albedoTex;
layout(set = 0, binding = 1) uniform sampler2D emissiveTex;
layout(set = 0, binding = 2) uniform sampler2D bumpTex;
layout(push_constant) uniform Push {
    vec4 tintOpacity;     // rgb=tint, a=opacity
    vec4 screenEmission;  // rgb=screen, a=emission
} pc;
layout(location = 0) in vec2 outUv;
layout(location = 0) out vec4 outColor;
void main() {
    vec4 albedo = texture(albedoTex, outUv);
    vec4 emissive = texture(emissiveTex, outUv);
    if (albedo.a <= 0.0001) {
        discard;
    }
    vec3 base = albedo.rgb * pc.tintOpacity.rgb;
    vec3 screen = vec3(1.0) - ((vec3(1.0)-base) * (vec3(1.0)-pc.screenEmission.rgb*albedo.a));
    vec3 lit = screen + emissive.rgb * max(pc.screenEmission.a, 0.0) * albedo.a;
    outColor = vec4(clamp(lit, 0.0, 1.0), albedo.a * pc.tintOpacity.a);
}
