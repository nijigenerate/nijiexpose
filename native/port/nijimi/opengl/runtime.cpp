// Nijimi renderer lifecycle with nijiexpose scene/lighting/readback adapters.
// Lifecycle translated from nijimi/source/render/opengl/opengl_backend.d.
#include "runtime.hpp"
#include "dynamic_composite.hpp"
#include "debug.hpp"
#include "blending.hpp"
#include "assets.hpp"
#include <core/runtime_state.hpp>
#include <array>
#include <algorithm>
#include <unordered_map>
namespace nijiexpose::ogl {
using nicxlive::core::inSetViewport;
using nicxlive::core::inGetClearColor;
using std::max;

std::shared_ptr<nicxlive::core::Texture> boundAlbedo;
static int viewportWidth() { int w,h; nicxlive::core::inGetViewport(w,h); return w; }
static int viewportHeight() { int w,h; nicxlive::core::inGetViewport(w,h); return h; }
static vec4 clearColor() { vec4 c; inGetClearColor(c.x,c.y,c.z,c.w); return c; }
struct PostProcessingShader {
    PostProcessingShader() = default;

    std::unordered_map<std::string, GLint> uniformCache;


    Shader shader;
    PostProcessingShader(Shader shader) {
        this->shader = shader;

        shader.use();
        shader.setUniform(shader.getUniformLocation("albedo"), 0);
        shader.setUniform(shader.getUniformLocation("emissive"), 1);
        shader.setUniform(shader.getUniformLocation("bumpmap"), 2);
    }

    /**
        Gets the location of the specified uniform
    */
    GLuint getUniform(const std::string& name) {
        if (this->hasUniform(name)) return uniformCache[name];
        GLint element = shader.getUniformLocation(name);
        uniformCache[name] = element;
        return element;
    }

    /**
        Returns true if the uniform is present in the shader cache 
    */
    bool hasUniform(const std::string& name) {
        return uniformCache.contains(name);
    }
};

// Internal rendering constants
namespace {
    thread_local GLuint sceneVAO;
    thread_local GLuint sceneVBO;

    thread_local GLuint fBuffer;
    thread_local GLuint fAlbedo;
    thread_local GLuint fEmissive;
    thread_local GLuint fBump;
    thread_local GLuint fStencil;

    thread_local GLuint cfBuffer;
    thread_local GLuint cfAlbedo;
    thread_local GLuint cfEmissive;
    thread_local GLuint cfBump;
    thread_local GLuint cfStencil;

    thread_local GLuint blendFBO;
    thread_local GLuint blendAlbedo;
    thread_local GLuint blendEmissive;
    thread_local GLuint blendBump;
    thread_local GLuint blendStencil;

    thread_local PostProcessingShader basicSceneShader;
    thread_local PostProcessingShader basicSceneLighting;
    thread_local std::vector<PostProcessingShader> postProcessingStack;
    
    

    thread_local bool isCompositing;
    struct CompositeFrameState {
        GLint framebuffer;
        std::array<GLint, 4> viewport;
    };
    thread_local std::vector<CompositeFrameState> compositeScopeStack;

    void renderScene(vec4 area, PostProcessingShader shaderToUse, GLuint albedo, GLuint emissive, GLuint bump) {
        glViewport(0, 0, static_cast<int>(area.z), static_cast<int>(area.w));

        // Bind our vertex array
        glBindVertexArray(sceneVAO);
        
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        shaderToUse.shader.use();
        shaderToUse.shader.setUniform(shaderToUse.getUniform("mvp"), 
            mat4::orthographic(0, area.z, area.w, 0, 0, max(area.z, area.w)) * 
            mat4::translation(area.x, area.y, 0)
        );

        // Ambient light
        GLint ambientLightUniform = shaderToUse.getUniform("ambientLight");
        if (ambientLightUniform != -1) shaderToUse.shader.setUniform(ambientLightUniform, inSceneAmbientLight);

        // framebuffer size
        GLint fbSizeUniform = shaderToUse.getUniform("fbSize");
        if (fbSizeUniform != -1) shaderToUse.shader.setUniform(fbSizeUniform, vec2{float(viewportWidth()), float(viewportHeight())});

        // Bind the texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, albedo);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, emissive);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, bump);

        // Enable points array
        glEnableVertexAttribArray(0); // verts
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), nullptr);

        // Enable UVs array
        glEnableVertexAttribArray(1); // uvs
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), reinterpret_cast<float*>(2*sizeof(float)));

        // Draw
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Disable the vertex attribs after use
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);

        glDisable(GL_BLEND);
    }
}

// Things only available internally for nijilive rendering

    
    /**
        Initializes the renderer (OpenGL-specific portion)
    */
    void oglInitRenderer() {
        // Set a default logical viewport before first explicit size arrives.
        inSetViewport(640, 480);
        oglInitDebugRenderer();
        basicSceneShader=PostProcessingShader(Shader(SceneShaderSource));

        glGenVertexArrays(1, &sceneVAO);
        glGenBuffers(1, &sceneVBO);

        // Generate the framebuffer we'll be using to render the model and composites
        glGenFramebuffers(1, &fBuffer);
        glGenFramebuffers(1, &cfBuffer);

        // Generate the color and stencil-depth textures needed
        // Note: we're not using the depth buffer but OpenGL 3.4 does not support stencil-only buffers
        glGenTextures(1, &fAlbedo);
        glGenTextures(1, &fEmissive);
        glGenTextures(1, &fBump);
        glGenTextures(1, &fStencil);

        glGenTextures(1, &cfAlbedo);
        glGenTextures(1, &cfEmissive);
        glGenTextures(1, &cfBump);
        glGenTextures(1, &cfStencil);

        // Attach textures to framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, fBuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fAlbedo, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, fEmissive, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, fBump, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, fStencil, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, cfBuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cfAlbedo, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, cfEmissive, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, cfBump, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, cfStencil, 0);

        // go back to default fb
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        nlApplyBlendingCapabilities();
    }

/**
    Begins rendering to the framebuffer
*/
void oglBeginScene() {
    oglCollectDynamicFramebuffers();
        while (glGetError() != GL_NO_ERROR) {}

        boundAlbedo.reset();
        glBindVertexArray(sceneVAO);
        glEnable(GL_BLEND);
        glEnablei(GL_BLEND, 0);
        glEnablei(GL_BLEND, 1);
        glEnablei(GL_BLEND, 2);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        int viewportWidth;
        int viewportHeight;
        nicxlive::core::inGetViewport(viewportWidth, viewportHeight);
        glViewport(0, 0, viewportWidth, viewportHeight);

        glBindFramebuffer(GL_FRAMEBUFFER, fBuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fAlbedo, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, fEmissive, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, fBump, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, fStencil, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, cfBuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cfAlbedo, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, cfEmissive, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, cfBump, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, cfStencil, 0);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, cfBuffer);
        std::array<GLenum,3> cfTargets{GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1,GL_COLOR_ATTACHMENT2};
        glDrawBuffers(static_cast<GLsizei>(cfTargets.size()), cfTargets.data());
        glClearColor(0, 0, 0, 0);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fBuffer);

        glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
        glGetError();

        glDrawBuffers(1, std::array<GLenum, 1>{GL_COLOR_ATTACHMENT0}.data());
        glClearColor(clearColor().x, clearColor().y, clearColor().z, clearColor().w);
        glClear(GL_COLOR_BUFFER_BIT);

        glDrawBuffers(2, std::array<GLenum, 2>{GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2}.data());
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glDrawBuffers(3, std::array<GLenum, 3>{GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2}.data());
    }

/**
    Begins a composition step
*/
// Legacy composite entry points (dynamic composite handles the current path).
// Retained as no-op to avoid GL state churn/confusion.
void oglBeginComposite() {}

/**
    Ends a composition step, re-binding the internal framebuffer
*/
void oglEndComposite() {}
/**
    Ends rendering to the framebuffer
*/
void oglEndScene() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glDisablei(GL_BLEND, 0);
        glDisablei(GL_BLEND, 1);
        glDisablei(GL_BLEND, 2);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glFlush();
        glDrawBuffers(1, std::array<GLenum, 1>{GL_COLOR_ATTACHMENT0}.data());
    }

/**
    Runs post processing on the scene
*/
void oglPostProcessScene() {
        if (postProcessingStack.size() == 0) return;

        bool targetBuffer=false;
        auto color = clearColor();

        int viewportWidth;
        int viewportHeight;
        nicxlive::core::inGetViewport(viewportWidth, viewportHeight);

        vec4 area = vec4(0, 0, float(viewportWidth), float(viewportHeight));

        std::array<float,24> data = {
            area.x,         area.y + area.w,          0, 0,
            area.x,         area.y,                   0, 1,
            area.x + area.z,area.y + area.w,          1, 0,

            area.x + area.z,area.y + area.w,          1, 0,
            area.x,         area.y,                   0, 1,
            area.x + area.z,area.y,                   1, 1,
        };
        glBindBuffer(GL_ARRAY_BUFFER, sceneVBO);
        glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(float), data.data(), GL_DYNAMIC_DRAW);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, fEmissive);
        glGenerateMipmap(GL_TEXTURE_2D);

        glBindFramebuffer(GL_FRAMEBUFFER, cfBuffer);
        glDrawBuffers(3, std::array<GLenum, 3>{GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2}.data());
        glClearColor(color.x, color.y, color.z, color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindFramebuffer(GL_FRAMEBUFFER, fBuffer);
        glDrawBuffers(3, std::array<GLenum, 3>{GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2}.data());

        for(auto shader:postProcessingStack) {
            targetBuffer = !targetBuffer;
            if (targetBuffer) {
                glBindFramebuffer(GL_FRAMEBUFFER, cfBuffer);
                renderScene(area, shader, fAlbedo, fEmissive, fBump);
            } else {
                glBindFramebuffer(GL_FRAMEBUFFER, fBuffer);
                renderScene(area, shader, cfAlbedo, cfEmissive, cfBump);
            }
        }

        if (targetBuffer) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, cfBuffer);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fBuffer);
            glBlitFramebuffer(
                0, 0, viewportWidth, viewportHeight,
                0, 0, viewportWidth, viewportHeight,
                GL_COLOR_BUFFER_BIT,
                GL_LINEAR
            );
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

/**
    Add basic lighting shader to processing stack
*/
void oglAddBasicLightingPostProcess() {
    postProcessingStack.push_back(PostProcessingShader(Shader(LightingShaderSource)));
}

/**
    Clears the post processing stack
*/
std::vector<PostProcessingShader>& oglGetPostProcessingStack() {
    return postProcessingStack;
}

/**
    Draw scene to area
*/
void oglDrawScene(vec4 area) {
    std::array<float, 24> data = {
        area.x,         area.y+area.w,          0, 0,
        area.x,         area.y,                 0, 1,
        area.x+area.z,  area.y+area.w,          1, 0,
        
        area.x+area.z,  area.y+area.w,          1, 0,
        area.x,         area.y,                 0, 1,
        area.x+area.z,  area.y,                 1, 1,
    };

    glBindBuffer(GL_ARRAY_BUFFER, sceneVBO);
    glBufferData(GL_ARRAY_BUFFER, 24*sizeof(float), data.data(), GL_DYNAMIC_DRAW);
    renderScene(area, basicSceneShader, fAlbedo, fEmissive, fBump);
}

void oglPrepareCompositeScene() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cfAlbedo);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, cfEmissive);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, cfBump);
}

/**
    Gets the nijilive framebuffer 

    DO NOT MODIFY THIS IMAGE!
*/
GLuint oglGetFramebuffer() {
    return fBuffer;
}

/**
    Gets the nijilive framebuffer render image

    DO NOT MODIFY THIS IMAGE!
*/
GLuint oglGetRenderImage() {
    return fAlbedo;
}

/**
    Gets the nijilive composite render image

    DO NOT MODIFY THIS IMAGE!
*/
GLuint oglGetCompositeImage() {
    return cfAlbedo;
}

 GLuint oglGetCompositeFramebuffer() {
    return cfBuffer;
}

 GLuint oglGetBlendFramebuffer() {
    return blendFBO;
}

 GLuint oglGetMainEmissive() {
    return fEmissive;
}

 GLuint oglGetMainBump() {
    return fBump;
}

 GLuint oglGetCompositeEmissive() {
    return cfEmissive;
}

 GLuint oglGetCompositeBump() {
    return cfBump;
}

 GLuint oglGetBlendAlbedo() {
    return blendAlbedo;
}

 GLuint oglGetBlendEmissive() {
    return blendEmissive;
}

 GLuint oglGetBlendBump() {
    return blendBump;
}

// Reattach the currently active main/composite textures to their FBOs.
 void oglRebindActiveTargets() {
    GLint prev;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev);

    glBindFramebuffer(GL_FRAMEBUFFER, fBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fAlbedo, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, fEmissive, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, fBump, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, fStencil, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, cfBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cfAlbedo, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, cfEmissive, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, cfBump, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, cfStencil, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, prev);
}

/**
    Gets the nijilive main albedo render image

    DO NOT MODIFY THIS IMAGE!
*/
GLuint oglGetMainAlbedo() {
    return fAlbedo;
}

/**
    Gets the blend shader for the specified mode
*/
 void oglSwapMainCompositeBuffers() {
    // No-op swap: we avoid ping-pong to keep attachments stable.
}

void oglResizeViewport(int width, int height) {
        // Work on texture unit 0 to avoid "no texture bound" errors.
        glActiveTexture(GL_TEXTURE0);
        // Render Framebuffer
        glBindTexture(GL_TEXTURE_2D, fAlbedo);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, fEmissive);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, fBump);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, fStencil);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);

        glBindFramebuffer(GL_FRAMEBUFFER, fBuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fAlbedo, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, fEmissive, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, fBump, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, fStencil, 0);

        // Composite framebuffer
        glBindTexture(GL_TEXTURE_2D, cfAlbedo);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, cfEmissive);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, cfBump);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, cfStencil);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);

        glBindFramebuffer(GL_FRAMEBUFFER, cfBuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cfAlbedo, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, cfEmissive, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, cfBump, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, cfStencil, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glViewport(0, 0, width, height);
    }

/**
    Dumps viewport data to texture stream
*/

void oglDumpViewport(std::vector<uint8_t>& dumpTo, int width, int height) {
     {
        if (width == 0 || height == 0) return;
        glBindTexture(GL_TEXTURE_2D, fAlbedo);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, dumpTo.data());
    }
}

}
