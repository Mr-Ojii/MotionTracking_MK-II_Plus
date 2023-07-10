# MotionTracking_MKII_Plus
AviUtlでオブジェクトトラッキングを行うプラグイン

## 必要動作環境
- AVXをサポートしたCPU
- 2GB以上のメインメモリ
- Windows 7以降のOS
- AviUtl 1.10
- 拡張編集 0.92

## インストール
zip内の.aufファイルと.xmlファイルを  
`aviutl.exeと同じディレクトリ`  
に置いてください。

AviUtlのフィルタメニューに"MotionTracking MK-II Plus"が追加されていたら成功です。

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
