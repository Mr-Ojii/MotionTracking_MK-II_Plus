# MotionTracking_MKII_Plus
AviUtlでオブジェクトトラッキングを行うプラグイン


## 必要動作環境
- AVXをサポートしたCPU
- 2GB以上のメインメモリ
- Windows 7以降のOS
- AviUtl 1.10
- 拡張編集 0.92


## インストール
zip内の.aufファイルと.xmlファイルを`aviutl.exeと同じディレクトリ`に置いてください。

AviUtlのフィルタメニューに"MotionTracking MK-II Plus"が追加されていたら成功です。

また、機械学習を用いたトラッキングアルゴリズムであるMethod 4, 5, 6を使用する場合、追加で作業が必要です。(学習データを同梱することが困難であるため)

### Method 4 (GOTURN)用
[こちらのURL](https://github.com/opencv/opencv_extra/tree/c4219d5eb3105ed8e634278fad312a1a8d2c182d/testdata/tracking)よりそれぞれのデータをダウンロード・分割ZIPを解凍し、

- goturn.caffemodel
- goturn.prototxt

を`aviutl.exeと同じディレクトリ`に置いてください。

### Method 5 (DaSiamRPN)用
[こちらのURL](https://github.com/opencv/opencv/blob/4.x/samples/dnn/dasiamrpn_tracker.cpp)のソースコードにコメントアウトとして記載されているURLより

- dasiamrpn_model.onnx
- dasiamrpn_kernel_r1.onnx
- dasiamrpn_kernel_cls1.onnx

をダウンロードし、`aviutl.exeと同じディレクトリ`に置いてください。

### Method 6 (Nano)用
[こちらのURL](https://github.com/HonglinChu/SiamTrackers/tree/8211ff3f862fc68a870dde1ab00451f35af3b1d4/NanoTrack/models/nanotrackv2)より

- nanotrack_backbone.onnx
- nanotrack_head.onnx

をダウンロードし、`aviutl.exeと同じディレクトリ`に置いてください。


## ヘルパープラグイン
一つのAUFファイルに2つのヘルパープラグインを同梱しています。

1. Pre-track: HSV Cvt  
RGB画像をHSVに変換し、それをRGB画像の様に表示させます。また、HSVチャンネルの一つのみを表示させることができます It can also display only one of the HSV channels.

2. Pre-track: BGSubtraction
背景から動く物体を分離することを目的とするプラグインです。分離したRGB画像を出力するか、グレースケールのマスクを出力することができます。Rangeの値を大きくしすぎた場合、メモリ不足を引き起こす可能性がありますので、ご注意ください。


## ヘルプ
1. 何らかの動画を読み込むか、新しいプロジェクトを作成
2. プラグインを有効化
3. "HELP"ボタンをクリック


## ソースからのビルド
1. `git clone https://github.com/Mr-Ojii/MotionTracking_MKII_Plus.git`を実行
2. OpenCV 4.8.0 を静的ライブラリとしてビルド(DebugとRelease両方)
3. `src/CMakeLists.txt`よりVSのプロジェクトを生成
4. Visual Studioを用いてビルド


## バグ報告
* [GitHub](https://github.com/Mr-Ojii/MotionTracking_MKII_Plus)
