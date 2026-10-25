#pragma once
#include "shader.hpp"
#include "render/settings.hpp"
#include <core/texture.hpp>
#include <vector>
namespace nijiexpose::ogl {
using render::inSceneAmbientLight;
extern std::shared_ptr<nicxlive::core::Texture> boundAlbedo;
void oglInitRenderer();
void oglBeginScene();
void oglEndScene();
void oglPostProcessScene();
void oglAddBasicLightingPostProcess();
void oglDrawScene(vec4 area);
void oglPrepareCompositeScene();
GLuint oglGetFramebuffer();
GLuint oglGetRenderImage();
GLuint oglGetCompositeImage();
GLuint oglGetCompositeFramebuffer();
GLuint oglGetBlendFramebuffer();
GLuint oglGetMainAlbedo();
GLuint oglGetMainEmissive();
GLuint oglGetMainBump();
GLuint oglGetCompositeEmissive();
GLuint oglGetCompositeBump();
GLuint oglGetBlendAlbedo();
GLuint oglGetBlendEmissive();
GLuint oglGetBlendBump();
void oglRebindActiveTargets();
void oglSwapMainCompositeBuffers();
void oglResizeViewport(int width, int height);
void oglDumpViewport(std::vector<uint8_t>& dumpTo, int width, int height);
}
