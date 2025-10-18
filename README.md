# AviUtl2 DepthMapFilter

[Depth Anything V2](https://github.com/DepthAnything/Depth-Anything-V2) を使って深度マップを生成する [AviUtl2](https://spring-fragrance.mints.ne.jp/aviutl/) フィルタープラグインです。
[Windows ML](https://learn.microsoft.com/ja-jp/windows/ai/new-windows-ml/overview) の [ONNX ランタイム](https://onnxruntime.ai/docs/)を使って推論を行います。

![DepthMapFilter](assets/DepthMapFilter.png)
Image by <a href="https://pixabay.com/users/anselmo7511-43046719/?utm_source=link-attribution&utm_medium=referral&utm_campaign=image&utm_content=9887875">Anselmo Rodrigues</a> from <a href="https://pixabay.com//?utm_source=link-attribution&utm_medium=referral&utm_campaign=image&utm_content=9887875">Pixabay</a>

## 動作環境

- バージョン 24H2（ビルド 26100）以降の Windows 11 PC

- [AviUtl ExEdit2](https://spring-fragrance.mints.ne.jp/aviutl/)
    <br>`beta15` で動作確認済み。

## 導入方法

1. 動作には**バージョン 1.8.0 (1.8.250907003) 以上**の [Windows App SDK](https://learn.microsoft.com/ja-jp/windows/apps/windows-app-sdk/downloads) が必要です。お使いの PC にインストールされていない場合は Microsoft の公式サイトからダウンロードしてください。
    - [インストールされている Windows App SDK ランタイムのバージョンを確認する](https://learn.microsoft.com/ja-jp/windows/apps/windows-app-sdk/check-windows-app-sdk-versions)
    - [最新の Windows App SDK ダウンロード](https://learn.microsoft.com/ja-jp/windows/apps/windows-app-sdk/downloads)

2. [Release](https://github.com//azurite581/AviUtl2-DepthMapFilter/releases/latest) から zipファイルをダウンロードします。

3. zipファイルを展開し、中身を以下のように配置してください

    |ファイル・フォルダ名|配置先|
    |:---|:---|
    |`DepthMapFilter.auf2`<br>`DepthMapFilter_model`フォルダ|`C:\ProgramData\aviutl2\Plugin`フォルダ|
    |`Microsoft.WindowsAppRuntime.Bootstrap.dll`|`aviutl2.exe` と同じフォルダ|

> [!NOTE]
> `Microsoft.WindowsAppRuntime.Bootstrap.dll` は PC から WindowsAppSDK を読み込むためのものです。詳細は [ブートストラッパー](https://learn.microsoft.com/ja-jp/windows/apps/windows-app-sdk/deployment-architecture#bootstrapper) を参照してください。

## パラメーター

- #### カラーマップ

    深度マップに適用するカラーマップのタイプを選択します。`GRAY`（白黒）+ [`cv::ColormapTypes`](https://docs.opencv.org/4.x/d3/d50/group__imgproc__colormap.html#ga9a805d8262bcbe273f16be9ea2055a65)の中から選択できます。


## 使用方法

オブジェクトに DepthMapFilter を適用すると自動的に推論が始まり、深度マップが生成されます。デフォルトでは `加工` カテゴリの中にあります。

> [!NOTE]
> - 推論には [Direct ML](https://learn.microsoft.com/ja-jp/windows/ai/directml/dml) を使用します。Direct ML の動作には [DirectX 12 に対応した GPU](https://github.com/microsoft/DirectML#hardware-requirements) が必要です。
> - CPU と Direct ML 以外の[実行プロバイダー](https://onnxruntime.ai/docs/execution-providers/)には対応していません。
> - 複数の GPU を搭載した PC では最もメモリサイズが大きい GPU が自動的に選択されます。

> [!WARNING]
> - フレーム間の一貫性が保たれないため動画には向いていません。

## ライセンス

[MIT Lisence](LICENSE.txt) に基づくものとします。

## クレジット

- 使用した[サードパーティーライブラリ](ThirdPartyNotices.md)

- [Depth Anything V2](https://github.com/DepthAnything/Depth-Anything-V2)

## 更新履歴

- #### v1.0.0 (2025/10/18)
  初版
