# C++版の機能・実装差に対する今後のタスク

更新日: 2026-10-18。対象: `native-cpp`、実装の基準点: ローカルコミット `9a5a17a`。

Windows版を最優先とする。既存の差分監査のIDを引き継ぎ、その後の修正を反映した。これは既知の差と検証不足の一覧であり、全実装の等価性を確認し終えた一覧ではない。未発見の差を洗い出す作業も以下に含む。

## 継続する条件

- 機能を簡略化せず、関数・分岐・数式・状態遷移・副作用・処理順まで対応させる。起動テストだけで機能確認済みにしない。
- Boost.JSON、SDL3、gettext、新版ImGuiへの変更は許可済み。旧依存へ戻すタスクは作らない。
- nicxliveを基盤とし、nijimiのOpenGLバックエンドを使用する。要求された構成変更は維持したうえで、不足している処理を埋める。
- 読込時のfit、外部制御、native nijitrackの別スレッド埋め込み、設定・入力の直接接続、taskファイル同梱、WindowsのWinMainを維持する。
- Trackerのダウンロード・自動導入コードは不要であり、削除済み。今後の実装・検証タスクから除外する。
- **今月変更されたD版Weldingは復元基準にしない。拒否された接続の削除・再構成・平均化をC++へ取り込まない。D版の修正は後回しという指示を維持する。**
- 禁止された資料・由来情報・履歴は参照しない。許可された現在のプロジェクトソースを対象にする。

## P0: 確認済みの欠落・実装差を解消する

- [ ] **R01: Triple buffer fallbackの欠落を解消する。**
  対象: [part.cpp](native/port/nijimi/opengl/part.cpp)。設定・補助関数はあるが、D側の専用シェーダー・退避・合成の経路がない。対応する全分岐とGL状態の保存・復元を実装する。
  完了条件: 拡張を使える場合とfallbackの場合を実行し、対象ブレンド・マスクを使う実モデルの画像と読み戻し結果を比較する。設定が実際の描画経路を切り替えることも確認する。

- [ ] **R02: 高度なブレンド式を実装する。**
  対象: [blend.cpp](native/port/nijimi/opengl/blend.cpp)。advanced経路がlegacyへ流れ、Overlay、Darken、ColorBurn、HardLight、SoftLight、Difference等が通常の処理へ落ちる差を解消する。
  完了条件: 全ブレンド種の式・境界値・半透明・重なり順を照合し、実モデルと小さい比較用画像の両方で結果を残す。

- [ ] **R03: 高度なブレンドの同期処理を実装する。**
  対象: [blend.cpp](native/port/nijimi/opengl/blend.cpp)、[blending.cpp](native/port/nijimi/opengl/blending.cpp)。能力検出に対してcoherent設定・barrierが実処理をしない差を解消する。
  完了条件: 対応・非対応の能力分岐、連続描画、別FBOへの切替を検証し、同期漏れやGL状態の持ち越しがないことを確認する。

- [ ] **R04: ミップマップ指定を保持する。**
  対象: [backend.cpp](native/port/nijimi/opengl/backend.cpp)。生成時の`false`への上書きと、フィルタ設定で指定を無視する処理を修正する。
  完了条件: 指定の有無、縮小表示、フィルタ切替、テクスチャ更新後の各段を比較する。R05のDynamicComposite更新にも適用する。

- [ ] **M06: SceneItemの親子所有権の循環を解消する。**
  対象: [item.hpp](native/port/nijiexpose/scene/item.hpp)。親と子が双方`shared_ptr`を保持する構造を、D版の親子操作を保持しつつ解放できる所有権にする。
  完了条件: attach/detach、親子削除、モデルの繰返し読込・解放でオブジェクトとGPU資源が解放され、残った参照が無効アクセスを起こさないことを確認する。現時点では構造上のリスクであり、漏出量は未測定。

## P1: 描画・モデル処理の差を詰める

- [ ] **R05: DynamicComposite終了処理を照合・修正する。**
  対象: [dynamic_composite.cpp](native/port/nijimi/opengl/dynamic_composite.cpp)。D側の終了時flushとミップマップ更新がない。単にflushを足して完了とせず、生成・描画・終了・再利用の順を対応させる。
  完了条件: 入れ子、連続更新、縮小、resizeを含む比較で更新遅延・古い画像・合成差がない。

- [ ] **R06: DynamicCompositeのFBO寿命を照合する。**
  対象: 同上。Dはsurface破棄時、C++はtexture別キャッシュをtexture破棄時に解放する差がある。
  完了条件: surface再作成・texture差替え・handle再利用・モデル解放の寿命を記録し、不要な保持や別surfaceの誤再利用を解消する。現時点でリーク確定とは扱わない。

- [ ] **R07: 命令キューから最終表示までの対応を検証する。**
  対象: [main.cpp](native/port/nijiexpose/windows/main.cpp)、[nijimi backend](native/port/nijimi/opengl)。要求されたnicxlive命令キュー／nijimi表示構成のまま、描画順、マスク、合成、postprocess、viewport、GL状態を対応させる。
  完了条件: 実INPの画面表示・画像保存・Spout出力を確認し、各出力で同じモデル状態が描画される。コマンド種別が揃っているだけでは完了にしない。

- [ ] **M01: 最初のupdateより前の初期化を検証する。**
  対象: [model_io.cpp](native/port/nijilive/model_io.cpp)、[coreパッチ生成](native/tools/prepare-nicxlive-native.py)。ignorePuppet再帰設定、rescan、build、finalize、setupSelfの配置がDと異なる。
  完了条件: 初回update・初回描画より前の呼出し順、副作用、再読込時の重複実行を照合する。目・マスク・Projectableを含む実INPの最初の表示を画像で確認する。初期化が全く未実装という扱いにはしない。

- [ ] **M02: モデルコア全体の関数対応台帳と比較テストを完成させる。**
  対象: [依存固定](cmake/Dependencies.cmake)、[coreパッチ生成](native/tools/prepare-nicxlive-native.py)。固定nicxlive `6ff544470b0f851ab998d8a12220aa750dac0d6b`とパッチを対象にする。隣の作業コピーや非ビルド対象ファイルを実行中の実装と取り違えない。
  作業単位: ノード／変形・投影、パラメータ／binding・補間、driver／animation、物理／数学、更新スケジューラ／render task、読込／保存／extData。それぞれ関数、分岐、式、状態、呼出し順、未対応箇所を記録する。
  完了条件: 各単位で同一入力の中間状態と出力を比較し、差を解消する。パッチ各箇所の根拠と回帰テストを持つ。**拒否されたD版Weldingをこの作業で再導入しない。**

- [ ] **M03: Projectable offscreen scale signの必要性を確認する。**
  Dのset/clear/getOffscreenScaleSignに相当するAPI・状態が生成C++ coreにない。現在の呼出し元と実際に使われる分岐を確認し、必要な処理を移植する。
  完了条件: 負scale、親子の反転、offscreen合成の比較で必要性と対応結果を説明できる。API名の欠落だけで画面不具合と断定しない。

- [ ] **M05: 画像デコードの互換性を確認・修正する。**
  対象: 生成nicxliveの`texture.cpp`とそのパッチ生成処理。imagefmt→stb_imageの差は包括的に許可された変更ではない。
  完了条件: 対応形式、チャンネル、alpha、画素値、壊れた画像・エラー処理を比較し、従来扱えた画像の機能を省かず対応する。

## P1: アプリの動作と入力の差を詰める

- [ ] **A07: noiseの乱数処理を照合する。**
  対象: [expr.cpp](native/port/nijiexpose/tracking/expr.cpp)。Dの乱数初期化とC++の`random_device`由来seedの差を調べる。
  完了条件: アルゴリズム、分布、seedの扱い、更新タイミングを対応させる。制御したseedで比較し、偶然同じ値になることを判定基準にしない。

- [ ] **A08: Luaの型変換・エラー・寿命を対応させる。**
  対象: [plugins/package.cpp](native/port/nijiexpose/plugins/package.cpp)。lumarsからLua 5.1 C APIへの手動登録の差を洗い出す。
  完了条件: 全公開APIで正常値・nil・不正型・例外・callback・オブジェクト解放を比較する。式評価と実プラグインの実行まで確認する。

- [ ] **A09: Windowsファイルダイアログの挙動を対応させる。**
  対象: [io/package.cpp](native/port/nijiexpose/io/package.cpp)。tinyfiledialogsとGetOpenFileNameW実装の差を調べる。
  完了条件: 実際に提供する各ダイアログのフィルタ、初期パス、Unicode、選択数、取消、保存・上書きの挙動を比較し、省略を解消する。

- [ ] **U04: Throttle・FPS・GPU負荷の比較を完了する。**
  対象: [appwin.cpp](native/port/nijiui/core/window/appwin.cpp)、[settings.cpp](native/port/nijiexpose/windows/settings.cpp)。主要な間引きと表示FPSの修正は実装済み。rate 0でのカウンタ更新などの差と全条件の回帰を確認する。
  完了条件: 同一モデル・画面サイズ・V-Sync・GPUでrate 0/1/2/6、設定変更、resize、複数window、最小化・復帰を比較する。モデル描画FPSとpresent FPSを分けて記録し、GPU利用率も示す。「常に60fps」を現在の未修正事実として扱わず再測定する。

- [ ] **T01: 埋め込みTrackerのzone統合を確定する。**
  対象: [vspace.cpp](native/port/nijiexpose/tracking/vspace.cpp)、[embedded.hpp](native/port/nijiexpose/tracking/ext/embedded.hpp)。専用`nijitrack` zoneの自動生成は実装上の選択であり、ユーザーが明示的に指定した構成ではない。
  完了条件: 既存zone・binding・自動切替・再起動を確認し、zone重複や意図しない切替を防ぐ。直接接続を維持し、既存設定を使った操作結果を示してzone配置方針を確定する。

- [ ] **T02: 実カメラからモデル操作までの機能確認を終える。**
  対象: [tracker.cpp](native/port/nijiexpose/tracking/tracker.cpp)、[native tracker](native/tracker)。画像を使った実推論とカメラのフレーム取得は確認済みだが、実カメラの顔検出から実INPへの反映は完了扱いにしない。
  完了条件: 頭・目・口の入力とbinding、顔消失、抜差し、利用不可、開始・停止・再起動、直接設定変更、preview、keyboard入力解除を確認する。実INPの変化を画像で残す。別プロセス起動やOSC経由への後戻りはしない。

- [ ] **V01: アプリ全体の機能確認表を完成させる。**
  完了条件: モデル読込・切替・解放、パラメータ候補の選択、全外部Tracker adaptor、zone切替、binding／式、animation／physics、Lua、設定保存・再起動、翻訳、画像保存、Spout、複数window、IME／HiDPIを実操作で確認する。既存単体テストと未確認操作を分け、結果・入力・再現手順・画像を残す。単なるフレームワーク起動画像は代用しない。

## P1: CI・配布・台帳

- [ ] **B03: リリースのversion/resourceを統一する。**
  対象: [ui.rc.in](native/res/ui.rc.in)、[native CMake](native/CMakeLists.txt)。D側のresource値とC++の`0.1.0`・実行ファイル名に差がある。
  完了条件: リリースversionを一元生成し、PE情報、`--version`、配布名、アイコン、同梱DLL・task・翻訳・ライセンスを整合させる。古いD側の値を無条件に正としない。

- [ ] **B04: CIで描画機能を検証し、結果画像を保存する。**
  対象: [native-cpp.yml](.github/workflows/native-cpp.yml)。GPUテストは既定OFFで、通常presetでは有効化されない。ローカルテスト成功とリモートCIでのGPU検証を区別する。
  完了条件: GPUを使える実行環境・jobを用意し、実INPの描画・マスク・合成・画像比較を実行する。環境情報、失敗時の画像、比較差分をartifactに保存する。embedded Trackerの実推論テストも維持し、実際のCI成功を確認する。

- [ ] **D01: 移植台帳・設計・READMEの古い記載を更新する。**
  対象: [設計](NATIVE_CPP_DESIGN.md)、[README](native/README.md)、[移植台帳](native/PORTING_STATUS.md)。初期ハーネスの説明、古いテスト数、Python Tracker前提と現在の実装を整理する。
  完了条件: 実装済み・検証済み・未検証・要求により廃止を分ける。参照元ハッシュだけで等価性を保証したと書かない。本タスク表と矛盾させない。

## P2: Windows優先作業の後

- [ ] **B01: Linux/macOS対応を移植する。** 非Windowsで停止するCMake、Linux portal、macOS Syphon・bundle等を扱う。Windows完了後に各OSでビルドと機能確認を実施する。
- [ ] **W-D: D版Weldingの拒否された変更を整理する。** D版は後という指示に従い、今は変更しない。禁止された履歴の復元は行わず、許可された根拠から対象を特定する。C++へ現在のD版の変更を取り込む作業と混同しない。

## 旧監査からの状態更新

| ID | 現在の扱い |
| --- | --- |
| M04 | C++パッチの接続配列修復・再構成ループは削除済み。初期化自体は維持。接続配列とweightの保存、逆リンクを勝手に作らない回帰テストを維持する。Dの修復ログ・修復仕様を追加するタスクにはしない。 |
| A01 / A02 | 読込時fit・外部制御は要求された変更。維持して機能確認する。 |
| A03 / A04 / A05 | zoneなし時の状態・候補・パネルのガードは実装済み。クラッシュする挙動へ戻さない。V01で再確認する。 |
| A06 | 埋め込みViewのFPS計測開始・積算リセットは実装済み。U04で確認する。 |
| A10 / A11 | 外部TrackerのHTTPダウンロード・ZIP展開はnative埋め込みへの変更により廃止。旧経路の復元は不要。 |
| U01 / U02 | 新版ImGui・gettextは許可済み。依存変更を戻さず、アプリの挙動をV01で確認する。 |
| U03 | Windowsのwindow icon設定は実装済み。配布物のresource確認はB03。 |
| B02 | WinMain、Unicode引数、Windows GUI subsystemは実装・確認済み。 |

既存のローカル36テスト成功は、全機能の同一性を保証しない。視覚的なタスクには実モデル・操作条件・結果画像を、非視覚的なタスクには状態・呼出し順・出力の比較結果を添えて完了にする。
