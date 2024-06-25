module nijiexpose.windows.utils;
import bindbc.imgui;
import nijiui.widgets.input;


void neSetStyle() {
    auto style = igGetStyle();
    // 文字色を黒に変更
    style.Colors[ImGuiCol.Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);

    // 背景色の設定
    style.Colors[ImGuiCol.WindowBg] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    style.Colors[ImGuiCol.PopupBg] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    style.Colors[ImGuiCol.MenuBarBg] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);

    // ウィンドウのタイトルバーの背景色を変更
    style.Colors[ImGuiCol.TitleBg] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    style.Colors[ImGuiCol.TitleBgActive] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    style.Colors[ImGuiCol.TitleBgCollapsed] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);

    // プログレスバーの色を変更
    style.Colors[ImGuiCol.PlotHistogram] = ImVec4(0.95f, 0.47f, 0.08f, 1.00f); // オレンジ色に設定
    style.Colors[ImGuiCol.PlotHistogramHovered] = ImVec4(0.85f, 0.37f, 0.00f, 1.00f); // ホバー時は濃いオレンジ色に設定

    // ボタンのスタイル設定
    style.Colors[ImGuiCol.Button] = ImVec4(0.70f, 0.25f, 0.00f, 1.00f); // さらに濃いオレンジ色に設定
    style.Colors[ImGuiCol.ButtonHovered] = ImVec4(0.75f, 0.30f, 0.05f, 1.00f); // ホバー時もさらに濃いオレンジ色に設定
    style.Colors[ImGuiCol.ButtonActive] = ImVec4(0.60f, 0.20f, 0.00f, 1.00f); // アクティブ時はさらに濃いオレンジ色に設定

    style.Colors[ImGuiCol.SliderGrabActive] = ImVec4(0.70f, 0.25f, 0.00f, 1.00f); // さらに濃いオレンジ色に設定
    style.Colors[ImGuiCol.SliderGrab] = ImVec4(0.95f, 0.47f, 0.08f, 1.00f); // オレンジ色に設定

    // チェックボックスのチェックマークの色をボタンと同じさらに濃いオレンジ色に変更
    style.Colors[ImGuiCol.CheckMark] = ImVec4(0.70f, 0.25f, 0.00f, 1.00f);

    // InputTextの背景色を白に変更
    style.Colors[ImGuiCol.FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol.FrameBgHovered] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    style.Colors[ImGuiCol.FrameBgActive] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);

    // タブの背景色を変更
    style.Colors[ImGuiCol.Tab] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    style.Colors[ImGuiCol.TabHovered] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    style.Colors[ImGuiCol.TabActive] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    style.Colors[ImGuiCol.TabUnfocused] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    style.Colors[ImGuiCol.TabUnfocusedActive] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);

    // リストなどの選択行の背景色を変更
    style.Colors[ImGuiCol.Header] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
    style.Colors[ImGuiCol.HeaderHovered] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    style.Colors[ImGuiCol.HeaderActive] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);

    // スクロールバーの背景色をバックグラウンドと同じ色に設定
    style.Colors[ImGuiCol.ScrollbarBg] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);

    // スクロールバーの色を黄色に変更
    style.Colors[ImGuiCol.ScrollbarGrab] = ImVec4(1.00f, 0.92f, 0.23f, 1.00f);
    style.Colors[ImGuiCol.ScrollbarGrabHovered] = ImVec4(1.00f, 0.82f, 0.13f, 1.00f);
    style.Colors[ImGuiCol.ScrollbarGrabActive] = ImVec4(1.00f, 0.72f, 0.03f, 1.00f);

    // 他のスタイルパラメータを設定
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.WindowRounding = 4.0f;
    style.ChildRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.TabRounding = 4.0f;

    // ボタンとテキスト間のマージンを設定
    style.FramePadding = ImVec2(12.0f, 8.0f); // ボタン内のテキストとボタンの間のパディング
    style.ItemSpacing = ImVec2(8.0f, 4.0f); // ボタン同士のスペーシング
    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f); // ボタン内の要素間のスペーシング
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f); // ボタンのテキストを中央揃え
    style.DisplaySafeAreaPadding = ImVec2(10.0f, 10.0f); // ボタンの周りにマージンを追加

    uiSetColor(UIColor.ButtonTextColor, ImVec4(1, 1, 1, 1));
}