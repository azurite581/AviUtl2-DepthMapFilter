# AviUtl2 DepthMapFilter

[Depth Anything V2](https://github.com/DepthAnything/Depth-Anything-V2) を使って深度マップを生成する [AviUtl2](https://spring-fragrance.mints.ne.jp/aviutl/) のフィルタープラグインです。

![DepthMapFilter](assets/DepthMapFilter.png)
Image by <a href="https://pixabay.com/users/anselmo7511-43046719/?utm_source=link-attribution&utm_medium=referral&utm_campaign=image&utm_content=9887875">Anselmo Rodrigues</a> from <a href="https://pixabay.com//?utm_source=link-attribution&utm_medium=referral&utm_campaign=image&utm_content=9887875">Pixabay</a>

## 動作環境

- [AviUtl ExEdit2](https://spring-fragrance.mints.ne.jp/aviutl/)  
  `beta46` 以降必須（`beta51` で動作確認済み）。

## インストール

### 手動インストール

[Releases](https://github.com/azurite581/AviUtl2-DepthMapFilter/releases/latest) から `DepthMapFilter_v{version}.au2pkg.zip` をダウンロードし、AviUtl2 のプレビューにドラッグ&ドロップしてください。

#### アンインストール

`C:\ProgramData\aviutl2\Plugin` 下の `DepthMapFilter` フォルダーを削除してください。

> [!Note]
> ### For non-Japanese speaking users
> Please download the translation files from [here](https://github.com/azurite581/aviutl2_translations_azurite/releases/latest).

### 配布ファイルの変更点

#### v1.1.0 の変更点

v1.0.0 では `aviutl2.exe` と同階層のフォルダに `Microsoft.WindowsAppRuntime.Bootstrap.dll` を配置する必要がありましたが、ライブラリの変更に伴い v1.1.0 で不要になりました。また、モデルの配置場所も変更したため、`Plugin` フォルダー下の `DepthMapFilter_model` フォルダーも不要になりました。これらは手動で削除していただく必要があります。

## 使い方

1. `加工` カテゴリから `DepthMapFilter` を適用します。
2. 自動的に推論が始まり、深度マップが生成されます（推論は常に CPU で行われます）。

一度推論した画像はキャッシュに保存されます（キャッシュを削除する場合は `F5` キーを押してください）。

> [!WARNING]
> 内部で画像を 518×518 にリサイズしてから推論を行うため、大きな画像では細かなディテールが失われることがあります。

> [!WARNING]
> 動画にも適用できますが、フレーム間の一貫性は保証されません。

## パラメーター

### カラーマップ

深度マップに適用するカラーマップのタイプを選択します。`GRAY`（白黒）+ [`cv::ColormapTypes`](https://docs.opencv.org/5.0/main_modules/imgproc_colormap.html#enumeration-type-documentation) の中から選択できます。

## ビルド

### 環境

- Windows 11
- MSVC 2026
- Git
- PowerShell v7 ※パスが通っており pwsh で起動できること。
- [mise](https://mise.jdx.dev/)

### 手順

ツールのインストールと OpenCV のビルド、モデルの ONNX 変換を行うため、あらかじめ 10GB 程度の空きが必要になります。

1. 本リポジトリを任意の場所にクローンします。

    ```shell
    git clone --recursive https://github.com/azurite581/AviUtl2-DepthMapFilter.git
    ```

    > [!Note]
    > submodule を含めるため、`--recursive` オプションを付ける必要があります。

2. mise をアクティベートします。

3. 必要なツールをインストールします。インストールされるツールについては [mise.toml](mise.toml) の `[tools]` セクションを参照してください。

    ```shell
    mise i
    ```

3. mise の tasks でビルドします。成功すると `release` フォルダー下に `DepthMapFilter_v{version}.au2pkg.zip` が生成されます。

    ```shell
    mise run build
    ```

## ライセンス

[MIT License](LICENSE.txt) に基づくものとします。

## クレジット

### 使用したサードパーティライブラリ

[ThirdPartyNotices](ThirdPartyNotices.md) を参照してください。

### 使用したツール

### [aviutl2-cli](https://github.com/sevenc-nanashi/aviutl2-cli)

<details>
<summary>MIT License</summary>

```text
MIT License

Copyright (c) 2026 Nanashi. <sevenc7c.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

</details>

## 更新履歴

[CHANGELOG](CHANGELOG.md) を参照してください。
