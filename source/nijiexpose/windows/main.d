/*
    Copyright © 2022, Inochi2D Project
    Copyright © 2024, nijigenerate Project
    Distributed under the 2-Clause BSD License, see LICENSE file.
    
    Authors: Luna Nielsen
*/
module nijiexpose.windows.main;
import nijiexpose.windows;
import nijiexpose.windows.utils;
import nijiexpose.scene;
import nijiexpose.log;
import nijiexpose.framesend;
import nijiexpose.plugins;
import nijiexpose.io;
import nijiexpose.io.image;
import nijiexpose.tracking.tracker;
import nijiui;
import nijiui.widgets;
import nijiui.toolwindow;
import nijiui.panel;
import nijiui.input;
import nijilive;
import nijilive.core.render.backends.opengl.runtime : oglDrawScene;
import ft;
import i18n;
import nijiui.utils.link;
import nijiui.core.settings : inSettingsGet, inSettingsSet;
import std.format;
import nijiexpose.ver;
import bindbc.opengl;
import bindbc.imgui;
import std.algorithm.comparison : min, max;
import std.exception : enforce;
import std.path;
import std.string;

version(linux) import dportals;

private {
    enum NavSurfaceMode {
        DesktopRail,
        CompactBar,
    }

    enum ActivePanelId {
        Parameters,
        Tracking,
        View,
        Animations,
        Blendshapes,
        Plugins,
    }

    struct NavItem {
        ActivePanelId id;
        string panelName;
        string icon;
        string label;
    }

    immutable NavItem[] NAV_ITEMS = [
        NavItem(ActivePanelId.Parameters, "Tracking", "\ue429", "Parameters"),
        NavItem(ActivePanelId.Tracking, "", "\ue3b4", "Tracking"),
        NavItem(ActivePanelId.View, "Scene Settings", "\ue8f4", "View"),
        NavItem(ActivePanelId.Animations, "Animations", "\ue037", "Animations"),
        NavItem(ActivePanelId.Blendshapes, "Blendshapes", "\ue3f4", "Blends"),
        NavItem(ActivePanelId.Plugins, "Plugins", "\ue87b", "Plugins"),
    ];

    enum vec4 RAIL_BG = vec4(0.97f, 0.98f, 0.99f, 0.84f);
    enum vec4 RAIL_BORDER = vec4(0.12f, 0.16f, 0.23f, 0.10f);
    enum vec4 OVERLAY_BG = vec4(0.97f, 0.98f, 0.99f, 0.72f);
    enum vec4 OVERLAY_BORDER = vec4(0.12f, 0.16f, 0.23f, 0.10f);
    enum vec4 ACCENT = vec4(0.70f, 0.25f, 0.00f, 1.00f);
    enum vec4 ACCENT_SOFT = vec4(0.70f, 0.25f, 0.00f, 0.14f);
    enum vec4 SHADOW_NEAR = vec4(0.12f, 0.16f, 0.23f, 0.10f);
    enum vec4 SHADOW_FAR = vec4(0.12f, 0.16f, 0.23f, 0.03f);
    enum float RAIL_COLLAPSED_WIDTH = 62.0f;
    enum float RAIL_EXPANDED_WIDTH = 210.0f;
    enum float OUTER_GAP = 18.0f;
    enum float RAIL_TOP = 18.0f;
    enum float RAIL_BOTTOM = 18.0f;
    enum float NAV_ICON_SCALE = 1.18f;
    enum float NAV_LABEL_SCALE = 1.08f;
    enum float NAV_CLOSE_SCALE = 1.10f;

    struct InochiWindowSettings {
        int width;
        int height;
    }

    struct PuppetSavedData {
        float scale;
    }
    nijiexposeWindow window_ = null;
}

nijiexposeWindow neCreateWindow(string[] args) {
    if (!window_) {
        window_ = new nijiexposeWindow(args);
    }
    return window_;
}

void neWindowSetThrottlingRate(int rate) {
    if (window_) {
        window_.setThrottlingRate(rate);
    }
}

class nijiexposeWindow : InApplicationWindow {
private:
    Adaptor adaptor;
    version (InBranding) Texture logo;
    SettingWindow settingWindow;
    SpaceEditor spaceEditor;
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
    ActivePanelId activePanel = ActivePanelId.Parameters;
    bool navExpanded = false;
    bool overlayOpen = true;
    bool navFaded = false;
    vec2 lastPointerPos;
    double lastNavInteractionAt = 0;

    ActivePanelId sanitizeActivePanel(int rawValue) {
        if (rawValue < cast(int)ActivePanelId.Parameters || rawValue > cast(int)ActivePanelId.Plugins) {
            return ActivePanelId.Parameters;
        }
        return cast(ActivePanelId)rawValue;
    }

    Panel panelFor(ActivePanelId id) {
        foreach(item; NAV_ITEMS) {
            if (item.id != id) continue;
            if (item.panelName.length == 0) return null;
            foreach(panel; inPanels) {
                if (panel.name() == item.panelName) return panel;
            }
        }
        return null;
    }

    ToolWindow toolWindowFor(ActivePanelId id) {
        final switch(id) {
        case ActivePanelId.Parameters:
        case ActivePanelId.Tracking:
        case ActivePanelId.View:
        case ActivePanelId.Animations:
        case ActivePanelId.Blendshapes:
        case ActivePanelId.Plugins:
            return null;
        }
    }

    NavSurfaceMode navSurfaceMode() {
        if (width <= 1100 || height <= 760) return NavSurfaceMode.CompactBar;
        return NavSurfaceMode.DesktopRail;
    }

    void touchNav() {
        lastNavInteractionAt = inGetTime();
        navFaded = false;
    }

    void updateNavFadeState() {
        vec2 mousePos = inInputMousePosition();
        bool pointerMoved = mousePos.x != lastPointerPos.x || mousePos.y != lastPointerPos.y;
        bool interacted = pointerMoved
            || inInputMouseClicked(MouseButton.Left)
            || inInputMouseClicked(MouseButton.Right)
            || inInputMouseClicked(MouseButton.Middle)
            || inInputMouseScrollDelta() != 0;

        if (interacted || overlayOpen || navExpanded) {
            touchNav();
        } else if ((inGetTime() - lastNavInteractionAt) > 2.6) {
            navFaded = true;
        }

        lastPointerPos = mousePos;
    }

    void syncPanelVisibility() {
        foreach(panel; inPanels) {
            panel.visible = false;
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
        inSettingsSet("ui.activePanel", cast(int)activePanel);
        inSettingsSet("ui.overlayOpen", overlayOpen);
    }

    bool usesParameterOverlay(ActivePanelId id) {
        return id == ActivePanelId.Parameters;
    }

    string panelTitle(ActivePanelId id, Panel active, ToolWindow activeWindow) {
        final switch (id) {
        case ActivePanelId.Parameters:
            return "Parameters";
        case ActivePanelId.Tracking:
            return "Tracking";
        case ActivePanelId.View:
            return "View";
        case ActivePanelId.Animations:
        case ActivePanelId.Blendshapes:
        case ActivePanelId.Plugins:
            return active !is null ? active.displayName() : (activeWindow !is null ? activeWindow.name() : "");
        }
    }

    void destroyOverlayBlurResources() {
        if (overlayBlurProgram != 0) {
            glDeleteProgram(overlayBlurProgram);
            overlayBlurProgram = 0;
        }
        if (overlayBlurVao != 0) {
            glDeleteVertexArrays(1, &overlayBlurVao);
            overlayBlurVao = 0;
        }
        if (overlayBlurFboA != 0) {
            glDeleteFramebuffers(1, &overlayBlurFboA);
            overlayBlurFboA = 0;
        }
        if (overlayBlurFboB != 0) {
            glDeleteFramebuffers(1, &overlayBlurFboB);
            overlayBlurFboB = 0;
        }
        if (overlayBlurTextureA != 0) {
            glDeleteTextures(1, &overlayBlurTextureA);
            overlayBlurTextureA = 0;
        }
        if (overlayBlurTextureB != 0) {
            glDeleteTextures(1, &overlayBlurTextureB);
            overlayBlurTextureB = 0;
        }
        overlayBlurWidth = 0;
        overlayBlurHeight = 0;
        overlayBlurSourceLocation = -1;
        overlayBlurTexelStepLocation = -1;
    }

    GLuint compileOverlayBlurShader(GLenum stage, string source) {
        GLuint shader = glCreateShader(stage);
        auto ptr = source.ptr;
        glShaderSource(shader, 1, &ptr, null);
        glCompileShader(shader);

        GLint status = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (status == GL_TRUE) {
            return shader;
        }

        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        string message = "unknown";
        if (length > 0) {
            char[] buffer = new char[length];
            glGetShaderInfoLog(shader, length, null, buffer.ptr);
            message = cast(string)buffer;
        }

        glDeleteShader(shader);
        throw new Exception("Failed to compile overlay blur shader: " ~ message);
    }

    GLuint linkOverlayBlurProgram(GLuint vertexShader, GLuint fragmentShader) {
        GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        GLint status = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if (status == GL_TRUE) {
            return program;
        }

        GLint length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        string message = "unknown";
        if (length > 0) {
            char[] buffer = new char[length];
            glGetProgramInfoLog(program, length, null, buffer.ptr);
            message = cast(string)buffer;
        }

        glDeleteProgram(program);
        throw new Exception("Failed to link overlay blur shader program: " ~ message);
    }

    void ensureOverlayBlurProgram() {
        if (overlayBlurProgram != 0 && overlayBlurVao != 0) return;

        enum string vertSource = q{
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
        };
        enum string fragSource = q{
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
        };

        GLuint vertShader = compileOverlayBlurShader(GL_VERTEX_SHADER, vertSource);
        GLuint fragShader = compileOverlayBlurShader(GL_FRAGMENT_SHADER, fragSource);
        overlayBlurProgram = linkOverlayBlurProgram(vertShader, fragShader);
        glDeleteShader(vertShader);
        glDeleteShader(fragShader);

        overlayBlurSourceLocation = glGetUniformLocation(overlayBlurProgram, "uSource");
        overlayBlurTexelStepLocation = glGetUniformLocation(overlayBlurProgram, "uTexelStep");

        glGenVertexArrays(1, &overlayBlurVao);
    }

    GLuint createOverlayBlurTexture(int texWidth, int texHeight) {
        GLuint texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, null);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        return texture;
    }

    GLuint createOverlayBlurFramebuffer(GLuint texture) {
        GLuint fbo = 0;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
        enforce(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
            "Failed to create overlay blur framebuffer");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return fbo;
    }

    void ensureOverlayBlurResources(int blurW, int blurH) {
        ensureOverlayBlurProgram();
        if (overlayBlurWidth == blurW && overlayBlurHeight == blurH
            && overlayBlurTextureA != 0 && overlayBlurTextureB != 0
            && overlayBlurFboA != 0 && overlayBlurFboB != 0) {
            return;
        }

        if (overlayBlurFboA != 0) glDeleteFramebuffers(1, &overlayBlurFboA);
        if (overlayBlurFboB != 0) glDeleteFramebuffers(1, &overlayBlurFboB);
        if (overlayBlurTextureA != 0) glDeleteTextures(1, &overlayBlurTextureA);
        if (overlayBlurTextureB != 0) glDeleteTextures(1, &overlayBlurTextureB);

        overlayBlurWidth = blurW;
        overlayBlurHeight = blurH;
        overlayBlurTextureA = createOverlayBlurTexture(blurW, blurH);
        overlayBlurTextureB = createOverlayBlurTexture(blurW, blurH);
        overlayBlurFboA = createOverlayBlurFramebuffer(overlayBlurTextureA);
        overlayBlurFboB = createOverlayBlurFramebuffer(overlayBlurTextureB);
    }

    void runOverlayBlurPass(GLuint sourceTexture, GLuint targetFbo, float texelStepX, float texelStepY) {
        glBindFramebuffer(GL_FRAMEBUFFER, targetFbo);
        glViewport(0, 0, overlayBlurWidth, overlayBlurHeight);
        glUseProgram(overlayBlurProgram);
        glUniform1i(overlayBlurSourceLocation, 0);
        glUniform2f(overlayBlurTexelStepLocation, texelStepX, texelStepY);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sourceTexture);
        glBindVertexArray(overlayBlurVao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    void updateOverlayBlurTexture() {
        if (!showUI || !overlayOpen || width <= 0 || height <= 0) return;

        immutable int blurW = max(1, width / 4);
        immutable int blurH = max(1, height / 4);
        ensureOverlayBlurResources(blurW, blurH);

        GLint previousReadFbo = 0;
        GLint previousDrawFbo = 0;
        GLint previousProgram = 0;
        GLint previousVao = 0;
        GLint previousTexture = 0;
        GLint previousActiveTexture = 0;
        GLint[4] previousViewport;
        bool wasBlendEnabled = glIsEnabled(GL_BLEND) == GL_TRUE;
        bool wasDepthEnabled = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
        bool wasScissorEnabled = glIsEnabled(GL_SCISSOR_TEST) == GL_TRUE;
        bool wasCullEnabled = glIsEnabled(GL_CULL_FACE) == GL_TRUE;

        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFbo);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDrawFbo);
        glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVao);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
        glGetIntegerv(GL_VIEWPORT, previousViewport.ptr);

        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_CULL_FACE);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, overlayBlurFboA);
        glBlitFramebuffer(0, 0, width, height, 0, 0, blurW, blurH, GL_COLOR_BUFFER_BIT, GL_LINEAR);

        immutable float texelX = 1.0f / cast(float)blurW;
        immutable float texelY = 1.0f / cast(float)blurH;
        runOverlayBlurPass(overlayBlurTextureA, overlayBlurFboB, texelX * 2.2f, 0.0f);
        runOverlayBlurPass(overlayBlurTextureB, overlayBlurFboA, 0.0f, texelY * 2.2f);
        runOverlayBlurPass(overlayBlurTextureA, overlayBlurFboB, texelX * 3.6f, 0.0f);
        runOverlayBlurPass(overlayBlurTextureB, overlayBlurFboA, 0.0f, texelY * 3.6f);

        glBindTexture(GL_TEXTURE_2D, cast(GLuint)previousTexture);
        glActiveTexture(cast(GLenum)previousActiveTexture);
        glBindVertexArray(cast(GLuint)previousVao);
        glUseProgram(cast(GLuint)previousProgram);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, cast(GLuint)previousReadFbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, cast(GLuint)previousDrawFbo);
        glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);

        if (wasBlendEnabled) glEnable(GL_BLEND); else glDisable(GL_BLEND);
        if (wasDepthEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        if (wasScissorEnabled) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
        if (wasCullEnabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    }

    void drawBlurBackdrop(float x, float y, float w, float h, float rounding) {
        if (overlayBlurTextureA == 0 || overlayBlurWidth <= 0 || overlayBlurHeight <= 0) return;

        auto bgDrawList = igGetBackgroundDrawList_Nil();
        auto textureId = cast(ImTextureID)overlayBlurTextureA;
        float u0 = x / cast(float)width;
        float v0 = 1.0f - (y / cast(float)height);
        float u1 = (x + w) / cast(float)width;
        float v1 = 1.0f - ((y + h) / cast(float)height);

        ImDrawList_AddImageRounded(
            bgDrawList,
            textureId,
            ImVec2(x, y),
            ImVec2(x + w, y + h),
            ImVec2(u0, v0),
            ImVec2(u1, v1),
            igColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 0.88f)),
            rounding
        );

        immutable float inset = 2.0f;
        ImDrawList_AddImageRounded(
            bgDrawList,
            textureId,
            ImVec2(x + inset, y + inset),
            ImVec2(x + w - inset, y + h - inset),
            ImVec2(u0, v0),
            ImVec2(u1, v1),
            igColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 0.45f)),
            max(0.0f, rounding - 2.0f)
        );
    }

    void drawSoftWindowShadow(float x, float y, float w, float h, float rounding, float alphaScale = 1.0f) {
        if (w <= 0 || h <= 0 || alphaScale <= 0.0f) return;

        auto bgDrawList = igGetBackgroundDrawList_Nil();
        immutable float[] spreads = [24.0f, 18.0f, 13.0f, 9.0f, 6.0f, 3.0f];
        immutable float[] weights = [0.005f, 0.008f, 0.011f, 0.016f, 0.023f, 0.032f];

        foreach (index, spread; spreads) {
            float layerAlpha = weights[index] * alphaScale;
            ImDrawList_AddRectFilled(
                bgDrawList,
                ImVec2(x - spread, y - spread),
                ImVec2(x + w + spread, y + h + spread),
                igColorConvertFloat4ToU32(ImVec4(SHADOW_NEAR.x, SHADOW_NEAR.y, SHADOW_NEAR.z, layerAlpha)),
                rounding + spread
            );
        }
    }

    string iconFor(ActivePanelId id) {
        foreach (item; NAV_ITEMS) {
            if (item.id == id) {
                return item.icon;
            }
        }
        return "\ue2c8";
    }

    bool drawNavEntry(string id, string icon, string label, bool selected, bool compact) {
        ImVec2 pos;
        igGetCursorScreenPos(&pos);

        float width = compact ? 44.0f : (navExpanded ? 172.0f : 44.0f);
        float height = 44.0f;
        bool clicked = igInvisibleButton(id.toStringz, ImVec2(width, height));
        bool hovered = igIsItemHovered();

        auto drawList = igGetWindowDrawList();
        ImVec2 minPos = pos;
        ImVec2 maxPos = ImVec2(pos.x + width, pos.y + height);
        float visualAlpha = navVisualAlpha();
        if (selected || hovered) {
            vec4 bg = selected ? ACCENT_SOFT : vec4(0.12f, 0.16f, 0.23f, 0.05f);
            bg = withAlpha(bg, visualAlpha);
            ImDrawList_AddRectFilled(drawList, minPos, maxPos, igColorConvertFloat4ToU32(ImVec4(bg.x, bg.y, bg.z, bg.w)), 16.0f);
        }

        vec4 iconColor = selected ? ACCENT : vec4(0.70f, 0.25f, 0.00f, 0.90f);
        iconColor = withAlpha(iconColor, visualAlpha);
        auto font = igGetFont();
        float baseFontSize = igGetFontSize();
        float iconFontSize = baseFontSize * NAV_ICON_SCALE;
        ImVec2 iconSize;
        igCalcTextSize(&iconSize, icon.toStringz);
        iconSize.x *= NAV_ICON_SCALE;
        iconSize.y *= NAV_ICON_SCALE;
        ImVec2 iconPos = ImVec2(minPos.x + 22.0f - (iconSize.x * 0.5f), minPos.y + ((height - iconSize.y) * 0.5f));
        ImDrawList_AddText(drawList, font, iconFontSize, iconPos, igColorConvertFloat4ToU32(ImVec4(iconColor.x, iconColor.y, iconColor.z, iconColor.w)), icon.toStringz);

        if (!compact && navExpanded) {
            vec4 labelColor = selected ? vec4(0.45f, 0.18f, 0.00f, 1.00f) : vec4(0.12f, 0.16f, 0.23f, 0.90f);
            labelColor = withAlpha(labelColor, visualAlpha);
            float labelFontSize = baseFontSize * NAV_LABEL_SCALE;
            ImVec2 labelSize;
            auto translated = _(label);
            igCalcTextSize(&labelSize, translated.toStringz);
            labelSize.x *= NAV_LABEL_SCALE;
            labelSize.y *= NAV_LABEL_SCALE;
            ImVec2 labelPos = ImVec2(minPos.x + 48.0f, minPos.y + ((height - labelSize.y) * 0.5f));
            ImDrawList_AddText(drawList, font, labelFontSize, labelPos, igColorConvertFloat4ToU32(ImVec4(labelColor.x, labelColor.y, labelColor.z, labelColor.w)), translated.toStringz);
        }

        if (hovered) {
            uiImTooltip(_(label));
        }
        return clicked;
    }

    bool drawIconOnlyEntry(string id, string icon, ImVec2 pos, float size, vec4 iconColor, bool hoveredBg) {
        igSetCursorScreenPos(pos);
        bool clicked = igInvisibleButton(id.toStringz, ImVec2(size, size));
        bool hovered = igIsItemHovered();
        float visualAlpha = navVisualAlpha();
        auto drawList = igGetWindowDrawList();
        if (hovered && hoveredBg) {
            vec4 bgColor = withAlpha(vec4(0.12f, 0.16f, 0.23f, 0.05f), visualAlpha);
            ImDrawList_AddRectFilled(
                drawList,
                pos,
                ImVec2(pos.x + size, pos.y + size),
                igColorConvertFloat4ToU32(ImVec4(bgColor.x, bgColor.y, bgColor.z, bgColor.w)),
                10.0f
            );
        }
        iconColor = withAlpha(iconColor, visualAlpha);
        auto font = igGetFont();
        float iconFontSize = igGetFontSize() * NAV_ICON_SCALE;
        ImVec2 iconSize;
        igCalcTextSize(&iconSize, icon.toStringz);
        iconSize.x *= NAV_ICON_SCALE;
        iconSize.y *= NAV_ICON_SCALE;
        ImVec2 iconPos = ImVec2(pos.x + ((size - iconSize.x) * 0.5f), pos.y + ((size - iconSize.y) * 0.5f));
        ImDrawList_AddText(drawList, font, iconFontSize, iconPos, igColorConvertFloat4ToU32(ImVec4(iconColor.x, iconColor.y, iconColor.z, iconColor.w)), icon.toStringz);
        return clicked;
    }

    void drawRailButton(NavItem item, bool compact) {
        if (drawNavEntry("nav_" ~ item.label, item.icon, item.label, overlayOpen && activePanel == item.id, compact)) {
            togglePanel(item.id);
        }
    }

    void drawUtilityButton(string icon, string label, void delegate() action, bool compact) {
        if (drawNavEntry("utility_" ~ label, icon, label, false, compact)) {
            action();
        }
    }

    void drawTrashButton(bool compact) {
        bool clicked;
        if (!compact) {
            ImVec2 winPos;
            igGetWindowPos(&winPos);
            ImVec2 winSize;
            igGetWindowSize(&winSize);
            immutable float slotSize = 44.0f;
            immutable float slotX = winPos.x + ((winSize.x - slotSize) * 0.5f);
            immutable float slotY = winPos.y + winSize.y - slotSize - 10.0f;
            clicked = drawIconOnlyEntry("trash_button", "\ue872", ImVec2(slotX, slotY), slotSize, vec4(0.70f, 0.25f, 0.00f, 0.90f), true);
        } else {
            ImVec2 pos;
            igGetCursorScreenPos(&pos);
            clicked = drawIconOnlyEntry("trash_button", "\ue872", pos, 44.0f, vec4(0.70f, 0.25f, 0.00f, 0.90f), true);
        }

        if (clicked) {
            insScene.deleteSelectedSceneItem();
        }
    }

    void drawNavigationShell() {
        if (!showUI) return;
        auto compact = navSurfaceMode() == NavSurfaceMode.CompactBar;
        ImGuiWindowFlags flags = ImGuiWindowFlags.NoDecoration
            | ImGuiWindowFlags.NoMove
            | ImGuiWindowFlags.NoResize
            | ImGuiWindowFlags.NoSavedSettings
            | ImGuiWindowFlags.NoNavFocus;
        if (compact) {
            flags |= ImGuiWindowFlags.AlwaysAutoResize;
        }

        float visualAlpha = navVisualAlpha();
        float railAlpha = navFaded && !overlayOpen && !navExpanded ? 0.22f : RAIL_BG.w;
        float borderAlpha = navFaded && !overlayOpen && !navExpanded ? 0.06f : RAIL_BORDER.w;
        float shadowNearAlpha = SHADOW_NEAR.w * visualAlpha;
        float shadowFarAlpha = SHADOW_FAR.w * visualAlpha;

        float railX = OUTER_GAP;
        float railY = RAIL_TOP;
        float railW = compact ? 0.0f : (navExpanded ? RAIL_EXPANDED_WIDTH : RAIL_COLLAPSED_WIDTH);
        float railH = compact ? 0.0f : cast(float)height - (RAIL_TOP + RAIL_BOTTOM);

        if (!compact) {
            drawSoftWindowShadow(railX, railY, railW, railH, 24.0f, visualAlpha);
        }

        igPushStyleColor(ImGuiCol.WindowBg, ImVec4(RAIL_BG.x, RAIL_BG.y, RAIL_BG.z, railAlpha));
        igPushStyleColor(ImGuiCol.Border, ImVec4(RAIL_BORDER.x, RAIL_BORDER.y, RAIL_BORDER.z, borderAlpha));
        igPushStyleVar(ImGuiStyleVar.WindowBorderSize, 1.0f);
        igPushStyleVar(ImGuiStyleVar.WindowPadding, compact ? ImVec2(10, 10) : ImVec2(10, 10));
        igPushStyleVar(ImGuiStyleVar.WindowRounding, 24.0f);
        scope(exit) {
            igPopStyleVar(3);
            igPopStyleColor(2);
        }

        igSetNextWindowBgAlpha(railAlpha);
        igSetNextWindowPos(ImVec2(railX, railY), ImGuiCond.Always, ImVec2(0, 0));
        igSetNextWindowSize(compact ? ImVec2(0, 0) : ImVec2(railW, railH), ImGuiCond.Always);

        if (igBegin("nijikan_shell_nav###nijikan_shell_nav", null, flags)) {
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
            drawUtilityButton("\ue2c8", "Models", {
                const TFD_Filter[] filters = [{ ["*.inp"], "nijilive Puppet (*.inp)" }];
                string parentWindow = "";
                version(linux) {
                    static if (is(typeof(&getWindowHandle))) {
                        parentWindow = getWindowHandle();
                    }
                }
                string file = insShowOpenDialog(filters, _("Open..."), parentWindow);
                if (file) loadModels([file]);
            }, compact);

            foreach(index, item; NAV_ITEMS) {
                if (compact) uiImSameLine();
                drawRailButton(item, compact);
            }

            if (compact) {
                uiImSameLine();
                drawTrashButton(true);
            } else {
                drawTrashButton(false);
            }
        }
        igEnd();
    }

    void drawOverlayHost() {
        if (!showUI || !overlayOpen) return;
        Panel active = panelFor(activePanel);
        ToolWindow activeWindow = toolWindowFor(activePanel);
        bool customTracking = activePanel == ActivePanelId.Tracking;
        bool customView = activePanel == ActivePanelId.View;
        if (active is null && activeWindow is null && !customTracking && !customView) return;

        auto compact = navSurfaceMode() == NavSurfaceMode.CompactBar;
        ImGuiWindowFlags flags = ImGuiWindowFlags.NoCollapse
            | ImGuiWindowFlags.NoTitleBar
            | ImGuiWindowFlags.NoSavedSettings
            | ImGuiWindowFlags.NoNavFocus;

        bool parameterOverlay = usesParameterOverlay(activePanel);
        immutable float compactBarHeight = 64.0f;
        immutable float compactOverlayGap = 10.0f;
        float overlayW;
        float overlayH;
        float overlayX;
        float overlayY;
        if (parameterOverlay && !compact) {
            overlayW = min(max(cast(float)width * 0.22f, 280.0f), 360.0f);
            overlayH = cast(float)height - (RAIL_TOP + RAIL_BOTTOM);
            overlayX = OUTER_GAP + (navExpanded ? RAIL_EXPANDED_WIDTH : RAIL_COLLAPSED_WIDTH) + 10.0f;
            overlayY = RAIL_TOP;
        } else if (parameterOverlay && compact) {
            overlayW = min(cast(float)width - 28.0f, 320.0f);
            overlayX = OUTER_GAP;
            overlayY = RAIL_TOP + compactBarHeight + compactOverlayGap;
            overlayH = cast(float)height - overlayY - OUTER_GAP;
        } else {
            overlayW = compact
                ? min(cast(float)width - 32.0f, 520.0f)
                : min(cast(float)width * 0.62f, 760.0f);
            overlayH = compact
                ? min(cast(float)height - 120.0f, 620.0f)
                : min(cast(float)height * 0.76f, 760.0f);
            overlayX = (cast(float)width - overlayW) * 0.5f;
            overlayY = (cast(float)height - overlayH) * 0.5f;
        }

        {
            drawBlurBackdrop(overlayX, overlayY, overlayW, overlayH, 18.0f);
            drawSoftWindowShadow(overlayX, overlayY, overlayW, overlayH, 18.0f);
        }

        igPushStyleColor(ImGuiCol.WindowBg, ImVec4(OVERLAY_BG.x, OVERLAY_BG.y, OVERLAY_BG.z, OVERLAY_BG.w));
        igPushStyleColor(ImGuiCol.Border, ImVec4(OVERLAY_BORDER.x, OVERLAY_BORDER.y, OVERLAY_BORDER.z, OVERLAY_BORDER.w));
        igPushStyleVar(ImGuiStyleVar.WindowBorderSize, 1.0f);
        igPushStyleVar(ImGuiStyleVar.WindowPadding, parameterOverlay ? ImVec2(0, 0) : ImVec2(16, 14));
        igPushStyleVar(ImGuiStyleVar.WindowRounding, 13.0f);
        scope(exit) {
            igPopStyleVar(3);
            igPopStyleColor(2);
        }

        igSetNextWindowPos(ImVec2(overlayX, overlayY), ImGuiCond.Always, ImVec2(0, 0));
        igSetNextWindowSize(ImVec2(overlayW, overlayH), ImGuiCond.Always);

        string title = panelTitle(activePanel, active, activeWindow);
        string windowTitle = "%s###nijikan_overlay_host".format(title);

        if (!parameterOverlay && igIsMouseClicked(ImGuiMouseButton.Left)) {
            ImVec2 mousePos;
            igGetMousePos(&mousePos);
            auto overlayRect = ImRect_ImRect(overlayX, overlayY, overlayX + overlayW, overlayY + overlayH);
            scope(exit) ImRect_destroy(overlayRect);
            if (!ImRect_Contains(overlayRect, mousePos)) {
                overlayOpen = false;
                inSettingsSet("ui.overlayOpen", overlayOpen);
                return;
            }
        }

        if (igBegin(windowTitle.toStringz, null, flags)) {
            if (parameterOverlay) {
                igSetCursorPos(ImVec2(16, 14));
            }
            igPushStyleVar(ImGuiStyleVar.FramePadding, ImVec2(8, 6));
            scope(exit) igPopStyleVar();

            ImVec2 headerStart;
            igGetCursorScreenPos(&headerStart);
            auto drawList = igGetWindowDrawList();
            auto font = igGetFont();
            float headerIconSize = igGetFontSize() * NAV_ICON_SCALE;
            string headerIcon = iconFor(activePanel);
            ImVec2 iconSize;
            igCalcTextSize(&iconSize, headerIcon.toStringz);
            iconSize.x *= NAV_ICON_SCALE;
            iconSize.y *= NAV_ICON_SCALE;
            ImVec2 iconPos = ImVec2(headerStart.x, headerStart.y + max(0.0f, (20.0f - iconSize.y) * 0.5f));
            ImDrawList_AddText(drawList, font, headerIconSize, iconPos, igColorConvertFloat4ToU32(ImVec4(ACCENT.x, ACCENT.y, ACCENT.z, ACCENT.w)), headerIcon.toStringz);
            igSetCursorScreenPos(ImVec2(headerStart.x + 30.0f, headerStart.y));
            uiImLabel(title);
            ImVec2 closePos = ImVec2(overlayX + overlayW - 44.0f, headerStart.y);
            if (drawIconOnlyEntry("overlay_close", "\ue5cd", closePos, 28.0f, vec4(0.12f, 0.16f, 0.23f, 0.90f), false)) {
                overlayOpen = false;
                inSettingsSet("ui.overlayOpen", overlayOpen);
            }
            igSetCursorScreenPos(ImVec2(headerStart.x, headerStart.y + 28.0f));

            if (parameterOverlay) {
                igSetCursorPosX(0);
            }
            igPushStyleColor(ImGuiCol.ChildBg, ImVec4(0, 0, 0, 0));
            igPushStyleVar(ImGuiStyleVar.ChildBorderSize, 0.0f);
            scope(exit) {
                igPopStyleVar();
                igPopStyleColor();
            }
            immutable bool hasOverlayFooter = customTracking || customView;
            immutable float overlayFooterHeight = hasOverlayFooter ? 46.0f : 0.0f;
            if (uiImBeginChild("nijikan_overlay_body###nijikan_overlay_body", vec2(0, -overlayFooterHeight), false)) {
                if (customTracking) {
                    settingWindow.renderTrackingSettingsSection();
                    uiImSeperator();
                    spaceEditor.renderEditorSection(false);
                } else if (customView) {
                    if (active !is null) {
                        active.updateEmbedded();
                    }
                    uiImSeperator();
                    settingWindow.renderRenderingSettingsSection();
                } else if (active !is null) {
                    active.updateEmbedded();
                } else {
                    activeWindow.updateEmbedded();
                }
            }
            uiImEndChild();
            if (hasOverlayFooter) {
                igDummy(ImVec2(0, 6));
                igSetCursorPosX(max(0.0f, uiImAvailableSpace().x - 72.0f));
                if (uiImButton(__("Apply"), vec2(64, 0))) {
                    if (customTracking) {
                        settingWindow.applyTrackingSettings();
                        spaceEditor.applyChanges();
                    } else if (customView) {
                        settingWindow.applyRenderingSettings();
                    }
                }
            }
        }
        igEnd();
    }

    void loadModels(string[] args) {
        foreach(arg; args) {
            string filebase = arg.baseName;

            switch(filebase.extension.toLower) {                
                case ".png", ".tga", ".jpeg", ".jpg":
                    insScene.addPuppet(arg, neLoadModelFromImage(arg));
                    break;

                case ".inp", ".inx":
                    import std.file : exists;
                    if (!exists(arg)) continue;
                    try {
                        insScene.addPuppet(arg, inLoadPuppet(arg));
                    } catch(Exception ex) {
                        uiImDialog(__("Error"), "Could not load %s, %s".format(arg, ex.msg));
                    }
                    break;
                default:
                    uiImDialog(__("Error"), _("Could not load %s, unsupported file format.").format(arg));
                    break;
            }
        }
    }

protected:
    override
    void onEarlyUpdate() {
        insScene.update();
        insSendFrame();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        oglDrawScene(vec4(0, 0, width, height));
        updateOverlayBlurTexture();
    }

    override
    void onUpdate() {
        syncPanelVisibility();
        updateNavFadeState();
        if (!inInputIsnijiui()) {
            if (inInputMouseDoubleClicked(MouseButton.Left)) this.showUI = !showUI;
            insScene.interact();

            if (getDraggedFiles().length > 0) {
                loadModels(getDraggedFiles());
            }
        }

        drawNavigationShell();
        drawOverlayHost();

        version(linux) dpUpdate();
    }

    override
    void onResized(int w, int h) {
        inSetViewport(w, h);
        inSettingsSet("window", InochiWindowSettings(width, height));
        super.onResized(w, h);
    }

    override
    void onClosed() {
        destroyOverlayBlurResources();
    }
public:

    /**
        Construct nijiexpose
    */
    this(string[] args) {
        InochiWindowSettings windowSettings = 
            inSettingsGet!InochiWindowSettings("window", InochiWindowSettings(1024, 1024));

        import nijiexpose.ver;

        int throttlingRate = inSettingsGet!(int)("throttlingRate", 1);

        super("nijiexpose %s".format(INS_VERSION), windowSettings.width, windowSettings.height, throttlingRate);
        
        // Initialize nijilive
        inInit(&inGetTime);
        neSetStyle();
        inSetPanelsSuspended(true);
        inSetViewport(windowSettings.width, windowSettings.height);
        settingWindow = new SettingWindow();
        spaceEditor = new SpaceEditor();
        lastPointerPos = inInputMousePosition();
        lastNavInteractionAt = inGetTime();
        activePanel = sanitizeActivePanel(inSettingsGet!(int)("ui.activePanel", cast(int)ActivePanelId.Parameters));
        overlayOpen = inSettingsGet!bool("ui.overlayOpen", true);

        // Preload any specified models
        loadModels(args);

        // uiImDialog(
        //     __("nijiexpose"), 
        //     _("THIS IS BETA SOFTWARE\n\nThis software is incomplete, please lower your expectations."), 
        //     DialogLevel.Warning
        // );

        inGetCamera().scale = vec2(0.5);

        version (InBranding) {
            logo = new Texture(ShallowTexture(cast(ubyte[])import("tex/logo.png")));
            auto tex = ShallowTexture(cast(ubyte[])import("icon_x256.png"));
            setIcon(tex);
        }

        version(linux) dpInit();
    }
}
