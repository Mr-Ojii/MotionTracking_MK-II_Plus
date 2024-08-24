# MotionTracking MK-II Plus
AviUtlでオブジェクトトラッキングを行うプラグイン


## 必要動作環境
- AVXをサポートしたCPU
- 2GB以上のメインメモリ
- Windows 7以降のOS
- AviUtl 1.10
- 拡張編集 0.92


## インストール
zip内の.aufファイルとMotionTracking_modelディレクトリを`AviUtlがフィルタプラグインを読み込むお好きなディレクトリ`に置いてください。

AviUtlのフィルタメニューに"MotionTracking MK-II Plus"が追加されていたら成功です。

また、機械学習を用いたトラッキングアルゴリズムであるMethod 4, 5, 6を使用する場合、追加で作業が必要です。(学習データを同梱することが困難であるため)

### Method 4 (DaSiamRPN)用
[こちらのURL](https://github.com/opencv/opencv/blob/4.x/samples/dnn/dasiamrpn_tracker.cpp)のソースコードにコメントアウトとして記載されているURLより

- dasiamrpn_model.onnx
- dasiamrpn_kernel_r1.onnx
- dasiamrpn_kernel_cls1.onnx

をダウンロードし、`MotionTracking_modelディレクトリ内`に置いてください。

### Method 5 (Nano)用
[こちらのURL](https://github.com/HonglinChu/SiamTrackers/tree/18b7791360acb3f6d276d47376a6f1ed516f1628/NanoTrack/models/nanotrackv2)より

- nanotrack_backbone_sim.onnx
- nanotrack_head_sim.onnx

をダウンロードし、`MotionTracking_modelディレクトリ内`に置いてください。

### Method 6 (Vit)用
[こちらのURL](https://github.com/opencv/opencv_extra/blob/4.x/testdata/dnn/onnx/models/vitTracker.onnx)より

- vitTracker.onnx

をダウンロードし、`MotionTracking_modelディレクトリ内`に置いてください。

## ヘルパープラグイン
一つのAUFファイルに2つのヘルパープラグインを同梱しています。

1. Pre-track: HSV Cvt  
RGB画像をHSVに変換し、それをRGB画像の様に表示させます。また、HSVチャンネルの一つのみを表示させることができます。

2. Pre-track: BGSubtraction  
背景から動く物体を分離することを目的とするプラグインです。分離したRGB画像を出力するか、グレースケールのマスクを出力することができます。Rangeの値を大きくしすぎた場合、メモリ不足を引き起こす可能性がありますので、ご注意ください。


## ヘルプ
### MotionTracking MK-II Plus
#### 使用方法
0. トラッキングしたいフレームの範囲を選択する。
1. 「Select Object」ボタンをクリックし、ポップアップウィンドウ内で追跡するオブジェクトをドラッグして指定する。ポップアップウィンドウを閉じる。
2. 「Analyze」ボタンをクリックし、解析終了まで待つ。
3. 「View Result」を有効化し、結果を確認。もし結果が良かった場合は、「Invert Position」オプションを用途によって有効化し、「Save EXO」をクリックしてEXOを保存するか、「QuickBlur」を有効化する。よくなかった場合は、「Clear Result」をクリックして、結果を削除し、ステップ0か1に戻る。
#### Save EXO
正常な結果に1フレームのみ挟まれたエラーの自動補正機能が搭載されています。
CJKファイル名もサポートされています。
#### オプション
##### トラックバーのオプション
###### Method
解析で使用するアルゴリズムを指定します
1. Multi Instance Learning
2. KCF
3. CSRT
4. DaSiamRPN
5. Nano
6. Vit
###### Rect Hue
Object SelectionやView Resultで表示される矩形の色相を指定します
##### Save EXOのオプション
- As English EXO? : 英語版パッチを当てた拡張編集用のexoを出力するか
- As Sub-filter/部分フィルター？ : 部分フィルタとして出力するか
- Invert Position : トラッキング結果の座標を反転させるか
##### 映像エフェクトオプション
- Quick Blur : トラッキング結果を元に、直接ぼかしをかける。
- Easy Privacy : 検出されたすべての顔(実写のみ)をぼかします。正面からの顔は検出精度が高いですが、横顔には弱いです。

### Pre-track:BGSubtraction
#### 共通パラメータ
- Range : 現在のフレームの前後何フレームを解析に使用するか [30]
- Shadow : 1= 影の検出を有効化 [0]
#### MOG2のみ
- NMix : 背景モデルのガウス成分の数 [5]
- BG% : 背景比率 [70%]
#### KNNのみ
- d2T : あるピクセルがそのサンプルに近いかどうかを判断するための、ピクセルとサンプルの距離の2乗のしきい値


## ソースからのビルド
1. `git clone https://github.com/Mr-Ojii/MotionTracking_MK-II_Plus.git`を実行
2. OpenCV 4.9.0 を静的ライブラリとしてビルド(DebugとRelease両方)
3. `src/CMakeLists.txt`よりVSのプロジェクトを生成
4. Visual Studioを用いてビルド


## バグ報告
* [GitHub](https://github.com/Mr-Ojii/MotionTracking_MK-II_Plus)
