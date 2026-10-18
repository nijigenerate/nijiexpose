# Windows実装移植の対応台帳

**アプリ全体の移植・機能確認は未完了。** 下表はモジュール別の進捗。開発用SDL3ハーネスの起動成功を製品機能の移植完了とは数えない。gettext・Boost.JSON・ImGui新版の依存内部差異はユーザー承認済み。

| D参照元 | C++予定配置 | 状態 |
|---|---|---|
| `source/app.d` | `native/port/app.cpp / .hpp` | 未移植・未検証 |
| `source/nijiexpose/animation/package.d` | `native/port/nijiexpose/animation/package.cpp / .hpp` | 関数を移植。元Dの60状態遷移と一致。Scene更新へ接続。 |
| `source/nijiexpose/framesend.d` | `native/port/nijiexpose/framesend.cpp / .hpp` | Windows関数を移植。元D配布Spout DLLでGPUテクスチャ共有・サイズ/形式・終了を実機確認。製品ループ接続は未完了 |
| `source/nijiexpose/i18n.d` | `native/port/nijiexpose/i18n.cpp / .hpp` | 関数移植済み。gettext・設定・言語切替の統合テスト成功。製品UIへの接続は未完了 |
| `source/nijiexpose/io/image.d` | `native/port/nijiexpose/io/image.cpp / .hpp` | 関数移植。1～4チャネルPNGのデコード・切抜き・premultiply・GPU読戻しを元Dと全バイト比較。JPEG/BMP/TGA全域のデコード互換性は未保証 |
| `source/nijiexpose/io/package.d` | `native/port/nijiexpose/io/package.cpp / .hpp` | 未移植・未検証 |
| `source/nijiexpose/log.d` | `native/port/nijiexpose/log.cpp / .hpp` | Windows実装あり。初期化・追記・ローテーション等5シナリオをD比較。汎用Phobos format全域の互換性は未保証 |
| `source/nijiexpose/panels/animations.d` | `native/port/nijiexpose/panels/animations.cpp / .hpp` | 未移植・未検証 |
| `source/nijiexpose/panels/blends.d` | `native/port/nijiexpose/panels/blends.cpp / .hpp` | 未移植・未検証 |
| `source/nijiexpose/panels/plugin.d` | `native/port/nijiexpose/panels/plugin.cpp / .hpp` | 未移植・未検証 |
| `source/nijiexpose/panels/scene.d` | `native/port/nijiexpose/panels/scene.cpp / .hpp` | 未移植・未検証 |
| `source/nijiexpose/panels/tracking.d` | `native/port/nijiexpose/panels/tracking.cpp / .hpp` | 未移植・未検証 |
| `source/nijiexpose/plugins/api/base.d` | `native/port/nijiexpose/plugins/package.cpp / .hpp` | Windows処理を移植し統合テスト成功。元Dのrequire/エラー経路はアクセス違反を切り分け中で、等価性未確認。 |
| `source/nijiexpose/plugins/api/package.d` | `native/port/nijiexpose/plugins/package.cpp / .hpp` | Windows処理を移植し統合テスト成功。元Dのrequire/エラー経路はアクセス違反を切り分け中で、等価性未確認。 |
| `source/nijiexpose/plugins/api/scene.d` | `native/port/nijiexpose/plugins/package.cpp / .hpp` | Windows処理を移植し統合テスト成功。元Dのrequire/エラー経路はアクセス違反を切り分け中で、等価性未確認。 |
| `source/nijiexpose/plugins/api/ui.d` | `native/port/nijiexpose/plugins/package.cpp / .hpp` | Windows処理を移植し統合テスト成功。元Dのrequire/エラー経路はアクセス違反を切り分け中で、等価性未確認。 |
| `source/nijiexpose/plugins/package.d` | `native/port/nijiexpose/plugins/package.cpp / .hpp` | Windows処理を移植し統合テスト成功。元Dのrequire/エラー経路はアクセス違反を切り分け中で、等価性未確認。 |
| `source/nijiexpose/plugins/plugin.d` | `native/port/nijiexpose/plugins/package.cpp / .hpp` | Windows処理を移植し統合テスト成功。元Dのrequire/エラー経路はアクセス違反を切り分け中で、等価性未確認。 |
| `source/nijiexpose/render/spritebatch.d` | `native/port/nijiexpose/render/spritebatch.cpp / .hpp` | 通常描画経路を移植。回転/反転12ケースのGPU頂点ビット列・描画像が元Dと一致。元Dで未定義currentFboTexを使うflush!trueは未実装 |
| `source/nijiexpose/scene/package.d` | `native/port/nijiexpose/scene/package.cpp / .hpp` | Scene/SceneItemを移植。保存・読込・親子付け・解除・更新・Sleep/FPSの実モデル統合確認。操作UIと元D全経路比較は未完了。 |
| `source/nijiexpose/tracking/compoundbinding.d` | `native/port/nijiexpose/tracking/compoundbinding.cpp / .hpp` | 式・分岐を移植。元D比較あり。 |
| `source/nijiexpose/tracking/eventbinding.d` | `native/port/nijiexpose/tracking/eventbinding.cpp / .hpp` | 式・分岐を移植。元D比較あり。 |
| `source/nijiexpose/tracking/expr.d` | `native/port/nijiexpose/tracking/expr.cpp / .hpp` | Lua5.1で移植。110式と804ノイズ値を元Dと比較。乱数種生成は監査未完了。 |
| `source/nijiexpose/tracking/exprbinding.d` | `native/port/nijiexpose/tracking/exprbinding.cpp / .hpp` | 式・分岐を移植。元D比較あり。 |
| `source/nijiexpose/tracking/ext/exvmc.d` | `native/port/nijiexpose/tracking/ext/exvmc.cpp / .hpp` | OSC受信・状態更新を移植。実UDPのD比較成功。拡張形式の元D範囲外アクセスは保持。製品接続未完了 |
| `source/nijiexpose/tracking/ext/package.d` | `native/port/nijiexpose/tracking/ext/package.cpp / .hpp` | アプリ標準7受信部の生成・VMC特例を移植。生成/開始条件をテスト |
| `source/nijiexpose/tracking/package.d` | `native/port/nijiexpose/tracking/package.cpp / .hpp` | 全Binding型を移植。25記録を元D比較。JSON保存・再読込の統合確認あり。 |
| `source/nijiexpose/tracking/ratiobinding.d` | `native/port/nijiexpose/tracking/ratiobinding.cpp / .hpp` | 式・分岐を移植。元D比較あり。 |
| `source/nijiexpose/tracking/tracker.d` | `native/port/nijiexpose/tracking/tracker.cpp / .hpp` | 未移植・未検証 |
| `source/nijiexpose/tracking/vspace.d` | `native/port/nijiexpose/tracking/vspace.cpp / .hpp` | ゾーン選択/更新/平均/骨補間/JSON/settingsを移植。Dの更新順・20補間ケース一致。設定往復・開始失敗保持を確認。AAキー列挙順は未一致 |
| `source/nijiexpose/utils/launchpath.d` | `native/port/nijiexpose/utils/launchpath.cpp / .hpp` | Windows分岐を移植。文字列・ファイル判定21,898件でD比較成功 |
| `source/nijiexpose/utils/subprocess.d` | `native/port/nijiexpose/utils/subprocess.hpp` | Windows分岐を移植。5シナリオでD比較成功。rawData=trueは元D実装がコンパイルできず比較未完了 |
| `source/nijiexpose/windows/main.d` | `native/port/nijiexpose/windows/main.cpp / .hpp` | 未移植・未検証 |
| `source/nijiexpose/windows/package.d` | `native/port/nijiexpose/windows/package.cpp / .hpp` | 未移植・未検証 |
| `source/nijiexpose/windows/settings.d` | `native/port/nijiexpose/windows/settings.cpp / .hpp` | 未移植・未検証 |
| `source/nijiexpose/windows/spaceedit.d` | `native/port/nijiexpose/windows/spaceedit.cpp / .hpp` | 未移植・未検証 |
| `source/nijiexpose/windows/utils.d` | `native/port/nijiexpose/windows/utils.cpp / .hpp` | スタイル代入を移植。D値比較成功。ImGui新版のタブ色名称変更に対応 |

## 各モジュールを移植する際に追加する記録

- Dの型・関数名とC++の対応箇所。元ファイルのハッシュは `reference-sources.json` を参照。
- 分岐・式・状態代入・副作用・呼び出し順の対応レビュー。
- 言語/SDL3 APIによる必須差分の一覧と根拠。
- 同一入力のD/C++比較結果と、未検証条件・不一致。

## 別途対応が必要な依存実装

- nijiui/core/settings: Boost.JSONで実装。D実行結果を基に16ケース（既定値・null・型違い・配列・未知項目・保存/再読込）を確認。JSONの書式・項目順・構文エラー詳細の差は承認範囲。
- i18n: GNU gettext libintl動的リンク。MO配置の橋渡し・UTF-8翻訳・言語名取得・切替・クリアを統合テストで確認。
- nijiui/core/app, path・input・基本ウィジェット・入力ウィジェットにはC++実装あり。全nijiuiモジュールの移植は未完了。
- nijiui/panel, toolwindow, widgets/category: 関数実装あり。パネルの表示保存/停止・embedded例外復元、ツールstack/listと終了通知をImGuiフレームでテスト。categoryは元D関数本文の8フレーム描画バイト列比較と新版での開閉・レイアウト復元を確認。

- nijiui: D版UI/入力/状態管理を省略せず移植する。
- nijilive → nicxlive: 呼び出す関数の実装対応を監査する。nicxlive採用だけでは等価性の証拠にならない。
- OpenGL backend/shaders、facetrack/OSC/VMC、Lua/bindings、tinyfiledialogs、Spout、i18n、数値/文字列補助関数。
- 生成されるバージョン情報、resources、Windowsのログ/プロセス/梱包処理。

この台帳と参照ハッシュ検査は等価性を保証しない。実装構造の確認と差分検証を完了した単位だけ状態を更新する。

## 追加した描画・UI・ファイル処理（アプリ全体の完了を意味しない）

- `native/port/nijilive/opengl`: 元DのWindows OpenGL処理を移植。テクスチャ、シェーダ、共有SoA/IBO、Part、mask/dodge、DynamicComposite、後処理、差分集計、即時描画、debug、RenderQueueをnicxliveへ接続。テクスチャ144記録、Puppet/マスク/差分6記録をD比較。初期化・マスク・endSceneで元Dも発生させるGL_INVALID_OPERATIONを勝手に修正せず保持。
- `native/port/nicxlive/native_backend.hpp` と `cmake/NativeNicxlive.cmake`: 固定nicxliveのtexture/part/puppet翻訳単位を生成時に接続。取得した依存ソースは変更しない。元Dの入力チャネル、mipmap、lock/unlock、blend選択、コマンド実行を渡す。
- `native/port/nijiui/core/window/appwin.*`: win32-fullの実装分岐を移植。元構成で無効なSafeThrottling/UIViewportsは未対応。SDL3イベント・ウィンドウ更新順序・フォント倍率・日本語フォント・ダイアログの実描画を確認。新ImGuiのフォントAPIとSDL3イベントへの適応あり。IME/HiDPI/全ダイアログ操作は未確認。
- `native/port/nijilive/animation`: Animation/Lane/Keyframe/Player/Playbackを元Dから移植。5補間×snap有無×88入力の浮動小数点ビット列と70再生状態、計950記録を一致確認。Dのfloat累乗のWindows中間精度を保持。アプリ側AnimationControlも移植し、別途60状態遷移を比較。
- `native/port/nijilive/model_io.*`: INP/INXのテクスチャ先行読込、animationデコード/finalize、拡張情報読込と差替え/削除を実装。Dで生成した自作モデルの描画・再生と保存バイト保持を確認。モデル本体のnode/parameterデコードはnicxliveを使用しており、全ノード種・metadata・automation・全体保存のD実装対応は未完了。
- Spout DLLは元D依存bindbc-spout2 0.1.1のx64配布物と同一。SHA256をCMakeで固定し、ライセンスを同梱。
- D比較記録と入力のハッシュは `tests/reference-provenance.json`。比較の入力範囲外について同一性を保証したものではない。

## トラッキング受信と仮想空間の追加確認

- `native/port/ft` に標準7受信部（VMCはアプリ独自ExVMC）を実装。OpenSeeFace、Phiz OSC、LiveLinkFace、iFacialMocap、Facemotion3D、VTS形式は独立したfacetrack-d 0.8.0の現行コードから移植。
- `native_tracking_udp`: 元D/LDCによる実localhost UDP入力44時点との比較。キー・Unicode大文字変換・OSC入れ子bundle・骨と視線・ブレンドシェイプ・短いパケット・3Dなし・受信断カウントを照合。スマートフォン実機との接続や異常入力全域は未確認。
- Dの明示的stopは完了するが、その後のGC終了処理でWindowsアクセス違反を再現。参照用テストホストはstop完了後に`_Exit`を呼ぶ。これを終了処理の同等性確認とは数えない。
- VTS要求送信はテストでSO_BROADCAST未設定の拒否経路を使う。実ネットワークへの要求送信と相手機器の応答は未検証。
- `native_vspace_d_parity`: 元DクラスからJSONメソッドだけ除いた比較用ソースを生成。更新・選択・poll順と20ケースの骨補間ビット列を比較。`acos(float)`のfloat中間丸めを維持。
- `native_vspace_settings`: 7種類の生成、オプション有無による開始差、appName/address上書き、開始失敗したソースの保持、先頭ソースのfocus、保存/再読込・追記を確認。
- Dの連想配列とC++ mapのキー列挙順、例外の型/文言、GCとRAIIの寿命、異常な数値文字列、全NaN入力の互換性は引き続き監査が必要。アプリ全体の一致を保証するものではない。
- 追加後のWindows Release回帰テストは28/28成功（GPU 10件を含む）。

## Scene・Lua・式評価の追加確認

- `native/port/nijiexpose/scene`: Scene/SceneItemを元Dの順で移植。保存したbinding/animation拡張の再読込、生成の重複回避、矩形端の除外、メッシュ親子付け/解除、SceneのSleep移行とFPSを実モデルで確認。C++の配列参照の無効化を避け、D foreachと同じオブジェクト参照の値コピーを使用。Scene globalsはDに合わせthread_local。
- nicxlive coreはビルド木へコピーし、元Dの可変rootParts/drivers取得を追加。元の取得ソースは変更しない。Node/Drawableのbounds計算にはDとの相違を検出しており、対応監査は未完了。
- `plugins/package.cpp`: プラグイン環境、require、キャッシュ、イベント、無効化/再有効化、UI APIを移植。元Dがtonumberにもtostringを登録する点を維持。C++統合テストは成功。元D/LDCはrequireと失敗assert経路でアクセス違反が発生し、これらを比較成功とは記録しない。列挙・GC/RAII寿命・エラー文字列・Luaスタックの全経路比較も未完了。
- `tracking/expr.cpp`: 110式の結果とエラー文字列、2D OpenSimplexの804個のdoubleビット列を元D比較。乱数種のPRNG生成と全入力範囲の数学関数同等性は未確認。
- `tracking/package.cpp`: Ratio/Expression/Event/Compound/Externalの分岐を移植。元Dにあるfocus喪失時の上書き、空イベントの既定値、積の初期値0、plugin重みの累積維持を含め25記録を比較。
- `animation/package.cpp`: Load待機3フレーム、Idle/Sleep、しきい値/保持時間、停止優先順位の60記録を元D比較。
- 製品のapp/main/window、追跡設定UI、各パネル、trackerインストール/起動設定、ファイルダイアログ等は引き続き未完了。ハーネスを製品ビルドとして扱わない。


## 本体への接続と操作テスト（2026-10-18）

- `src/app/main.cpp` は開発用起動ハーネスから本体の入口へ変更。元 `source/app.d` の初期化・終了順を `application.cpp` に移植し、同じ経路を本体と操作テストで使用。
- `windows/main.cpp`, `settings.cpp`, `spaceedit.cpp` と `panels/tracking.cpp`, `animations.cpp`, `blends.cpp` を追加。元Dの分岐とUI処理を移植。SDL3/新版ImGuiのAPI差異に適応。旧ハーネスの `--smoke-test` をアプリの合否判定から削除。
- `tracking/tracker.cpp`: native Trackerを別スレッドで埋め込み、設定と入力を直接接続。`tracker_install.cpp`とTrackerのダウンロード・自動導入処理は削除済み。同梱taskを使用する。実カメラから実INP操作までの残検証は [T02](../NATIVE_CPP_TASKS.md) を参照。
- 通常の `ft::VMCAdaptor` も移植。画面から生成する通常VMCとアプリ設定読込時のExVMCをDと同様に区別。
- `native_application_actions`: 本体に検証用INPを読み込ませ、実際のマウス/キー入力を使用。モデル選択、式入力による100px移動、保存ボタンとINP再読込、アニメーション再生/一時停止/再開/Shift停止/Loop保存、localhost UDPから式・パラメータ・実描画までの200px移動、仮想空間ゾーン追加とApply保存を確認。
- Spoutは独立したD3D11デバイスで共有テクスチャを開き、追跡後のモデル画素の中心が本体と一致することまで確認。単なる送信者登録を合格基準にしない。
- Luaプラグインの環境生成で、元Lumarsの `meta.push()` がスタックに残る点を反映。3個のプラグインを本体から列挙しUIへ接続。
- PNGは本体の実フレームバッファから生成。表示結果を作画・模擬していない。合成検証モデルの範囲の確認であり、任意の実運用モデル・全機能・全入力のD同一性を保証したものではない。
- 以前の「メイン/設定/パネルが未移植」「製品接続未完了」という記載は上記実装により更新。全ノード/automation/metadataのD対応、異常系・寿命・数値境界の監査、外部機器とCI実行は引き続き未完了。
