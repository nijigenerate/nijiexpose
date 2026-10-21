// Direct translation of dlang_archive/source/nijiexpose/windows/utils.d.
#include "nijiexpose/windows/utils.hpp"
#include "nijiui/widgets/colors.hpp"
#include <imgui.h>
namespace nijiexpose {


void neSetStyle() {
    auto* style = &ImGui::GetStyle();
    // 文字色を黒に変更
    style->Colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);

    // 背景色の設定
    style->Colors[ImGuiCol_WindowBg] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    style->Colors[ImGuiCol_PopupBg] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);

    // ウィンドウのタイトルバーの背景色を変更
    style->Colors[ImGuiCol_TitleBg] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);

    // プログレスバーの色を変更
    style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.95f, 0.47f, 0.08f, 1.00f); // オレンジ色に設定
    style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.85f, 0.37f, 0.00f, 1.00f); // ホバー時は濃いオレンジ色に設定

    // ボタンのスタイル設定
    style->Colors[ImGuiCol_Button] = ImVec4(0.70f, 0.25f, 0.00f, 1.00f); // さらに濃いオレンジ色に設定
    style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.75f, 0.30f, 0.05f, 1.00f); // ホバー時もさらに濃いオレンジ色に設定
    style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.60f, 0.20f, 0.00f, 1.00f); // アクティブ時はさらに濃いオレンジ色に設定

    style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.70f, 0.25f, 0.00f, 1.00f); // さらに濃いオレンジ色に設定
    style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.95f, 0.47f, 0.08f, 1.00f); // オレンジ色に設定

    // チェックボックスのチェックマークの色をボタンと同じさらに濃いオレンジ色に変更
    style->Colors[ImGuiCol_CheckMark] = ImVec4(0.70f, 0.25f, 0.00f, 1.00f);

    // InputTextの背景色を白に変更
    style->Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);

    // タブの背景色を変更
    style->Colors[ImGuiCol_Tab] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    style->Colors[ImGuiCol_TabHovered] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
#if IMGUI_VERSION_NUM >= 19090
    style->Colors[ImGuiCol_TabSelected] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    style->Colors[ImGuiCol_TabDimmed] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    style->Colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
#else
    style->Colors[ImGuiCol_TabActive] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    style->Colors[ImGuiCol_TabUnfocused] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    style->Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
#endif

    // リストなどの選択行の背景色を変更
    style->Colors[ImGuiCol_Header] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
    style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);

    // スクロールバーの背景色をバックグラウンドと同じ色に設定
    style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.95f, 0.94f, 0.92f, 0.90f);

    // スクロールバーの色を黄色に変更
    style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.67f, 0.63f, 0.59f, 0.95f);
    style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.58f, 0.53f, 0.48f, 0.98f);
    style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.48f, 0.42f, 0.37f, 1.00f);

    // 他のスタイルパラメータを設定
    style->FrameRounding = 4.0f;
    style->GrabRounding = 4.0f;
    style->WindowRounding = 4.0f;
    style->ChildRounding = 4.0f;
    style->PopupRounding = 4.0f;
    style->ScrollbarRounding = 4.0f;
    style->TabRounding = 4.0f;

    // ボタンとテキスト間のマージンを設定
    style->FramePadding = ImVec2(12.0f, 8.0f); // ボタン内のテキストとボタンの間のパディング
    style->ItemSpacing = ImVec2(8.0f, 4.0f); // ボタン同士のスペーシング
    style->ItemInnerSpacing = ImVec2(8.0f, 6.0f); // ボタン内の要素間のスペーシング
    style->ButtonTextAlign = ImVec2(0.5f, 0.5f); // ボタンのテキストを中央揃え
    style->DisplaySafeAreaPadding = ImVec2(10.0f, 10.0f); // ボタンの周りにマージンを追加

    nijiui::uiSetColor(nijiui::UIColor::ButtonTextColor, ImVec4(1, 1, 1, 1));
}

}

