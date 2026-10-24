#include "version.hpp"
#include "nijimi/opengl/present.hpp"
// Direct translation of dlang_archive/source/nijiexpose/windows/main.d (Windows).
#include "main.hpp"
#include "control.hpp"
#include "settings.hpp"
#include "spaceedit.hpp"
#include "utils.hpp"
#include "scene/package.hpp"
#include "framesend.hpp"
#include "io/package.hpp"
#include "io/image.hpp"
#include "panels/scene.hpp"
#include "nijilive/model_io.hpp"
#include "nijilive/opengl/backend.hpp"
#include "render/renderer.hpp"
#include "nijilive/opengl/runtime.hpp"
#include "nijiui/core/core.hpp"
#include "nijiui/core/resource.hpp"
#include "nijiui/core/settings.hpp"
#include "nijiui/widgets/input.hpp"
#include "nijiui/widgets/dialog.hpp"
#include "nijiui/panel.hpp"
#include "i18n/tr.hpp"
#include <core/runtime_state.hpp>
#include <core/timing.hpp>
#include <imgui_internal.h>
#include <filesystem>
#include <functional>
#include "render/window_mode.hpp"
#include <optional>
#include <deque>
namespace nijiexpose::windows {
namespace core=nicxlive::core;
using namespace nijiui; using namespace scene; using namespace io; using panels::ScenePanel;
using i18n::_; using i18n::__;
using ogl::oglDrawScene;
namespace { struct ScopeExit {std::function<void()> fn;~ScopeExit(){fn();}};
    enum class NavSurfaceMode {
        DesktopRail,
        CompactBar,
    };

    enum class ActivePanelId {
        Parameters,
        Tracking,
        View,
        Animations,
        Blendshapes,
        Plugins,
        Language,
    };

    struct NavItem {
        ActivePanelId id;
        std::string panelName;
        std::string icon;
        std::string label;
    };

    const NavItem NAV_ITEMS[] = {
        NavItem(ActivePanelId::Parameters, "Tracking", "\ue429", "Parameters"),
        NavItem(ActivePanelId::Tracking, "", "\ue3b4", "Tracking"),
        NavItem(ActivePanelId::View, "Scene Settings", "\ue8f4", "View"),
        NavItem(ActivePanelId::Animations, "Animations", "\ue037", "Animations"),
        NavItem(ActivePanelId::Blendshapes, "Blendshapes", "\ue3f4", "Blends"),
        NavItem(ActivePanelId::Plugins, "Plugins", "\ue87b", "Plugins"),
        NavItem(ActivePanelId::Language, "", "\ue8e2", "Language"),
    };

    const vec4 RAIL_BG = vec4(0.97f, 0.98f, 0.99f, 0.84f);
    const vec4 RAIL_BORDER = vec4(0.12f, 0.16f, 0.23f, 0.10f);
    const vec4 OVERLAY_BG = vec4(0.97f, 0.98f, 0.99f, 0.72f);
    const vec4 OVERLAY_BORDER = vec4(0.12f, 0.16f, 0.23f, 0.10f);
    const vec4 ACCENT = vec4(0.70f, 0.25f, 0.00f, 1.00f);
    const vec4 ACCENT_SOFT = vec4(0.70f, 0.25f, 0.00f, 0.14f);
    const vec4 SHADOW_NEAR = vec4(0.12f, 0.16f, 0.23f, 0.10f);
    const vec4 SHADOW_FAR = vec4(0.12f, 0.16f, 0.23f, 0.03f);
    constexpr float RAIL_COLLAPSED_WIDTH = 62.0f;
    constexpr float RAIL_EXPANDED_WIDTH = 210.0f;
    constexpr float OUTER_GAP = 18.0f;
    constexpr float RAIL_TOP = 18.0f;
    constexpr float RAIL_BOTTOM = 18.0f;
    constexpr float NAV_ICON_SCALE = 1.18f;
    constexpr float NAV_LABEL_SCALE = 1.08f;
    constexpr float NAV_CLOSE_SCALE = 1.10f;

    struct InochiWindowSettings {
        int width;
        int height;
    };

    struct PuppetSavedData {
        float scale;
    };

}
class nijiexposeWindow : public InApplicationWindow {
private:
    SDL_Tray* tray=nullptr;
    struct TrayAction { nijiexposeWindow* owner; int command; };
    std::deque<TrayAction> trayActions;
    std::deque<int> pendingTrayCommands;
    std::optional<bool> pendingTransparent;
    bool transparentMode=false;
    void openModels() {
        const std::array<TFD_Filter,1> filters{{{{"*.inp","*.inx"},"Puppet Files (*.inp, *.inx)"}}};
        auto file=insShowOpenDialog(filters,_("Open..."),nativeWindow());
        if(!file.empty())loadModels({file});
    }
    void destroyTray() {
        if(tray)SDL_DestroyTray(tray);
        tray=nullptr;trayActions.clear();pendingTrayCommands.clear();
    }
    void createTray() {
        tray=SDL_CreateTray(nativeIcon(),"nijiexpose");
        if(!tray)throw std::runtime_error(SDL_GetError());
        auto* menu=SDL_CreateTrayMenu(tray);
        if(!menu){destroyTray();throw std::runtime_error(SDL_GetError());}
        auto add=[&](const std::string& label,int command) {
            auto translated=_(label);
            auto* entry=SDL_InsertTrayEntryAt(menu,-1,translated.c_str(),SDL_TRAYENTRY_BUTTON);
            if(!entry)throw std::runtime_error(SDL_GetError());
            trayActions.push_back({this,command});
            SDL_SetTrayEntryCallback(entry,[](void* data,SDL_TrayEntry*) {
                auto& action=*static_cast<TrayAction*>(data);
                action.owner->pendingTrayCommands.push_back(action.command);
            },&trayActions.back());
        };
        try {
            add("Menu",100);add("Models",101);
            for(const auto& item:NAV_ITEMS)add(item.label,static_cast<int>(item.id));
            add("Delete selected item",102);
            add("Exit transparent mode",103);add("Quit",104);
        } catch(...) {destroyTray();throw;}
    }
    void applyTransparentMode(bool enabled) {
        if(enabled==transparentMode)return;
        if(enabled)createTray();
        try {setBorderlessTransparency(enabled);}
        catch(...) {if(enabled)destroyTray();throw;}
        transparentMode=enabled;
        overlayOpen=false;
        showUI=true;
        if(!enabled)destroyTray();
    }
    std::shared_ptr<ft::Adaptor> adaptor;
    std::shared_ptr<ApplicationRenderBackend> backend;
    std::shared_ptr<core::Texture> logo;
    std::shared_ptr<SettingWindow> settingWindow;
    std::shared_ptr<SpaceEditor> spaceEditor;
    GLuint overlayBlurTextureA = 0;
    GLuint overlayBlurTextureB = 0;
    GLuint overlayBlurFboA = 0;
    GLuint overlayBlurFboB = 0;
    GLuint overlayBlurProgram = 0;
    GLuint overlayBlurVao = 0;
    GLint overlayBlurSourceLocation = -1;
    GLint overlayBlurTexelStepLocation = -1;
    int overlayBlurWidth = 0;
    int overlayBlurHeight = 0;
    ActivePanelId activePanel = ActivePanelId::Parameters;
    bool navExpanded = false;
    bool overlayOpen = true;
    bool navFaded = false;
    vec2 lastPointerPos;
    double lastNavInteractionAt = 0;

    ActivePanelId sanitizeActivePanel(int rawValue) {
        if (rawValue < static_cast<int>(ActivePanelId::Parameters) || rawValue > static_cast<int>(ActivePanelId::Language)) {
            return ActivePanelId::Parameters;
        }
        return static_cast<ActivePanelId>(rawValue);
    }

    std::shared_ptr<Panel> panelFor(ActivePanelId id) {
        for (const auto& item : NAV_ITEMS) {
            if (item.id != id) continue;
            if (item.panelName.empty()) return nullptr;
            for (auto& panel : inPanels) {
                if (panel->name() == item.panelName) return panel;
            }
        }
        return nullptr;
    }

    std::shared_ptr<ToolWindow> toolWindowFor(ActivePanelId id) {
        switch(id) {
        case ActivePanelId::Parameters:
        case ActivePanelId::Tracking:
        case ActivePanelId::View:
        case ActivePanelId::Animations:
        case ActivePanelId::Blendshapes:
        case ActivePanelId::Plugins:
        case ActivePanelId::Language:
            return nullptr;
        }
    }

    NavSurfaceMode navSurfaceMode() {
        if (width() <= 1100 || height() <= 760) return NavSurfaceMode::CompactBar;
        return NavSurfaceMode::DesktopRail;
    }

    void touchNav() {
        lastNavInteractionAt = inGetTime();
        navFaded = false;
    }

    void updateNavFadeState() {
        vec2 mousePos = inInputMousePosition();
        bool pointerMoved = mousePos.x != lastPointerPos.x || mousePos.y != lastPointerPos.y;
        bool interacted = pointerMoved
            || inInputMouseClicked(MouseButton::Left)
            || inInputMouseClicked(MouseButton::Right)
            || inInputMouseClicked(MouseButton::Middle)
            || inInputMouseScrollDelta() != 0;

        if (interacted || overlayOpen || navExpanded) {
            touchNav();
        } else if ((inGetTime() - lastNavInteractionAt) > 2.6) {
            navFaded = true;
        }

        lastPointerPos = mousePos;
    }

    void syncPanelVisibility() {
        for (auto& panel : inPanels) {
            panel->visible = false;
        }
    }

    vec4 withAlpha(vec4 color, float alphaScale) {
        return vec4(color.x, color.y, color.z, color.w * alphaScale);
    }

    float navVisualAlpha() {
        return navFaded && !overlayOpen && !navExpanded ? 0.24f : 1.0f;
    }

    void togglePanel(ActivePanelId id) {
        if (overlayOpen && activePanel == id) {
            overlayOpen = false;
        } else {
            activePanel = id;
            overlayOpen = true;
        }
        inSettingsSet("ui.activePanel", static_cast<int>(activePanel));
        inSettingsSet("ui.overlayOpen", overlayOpen);
    }

    bool usesParameterOverlay(ActivePanelId id) {
        return id == ActivePanelId::Parameters;
    }

    std::string panelTitle(ActivePanelId id, std::shared_ptr<Panel> active, std::shared_ptr<ToolWindow> activeWindow) {
        switch (id) {
        case ActivePanelId::Parameters:
            return "Parameters";
        case ActivePanelId::Tracking:
            return "Tracking";
        case ActivePanelId::View:
            return "View";
        case ActivePanelId::Language:
            return "Language";
        case ActivePanelId::Animations:
        case ActivePanelId::Blendshapes:
        case ActivePanelId::Plugins:
            return active != nullptr ? active->displayName() : (activeWindow != nullptr ? activeWindow->name() : "");
        }
    }

#ifndef NIJIEXPOSE_VULKAN
    void destroyOverlayBlurResources() {
        if (overlayBlurProgram != 0) {
            ogl::glDeleteProgram(overlayBlurProgram);
            overlayBlurProgram = 0;
        }
        if (overlayBlurVao != 0) {
            ogl::glDeleteVertexArrays(1, &overlayBlurVao);
            overlayBlurVao = 0;
        }
        if (overlayBlurFboA != 0) {
            ogl::glDeleteFramebuffers(1, &overlayBlurFboA);
            overlayBlurFboA = 0;
        }
        if (overlayBlurFboB != 0) {
            ogl::glDeleteFramebuffers(1, &overlayBlurFboB);
            overlayBlurFboB = 0;
        }
        if (overlayBlurTextureA != 0) {
            ogl::glDeleteTextures(1, &overlayBlurTextureA);
            overlayBlurTextureA = 0;
        }
        if (overlayBlurTextureB != 0) {
            ogl::glDeleteTextures(1, &overlayBlurTextureB);
            overlayBlurTextureB = 0;
        }
        overlayBlurWidth = 0;
        overlayBlurHeight = 0;
        overlayBlurSourceLocation = -1;
        overlayBlurTexelStepLocation = -1;
    }

    GLuint compileOverlayBlurShader(GLenum stage, std::string source) {
        GLuint shader = ogl::glCreateShader(stage);
        auto ptr = source.c_str();
        ogl::glShaderSource(shader, 1, &ptr, nullptr);
        ogl::glCompileShader(shader);

        GLint status = GL_FALSE;
        ogl::glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (status == GL_TRUE) {
            return shader;
        }

        GLint length = 0;
        ogl::glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::string message = "unknown";
        if (length > 0) {
            std::string buffer(length,'\0');
            ogl::glGetShaderInfoLog(shader, length, nullptr, buffer.data());
            message = buffer;
        }

        ogl::glDeleteShader(shader);
        throw std::runtime_error("Failed to compile overlay blur shader: " + message);
    }

    GLuint linkOverlayBlurProgram(GLuint vertexShader, GLuint fragmentShader) {
        GLuint program = ogl::glCreateProgram();
        ogl::glAttachShader(program, vertexShader);
        ogl::glAttachShader(program, fragmentShader);
        ogl::glLinkProgram(program);

        GLint status = GL_FALSE;
        ogl::glGetProgramiv(program, GL_LINK_STATUS, &status);
        if (status == GL_TRUE) {
            return program;
        }

        GLint length = 0;
        ogl::glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        std::string message = "unknown";
        if (length > 0) {
            std::string buffer(length,'\0');
            ogl::glGetProgramInfoLog(program, length, nullptr, buffer.data());
            message = buffer;
        }

        ogl::glDeleteProgram(program);
        throw std::runtime_error("Failed to link overlay blur shader program: " + message);
    }

    void ensureOverlayBlurProgram() {
        if (overlayBlurProgram != 0 && overlayBlurVao != 0) return;

        const std::string vertSource = R"NIJIGLSL(
            #version 330 core
            out vec2 vUV;
            const vec2 POSITIONS[3] = vec2[](
                vec2(-1.0, -1.0),
                vec2( 3.0, -1.0),
                vec2(-1.0,  3.0)
            );
            const vec2 UVS[3] = vec2[](
                vec2(0.0, 0.0),
                vec2(2.0, 0.0),
                vec2(0.0, 2.0)
            );
            void main() {
                gl_Position = vec4(POSITIONS[gl_VertexID], 0.0, 1.0);
                vUV = UVS[gl_VertexID];
            }
        )NIJIGLSL";
        const std::string fragSource = R"NIJIGLSL(
            #version 330 core
            in vec2 vUV;
            out vec4 fragColor;
            uniform sampler2D uSource;
            uniform vec2 uTexelStep;
            vec4 sampleAt(vec2 uv) {
                return texture(uSource, clamp(uv, vec2(0.0), vec2(1.0)));
            }
            void main() {
                vec4 color = sampleAt(vUV) * 0.2270270270;
                color += sampleAt(vUV + uTexelStep * 1.3846153846) * 0.3162162162;
                color += sampleAt(vUV - uTexelStep * 1.3846153846) * 0.3162162162;
                color += sampleAt(vUV + uTexelStep * 3.2307692308) * 0.0702702703;
                color += sampleAt(vUV - uTexelStep * 3.2307692308) * 0.0702702703;
                fragColor = color;
            }
        )NIJIGLSL";

        GLuint vertShader = compileOverlayBlurShader(GL_VERTEX_SHADER, vertSource);
        GLuint fragShader = compileOverlayBlurShader(GL_FRAGMENT_SHADER, fragSource);
        overlayBlurProgram = linkOverlayBlurProgram(vertShader, fragShader);
        ogl::glDeleteShader(vertShader);
        ogl::glDeleteShader(fragShader);

        overlayBlurSourceLocation = ogl::glGetUniformLocation(overlayBlurProgram, "uSource");
        overlayBlurTexelStepLocation = ogl::glGetUniformLocation(overlayBlurProgram, "uTexelStep");

        ogl::glGenVertexArrays(1, &overlayBlurVao);
    }

    GLuint createOverlayBlurTexture(int texWidth, int texHeight) {
        GLuint texture = 0;
        ogl::glGenTextures(1, &texture);
        ogl::glBindTexture(GL_TEXTURE_2D, texture);
        ogl::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        ogl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ogl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ogl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        ogl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        ogl::glBindTexture(GL_TEXTURE_2D, 0);
        return texture;
    }

    GLuint createOverlayBlurFramebuffer(GLuint texture) {
        GLuint fbo = 0;
        ogl::glGenFramebuffers(1, &fbo);
        ogl::glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        ogl::glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
        if(ogl::glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)throw std::runtime_error("Failed to create overlay blur framebuffer");
        ogl::glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return fbo;
    }

    void ensureOverlayBlurResources(int blurW, int blurH) {
        ensureOverlayBlurProgram();
        if (overlayBlurWidth == blurW && overlayBlurHeight == blurH
            && overlayBlurTextureA != 0 && overlayBlurTextureB != 0
            && overlayBlurFboA != 0 && overlayBlurFboB != 0) {
            return;
        }

        if (overlayBlurFboA != 0) ogl::glDeleteFramebuffers(1, &overlayBlurFboA);
        if (overlayBlurFboB != 0) ogl::glDeleteFramebuffers(1, &overlayBlurFboB);
        if (overlayBlurTextureA != 0) ogl::glDeleteTextures(1, &overlayBlurTextureA);
        if (overlayBlurTextureB != 0) ogl::glDeleteTextures(1, &overlayBlurTextureB);

        overlayBlurWidth = blurW;
        overlayBlurHeight = blurH;
        overlayBlurTextureA = createOverlayBlurTexture(blurW, blurH);
        overlayBlurTextureB = createOverlayBlurTexture(blurW, blurH);
        overlayBlurFboA = createOverlayBlurFramebuffer(overlayBlurTextureA);
        overlayBlurFboB = createOverlayBlurFramebuffer(overlayBlurTextureB);
    }

    void runOverlayBlurPass(GLuint sourceTexture, GLuint targetFbo, float texelStepX, float texelStepY) {
        ogl::glBindFramebuffer(GL_FRAMEBUFFER, targetFbo);
        ogl::glViewport(0, 0, overlayBlurWidth, overlayBlurHeight);
        ogl::glUseProgram(overlayBlurProgram);
        ogl::glUniform1i(overlayBlurSourceLocation, 0);
        ogl::glUniform2f(overlayBlurTexelStepLocation, texelStepX, texelStepY);
        ogl::glActiveTexture(GL_TEXTURE0);
        ogl::glBindTexture(GL_TEXTURE_2D, sourceTexture);
        ogl::glBindVertexArray(overlayBlurVao);
        ogl::glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    void updateOverlayBlurTexture() {
        if (!showUI || !overlayOpen || width() <= 0 || height() <= 0) return;

        const int blurW = std::max(1, width() / 4);
        const int blurH = std::max(1, height() / 4);
        ensureOverlayBlurResources(blurW, blurH);

        GLint previousReadFbo = 0;
        GLint previousDrawFbo = 0;
        GLint previousProgram = 0;
        GLint previousVao = 0;
        GLint previousTexture = 0;
        GLint previousActiveTexture = 0;
        GLint previousViewport[4]{};
        bool wasBlendEnabled = ogl::glIsEnabled(GL_BLEND) == GL_TRUE;
        bool wasDepthEnabled = ogl::glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
        bool wasScissorEnabled = ogl::glIsEnabled(GL_SCISSOR_TEST) == GL_TRUE;
        bool wasCullEnabled = ogl::glIsEnabled(GL_CULL_FACE) == GL_TRUE;

        ogl::glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFbo);
        ogl::glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDrawFbo);
        ogl::glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
        ogl::glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVao);
        ogl::glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
        ogl::glActiveTexture(GL_TEXTURE0);
        ogl::glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
        ogl::glGetIntegerv(GL_VIEWPORT, previousViewport);

        ogl::glDisable(GL_BLEND);
        ogl::glDisable(GL_DEPTH_TEST);
        ogl::glDisable(GL_SCISSOR_TEST);
        ogl::glDisable(GL_CULL_FACE);

        ogl::glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        ogl::glBindFramebuffer(GL_DRAW_FRAMEBUFFER, overlayBlurFboA);
        ogl::glBlitFramebuffer(0, 0, width(), height(), 0, 0, blurW, blurH, GL_COLOR_BUFFER_BIT, GL_LINEAR);

        const float texelX = 1.0f / static_cast<float>(blurW);
        const float texelY = 1.0f / static_cast<float>(blurH);
        runOverlayBlurPass(overlayBlurTextureA, overlayBlurFboB, texelX * 2.2f, 0.0f);
        runOverlayBlurPass(overlayBlurTextureB, overlayBlurFboA, 0.0f, texelY * 2.2f);
        runOverlayBlurPass(overlayBlurTextureA, overlayBlurFboB, texelX * 3.6f, 0.0f);
        runOverlayBlurPass(overlayBlurTextureB, overlayBlurFboA, 0.0f, texelY * 3.6f);

        ogl::glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
        ogl::glActiveTexture(static_cast<GLenum>(previousActiveTexture));
        ogl::glBindVertexArray(static_cast<GLuint>(previousVao));
        ogl::glUseProgram(static_cast<GLuint>(previousProgram));
        ogl::glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(previousReadFbo));
        ogl::glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(previousDrawFbo));
        ogl::glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);

        if (wasBlendEnabled) ogl::glEnable(GL_BLEND); else ogl::glDisable(GL_BLEND);
        if (wasDepthEnabled) ogl::glEnable(GL_DEPTH_TEST); else ogl::glDisable(GL_DEPTH_TEST);
        if (wasScissorEnabled) ogl::glEnable(GL_SCISSOR_TEST); else ogl::glDisable(GL_SCISSOR_TEST);
        if (wasCullEnabled) ogl::glEnable(GL_CULL_FACE); else ogl::glDisable(GL_CULL_FACE);
    }

#endif

    void drawBlurBackdrop(float x, float y, float w, float h, float rounding) {
#ifdef NIJIEXPOSE_VULKAN
        auto textureId=backend->overlayBlurTexture();
#else
        if (overlayBlurTextureA == 0 || overlayBlurWidth <= 0 || overlayBlurHeight <= 0) return;
        auto textureId = static_cast<ImTextureID>(overlayBlurTextureA);
#endif
        auto bgDrawList = ImGui::GetBackgroundDrawList();
        float u0 = x / static_cast<float>(width());
        float v0 = 1.0f - (y / static_cast<float>(height()));
        float u1 = (x + w) / static_cast<float>(width());
        float v1 = 1.0f - ((y + h) / static_cast<float>(height()));
#ifdef NIJIEXPOSE_VULKAN
        v0=1-v0;v1=1-v1;
#endif

        bgDrawList->AddImageRounded(textureId,
            ImVec2(x, y),
            ImVec2(x + w, y + h),
            ImVec2(u0, v0),
            ImVec2(u1, v1),
            ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 0.88f)),
            rounding
        );

        const float inset = 2.0f;
        bgDrawList->AddImageRounded(textureId,
            ImVec2(x + inset, y + inset),
            ImVec2(x + w - inset, y + h - inset),
            ImVec2(u0, v0),
            ImVec2(u1, v1),
            ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 0.45f)),
            std::max(0.0f, rounding - 2.0f)
        );
    }

    void drawSoftWindowShadow(float x, float y, float w, float h, float rounding, float alphaScale = 1.0f) {
        if (w <= 0 || h <= 0 || alphaScale <= 0.0f) return;

        auto bgDrawList = ImGui::GetBackgroundDrawList();
        const float spreads[] = {24.0f, 18.0f, 13.0f, 9.0f, 6.0f, 3.0f};
        const float weights[] = {0.005f, 0.008f, 0.011f, 0.016f, 0.023f, 0.032f};

        for (size_t index=0; index<std::size(spreads); ++index) {
            float spread=spreads[index];
            float layerAlpha = weights[index] * alphaScale;
            bgDrawList->AddRectFilled(ImVec2(x - spread, y - spread),
                ImVec2(x + w + spread, y + h + spread),
                ImGui::ColorConvertFloat4ToU32(ImVec4(SHADOW_NEAR.x, SHADOW_NEAR.y, SHADOW_NEAR.z, layerAlpha)),
                rounding + spread
            );
        }
    }

    void drawOverlayBackdrop(float alphaScale = 1.0f) {
        if(transparentMode)return;
        if (alphaScale <= 0.0f) return;
        auto bgDrawList = ImGui::GetBackgroundDrawList();
        bgDrawList->AddRectFilled(ImVec2(0.0f, 0.0f),
            ImVec2(static_cast<float>(width()), static_cast<float>(height())),
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.58f, 0.64f, 0.72f, 0.10f * alphaScale))
        );
    }

    std::string iconFor(ActivePanelId id) {
        for (const auto& item : NAV_ITEMS) {
            if (item.id == id) {
                return item.icon;
            }
        }
        return "\ue2c8";
    }

    bool drawNavEntry(std::string id, std::string icon, std::string label, bool selected, bool compact) {
        ImVec2 pos;
        pos = ImGui::GetCursorScreenPos();

        float width = compact ? 44.0f : (navExpanded ? 172.0f : 44.0f);
        float height = 44.0f;
        bool clicked = ImGui::InvisibleButton(id.c_str(), ImVec2(width, height));
        bool hovered = ImGui::IsItemHovered();

        auto drawList = ImGui::GetWindowDrawList();
        ImVec2 minPos = pos;
        ImVec2 maxPos = ImVec2(pos.x + width, pos.y + height);
        float visualAlpha = navVisualAlpha();
        if (selected || hovered) {
            vec4 bg = selected ? ACCENT_SOFT : vec4(0.12f, 0.16f, 0.23f, 0.05f);
            bg = withAlpha(bg, visualAlpha);
            drawList->AddRectFilled(minPos, maxPos, ImGui::ColorConvertFloat4ToU32(ImVec4(bg.x, bg.y, bg.z, bg.w)), 16.0f);
        }

        vec4 iconColor = selected ? ACCENT : vec4(0.70f, 0.25f, 0.00f, 0.90f);
        iconColor = withAlpha(iconColor, visualAlpha);
        auto font = ImGui::GetFont();
        float baseFontSize = ImGui::GetFontSize();
        float iconFontSize = baseFontSize * NAV_ICON_SCALE;
        ImVec2 iconSize;
        iconSize = ImGui::CalcTextSize(icon.c_str());
        iconSize.x *= NAV_ICON_SCALE;
        iconSize.y *= NAV_ICON_SCALE;
        ImVec2 iconPos = ImVec2(minPos.x + 22.0f - (iconSize.x * 0.5f), minPos.y + ((height - iconSize.y) * 0.5f));
        drawList->AddText(font, iconFontSize, iconPos, ImGui::ColorConvertFloat4ToU32(ImVec4(iconColor.x, iconColor.y, iconColor.z, iconColor.w)), icon.c_str());

        if (!compact && navExpanded) {
            vec4 labelColor = selected ? vec4(0.45f, 0.18f, 0.00f, 1.00f) : vec4(0.12f, 0.16f, 0.23f, 0.90f);
            labelColor = withAlpha(labelColor, visualAlpha);
            float labelFontSize = baseFontSize * NAV_LABEL_SCALE;
            ImVec2 labelSize;
            auto translated = _(label);
            labelSize = ImGui::CalcTextSize(translated.c_str());
            labelSize.x *= NAV_LABEL_SCALE;
            labelSize.y *= NAV_LABEL_SCALE;
            ImVec2 labelPos = ImVec2(minPos.x + 48.0f, minPos.y + ((height - labelSize.y) * 0.5f));
            drawList->AddText(font, labelFontSize, labelPos, ImGui::ColorConvertFloat4ToU32(ImVec4(labelColor.x, labelColor.y, labelColor.z, labelColor.w)), translated.c_str());
        }

        if (hovered) {
            uiImTooltip(_(label));
        }
        return clicked;
    }

    bool drawIconOnlyEntry(std::string id, std::string icon, ImVec2 pos, float size, vec4 iconColor, bool hoveredBg) {
        ImGui::SetCursorScreenPos(pos);
        bool clicked = ImGui::InvisibleButton(id.c_str(), ImVec2(size, size));
        bool hovered = ImGui::IsItemHovered();
        float visualAlpha = navVisualAlpha();
        auto drawList = ImGui::GetWindowDrawList();
        if (hovered && hoveredBg) {
            vec4 bgColor = withAlpha(vec4(0.12f, 0.16f, 0.23f, 0.05f), visualAlpha);
            drawList->AddRectFilled(pos,
                ImVec2(pos.x + size, pos.y + size),
                ImGui::ColorConvertFloat4ToU32(ImVec4(bgColor.x, bgColor.y, bgColor.z, bgColor.w)),
                10.0f
            );
        }
        iconColor = withAlpha(iconColor, visualAlpha);
        auto font = ImGui::GetFont();
        float iconFontSize = ImGui::GetFontSize() * NAV_ICON_SCALE;
        ImVec2 iconSize;
        iconSize = ImGui::CalcTextSize(icon.c_str());
        iconSize.x *= NAV_ICON_SCALE;
        iconSize.y *= NAV_ICON_SCALE;
        ImVec2 iconPos = ImVec2(pos.x + ((size - iconSize.x) * 0.5f), pos.y + ((size - iconSize.y) * 0.5f));
        drawList->AddText(font, iconFontSize, iconPos, ImGui::ColorConvertFloat4ToU32(ImVec4(iconColor.x, iconColor.y, iconColor.z, iconColor.w)), icon.c_str());
        return clicked;
    }

    void drawRailButton(NavItem item, bool compact) {
        if (drawNavEntry("nav_" + item.label, item.icon, item.label, overlayOpen && activePanel == item.id, compact)) {
            togglePanel(item.id);
        }
    }

    void drawUtilityButton(std::string icon, std::string label, std::function<void()> action, bool compact) {
        if (drawNavEntry("utility_" + label, icon, label, false, compact)) {
            action();
        }
    }

    void drawTrashButton(bool compact) {
        bool clicked;
        if (!compact) {
            ImVec2 winPos;
            winPos = ImGui::GetWindowPos();
            ImVec2 winSize;
            winSize = ImGui::GetWindowSize();
            const float slotSize = 44.0f;
            const float slotX = winPos.x + ((winSize.x - slotSize) * 0.5f);
            const float slotY = winPos.y + winSize.y - slotSize - 10.0f;
            clicked = drawIconOnlyEntry("trash_button", "\ue872", ImVec2(slotX, slotY), slotSize, vec4(0.70f, 0.25f, 0.00f, 0.90f), true);
        } else {
            ImVec2 pos;
            pos = ImGui::GetCursorScreenPos();
            clicked = drawIconOnlyEntry("trash_button", "\ue872", pos, 44.0f, vec4(0.70f, 0.25f, 0.00f, 0.90f), true);
        }

        if (clicked) {
            insScene()->deleteSelectedSceneItem();
        }
    }

    void drawNavigationShell() {
        if (!showUI || transparentMode) return;
        auto compact = navSurfaceMode() == NavSurfaceMode::CompactBar;
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_NoNavFocus;
        if (compact) {
            flags |= ImGuiWindowFlags_AlwaysAutoResize;
        }

        float visualAlpha = navVisualAlpha();
        float railAlpha = navFaded && !overlayOpen && !navExpanded ? 0.22f : RAIL_BG.w;
        float borderAlpha = navFaded && !overlayOpen && !navExpanded ? 0.06f : RAIL_BORDER.w;
        float shadowNearAlpha = SHADOW_NEAR.w * visualAlpha;
        float shadowFarAlpha = SHADOW_FAR.w * visualAlpha;

        float railX = OUTER_GAP;
        float railY = RAIL_TOP;
        float railW = compact ? 0.0f : (navExpanded ? RAIL_EXPANDED_WIDTH : RAIL_COLLAPSED_WIDTH);
        float railH = compact ? 0.0f : static_cast<float>(height()) - (RAIL_TOP + RAIL_BOTTOM);

        if (!compact) {
            drawSoftWindowShadow(railX, railY, railW, railH, 24.0f, visualAlpha);
        }

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(RAIL_BG.x, RAIL_BG.y, RAIL_BG.z, railAlpha));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(RAIL_BORDER.x, RAIL_BORDER.y, RAIL_BORDER.z, borderAlpha));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, compact ? ImVec2(10, 10) : ImVec2(10, 10));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 24.0f);
        ScopeExit cleanup1{[&] {
            ImGui::PopStyleVar(3);
            ImGui::PopStyleColor(2);}};

        ImGui::SetNextWindowBgAlpha(railAlpha);
        ImGui::SetNextWindowPos(ImVec2(railX, railY), ImGuiCond_Always, ImVec2(0, 0));
        ImGui::SetNextWindowSize(compact ? ImVec2(0, 0) : ImVec2(railW, railH), ImGuiCond_Always);

        if (ImGui::Begin("nijikan_shell_nav###nijikan_shell_nav", nullptr, flags)) {
            if (compact) {
                if (drawNavEntry("menu_toggle", "\ue5d2", "Menu", false, true)) {
                    navExpanded = false;
                }
                uiImSameLine();
            } else {
                if (drawNavEntry("menu_toggle", "\ue5d2", "Menu", false, false)) {
                    navExpanded = !navExpanded;
                }
            }

            if (compact) uiImSameLine();
            drawUtilityButton("\ue2c8", "Models", [&] {
                openModels();
            }, compact);

            for (const auto& item : NAV_ITEMS) {
                if (compact) uiImSameLine();
                drawRailButton(item, compact);
            }

            if(compact)uiImSameLine();
            drawUtilityButton("\ue3c2","Transparent window",[&]{setTransparentMode(true);},compact);

            if (compact) {
                uiImSameLine();
                drawTrashButton(true);
            } else {
                drawTrashButton(false);
            }
        }
        ImGui::End();
    }

    void drawOverlayHost() {
        if (!showUI || !overlayOpen) return;
        std::shared_ptr<Panel> active = panelFor(activePanel);
        std::shared_ptr<ToolWindow> activeWindow = toolWindowFor(activePanel);
        bool customTracking = activePanel == ActivePanelId::Tracking;
        bool customView = activePanel == ActivePanelId::View;
        bool customLanguage = activePanel == ActivePanelId::Language;
        if (!active && !activeWindow && !customTracking && !customView && !customLanguage) return;

        auto compact = navSurfaceMode() == NavSurfaceMode::CompactBar;
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_NoNavFocus;

        bool parameterOverlay = usesParameterOverlay(activePanel);
        const float compactBarHeight = 64.0f;
        const float compactOverlayGap = 10.0f;
        float overlayW;
        float overlayH;
        float overlayX;
        float overlayY;
        if (parameterOverlay && !compact) {
            overlayW = std::min(std::max(static_cast<float>(width()) * 0.22f, 280.0f), 360.0f);
            overlayH = static_cast<float>(height()) - (RAIL_TOP + RAIL_BOTTOM);
            overlayX = OUTER_GAP + (navExpanded ? RAIL_EXPANDED_WIDTH : RAIL_COLLAPSED_WIDTH) + 10.0f;
            overlayY = RAIL_TOP;
        } else if (parameterOverlay && compact) {
            overlayW = std::min(static_cast<float>(width()) - 28.0f, 320.0f);
            overlayX = OUTER_GAP;
            overlayY = RAIL_TOP + compactBarHeight + compactOverlayGap;
            overlayH = static_cast<float>(height()) - overlayY - OUTER_GAP;
        } else {
            overlayW = compact
                ? std::min(static_cast<float>(width()) - 32.0f, 520.0f)
                : std::min(static_cast<float>(width()) * 0.62f, 760.0f);
            overlayH = compact
                ? std::min(static_cast<float>(height()) - 120.0f, 620.0f)
                : std::min(static_cast<float>(height()) * 0.76f, 760.0f);
            overlayX = (static_cast<float>(width()) - overlayW) * 0.5f;
            overlayY = (static_cast<float>(height()) - overlayH) * 0.5f;
        }

        {
            drawOverlayBackdrop(1.0f);
            drawBlurBackdrop(overlayX, overlayY, overlayW, overlayH, 13.0f);
            drawSoftWindowShadow(overlayX, overlayY, overlayW, overlayH, 13.0f);
        }

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(OVERLAY_BG.x, OVERLAY_BG.y, OVERLAY_BG.z, OVERLAY_BG.w));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(OVERLAY_BORDER.x, OVERLAY_BORDER.y, OVERLAY_BORDER.z, OVERLAY_BORDER.w));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, parameterOverlay ? ImVec2(0, 0) : ImVec2(18, 16));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 13.0f);
        ScopeExit cleanup2{[&] {
            ImGui::PopStyleVar(3);
            ImGui::PopStyleColor(2);}};

        ImGui::SetNextWindowPos(ImVec2(overlayX, overlayY), ImGuiCond_Always, ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(overlayW, overlayH), ImGuiCond_Always);

        std::string title = panelTitle(activePanel, active, activeWindow);
        std::string windowTitle = title + "###nijikan_overlay_host";

        if (!parameterOverlay && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            ImVec2 mousePos;
            mousePos = ImGui::GetMousePos();
            ImRect overlayRect(overlayX, overlayY, overlayX + overlayW, overlayY + overlayH);
            if (!overlayRect.Contains(mousePos)) {
                overlayOpen = false;
                inSettingsSet("ui.overlayOpen", overlayOpen);
                return;
            }
        }

        if (ImGui::Begin(windowTitle.c_str(), nullptr, flags)) {
            if (parameterOverlay) {
                ImGui::SetCursorPos(ImVec2(16, 14));
            }
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
            ScopeExit framePadding{[]{ImGui::PopStyleVar();}};

            ImVec2 headerStart;
            headerStart = ImGui::GetCursorScreenPos();
            auto drawList = ImGui::GetWindowDrawList();
            auto font = ImGui::GetFont();
            float headerIconSize = ImGui::GetFontSize() * NAV_ICON_SCALE;
            std::string headerIcon = iconFor(activePanel);
            ImVec2 iconSize;
            iconSize = ImGui::CalcTextSize(headerIcon.c_str());
            iconSize.x *= NAV_ICON_SCALE;
            iconSize.y *= NAV_ICON_SCALE;
            ImVec2 iconPos = ImVec2(headerStart.x, headerStart.y + std::max(0.0f, (20.0f - iconSize.y) * 0.5f));
            drawList->AddText(font, headerIconSize, iconPos, ImGui::ColorConvertFloat4ToU32(ImVec4(ACCENT.x, ACCENT.y, ACCENT.z, ACCENT.w)), headerIcon.c_str());
            ImGui::SetCursorScreenPos(ImVec2(headerStart.x + 30.0f, headerStart.y));
            uiImLabel(title);
            ImVec2 closePos = ImVec2(overlayX + overlayW - 42.0f, headerStart.y - 2.0f);
            if (drawIconOnlyEntry("overlay_close", "\ue5cd", closePos, 28.0f, vec4(0.12f, 0.16f, 0.23f, 0.90f), false)) {
                overlayOpen = false;
                inSettingsSet("ui.overlayOpen", overlayOpen);
            }
            ImGui::SetCursorScreenPos(ImVec2(headerStart.x, headerStart.y + 32.0f));

            if (parameterOverlay) {
                ImGui::SetCursorPosX(0);
            }
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, parameterOverlay ? ImVec2(8, 6) : ImVec2(8, 8));
            ScopeExit cleanup3{[&] {
                ImGui::PopStyleVar();
                ImGui::PopStyleVar();
                ImGui::PopStyleColor();}};
            const bool hasOverlayFooter = customTracking || customView || customLanguage;
            const float overlayFooterHeight = hasOverlayFooter ? 48.0f : 0.0f;
            if (uiImBeginChild("nijikan_overlay_body###nijikan_overlay_body", vec2(0, -overlayFooterHeight), false)) {
                if (customTracking) {
                    uiImLabel(_("Tracking Settings"));
                    ImGui::Dummy(ImVec2(0, 4));
                    settingWindow->renderTrackingSettingsContent(true);
                    uiImSeperator();
                    uiImNewLine();
                    uiImLabel(_("Virtual Space"));
                    ImGui::Dummy(ImVec2(0, 4));
                    spaceEditor->renderEmbeddedSection();
                } else if (customView) {
                    uiImLabel(_("Scene Settings"));
                    ImGui::Dummy(ImVec2(0, 4));
                    if (auto scenePanel = std::dynamic_pointer_cast<ScenePanel>(active)) {
                        scenePanel->renderSceneSettingsContent();
                    } else if (active != nullptr) {
                        active->updateEmbedded();
                    }
                    uiImSeperator();
                    uiImNewLine();
                    uiImLabel(_("Rendering"));
                    ImGui::Dummy(ImVec2(0, 4));
                    settingWindow->renderRenderingSettingsContent(true);
                } else if (customLanguage) {
                    settingWindow->renderLanguageSettingsContent(true);
                } else if (active != nullptr) {
                    active->updateEmbedded();
                } else {
                    activeWindow->updateEmbedded();
                }
            }
            uiImEndChild();
            if (hasOverlayFooter) {
                ImGui::Dummy(ImVec2(0, 4));
                ImGui::SetCursorPosX(std::max(0.0f, uiImAvailableSpace().x - 72.0f));
                if (uiImButton(__("Apply"), vec2(64, 0))) {
                    if (customTracking) {
                        settingWindow->applyTrackingSettings();
                        spaceEditor->applyChanges();
                    } else if (customView) {
                        settingWindow->applyRenderingSettings();
                    } else if (customLanguage) {
                        settingWindow->applyLanguageSettings();
                    }
                }
            }
        }
        ImGui::End();
    }

    void loadModels(const std::vector<std::string>& args) {
        for(const auto& arg:args){
            auto extension=std::filesystem::u8path(arg).filename().extension().string();
            for(auto& ch:extension)if(ch>='A'&&ch<='Z')ch+=32;
            if(extension==".png"||extension==".tga"||extension==".jpeg"||extension==".jpg"){
                try{insScene()->addPuppet(arg,neLoadModelFromImage(arg));}
                catch(const std::exception& ex){uiImDialog(__("Error"),"Could not load "+arg+", "+ex.what());}
            }else if(extension==".inp"||extension==".inx"){
                if(!std::filesystem::exists(std::filesystem::u8path(arg)))continue;
                try{insScene()->addPuppet(arg,model_io::inLoadPuppet(arg));}
                catch(const std::exception& ex){uiImDialog(__("Error"),"Could not load "+arg+", "+ex.what());}
            }else{
                auto message=_("Could not load %s, unsupported file format.");
                auto pos=message.find("%s");if(pos!=std::string::npos)message.replace(pos,2,arg);
                uiImDialog(__("Error"),message);
            }
        }
    }

protected:
    void onEarlyUpdate() {
        nijiexpose::control::poll();
        insScene()->update();
#ifndef NIJIEXPOSE_VULKAN
        insSendFrame();
        ogl::glBindFramebuffer(GL_FRAMEBUFFER, 0);
        ogl::oglPresentSceneToBackbuffer(width(),height(),
#ifdef _WIN32
            transparentMode
#else
            false
#endif
        );
#endif
#ifndef NIJIEXPOSE_VULKAN
        updateOverlayBlurTexture();
#endif
    }

    void onPlatformUpdate() override {
        SDL_UpdateTrays();
        if(pendingTransparent) {
            auto enabled=*pendingTransparent;pendingTransparent.reset();
            try{applyTransparentMode(enabled);}
            catch(const std::exception& ex){uiImDialog(__("Error"),ex.what());}
        }
        tracking::neTracker()->update();
    }
    bool onPlatformEvent(const SDL_Event& event) override { return tracking::neTracker()->handleEvent(event); }
    void onUpdate() {
        while(!pendingTrayCommands.empty()) {
            int command=pendingTrayCommands.front();pendingTrayCommands.pop_front();
            if(command==100||command==103)setTransparentMode(false);
            else if(command==101)openModels();
            else if(command==102)insScene()->deleteSelectedSceneItem();
            else if(command==104)close();
            else {showUI=true;activePanel=static_cast<ActivePanelId>(command);overlayOpen=true;focus();}
        }
        syncPanelVisibility();
        updateNavFadeState();
        if (!inInputIsnijiui()) {
            if (!transparentMode && inInputMouseDoubleClicked(MouseButton::Left)) this->showUI = !showUI;
            insScene()->interact();

            if (!getDraggedFiles().empty()) {
                loadModels(getDraggedFiles());
            }
        }

        drawNavigationShell();
        drawOverlayHost();

        
    }

    void onResized(int w, int h) {
        core::inSetViewport(w,h);
        inSettingsSetJson("window", json::Object{{"width",width()},{"height",height()}});
        InApplicationWindow::onResized(w,h);
    }

    void onClosed() {
#ifndef NIJIEXPOSE_VULKAN
        destroyOverlayBlurResources();
#endif
    }
public:
    void setTransparentMode(bool enabled) override {pendingTransparent=enabled;}
    ~nijiexposeWindow() override {destroyTray();render::transparentWindow=false;}

    /**
        Construct nijiexpose
    */
    nijiexposeWindow(const std::vector<std::string>& args, InochiWindowSettings ws)
        : InApplicationWindow("nijiexpose " NIJIEXPOSE_NATIVE_VERSION, ws.width, ws.height, inSettingsGet<int>("throttlingRate",1)) {
        core::inInit(&nijiui::inGetTime);
        backend=std::make_shared<ApplicationRenderBackend>();
        core::inSetRenderBackend(backend);
        backend->initializeRenderer();
        uiImInitDialogs();
        neSetStyle();
        inSetPanelsSuspended(true);
        core::inSetViewport(ws.width,ws.height);
        settingWindow=std::make_shared<SettingWindow>(neWindowSetThrottlingRate);
        spaceEditor=std::make_shared<SpaceEditor>();
        lastPointerPos=inInputMousePosition();
        lastNavInteractionAt=nijiui::inGetTime();
        activePanel=sanitizeActivePanel(inSettingsGet<int>("ui.activePanel",static_cast<int>(ActivePanelId::Parameters)));
        overlayOpen=inSettingsGet<bool>("ui.overlayOpen",true);
        loadModels(args);
        core::inGetCamera().scale={0.5f,0.5f};
        auto logoBytes=embeddedResource(4301);
        logo=std::make_shared<core::Texture>(core::ShallowTexture(std::vector<uint8_t>(logoBytes.begin(),logoBytes.end())));
        auto iconBytes=embeddedResource(4302);
        auto icon=core::ShallowTexture(std::vector<uint8_t>(iconBytes.begin(),iconBytes.end()));
        setIcon(icon);
    }
};

namespace { thread_local std::shared_ptr<nijiexposeWindow> window_; }
std::shared_ptr<InApplicationWindow> neCreateWindow(const std::vector<std::string>& args){
    if(!window_){
        InochiWindowSettings ws{1024,1024};
        if(inSettingsCanGet("window")){const auto& value=inSettingsGetJson("window");if(auto v=json::find(value,"width"))json::deserialize(*v,ws.width);if(auto v=json::find(value,"height"))json::deserialize(*v,ws.height);}
        window_=std::make_shared<nijiexposeWindow>(args,ws);
    }
    return window_;
}
void neWindowSetThrottlingRate(int rate){if(window_)window_->setThrottlingRate(rate);}
}
