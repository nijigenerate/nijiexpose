// nijimi RenderingBackend begin/endDynamicComposite, adapted to native surface lifetime.
#include "dynamic_composite.hpp"
#include "runtime.hpp"
#include <core/runtime_state.hpp>
#include <map>
namespace nijiexpose::ogl {
using namespace nicxlive::core;
namespace {
GLuint textureId(const std::shared_ptr<Texture>& texture) {
    if (!texture) return 0;
    auto handle = texture->backendId();
    if (handle == 0) return 0;
    return handle;
}
// nijimi's FBO cache is keyed by texture handles and invalidated on texture disposal.
using FramebufferKey = std::array<size_t, 5>;
std::map<FramebufferKey, GLuint> framebufferCache;
GLuint acquireFramebuffer(const DynamicCompositeSurface& surface) {
    FramebufferKey key{};
    for (size_t i=0; i<surface.textureCount; ++i) key[i]=textureId(surface.textures[i]);
    key[3]=surface.textureCount;
    key[4]=textureId(surface.stencil);
    auto found=framebufferCache.find(key);
    if (found!=framebufferCache.end()) return found->second;
    GLuint fbo{};
    glGenFramebuffers(1,&fbo);
    framebufferCache.emplace(key,fbo);
    return fbo;
}
}
void oglBeginDynamicComposite(DynamicCompositePass& pass) {
    auto surface = pass.surface;
    if (!surface) return;
    if (surface->textureCount == 0) return;
    auto tex = surface->textures[0];
    if (!tex) return;
    if (surface->framebuffer == 0) {
        surface->framebuffer = acquireFramebuffer(*surface);
    }
    GLint previousReadFramebuffer;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);
    GLint previousFramebuffer;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousFramebuffer);
    pass.origBuffer = static_cast<GLuint>(previousFramebuffer);
    glGetIntegerv(GL_VIEWPORT, pass.origViewport);
    glBindFramebuffer(GL_FRAMEBUFFER, surface->framebuffer);
    std::array<GLuint, 3> drawBuffers{};
    size_t bufferCount = 0;
    for (size_t i = 0; i < surface->textureCount; ++i) {
        auto attachment = GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i);
        auto attachmentTexture = surface->textures.at(i);
        if (attachmentTexture) {
            glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, textureId(attachmentTexture), 0);
            drawBuffers.at(bufferCount++) = attachment;
        } else {
            glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, 0, 0);
        }
    }
    if (bufferCount == 0) drawBuffers.at(bufferCount++) = GL_COLOR_ATTACHMENT0;
    if (surface->stencil) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, textureId(surface->stencil), 0);
        glClear(GL_STENCIL_BUFFER_BIT);
    } else {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
    }
    inPushViewport(tex->width(), tex->height());
    glDrawBuffers(static_cast<int>(bufferCount), drawBuffers.data());
    glViewport(0, 0, tex->width(), tex->height());
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE0);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}
void oglEndDynamicComposite(DynamicCompositePass& pass) {
    if (!pass.surface) return;
    oglRebindActiveTargets();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pass.origBuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, pass.origBuffer);
    inPopViewport();
    glViewport(pass.origViewport[0], pass.origViewport[1], pass.origViewport[2], pass.origViewport[3]);
    if (pass.origBuffer != 0) {
        glDrawBuffers(3, std::array<GLenum,3>{GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1,GL_COLOR_ATTACHMENT2}.data());
    } else {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glReadBuffer(GL_BACK);
        glDrawBuffer(GL_BACK);
    }
}
void oglDestroyDynamicComposite(const std::shared_ptr<DynamicCompositeSurface>& surface) {
    if (!surface) return;
    // The cache owns the GL object until its textures are released.
    surface->framebuffer = 0;
}
void oglReleaseDynamicFramebuffersForTexture(GLuint handle) {
    if (!handle) return;
    for (auto it=framebufferCache.begin(); it!=framebufferCache.end();) {
        const auto& key=it->first;
        bool uses=key[4]==handle;
        for(size_t i=0; i<key[3]; ++i) uses=uses || key[i]==handle;
        if (uses) {
            if(it->second) glDeleteFramebuffers(1,&it->second);
            it=framebufferCache.erase(it);
        } else ++it;
    }
}
}
