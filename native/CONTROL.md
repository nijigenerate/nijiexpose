# 実行中のモデルを外部から調整する

Windows版を `nijiexpose.exe --control-dir C:\work\control model.inp` で起動する。
指定しない場合、外部操作は無効。ネットワークポートは開かない。

モデルは読み込み後の最初の描画で自動的に中央へfitする（起動引数、ファイル選択、ドロップ共通）。小さなモデルは拡大せず、大きいモデルのみ縮小する。以後のフレームやウィンドウ変更で手動調整を上書きしない。外部の `fit` は明示操作として拡大も行う。Scale下限は既存操作と共通の0.25。

```powershell
python native/tools/control.py --directory C:\work\control list
python native/tools/control.py --directory C:\work\control set_transform 0 --scale 0.25 --x 0 --y 0
python native/tools/control.py --directory C:\work\control fit 0 --margin 90
```

`list` はモデルの現在のindex、ファイルパス、Scale、位置、ワールド座標のbounds、viewportを返す。モデルを追加・削除したらindexを再取得する。
`set_transform` は指定した値のみ変更し、マウス操作用の目標値も同期する。Scaleの範囲は既存UIと同じ0.25～5。位置はモデルのワールド座標。
`fit` は現在のモデル境界とカメラ倍率からScaleと中央位置を計算する。marginは画面ピクセル単位。Scaleの上下限に達する場合は指定余白を確保できないため、描画結果も確認する。
親モデルに取り付けた子は直接変更できない。グループの親を指定する。
変更対象は実行中のシーンで、INPファイルへの保存は行わない。

プロトコルはUTF-8 JSON。任意の一意なIDに対し、一時ファイルを同じフォルダーの `<id>.request.json` へrenameして要求を確定する。アプリはメインスレッドで約100msごとに処理し、`<id>.response.json` に `{"ok":true,"result":...}` または `{"ok":false,"error":"..."}` を原子的に公開して返す。クライアントが応答を削除する。要求は16 KiB以下。1アプリにつき専用フォルダーを使用する。

応答の一時ファイル `<id>.response.json.tmp` は排他的に新規作成し、既存のファイルやリンクを開いて上書きしない。応答ファイル自体が既に存在する場合も置換しない。競合時は要求を残すため、IDを再利用せず、残存した応答や一時ファイルはクライアント側で確認して処理する。

要求例: `{"command":"set_transform","index":0,"scale":0.25}`。
タイムアウト時は適用済みの可能性があるため、`list` で状態を確認してから再試行する。
検証: `python native/tests/control_live.py C:\work\control results.json`（モデル0のScaleと位置を変更し、最後にfitする）。
