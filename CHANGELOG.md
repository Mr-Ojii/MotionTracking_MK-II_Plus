# CHANGELOG

## r1

初版

### AviUtl 版からの変更点

#### 主な変更点

- AviUtl の対応を終了。AviUtl2 のみサポート。
- SSE2 の対応を終了。
- 32bit から 64bit に変更。
- 拡張子が `.auf` から `.aux2` に変更
- Method の選択が、プルダウンメニューから選べるように変更。
- AviUtl2 のテーマに対応。
- View Result ボタンを廃止。Insert Object に統一。
- Save EXO ボタンを廃止。Insert Object に統一。

#### 内部実装

- 移行可能なグローバル変数をクラスのメンバ変数に移行。
- テーマ対応のため、コントロールをオーナードローを用いた独自描画に変更。

#### Select Object

##### 追加機能
- ESC キーでキャンセルを追加。前回の選択状態を復元して返す。
- F3 キーで選択確定を追加。
- Select Object を再度開いたとき、前回の選択矩形を表示した状態で開くように。

##### 変更点
- × ボタンでウィンドウを閉じたとき、結果を保存して返すように変更。
  - 旧版はキャンセルという概念がなく、閉じても状態がそのまま残るだけだった。

##### 内部実装
- 画像取得を `get_pixel_filtered` から `rendering_scene_video` + `wait_rendering_task` に変更。
- 画像フォーマットを BGR（CV_8UC3）+ 上下反転から、RGBA（CV_8UC4）-> BGR 変換（反転不要）に変更。
- `std::mutex g_mutex` による排他制御を廃止。`wait_rendering_task` で代替。
