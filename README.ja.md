# MotionTracking_MK-II_Plus_for_AviUtl2

AviUtl ExEdit2 でオブジェクトトラッキングを行うプラグイン

## 必要動作環境

- AVX2をサポートしたCPU
- Windows 10以降のOS
- DirectX11.3 が利用できる環境
- AviUtl ExEdit2 version 2.0beta48 以降
- AviUtl ExEdit2 version 2.1.0 にて動作確認済み。

## インストール

zip内の.aux2ファイルとMotionTracking_modelディレクトリを`AviUtl ExEdit2 が汎用プラグインを読み込むお好きなディレクトリ`に置いてください。

AviUtl ExEdit2 の`表示`メニューに"MotionTracking MK-II Plus for AviUtl2"が追加されていたら成功です。

また、機械学習を用いたトラッキングアルゴリズムであるDaSiamRPN, Nano, Vitを使用する場合、追加で作業が必要です。(学習データを同梱することが困難であるため)

### DaSiamRPN 用

[こちらのURL](https://github.com/opencv/opencv/blob/4.x/samples/dnn/dasiamrpn_tracker.cpp)のソースコードにコメントアウトとして記載されているURLより

- dasiamrpn_model.onnx
- dasiamrpn_kernel_r1.onnx
- dasiamrpn_kernel_cls1.onnx

をダウンロードし、`MotionTracking_modelディレクトリ内`に置いてください。

### Nano 用

[こちらのURL](https://github.com/HonglinChu/SiamTrackers/tree/18b7791360acb3f6d276d47376a6f1ed516f1628/NanoTrack/models/nanotrackv2)より

- nanotrack_backbone_sim.onnx
- nanotrack_head_sim.onnx

をダウンロードし、`MotionTracking_modelディレクトリ内`に置いてください。

### Vit 用

[こちらのURL](https://github.com/opencv/opencv_extra/blob/4.x/testdata/dnn/onnx/models/vitTracker.onnx)より

- vitTracker.onnx

をダウンロードし、`MotionTracking_modelディレクトリ内`に置いてください。

## ヘルパープラグイン

> [!Important]
> ヘルパープラグインについては、アップデートで対応予定です。現段階では、未実装です。

一つのAUX2ファイルに2つのヘルパープラグインを同梱しています。

1. Pre-track: HSV Cvt
   RGB画像をHSVに変換し、それをRGB画像の様に表示させます。また、HSVチャンネルの一つのみを表示させることができます。

2. Pre-track: BGSubtraction  
   背景から動く物体を分離することを目的とするプラグインです。分離したRGB画像を出力するか、グレースケールのマスクを出力することができます。Rangeの値を大きくしすぎた場合、メモリ不足を引き起こす可能性がありますので、ご注意ください。

## ヘルプ

### MotionTracking MK-II Plus

#### 使用方法

0. トラッキングしたいフレームの範囲を選択する。
1. 「Select Object」ボタンをクリックし、ポップアップウィンドウ内で追跡するオブジェクトをドラッグして指定する。ポップアップウィンドウをxまたはF3で閉じる。
2. 「Analyze」ボタンをクリックし、解析終了まで待つ。解析を中断するには、x ボタンをクリックする。
3. 「View Result」ボタンをクリックし、結果を確認。もし結果が良かった場合は、「Invert Position」オプションを用途によって有効化し、「Insert Object」をクリックしてInset Object または、Object ファイルを保存する。よくなかった場合は、「Clear Result」をクリックして、結果を削除し、ステップ0か1に戻る。

#### Export Object File

正常な結果に1フレームのみ挟まれたエラーの自動補正機能が搭載されています。
CJKファイル名もサポートされています。

#### オプション

##### プルダウンメニューのオプション

###### Method

解析で使用するアルゴリズムを指定します

1. Multi Instance Learning
2. KCF
3. CSRT
4. DaSiamRPN
5. Nano
6. Vit

###### Hue

Object SelectionやView Resultで表示される矩形の色相を指定します

##### Insert Object のオプション

- As Sub-filter/部分フィルタ？ : 部分フィルタとして出力するか
- Invert Position : トラッキング結果の座標を反転させるか
- Ignore Aspect Ratio : アスペクト比を無視し、拡大率で出力するか

### Pre-track:BGSubtraction

> [!Important]
> Pre-track:BGSubtraction については、アップデートで対応予定です。現段階では、未実装です。

#### 共通パラメータ

- Range : 現在のフレームの前後何フレームを解析に使用するか [30]
- Shadow : 1= 影の検出を有効化 [0]

#### MOG2のみ

- NMix : 背景モデルのガウス成分の数 [5]
- BG% : 背景比率 [70%]

#### KNNのみ

- d2T : あるピクセルがそのサンプルに近いかどうかを判断するための、ピクセルとサンプルの距離の2乗のしきい値

## ソースからのビルド

`.github/workflows/build.yml`または、[Dockerfile](https://github.com/nullruptr/MotionTracking_MK-II_Plus_for_AviUtl2/tree/master/docker)をご覧ください。

## バグ報告

- [GitHub](https://github.com/nullruptr/MotionTracking_MK-II_Plus_for_AviUtl2)
