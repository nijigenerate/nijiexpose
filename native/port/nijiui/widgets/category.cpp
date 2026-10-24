// nijiui/widgets/category.d. Copyright 2022 Inochi2D, 2024 nijigenerate. BSD-2-Clause.
#include "nijiui/widgets/category.hpp"
#include <imgui_internal.h>
#include <array>
#include <vector>
#include <algorithm>
namespace nijiui {
#if IMGUI_VERSION_NUM >= 18970
constexpr auto categoryOverlapFlag = ImGuiTreeNodeFlags_AllowOverlap;
#else
constexpr auto categoryOverlapFlag = ImGuiTreeNodeFlags_AllowItemOverlap;
#endif

namespace {
    struct CategoryData {
        bool open;
        bool badColor;
        bool isDark;
        ImVec4 contentBounds;
        UiImCategoryFlags flags;
    };

    thread_local std::vector<std::array<ImVec4, ImGuiCol_COUNT>> colorSchemeStack;

    bool uiImGetDarkMode() {
        auto bg = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
        float h, s, v;
        ImGui::ColorConvertRGBtoHSV(bg.x, bg.y, bg.z, h, s, v);
        return v <= 0.5f;
    }

    void uiImApplyDarkColorScheme(ImGuiStyle& style) {
        style.Colors[ImGuiCol_Text]                 = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_TextDisabled]         = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        style.Colors[ImGuiCol_WindowBg]             = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
        style.Colors[ImGuiCol_ChildBg]              = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_PopupBg]              = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
        style.Colors[ImGuiCol_Border]               = ImVec4(0.00f, 0.00f, 0.00f, 0.16f);
        style.Colors[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.16f);
        style.Colors[ImGuiCol_FrameBg]              = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        style.Colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.15f, 0.15f, 0.15f, 0.40f);
        style.Colors[ImGuiCol_FrameBgActive]        = ImVec4(0.22f, 0.22f, 0.22f, 0.67f);
        style.Colors[ImGuiCol_Button]               = ImVec4(0.39f, 0.39f, 0.39f, 0.40f);
        style.Colors[ImGuiCol_ButtonHovered]        = ImVec4(0.44f, 0.44f, 0.44f, 1.00f);
        style.Colors[ImGuiCol_ButtonActive]         = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        style.Colors[ImGuiCol_Header]               = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        style.Colors[ImGuiCol_HeaderHovered]        = ImVec4(0.28f, 0.28f, 0.28f, 0.80f);
        style.Colors[ImGuiCol_HeaderActive]         = ImVec4(0.44f, 0.44f, 0.44f, 1.00f);
        style.Colors[ImGuiCol_Separator]            = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_CheckMark]            = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        style.Colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    }

    void uiImApplyLightColorScheme(ImGuiStyle& style) {
        style.Colors[ImGuiCol_Text]                 = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        style.Colors[ImGuiCol_TextDisabled]         = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
        style.Colors[ImGuiCol_WindowBg]             = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
        style.Colors[ImGuiCol_ChildBg]              = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_PopupBg]              = ImVec4(0.98f, 0.98f, 0.98f, 0.98f);
        style.Colors[ImGuiCol_Border]               = ImVec4(0.00f, 0.00f, 0.00f, 0.14f);
        style.Colors[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.08f);
        style.Colors[ImGuiCol_FrameBg]              = ImVec4(0.97f, 0.97f, 0.97f, 1.00f);
        style.Colors[ImGuiCol_FrameBgHovered]       = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_FrameBgActive]        = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        style.Colors[ImGuiCol_Button]               = ImVec4(0.98f, 0.98f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_ButtonHovered]        = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_ButtonActive]         = ImVec4(0.88f, 0.88f, 0.88f, 1.00f);
        style.Colors[ImGuiCol_Header]               = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
        style.Colors[ImGuiCol_HeaderHovered]        = ImVec4(0.98f, 0.98f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_HeaderActive]         = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        style.Colors[ImGuiCol_Separator]            = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
        style.Colors[ImGuiCol_CheckMark]            = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.68f, 0.68f, 0.68f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.58f, 0.58f, 0.58f, 1.00f);
    }

    void uiImPushDarkColorScheme() {
        auto* ctx = ImGui::GetCurrentContext();
        std::array<ImVec4, ImGuiCol_COUNT> saved;
        std::copy_n(ctx->Style.Colors, ImGuiCol_COUNT, saved.begin());
        colorSchemeStack.push_back(saved);
        uiImApplyDarkColorScheme(ctx->Style);
    }

    void uiImPushLightColorScheme() {
        auto* ctx = ImGui::GetCurrentContext();
        std::array<ImVec4, ImGuiCol_COUNT> saved;
        std::copy_n(ctx->Style.Colors, ImGuiCol_COUNT, saved.begin());
        colorSchemeStack.push_back(saved);
        uiImApplyLightColorScheme(ctx->Style);
    }

    void uiImPopColorScheme() {
        if (colorSchemeStack.empty()) return;
        auto* ctx = ImGui::GetCurrentContext();
        std::copy(colorSchemeStack.back().begin(), colorSchemeStack.back().end(), ctx->Style.Colors);
        colorSchemeStack.pop_back();
    }

    void uiImGetCategoryColors(ImVec4 color, ImVec4& hoverColor, ImVec4& activeColor, ImVec4& bgColor, ImVec4& shadowColor, ImVec4& textColor, bool& isDark) {
        
        // First get HSV version of color
        float h, s, v;
        hoverColor = color;
        activeColor = color;
        shadowColor = color;
        bgColor = color;

        // Assume all colors are dark and invert them if we're in light mode
        ImGui::ColorConvertRGBtoHSV(color.x, color.y, color.z, h, s, v);
        isDark = v <= 0.5;
        if (!isDark) textColor = ImVec4(0, 0, 0, 1);
        else textColor = ImVec4(1, 1, 1, 1);

        // Then darken/lighten it the first time for active color
        s -= 0.075*s;
        if (v > 0.5) v -= 0.05;
        else v += 0.15;
        ImGui::ColorConvertHSVtoRGB(h, s, v, activeColor.x, activeColor.y, activeColor.z);

        // Then darken/lighten it the second time for hover color
        s -= 0.075*s;
        if (v > 0.5) v -= 0.05;
        else v += 0.15;
        ImGui::ColorConvertHSVtoRGB(h, s, v, hoverColor.x, hoverColor.y, hoverColor.z);
        
        // Then darken it for bg color
        ImGui::ColorConvertRGBtoHSV(color.x, color.y, color.z, h, s, v);
        s *= 0.90;
        if (v <= 0.15) v = 0.15;
        else v *= 0.80;
        ImGui::ColorConvertHSVtoRGB(h, s, v, bgColor.x, bgColor.y, bgColor.z);
    
        // Finally the shadow color
        ImGui::ColorConvertRGBtoHSV(color.x, color.y, color.z, h, s, v);
        s *= 0.85;
        v *= 0.75;
        ImGui::ColorConvertHSVtoRGB(h, s, v, shadowColor.x, shadowColor.y, shadowColor.z);
        shadowColor.w *= 2;
    }
}



/**
    Begins a category using slightly darkened versions of the main UI colors
    
    Remember to call uiImEndCategory after!
*/
bool uiImBeginCategory(const char* title, UiImCategoryFlags flags, void (*callback)(float, float)) {
    ImVec4 col = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
//    col = ImVec4(col.x-0.025, col.y-0.025, col.z-0.025, col.w);
    return uiImBeginCategory(title, col, flags, callback);
}

/**
    Begins a category using the defined color

    Remember to call uiImEndCategory after!
*/
bool uiImBeginCategory(const char* title, ImVec4 color, UiImCategoryFlags flags, void (*callback)(float, float)) {

    // Preserve incoming alpha so embedded overlays can keep categories translucent.
    float baseAlpha = std::clamp(color.w, 0.0f, 1.0f);
    color.x = static_cast<float>(std::clamp<double>(color.x, 0.15, 1));
    color.y = static_cast<float>(std::clamp<double>(color.y, 0.15, 1));
    color.z = static_cast<float>(std::clamp<double>(color.z, 0.15, 1));
    
    // Calculate colors for category
    ImVec4 hoverColor;
    ImVec4 activeColor;
    ImVec4 shadowColor;
    ImVec4 bgColor;
    ImVec4 textColor;
    bool isDark;
    uiImGetCategoryColors(color, hoverColor, activeColor, bgColor, shadowColor, textColor, isDark);
    hoverColor.w = baseAlpha;
    activeColor.w = baseAlpha;
    bgColor = ImVec4(color.x, color.y, color.z, std::max(0.08f, baseAlpha * 0.14f));
    shadowColor.w = std::max(0.02f, baseAlpha * 0.08f);

    // Push ID of our category
    ImGui::PushID(title);
    auto* storage = ImGui::GetStateStorage();
    auto id = ImGui::GetID("CATEGORYDATA");
    auto* window = ImGui::GetCurrentWindow();    

    // The fun stuff starts
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, hoverColor);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, activeColor);
    ImGui::PushStyleColor(ImGuiCol_Text, textColor);
    CategoryData* data = static_cast<CategoryData*>(storage->GetVoidPtr(id));
    if (!data) {
        data = static_cast<CategoryData*>(ImGui::MemAlloc(sizeof(CategoryData)));
        data->open = false;
        data->contentBounds = ImVec4(
            0, 0, 0, 0
        );
        storage->SetVoidPtr(id, data);
    }

    // Calculate some values for drawing our background color.
    data->flags = flags;
    data->badColor = isDark != uiImGetDarkMode();
    data->isDark = isDark;
    data->contentBounds.x = ImGui::GetCursorPosX();
    data->contentBounds.y = ImGui::GetCursorPosY();
    data->contentBounds.z = uiImAvailableSpace().x;

    ImVec2 cursor = ImGui::GetCursorScreenPos();

    // Draw background color
    ImGui::GetWindowDrawList()->AddRectFilled(
        ImVec2(cursor.x, cursor.y),
        ImVec2(cursor.x+data->contentBounds.z, cursor.y+data->contentBounds.w+1),
        ImGui::GetColorU32(bgColor), 6.0
    );
    
    // Draw "shadow" underneath
    ImGui::GetWindowDrawList()->AddRectFilled(
        ImVec2(cursor.x, cursor.y+data->contentBounds.w-1),
        ImVec2(cursor.x+data->contentBounds.z, cursor.y+data->contentBounds.w+1),
        ImGui::GetColorU32(shadowColor), 6.0
    );

    // Our fancy tree node which will be used to open/close the category.
    uiImDummy(vec2(0, 2));

    float paddingX = ImGui::GetStyle().WindowPadding.x/2;
    window->ContentRegionRect.Min.x -= paddingX;
    window->WorkRect.Min.x -= paddingX;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX()+paddingX);

    if (data->open) {
        ImVec2 newCursor = ImGui::GetCursorScreenPos();

        float diffY = newCursor.y-cursor.y;

        // Draw lighter fill color for contents when open
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(cursor.x+1, cursor.y+1),
            ImVec2(cursor.x+data->contentBounds.z-1, newCursor.y+data->contentBounds.w-diffY),
            ImGui::GetColorU32(ImVec4(color.x, color.y, color.z, std::max(0.12f, baseAlpha * 0.30f))), 6.0
        );
    }

    if (callback != nullptr)
        callback(data->contentBounds.z, data->contentBounds.w);

    window->ContentRegionRect.Min.x += paddingX;
    window->WorkRect.Min.x += paddingX;

    if ((data->flags & UiImCategoryFlags::NoCollapse) == UiImCategoryFlags::NoCollapse) {
        data->open = true;
        ImGui::Indent();
            ImGui::Text(title);
        ImGui::Unindent();
    } else {
        bool defaultClosed = (data->flags & UiImCategoryFlags::DefaultClosed) == UiImCategoryFlags::DefaultClosed;
        data->open = ImGui::TreeNodeEx(title, defaultClosed ? 
            ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth | categoryOverlapFlag:
            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth | categoryOverlapFlag
        );
    }

    // NOTE: We use these instead of uiImDummy suiIme otherwise you can't drag
    // the tree node via drag/drop.
    ImGui::SetCursorPosY(ImGui::GetCursorPosY()+2);

    if (data->open) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY()+2);
        ImGui::Indent();
    }


    ImGui::PopStyleColor(3);

    // We force our childrens' content to fit better within ourselves
    // This gets undone after uiImEndCategory is called.
    float indentSpacing = ImGui::GetStyle().IndentSpacing;
    window->ContentRegionRect.Min.x -= indentSpacing;
    window->WorkRect.Min.x -= indentSpacing;

    if (data->badColor && data->isDark) uiImPushDarkColorScheme();
    else if (data->badColor && !data->isDark) uiImPushLightColorScheme();

    return data->open;
}

/**
    Ends a category
*/
void uiImEndCategory() {
    auto* window = ImGui::GetCurrentWindow();

    // Undo our fancy content fit math.
    // This needs to be here otherwise the usable size of the child
    // window will continue to shrink
    window->ContentRegionRect.Min.x += ImGui::GetStyle().IndentSpacing;
    window->WorkRect.Min.x += ImGui::GetStyle().IndentSpacing;

    auto* storage = ImGui::GetStateStorage();
    auto id = ImGui::GetID("CATEGORYDATA");
    if (CategoryData* data = static_cast<CategoryData*>(storage->GetVoidPtr(id))) {
        if (data->badColor) uiImPopColorScheme();
        if (data->open) {
            ImGui::Unindent();
            uiImDummy(vec2(0, 2));
        }

        data->contentBounds.w = ImGui::GetCursorPosY()-data->contentBounds.y;

        ImGui::Spacing();
        ImGui::Spacing();
    }

    ImGui::PopID();
}

}
