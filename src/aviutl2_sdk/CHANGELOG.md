# 変更履歴

## 2026/5/30

- \[aux2] 汎用プラグインのUninitializePluginのタイミングを調整
- \[auf2] 画像と音声を両対応した時に設定値が正しく取得出来ないことがあるのを修正
- \[auf2] FILTER_PROC_VIDEOのシェーダー実行の関数名を変更
- \[auf2] FILTER_PROC_VIDEOに幾つかの関数を追加、引数の修飾子を調整
- \[aux2] EDIT_HANDLEに幾つかの関数を追加
- \[cache2] CACHE_HANDLEに幾つかの関数を追加

## 2026/5/24

- \[auf2] FILTER_ITEM_TRACKの引数名を修正

## 2026/5/24

- \[aux2] get_object_alias()が複数スレッドから同時に呼ばれた時に正しく取得出来ないのを修正
- \[aux2] EDIT_SECTIONの参照時の排他制御を調整
- \[aux2] 汎用プラグインのUninitializePluginのタイミングを調整
- \[auf2] FILTER_ITEM_CHECK_SECTION,FILTER_ITEM_TRACK_GROUPを追加
- \[auf2] FILTER_ITEM_TRACKの引数を追加
- \[auf2] FILTER_PROC_VIDEOに幾つかの関数を追加
- \[auf2] サンプルフィルタプラグインを更新

## 2026/5/17

- \[auf2] OBJECT_INFOにlayer,index,numを追加
- \[auf2] FILTER_PROC_VIDEO,FILTER_PROC_AUDIOに幾つかの関数を追加
- \[auf2]\[mod2]\[aux2] キャッシュ関連機能の関数定義(InitializeCache)を追加
- \[auf2]\[mod2] EDIT_SECTIONの参照用の関数を利用出来るようにした
- \[auf2] サンプルフィルタプラグインを更新

## 2026/5/10

- \[auo2] OUTPUT_PLUGIN_TABLEにプロジェクトファイル側に出力設定を保持する為のフラグや関数を追加
- \[aux2] 編集レジュームが有効でレジュームデータが無い時にregister_project_load_handler()が呼ばれないのを修正
- \[aux2] EDIT_SECTIONにget_layer_enable(),set_layer_enable(),get_layer_lock(),set_layer_lock()を追加
- \[aux2] EDIT_SECTIONにget_object_section_num(),get_focus_object_section()を追加
- \[aux2] HOST_APP_TABLEにregister_script_module_name()を追加
- \[aux2] レイヤーメニュー、オブジェクトメニュー、設定項目メニューの登録でメニューを階層にする指定を追加

## 2026/5/4

- \[aux2] HOST_APP_TABLEにregister_object_item_menu(),register_object_item_menu_param()を追加

## 2026/4/28

- \[aux2] call_edit_section()のコールバック関数をメインスレッドで呼ばれるように修正 ※色々問題があるので戻します
- \[aux2] enum_effect_name()のエフェクトフラグにEFFECT_FLAG_CAMERAを追加

## 2026/4/26

- \[aux2] call_edit_section()のコールバック関数を呼び出し元と同じスレッドで呼ばれるように修正
- \[aux2] enum_effect_name()のエフェクト種別にEFFECT_TYPE_CONTROL,EFFECT_TYPE_OUTPUTを追加
- \[aux2] EDIT_HANDLEにenum_effect_item()を追加

## 2026/4/18

- \[aux2] EDIT_HANDLEにcall_read_section,call_read_section_param()を追加

## 2026/4/12

- \[auf2] FILTER_ITEM_SEPARATORを追加
- \[aux2] EDIT_HANDLEにget_edit_state()を追加
- \[aux2] EDIT_SECTIONの幾つかの関数の実装を改善
- \[au2pkg] package.iniにuninstallSubFolderFileの項目を追加

## 2026/3/29

- \[mod2] push_result_array_boolean(),push_result_table_boolean()を追加

## 2026/3/14

- \[aux2] 引数に存在しないファイルを指定してアプリを起動した時にregister_project_load_handler()が呼ばれないのを修正
- \[aux2] set_object_item_value()でテキスト設定を変更した時にオブジェクト設定の表示が更新されないことがあるのを修正
- \[aux2] get_mouse_layer_frame()でファイルD&D時のドロップ位置を取得出来るようにした
- 各種ヘッダーファイルに#pragma onceを追加

## 2026/3/8

- \[aux2] set_object_item_value()で対象レイヤー数を変更した時にレイヤー編集の表示に反映されないのを修正
- \[aux2] HOST_APP_TABLEにregister_file_drop_handler(),register_file_drop_param_handler()を追加

## 2026/2/28

- \[aux2] 汎用プラグイン情報取得関数定義(GetCommonPluginTable)を追加
- \[au2pkg] package.iniにパッケージ情報の項目を追加

## 2026/2/23

- \[auf2] FILTER_ITEM_BUTTONのコールバック関数の呼び出し時に各設定項目の設定値を更新するように修正
- \[auo2] OUTPUT_PLUGIN_TABLEに静止画出力向けのフラグを追加
- \[aux2] EDIT_HANDLEをRegisterPlugin処理内から利用した時の説明を追加
- \[au2pkg] パッケージファイルにパッケージ情報を記述したiniファイルを含められるようにした

## 2026/2/14

- \[aux2] get_edit_info()の処理時に同一スレッドが更新ロック状態でも取得出来るようにした
- \[aux2] レイヤー名、シーン名の取得・設定関数を追加
- \[aux2] シーン情報の設定関数を追加
- \[aui2]\[auo2]\[auf2]\[mod2]\[aux2] 必要な本体バージョン番号の関数定義(RequiredVersion)を追加

## 2026/2/8

- \[auf2] OBJECT_INFOのidの値を描画対象のオブジェクトIDとなるように修正
- \[aux2] メインスレッドからcall_edit_section()を呼び出した時は直接処理するように修正
- \[aux2] EDIT_HANDLEにget_host_app_window()を追加
- \[aux2] HOST_APP_TABLEにEDIT_SECTIONにしないメニュー登録関数を追加
- \[au2pkg] 同名のパッケージファイルが既にインストールされている場合は削除後にインストールするようにした

## 2026/2/1

- \[config2] get_color_code()で複数色定義の項目取得で例外になるのを修正
- \[config2] CONFIG_HANDLEにget_color_code_index()を追加
- \[au2pkg] パッケージファイルにインストール対象外のファイルを含められるようにした
- \[au2pkg] パッケージのインストール時のダイアログに表示するテキストファイルを含められるようにした

## 2026/1/25

- \[aui2]\[auo2]\[auf2]\[mod2]\[aux2] 設定関連機能の関数定義(InitializeConfig)を追加
- \[aux2] EDIT_HANDLEにenum_module_info()を追加
- ログ出力機能の文字制限をLOG_HANDLEのコメントに追記
- プラグインの配置場所をPluginフォルダの一つ下のフォルダも対象とするようにした
- プラグイン・スクリプト等を纏めてインストールするパッケージファイルの説明を追加

## 2026/1/12

- \[auf2] FILTER_ITEM_FOLDERを追加

## 2026/1/11

- \[aux2] EDIT_HANDLEにrestart_host_app(),enum_effect_name()を追加
- \[auf2] FILTER_ITEM_STRING,FILTER_ITEM_TEXTを追加
- \[auf2] サンプルのフィルタプラグインに対応する言語ファイルを追加

## 2025/12/27

- \[aux2] アプリ起動時のプロジェクト初期化時にregister_project_load_handler()が呼ばれないことがあるのを修正
- \[aux2] register_input_plugin()で登録した対応ファイルの拡張子の関連付けが反映されないのを修正
- \[aux2] HOST_APP_TABLEにregister_change_scene_handlerを追加
- \[aux2] EDIT_INFOにscene_idを追加
- \[auf2] FILTER_ITEM_BUTTONを追加
- \[auf2] OBJECT_INFOにフィルタオブジェクトかのフラグを追加

## 2025/12/20

- \[aux2] get_object_item_value(),set_object_item_value()が正しく取得反映されないことがあるのを修正
- \[aux2] HOST_APP_TABLEにregister_edit_menu(),register_clear_cache_handler()を追加

## 2025/12/14

- \[aux2] set_cursor_layer_frame()が正しく反映されないことがあるのを修正
- \[auf2] FILTER_PLUGIN_TABLEにフィルタオブジェクトのサポートフラグを追加

## 2025/12/7

- \[aux2] EDIT_INFOにレイヤー編集関連の情報を追加
- \[aux2] EDIT_SECTIONにset_display_layer_frame(),set_select_range(),set_grid_bpm()を追加
- \[aux2] EDIT_SECTIONにget_object_name(),set_object_name()を追加

## 2025/12/6

- \[aux2] EDIT_SECTIONにset_cursor_layer_frame()を追加
- \[aux2] HOST_APP_TABLEにregister_config_menu()を追加
- \[aux2] create_object_from_alias()で複数オブジェクトのエイリアスデータを利用出来るようにした

## 2025/12/2

- \[aux2] EDIT_HANDLEにget_edit_info()を追加
- \[aux2] EDIT_SECTIONにget_mouse_layer_frame(),pos_to_layer_frame()を追加
- \[aux2] EDIT_SECTIONにis_support_media_file(),get_media_info()を追加
- \[aux2] EDIT_SECTIONにcreate_object_from_media_file(),create_object()を追加
- \[aux2] EDIT_SECTIONにget_project_file()を追加
- \[aux2] PROJECT_FILEにget_project_file_path()を追加
- \[auf2] FILTER_ITEM_GROUPにグループの終端の設定方法を追加

## 2025/11/30

- \[auf2] FILTER_ITEM_GROUPを追加
- \[aux2] 登録したオブジェクトメニューが表示されないことがあるのを修正

## 2025/11/22

- \[auf2] FILTER_PROC_VIDEOにget_image_texture2d(),get_framebuffer_texture2d()を追加
- \[aux2] EDIT_SECTIONにcount_object_effect()を追加
- \[aux2] EDIT_HANDLEにcall_edit_section_param()を追加
- \[auf2] メディアオブジェクトのサンプルフィルタを更新
- \[aux2] ウィンドウクライアントのサンプルプラグインを更新

## 2025/11/9

- \[mod2] get_param_table_boolean()を追加

## 2025/11/8

- \[aui2]\[auo2]\[auf2]\[mod2]\[aux2] ログ出力機能の関数定義(InitializeLogger)を追加
- \[auf2] FILTER_ITEM_DATAを追加
- \[aux2] HOST_APP_TABLEにregister_layer_menu(),register_object_menu()を追加
- \[aux2] EDIT_SECTIONにget_selected_object(),get_selected_object_num()を追加

## 2025/11/2

- \[aux2] get_object_item_value(),set_object_item_value()のエフェクト指定方法を追加
- \[aux2] EDIT_SECTIONにoutput_log()を追加
- \[auf2] OBJECT_INFOにeffect_idを追加

## 2025/11/1

- \[aux2] プロジェクトファイルの独自データ保存向けの関数を追加
- \[aux2] プロジェクトデータ編集向けの関数を追加

## 2025/10/26

- \[mod2] get_param_boolean(),push_result_boolean()を追加
- \[aux2] 独自ウィンドウを追加する汎用プラグインのサンプルを追加

## 2025/10/12

- \[mod2] スクリプトモジュールのサンプルを追加

## 2025/10/5

- \[aui2]\[auo2] DLL初期化終了時の関数定義(InitializePlugin,UninitializePlugin)を追加
- \[auf2] フィルタプラグインのサンプルを追加

## 2025/8/16

- \[aui2] マルチトラック対応用の関数(func_set_track)を追加
- \[aui2] 時間指定読み込み対応用の関数(func_time_to_frame)を追加

## 2025/7/20

- \[auo2] 対応フォーマットにPA64,HF64,YC48を追加
- \[aui2] 対応フォーマットにPA64,HF64,YC48を追加
- \[aui2] 拡張用の予約項目を追加
