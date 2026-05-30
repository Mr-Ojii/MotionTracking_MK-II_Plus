#pragma once
//----------------------------------------------------------------------------------
//	フィルタプラグイン ヘッダーファイル for AviUtl ExEdit2
//	By ＫＥＮくん
//----------------------------------------------------------------------------------

//	フィルタプラグインは下記の関数を外部公開すると呼び出されます
//
//	フィルタ構造体のポインタを渡す関数 (必須)
//		FILTER_PLUGIN_TABLE* GetFilterPluginTable(void)
//
//	必要とする本体バージョン番号取得関数 (任意)
//		DWORD RequiredVersion() ※必要な本体のバージョン番号を返却します
//
//	プラグインDLL初期化関数 (任意)
//		bool InitializePlugin(DWORD version) ※versionは本体のバージョン番号
// 
//	プラグインDLL終了関数 (任意)
//		void UninitializePlugin()
// 
//	ログ出力機能初期化関数 (任意) ※logger2.h
//		void InitializeLogger(LOG_HANDLE* logger)
// 
//	設定関連機能初期化関数 (任意) ※config2.h
//		void InitializeConfig(CONFIG_HANDLE* config)
//
//	キャッシュ関連機能初期化関数 ※cache2.h
//		void InitializeCache(CACHE_HANDLE* cache)

//----------------------------------------------------------------------------------

// plugin2.hに定義されています
struct EDIT_SECTION;

// オブジェクトハンドル
typedef void* OBJECT_HANDLE;

// トラックバー項目構造体
// 例：FILTER_ITEM_TRACK track = { L"数値", 100.0, 0.0, 1000.0, 0.01 };
struct FILTER_ITEM_TRACK {
	FILTER_ITEM_TRACK(LPCWSTR name, double value, double s, double e, double step = 1.0, LPCWSTR zero_display = nullptr, double slider_ratio = 1.0)
		: name(name), value(value), s(s), e(e), step(step), zero_display(zero_display), slider_ratio(slider_ratio) {}
	LPCWSTR type = L"track2";	// 設定の種別 ※以前の"track"種別も互換対応されます
	LPCWSTR name;				// 設定名
	double value;				// 設定値 (フィルタ処理の呼び出し時に現在の値に更新されます)
	const double s, e;			// 設定値の最小、最大
	const double step;			// 設定値の単位( 1.0 / 0.1 / 0.01 / 0.001 ) ※0.0001以下も指定出来ますが最大最小値の範囲に応じて調整されます
	LPCWSTR zero_display;		// ゼロ値名称 (設定値が0の時にトラックバーに表示する文字列)
	const double slider_ratio;	// 操作倍率 (設定値の範囲に対してのトラックバー操作範囲の倍率)
};

// トラックバーグループ項目構造体
// 例：FILTER_ITEM_TRACK_GROUP trackGroup = { L"Group", tracks };
//     FILTER_ITEM_TRACK track0 = { ... }
//     FILTER_ITEM_TRACK track1 = { ... }
//     FILTER_ITEM_TRACK* tracks[] = { &track0, &track1, nullptr };
struct FILTER_ITEM_TRACK_GROUP {
	FILTER_ITEM_TRACK_GROUP(LPCWSTR name, FILTER_ITEM_TRACK** tracks) : name(name), tracks(tracks) {}
	LPCWSTR type = L"trackgroup";	// 設定の種別
	LPCWSTR name;					// 設定名
	FILTER_ITEM_TRACK** tracks;		// トラックバー項目グループ (FILTER_ITEM_TRACKポインタを列挙してnull終端したリストへのポインタ) ※2か3項目のみ
};

// チェックボックス項目構造体
// 例：FILTER_ITEM_CHECK check = { L"チェック", false };
struct FILTER_ITEM_CHECK {
	FILTER_ITEM_CHECK(LPCWSTR name, bool value) : name(name), value(value) {}
	LPCWSTR type = L"check";	// 設定の種別
	LPCWSTR name;				// 設定名
	bool value;					// 設定値 (フィルタ処理の呼び出し時に現在の値に更新されます)
};

// チェックボックス(セクション毎)項目構造体
// 例：FILTER_ITEM_CHECK_SECTION check = { L"チェック", false };
struct FILTER_ITEM_CHECK_SECTION {
	FILTER_ITEM_CHECK_SECTION(LPCWSTR name, bool value) : name(name), value(value) {}
	LPCWSTR type = L"checksection";	// 設定の種別
	LPCWSTR name;					// 設定名
	bool value;						// 設定値 (フィルタ処理の呼び出し時に現在の値に更新されます)
};

// 色選択項目構造体
// 例：FILTER_ITEM_COLOR color = { L"色", 0xffffff };
struct FILTER_ITEM_COLOR {
	union VALUE { // 設定値の色
		int code;
		struct { unsigned char b, g, r, x; };
	};
	FILTER_ITEM_COLOR(LPCWSTR name, int code) : name(name) { value.code = code; }
	LPCWSTR type = L"color";	// 設定の種別
	LPCWSTR name;				// 設定名
	VALUE value;				// 設定値 (フィルタ処理の呼び出し時に現在の値に更新されます)
};

// 選択リスト項目構造体
// 例：FILTER_ITEM_SELECT::ITEM list[] = { { L"項目1", 1 }, { L"項目2", 2 }, { L"項目3", 3 }, { nullptr } };
//     FILTER_ITEM_SELECT selectItem = { L"選択", 1, list };
struct FILTER_ITEM_SELECT {
	struct ITEM {		// 選択肢項目
		LPCWSTR name;	// 選択肢の名前
		int value;		// 選択肢の値
	};
	FILTER_ITEM_SELECT(LPCWSTR name, int value, ITEM* list) : name(name), value(value), list(list) {}
	LPCWSTR type = L"select";	// 設定の種別
	LPCWSTR name;				// 設定名
	int value;					// 設定値 (フィルタ処理の呼び出し時に現在の値に更新されます)
	ITEM* list;					// 選択肢リスト (ITEMを列挙して名前がnullのITEMで終端したリストへのポインタ)
};

// ファイル選択項目構造体
// 例：FILTER_ITEM_FILE file = { L"ファイル", L"", L"AviFile (*.avi)\0*.avi\0" };
struct FILTER_ITEM_FILE {
	FILTER_ITEM_FILE(LPCWSTR name, LPCWSTR value, LPCWSTR filefilter) : name(name), value(value), filefilter(filefilter) {}
	LPCWSTR type = L"file";		// 設定の種別
	LPCWSTR name;				// 設定名
	LPCWSTR value;				// 設定値 (フィルタ処理の呼び出し時に現在の値のポインタに更新されます)
	LPCWSTR filefilter;			// ファイルフィルタ
};

// 汎用データ項目構造体 (設定が表示されない項目になります)
// フィルタ処理関数内でvalueの参照先データを更新することが出来ます
// 例：struct Data {
//       int   item1 = 1;
//       float item2 = 2.0f;
//     };
//     FILTER_ITEM_DATA<Data> data = { L"データ" };
template<typename T>
struct FILTER_ITEM_DATA {
	FILTER_ITEM_DATA(LPCWSTR name) : name(name), size(sizeof(T)), value(&default_value) {}
	LPCWSTR type = L"data";		// 設定の種別
	LPCWSTR name;				// 設定名
	T* value;					// 設定値 (フィルタ処理の呼び出し時に現在の値のポインタに更新されます)
	const int size;				// 汎用データのサイズ(1024バイト以下)
	T default_value;			// デフォルト値 (Tの定義でデフォルト値を指定しておく)
};

// 設定グループ項目構造体
// 自身以降の設定項目をグループ化することが出来ます
// ※設定名を空にするとグループの終端を定義することが出来ます
// 例：FILTER_ITEM_GROUP group = { L"座標" };
struct FILTER_ITEM_GROUP {
	FILTER_ITEM_GROUP(LPCWSTR name, bool default_visible = true) : name(name), default_visible(default_visible) {}
	LPCWSTR type = L"group";	// 設定の種別
	LPCWSTR name;				// 設定名
	const bool default_visible;	// デフォルトの表示状態
};

// ボタン項目構造体
// ボタンを押すとコールバック関数が呼ばれます ※plugin2.hの編集のコールバック関数と同様な形になります
// 例：FILTER_ITEM_BUTTON button = { L"初期化", [](EDIT_SECTION* edit) { /* ボタンを押した時の処理 */ } };
struct FILTER_ITEM_BUTTON {
	FILTER_ITEM_BUTTON(LPCWSTR name, void (*callback)(EDIT_SECTION* edit)) : name(name), callback(callback) {}
	LPCWSTR type = L"button";			// 設定の種別
	LPCWSTR name;						// 設定名
	void (*callback)(EDIT_SECTION*);	// ボタンを押した時のコールバック関数 (呼び出し時に各設定項目の設定値が更新されます)
};

// 文字列項目構造体 ※1行の文字列
// 例：FILTER_ITEM_STRING string = { L"文字列", L"" };
struct FILTER_ITEM_STRING {
	FILTER_ITEM_STRING(LPCWSTR name, LPCWSTR value) : name(name), value(value) {}
	LPCWSTR type = L"string";	// 設定の種別
	LPCWSTR name;				// 設定名
	LPCWSTR value;				// 設定値 (フィルタ処理の呼び出し時に現在の値のポインタに更新されます)
};

// テキスト項目構造体 ※複数行の文字列
// 例：FILTER_ITEM_TEXT text = { L"テキスト", L"" };
struct FILTER_ITEM_TEXT {
	FILTER_ITEM_TEXT(LPCWSTR name, LPCWSTR value) : name(name), value(value) {}
	LPCWSTR type = L"text";		// 設定の種別
	LPCWSTR name;				// 設定名
	LPCWSTR value;				// 設定値 (フィルタ処理の呼び出し時に現在の値のポインタに更新されます)
};

// フォルダ選択項目構造体
// 例：FILTER_ITEM_FOLDER folder = { L"フォルダ", L"" };
struct FILTER_ITEM_FOLDER {
	FILTER_ITEM_FOLDER(LPCWSTR name, LPCWSTR value) : name(name), value(value) {}
	LPCWSTR type = L"folder";	// 設定の種別
	LPCWSTR name;				// 設定名
	LPCWSTR value;				// 設定値 (フィルタ処理の呼び出し時に現在の値のポインタに更新されます)
};

// セパレーター項目構造体
// 例：FILTER_ITEM_SEPARATOR separator = { L"中心座標" };
struct FILTER_ITEM_SEPARATOR {
	FILTER_ITEM_SEPARATOR(LPCWSTR name) : name(name) {}
	LPCWSTR type = L"separator";	// 設定の種別
	LPCWSTR name;					// 設定名
};

//----------------------------------------------------------------------------------

// 頂点データ構造体(描画色)
// { x, y, z, r, g, b, a }
struct VERTEX_COLOR {
	float x, y, z;		// 頂点座標
	float r, g, b, a;	// 頂点の色(0.0〜1.0の乗算済みα)
};

// 頂点データ構造体(描画色、法線)
// { x, y, z, r, g, b, a, vx, vy, vz }
struct VERTEX_COLOR_NORM {
	float x, y, z;		// 頂点座標
	float r, g, b, a;	// 頂点の色(0.0〜1.0の乗算済みα)
	float vx, vy, vz;	// 法線ベクトル
};

// 頂点データ構造体(テクスチャ)
// { x, y, z, u, v, a }
struct VERTEX_TEXTURE {
	float x, y, z;		// 頂点座標
	float u, v;			// テクスチャー座標(0.0〜1.0の正規化座標)
	float a;			// 頂点のα値
};

// 頂点データ構造体(テクスチャ、法線)
// { x, y, z, u, v, a, vx, vy, vz }
struct VERTEX_TEXTURE_NORM {
	float x, y, z;		// 頂点座標
	float u, v;			// テクスチャー座標(0.0〜1.0の正規化座標)
	float a;			// 頂点のα値
	float vx, vy, vz;	// 法線ベクトル
};

// 頂点リストの種別
enum class VERTEX_TYPE : int {
	TRIANGLE_COLOR			= 1,	// 三角形のVERTEX_COLORのリスト (頂点数は3の倍数になる)
	TRIANGLE_COLOR_NORM		= 2,	// 三角形のVERTEX_COLOR_NORMのリスト (頂点数は3の倍数になる)
	TRIANGLE_TEXTURE		= 3,	// 三角形のVERTEX_TEXTUREのリスト (頂点数は3の倍数になる)
	TRIANGLE_TEXTURE_NORM	= 4,	// 三角形のVERTEX_TEXTURE_NORMのリスト (頂点数は3の倍数になる)
	QUAD_COLOR				= 5,	// 四角形のVERTEX_COLORのリスト (頂点数は4の倍数になる)
	QUAD_COLOR_NORM			= 6,	// 四角形のVERTEX_COLOR_NORMのリスト (頂点数は4の倍数になる)
	QUAD_TEXTURE			= 7,	// 四角形のVERTEX_TEXTUREのリスト (頂点数は4の倍数になる)
	QUAD_TEXTURE_NORM		= 8,	// 四角形のVERTEX_TEXTURE_NORMのリスト (頂点数は4の倍数になる)
};

// 合成モードの種別
enum class BLEND_MODE : int {
	NONE		= 0,	// 通常
	ADD			= 1,	// 加算
	SUB			= 2,	// 減算
	MUL			= 3,	// 乗算
	SCREEN		= 4,	// スクリーン
	OVERLAY		= 5,	// オーバーレイ
	LIGHT		= 6,	// 比較(明)
	DARK		= 7,	// 比較(暗)
	BRIGHTNESS	= 8,	// 輝度
	CHROMA		= 9,	// 色差
	SHADOW		= 10,	// 陰影
	LIGHT_DARK	= 11,	// 明暗
	DIFF		= 12,	// 差分
};

// ビルボードの種別
enum class BILLBOARD_MODE : int {
	NONE		= 0,	// 標準の向き(何もしない)
	SIDE		= 1,	// 横方向のみカメラに向ける
	DIRECTION	= 2,	// 縦横方向のみカメラに向ける
	CAMERA		= 3,	// カメラに向ける
};

// サンプラー(SampleState)の種別
enum class SAMPLER_MODE : int {
	CLIP	= 0,	// 領域外は透明色
	CLAMP	= 1,	// 領域外は一番外側の色
	LOOP	= 2,	// 領域外はループ
	MIRROR	= 3,	// 領域外は領域を反転しながらループ
	DOT		= 4,	// 拡大縮小補間をしない(領域外は透明色)
};

// 出力ブレンド(BlendState)の種別
enum class BLEND_STATE_MODE : int {
	COPY	= 0,	// 出力をそのままコピー
	MASK	= 1,	// α値のみを乗算 ※RGB値は利用されません
	DRAW	= 2,	// 出力をアルファブレンド
	ADD		= 3,	// 出力を加算合成
};

//----------------------------------------------------------------------------------

// 画像入力のピクセルフォーマット種別
enum class INPUT_PIXEL_FORMAT : int {
	RGBA	= 28,	// DXGI_FORMAT_R8G8B8A8_UNORM ※PIXEL_RGBA
	BGRA	= 87,	// DXGI_FORMAT_B8G8R8A8_UNORM
	BGR		= 88,	// DXGI_FORMAT_B8G8R8X8_UNORM
	PA64	= 11,	// DXGI_FORMAT_R16G16B16A16_UNORM
	HF64	= 10,	// DXGI_FORMAT_R16G16B16A16_FLOAT
	YUY2	= 107,	// DXGI_FORMAT_YUY2
	YC48	= 13,	// DXGI_FORMAT_R16G16B16A16_SNORM ※互換対応
};

// 画像出力のピクセルフォーマット種別
enum class OUTPUT_PIXEL_FORMAT : int {
	RGBA	= 28,	// DXGI_FORMAT_R8G8B8A8_UNORM ※PIXEL_RGBA
	PA64	= 11,	// DXGI_FORMAT_R16G16B16A16_UNORM
	HF64	= 10,	// DXGI_FORMAT_R16G16B16A16_FLOAT
};

//----------------------------------------------------------------------------------
 
// RGBA32bit構造体
struct PIXEL_RGBA {
	unsigned char r, g, b, a;
};

// シーン情報構造体
struct SCENE_INFO {
	int width, height;		// シーンの解像度
	int rate, scale;		// シーンのフレームレート
	int sample_rate;		// シーンのサンプリングレート
};

// オブジェクト情報構造体
struct OBJECT_INFO {
	int64_t id;				// オブジェクトのID (アプリ起動毎の固有ID)
							// ※描画対象のオブジェクトの固有ID
	int frame;				// オブジェクトの現在のフレーム番号
	int frame_total;		// オブジェクトの総フレーム数
	double time;			// オブジェクトの現在の時間(秒)
	double time_total;		// オブジェクトの総時間(秒)
	int width, height;		// オブジェクトの現在の画像サイズ (画像フィルタのみ)
	int64_t sample_index;	// オブジェクトの現在の音声サンプル位置 (音声フィルタのみ)
	int64_t sample_total;	// オブジェクトの総サンプル数 (音声フィルタのみ)
	int sample_num;			// オブジェクトの現在の音声サンプル数 (音声フィルタのみ)
	int channel_num;		// オブジェクトの現在の音声チャンネル数 (音声フィルタのみ) ※通常2になります
	int64_t effect_id;		// オブジェクトの内の対象エフェクトのID (アプリ起動毎の固有ID)
							// ※処理対象のフィルタ効果、オブジェクト入出力の固有ID
	int flag;				// フラグ
	static constexpr int FLAG_FILTER_OBJECT = 1;	// フィルタオブジェクトか？
	int layer;				// オブジェクトの現在のレイヤー番号 ※描画対象のオブジェクトのレイヤー番号
	int	index;				// 複数オブジェクト時の現在の対象番号 ※個別オブジェクト用
	int	num;				// 複数オブジェクト時の対象数 (1 = 単体オブジェクト / 0 = 不定) ※個別オブジェクト用

	inline bool is_filter_object() const { return flag & FLAG_FILTER_OBJECT; }
};

// オブジェクトの画像パラメータ構造体
struct OBJECT_IMAGE_PARAM {
	float x, y, z;		// 基準座標
	float rx, ry, rz;	// 回転角度 (360.0で1回転)
	float sx, sy, sz;	// 拡大率 (1.0=等倍)
	float cx, cy, cz;	// 中心座標 (基準座標からの相対)	
	float alpha;		// 不透明度 (0.0〜1.0/0.0=透明/1.0=不透明)
};

// オブジェクトの音声パラメータ構造体
struct OBJECT_AUDIO_PARAM {
	float vol_l, vol_r;	// 音量倍率 (1.0=等倍)
};

//----------------------------------------------------------------------------------

// d3d11.h向けの前方宣言 ※includeしないで良いように定義
struct ID3D11Texture2D;
struct ID3D11BlendState;
struct ID3D11SamplerState;

// 画像フィルタ処理用構造体
struct FILTER_PROC_VIDEO {
	// シーン情報
	const SCENE_INFO* scene;

	// オブジェクト情報
	const OBJECT_INFO* object;

	// 現在のオブジェクトの画像データをPIXEL_RGBA形式で取得する (VRAMからデータを取得します) 
	// buffer		: 画像データの格納先へのポインタ
	void (*get_image_data)(PIXEL_RGBA* buffer);

	// 現在のオブジェクトの画像データをPIXEL_RGBA形式で設定する (VRAMへデータを書き込みます) 
	// buffer		: 画像データへのポインタ (nullptrの場合は初期データ無しで画像サイズを変更します)
	// width,height	: 画像サイズ
	void (*set_image_data)(const PIXEL_RGBA* buffer, int width, int height);

	// 現在のオブジェクトのD3D画像リソースのポインタを取得する (ID3D11Texture2Dのポインタを取得します) 
	// 戻り値		: オブジェクト画像のID3D11Texture2Dのポインタ
	//				  ※現在の画像が変更(set_image_data)されるかフィルタ処理の終了まで有効
	ID3D11Texture2D* (*get_image_texture2d)();

	// 現在のフレームバッファのD3D画像リソースのポインタを取得する (ID3D11Texture2Dのポインタを取得します) 
	// 戻り値		: フレームバッファのID3D11Texture2Dのポインタ
	//				  ※フィルタ処理の終了まで有効
	ID3D11Texture2D* (*get_framebuffer_texture2d)();

	// 編集セクション関数
	// フィルタ処理中は参照系の関数が利用出来ます
	EDIT_SECTION* edit;

	// 現在のオブジェクトの画像パラメータ情報
	// パラメータを直接変更することが出来ます
	// ※このパラメータは画像出力項目のパラメータからの相対設定になります (スクリプトのobj.ox等と同じ)
	OBJECT_IMAGE_PARAM* param;

	// 指定オブジェクトの画像出力項目のパラメータを取得する
	// object		: 対象のオブジェクトのハンドル (nullptrを指定すると現在のオブジェクトが対象)
	// offset		: 取得時間のオフセット(秒) (0なら現時間)
	// output		: パラメータの格納先へのポインタ
	// output_size	: パラメータの格納先のサイズ ※サイズ分のみ取得されます
	// 戻り値		: 取得出来ない場合はfalse (画像オブジェクト以外が指定された場合)
	bool (*get_output_image_param)(OBJECT_HANDLE object, double offset, OBJECT_IMAGE_PARAM* param, int param_size);

	// 指定のレイヤーにある画像オブジェクトを取得します
	// layer	: 対象のレイヤー番号
	// offset	: 取得時間のオフセット(秒) (0なら現時間)
	// 戻り値	: 取得したオブジェクトのハンドル (存在しない場合はnullptrを返却)
	OBJECT_HANDLE (*get_image_object)(int layer, double offset);

	// 指定の画像リソースをフレームバッファに描画します
	// resource	: 画像リソース名
	//			  "object"			= 現在のオブジェクト ※nullptrの指定でも現在のオブジェクトになります
	//			  "resource:xxxx"	= 標準リソース(xxxxは任意の名前)
	//			  "tempbuffer"		= 仮想バッファ
	//			  "cache:xxxx"		= キャッシュバッファ(xxxxは任意の名前)
	//			  "image:xxxx"		= 画像ファイル(xxxxは画像ファイルパス) ※画像はVRAMにキャッシュされます
	// x,y,z	: 基準座標
	// rx,ry,rz	: 回転角度 (360.0で1回転)
	// sx,sy,sz	: 拡大率 (1.0=等倍)
	// alpha	: 不透明度 (0.0〜1.0/0.0=透明/1.0=不透明)
	// 戻り値	: 失敗した場合はfalse (画像リソース名が不正な場合等)
	bool (*draw_image)(LPCWSTR resource, float x, float y, float z, float rx, float ry, float rz, float sx, float sy, float sz, float alpha);

	// 指定の頂点リストのポリゴンをフレームバッファに描画します
	// vertex_type	: 頂点リストの種別
	// vertex_list	: 頂点データリストへのポインタ (指定した種別の頂点データバッファへのポインタ)
	// vertex_num	: 頂点リストの頂点数 (頂点データの数)
	// resource		: テクスチャ画像リソース名 ※テクスチャ付きの場合のみ利用 
	//				  "object"			= 現在のオブジェクト ※nullptrの指定でも現在のオブジェクトになります
	//				  "resource:xxxx"	= 標準リソース(xxxxは任意の名前)
	//				  "tempbuffer"		= 仮想バッファ
	//				  "cache:xxxx"		= キャッシュバッファ(xxxxは任意の名前)
	//				  "image:xxxx"		= 画像ファイル(xxxxは画像ファイルパス) ※画像はVRAMにキャッシュされます
	// 戻り値		: 失敗した場合はfalse (頂点数が不正な場合等)
	bool (*draw_poly)(VERTEX_TYPE vertex_type, const void* vertex_list, int vertex_num, LPCWSTR resource);

	// 標準のアンカー枠を設定します ※func_proc_video()でtrueを返却した場合は自動で設定されます
	// draw_image()などを利用してオブジェクトの描画を全て自身で処理する場合に利用します
	// width,height		: オブジェクトのサイズ ※0を指定すると固定サイズのアンカー枠になります
	void (*set_default_anchor)(int width, int height);

	// 描画時の合成モードを設定します
	// 合成モードを利用すると描画処理が重くなります
	// blend	: 合成モード
	void (*set_blend_mode)(BLEND_MODE blend);

	// 描画時の光沢度を設定します
	// カメラ制御の光源設定が有効の時に利用されます
	// shine	: 光沢度(0.0〜1.0)
	void (*set_material_shine)(float shine);

	// 描画時のサンプラーを設定します
	// sampler	: サンプラー種別
	void (*set_sampler_mode)(SAMPLER_MODE sampler);

	// 描画時に裏面を非表示にするかを設定します
	// culling	: 裏面を非表示にするか？
	void (*set_culling_state)(bool culling);

	// 描画時にオブジェクトをカメラの方向に向けるかを設定します
	// billboard	: ビルボード種別
	void (*set_billboard_mode)(BILLBOARD_MODE billboard);

	// 画像リソースを作成する (VRAMへデータを書き込みます)
	// 画像リソースはdraw_image()などの描画に利用出来ます
	// ※既に存在する画像リソース名を指定した場合はリソースを更新します
	// resource		: 作成する画像リソース名
	//				  "object"			= 現在のオブジェクト ※nullptrの指定でも現在のオブジェクトになります
	//				  "resource:xxxx"	= 標準リソース(xxxxは任意の名前) ※フィルタ処理後に破棄されます
	//				  "tempbuffer"		= 仮想バッファ ※内部実装はキャッシュバッファと同じ
	//				  "cache:xxxx"		= キャッシュバッファ(xxxxは任意の名前) ※レンダリング処理共用のキャッシュバッファ
	// buffer		: 画像データへのポインタ ※PIXEL_RGBA形式 (nullptrの場合は初期データ無しで作成します)
	// width,height	: 画像サイズ
	void (*create_image_resource)(LPCWSTR resource, const PIXEL_RGBA* buffer, int width, int height);

	// 指定の画像リソースのD3D画像リソースのポインタを取得する (ID3D11Texture2Dのポインタを取得します) 
	// resource	: 画像リソース名
	//			  "object"			= 現在のオブジェクト ※nullptrの指定でも現在のオブジェクトになります
	//			  "resource:xxxx"	= 標準リソース(xxxxは任意の名前)
	//			  "tempbuffer"		= 仮想バッファ
	//			  "cache:xxxx"		= キャッシュバッファ(xxxxは任意の名前)
	//			  "image:xxxx"		= 画像ファイル(xxxxは画像ファイルパス) ※画像はVRAMにキャッシュされます
	//			  "random"			= 乱数バッファ(0.0〜1.0の乱数値の256x256の領域) ※DXGI_FORMAT_R32_FLOAT(r値のみ)になります
	// 戻り値	: 画像リソースのID3D11Texture2Dのポインタ (指定リソースが無い場合はnullptrを返却)
	//			  ※画像リソースが変更されるかフィルタ処理の終了まで有効
	ID3D11Texture2D* (*get_image_resource_texture2d)(LPCWSTR resource);

	// 画像リソースをコピーする
	// dst_resource	: コピー先の画像リソース名
	//				  ※既に存在する画像リソース名を指定した場合はリソースを更新します
	//				  "object"			= 現在のオブジェクト ※nullptrの指定でも現在のオブジェクトになります
	//				  "resource:xxxx"	= 標準リソース(xxxxは任意の名前) ※フィルタ処理後に破棄されます
	//				  "tempbuffer"		= 仮想バッファ ※内部実装はキャッシュバッファと同じ
	//				  "cache:xxxx"		= キャッシュバッファ(xxxxは任意の名前) ※レンダリング処理共用のキャッシュバッファ
	// src_resource	: コピー元の画像リソース名
	//				  "object"			= 現在のオブジェクト ※nullptrの指定でも現在のオブジェクトになります
	//				  "resource:xxxx"	= 標準リソース(xxxxは任意の名前)
	//				  "framebuffer"		= フレームバッファ
	//				  "tempbuffer"		= 仮想バッファ
	//				  "cache:xxxx"		= キャッシュバッファ(xxxxは任意の名前)
	//				  "image:xxxx"		= 画像ファイル(xxxxは画像ファイルパス) ※画像はVRAMにキャッシュされます
	//				  "random"			= 乱数バッファ(0.0〜1.0の乱数値の256x256の領域) ※DXGI_FORMAT_R32_FLOAT(r値のみ)になります
	// 戻り値		: 失敗した場合はfalse (画像リソース名が不正な場合等)
	bool (*copy_image_resource)(LPCWSTR dst_resource, LPCWSTR src_resource);

	// 画像リソースをクリアする
	// resource		: クリアする画像リソース名
	//				  "object"			= 現在のオブジェクト ※nullptrの指定でも現在のオブジェクトになります
	//				  "resource:xxxx"	= 標準リソース(xxxxは任意の名前)
	//				  "tempbuffer"		= 仮想バッファ
	//				  "cache:xxxx"		= キャッシュバッファ(xxxxは任意の名前)
	// color		: クリアする色
	// 戻り値		: 失敗した場合はfalse (画像リソース名が不正な場合等)
	bool (*clear_image_resource)(LPCWSTR resource, PIXEL_RGBA color);

	// 指定の画像リソースを描画先の画像リソースに描画します
	// dst_resource	: 描画先の画像リソース名
	//				  "object"			= 現在のオブジェクト ※nullptrの指定でも現在のオブジェクトになります
	//				  "resource:xxxx"	= 標準リソース(xxxxは任意の名前)
	//				  "tempbuffer"		= 仮想バッファ
	//				  "cache:xxxx"		= キャッシュバッファ(xxxxは任意の名前)
	// src_resource	: 画像リソース名
	//				  "object"			= 現在のオブジェクト ※nullptrの指定でも現在のオブジェクトになります
	//				  "resource:xxxx"	= 標準リソース(xxxxは任意の名前)
	//				  "tempbuffer"		= 仮想バッファ
	//				  "cache:xxxx"		= キャッシュバッファ(xxxxは任意の名前)
	//				  "image:xxxx"		= 画像ファイル(xxxxは画像ファイルパス) ※画像はVRAMにキャッシュされます
	// x,y,z		: 基準座標
	// rx,ry,rz		: 回転角度 (360.0で1回転)
	// sx,sy,sz		: 拡大率 (1.0=等倍)
	// alpha		: 不透明度 (0.0〜1.0/0.0=透明/1.0=不透明)
	// 戻り値		: 失敗した場合はfalse (画像リソース名が不正な場合等)
	bool (*draw_image_to_resource)(LPCWSTR dst_resource, LPCWSTR src_resource, float x, float y, float z, float rx, float ry, float rz, float sx, float sy, float sz, float alpha);

	// 指定の頂点リストのポリゴンを描画先の画像リソースに描画します
	// dst_resource	: 描画先の画像リソース名
	//				  "object"			= 現在のオブジェクト ※nullptrの指定でも現在のオブジェクトになります
	//				  "resource:xxxx"	= 標準リソース(xxxxは任意の名前)
	//				  "tempbuffer"		= 仮想バッファ
	//				  "cache:xxxx"		= キャッシュバッファ(xxxxは任意の名前)
	// vertex_type	: 頂点リストの種別
	// vertex_list	: 頂点データリストへのポインタ (指定した種別の頂点データバッファへのポインタ)
	// vertex_num	: 頂点リストの頂点数 (頂点データの数)
	// src_resource	: テクスチャ画像リソース名 ※テクスチャ付きの場合のみ利用 
	//				  "object"			= 現在のオブジェクト ※nullptrの指定でも現在のオブジェクトになります
	//				  "resource:xxxx"	= 標準リソース(xxxxは任意の名前)
	//				  "tempbuffer"		= 仮想バッファ
	//				  "cache:xxxx"		= キャッシュバッファ(xxxxは任意の名前)
	//				  "image:xxxx"		= 画像ファイル(xxxxは画像ファイルパス) ※画像はVRAMにキャッシュされます
	// 戻り値		: 失敗した場合はfalse (頂点数が不正な場合等)
	bool (*draw_poly_to_resource)(LPCWSTR dst_resource, VERTEX_TYPE vertex_type, const void* vertex_list, int vertex_num, LPCWSTR src_resource);

	// ピクセルシェーダーを実行します
	// cso_file			: コンパイル済みピクセルシェーダーのバイナリファイル名 ※ファイル名部分のみ
	//					  プラグインと同じフォルダのファイルから読み込んでキャッシュします
	//					  ※ピクセルシェーダー5.0でコンパイルしたものが利用出来ます
	//					  ピクセルシェーダーの入力は下記が利用できます
	//						float4 psmain(float4 pos : SV_Position) : SV_Target
	//						float4 psmain(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target
	//					  ※シェーダーリフレクションを利用してシグネチャから判別しています
	// target			: 出力先の画像リソース名
	//					  Direct3Dのレンダーターゲットに設定されます
	//					  "object"			= 現在のオブジェクト ※nullptrの指定でも現在のオブジェクトになります
	//					  "resource:xxxx"	= 標準リソース(xxxxは任意の名前)
	//					  "framebuffer"		= フレームバッファ
	//					  "tempbuffer"		= 仮想バッファ
	//					  "cache:xxxx"		= キャッシュバッファ(xxxxは任意の名前)
	// resource_list	: 参照する画像リソース名のリストへのポインタ ※nullptrの場合は設定無し
	//					  Direct3Dのシェーダーリソース(t0〜)に設定されます ※レンダーターゲットと同じリソースは利用出来ません
	//					  "object"			= 現在のオブジェクト ※nullptrの指定でも現在のオブジェクトになります
	//					  "resource:xxxx"	= 標準リソース(xxxxは任意の名前)
	//					  "tempbuffer"		= 仮想バッファ
	//					  "cache:xxxx"		= キャッシュバッファ(xxxxは任意の名前)
	//					  "image:xxxx"		= 画像ファイル(xxxxは画像ファイルパス) ※画像はVRAMにキャッシュされます
	//					  "random"			= 乱数バッファ(0.0〜1.0の乱数値の256x256の領域) ※DXGI_FORMAT_R32_FLOAT(r値のみ)になります
	// resource_num		: 参照する画像リソースの数
	// constant			: 定数バッファへのポインタ ※nullptrの場合は定数バッファの設定無し
	//					  Direct3Dの定数バッファ(b0)に設定します
	// constant_size	: 定数バッファのサイズ
	// blend_state		: Direct3DのBlendStateを設定します ※nullptrの場合は出力をそのままコピー
	// sampler_state	: Direct3DのSamplerState(s0)を設定します ※nullptrの場合は設定無し
	// 戻り値			: 失敗した場合はfalse (画像リソース名が不正な場合等)
	bool (*exec_pixelshader_file)(LPCWSTR cso_file, LPCWSTR target, LPCWSTR* resource_list, int resource_num, void* constant, int constant_size, ID3D11BlendState* blend_state, ID3D11SamplerState* sampler_state);

	// コンピュートシェーダーを実行します
	// cso_file			: コンパイル済みコンピュートシェーダーのバイナリファイル名 ※ファイル名部分のみ
	//					  プラグインと同じフォルダのファイルから読み込んでキャッシュします
	//					  ※コンピュートシェーダー5.0でコンパイルしたものが利用出来ます
	// target_list		: 読み書き先の画像リソース名のリストへのポインタ
	//					  Direct3DのUnorderedAccessリソース(u0〜)に設定されます
	//					  "object"			= 現在のオブジェクト ※nullptrの指定でも現在のオブジェクトになります
	//					  "resource:xxxx"	= 標準リソース(xxxxは任意の名前)
	//					  "framebuffer"		= フレームバッファ
	//					  "tempbuffer"		= 仮想バッファ
	//					  "cache:xxxx"		= キャッシュバッファ(xxxxは任意の名前)
	// target_num		: 読み書き先の画像リソースの数
	// resource_list	: 参照する画像リソース名のリストへのポインタ ※nullptrの場合は設定無し
	//					  Direct3Dのシェーダーリソース(t0〜)に設定されます ※UnorderedAccessリソースと同じリソースは利用出来ません
	//					  "object"			= 現在のオブジェクト ※nullptrの指定でも現在のオブジェクトになります
	//					  "resource:xxxx"	= 標準リソース(xxxxは任意の名前)
	//					  "tempbuffer"		= 仮想バッファ
	//					  "cache:xxxx"		= キャッシュバッファ(xxxxは任意の名前)
	//					  "image:xxxx"		= 画像ファイル(xxxxは画像ファイルパス) ※画像はVRAMにキャッシュされます
	//					  "random"			= 乱数バッファ(0.0〜1.0の乱数値の256x256の領域) ※DXGI_FORMAT_R32_FLOAT(r値のみ)になります
	// resource_num		: 参照する画像リソースの数
	// constant			: 定数バッファへのポインタ ※nullptrの場合は定数バッファの設定無し
	//					  Direct3Dの定数バッファ(b0)に設定します
	// constant_size	: 定数バッファのサイズ
	// count_x			: X軸スレッドグループ数
	// count_y			: Y軸スレッドグループ数
	// count_z			: Z軸スレッドグループ数
	// sampler_state	: Direct3DのSamplerState(s0)を設定します ※nullptrの場合は設定無し
	// 戻り値			: 失敗した場合はfalse (画像リソース名が不正な場合等)
	bool (*exec_computeshader_file)(LPCWSTR cso_file, LPCWSTR* target_list, int target_num, LPCWSTR* resource_list, int resource_num, void* constant, int constant_size, int count_x, int count_y, int count_z, ID3D11SamplerState* sampler_state);

	// 定義済みのD3Dの出力ブレンドのリソースのポインタを取得する (ID3D11BlendStateのポインタを取得します) 
	// blend	: 出力ブレンド種別
	// 戻り値	: ID3D11BlendStateのポインタ (指定種別が無い場合はnullptrを返却)
	ID3D11BlendState* (*get_blend_state)(BLEND_STATE_MODE blend);

	// 定義済みのD3Dのサンプラーのリソースのポインタを取得する (ID3D11SamplerStateのポインタを取得します) 
	// sampler	: サンプラー種別
	// 戻り値	: ID3D11SamplerStateのポインタ (指定種別が無い場合はnullptrを返却)
	ID3D11SamplerState* (*get_sampler_state)(SAMPLER_MODE sampler);

	// ピクセルシェーダーを実行します
	// data				: コンパイル済みピクセルシェーダーのデータへのポインタ(ヘッダーファイルとして出力したデータを利用する)
	// data_size		: コンパイル済みピクセルシェーダーのサイズ
	//					  ※ピクセルシェーダー5.0でコンパイルしたものが利用出来ます
	//					  ピクセルシェーダーの入力は下記が利用できます
	//						float4 psmain(float4 pos : SV_Position) : SV_Target
	//						float4 psmain(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target
	//					  ※シェーダーリフレクションを利用してシグネチャから判別しています
	// target			: 出力先の画像リソース名
	//					  Direct3Dのレンダーターゲットに設定されます
	//					  "object"			= 現在のオブジェクト ※nullptrの指定でも現在のオブジェクトになります
	//					  "resource:xxxx"	= 標準リソース(xxxxは任意の名前)
	//					  "framebuffer"		= フレームバッファ
	//					  "tempbuffer"		= 仮想バッファ
	//					  "cache:xxxx"		= キャッシュバッファ(xxxxは任意の名前)
	// resource_list	: 参照する画像リソース名のリストへのポインタ ※nullptrの場合は設定無し
	//					  Direct3Dのシェーダーリソース(t0〜)に設定されます ※レンダーターゲットと同じリソースは利用出来ません
	//					  "object"			= 現在のオブジェクト ※nullptrの指定でも現在のオブジェクトになります
	//					  "resource:xxxx"	= 標準リソース(xxxxは任意の名前)
	//					  "tempbuffer"		= 仮想バッファ
	//					  "cache:xxxx"		= キャッシュバッファ(xxxxは任意の名前)
	//					  "image:xxxx"		= 画像ファイル(xxxxは画像ファイルパス) ※画像はVRAMにキャッシュされます
	//					  "random"			= 乱数バッファ(0.0〜1.0の乱数値の256x256の領域) ※DXGI_FORMAT_R32_FLOAT(r値のみ)になります
	// resource_num		: 参照する画像リソースの数
	// constant			: 定数バッファへのポインタ ※nullptrの場合は定数バッファの設定無し
	//					  Direct3Dの定数バッファ(b0)に設定します
	// constant_size	: 定数バッファのサイズ
	// blend_state		: Direct3DのBlendStateを設定します ※nullptrの場合は出力をそのままコピー
	// sampler_state	: Direct3DのSamplerState(s0)を設定します ※nullptrの場合は設定無し
	// 戻り値			: 失敗した場合はfalse (画像リソース名が不正な場合等)
	bool (*exec_pixelshader_data)(const BYTE* data, int data_size, LPCWSTR target, LPCWSTR* resource_list, int resource_num, void* constant, int constant_size, ID3D11BlendState* blend_state, ID3D11SamplerState* sampler_state);

	// コンピュートシェーダーを実行します
	// data				: コンパイル済みコンピュートシェーダーのデータへのポインタ(ヘッダーファイルとして出力したデータを利用する)
	// data_size		: コンパイル済みコンピュートシェーダーのサイズ
	//					  ※コンピュートシェーダー5.0でコンパイルしたものが利用出来ます
	// target_list		: 読み書き先の画像リソース名のリストへのポインタ
	//					  Direct3DのUnorderedAccessリソース(u0〜)に設定されます
	//					  "object"			= 現在のオブジェクト ※nullptrの指定でも現在のオブジェクトになります
	//					  "resource:xxxx"	= 標準リソース(xxxxは任意の名前)
	//					  "framebuffer"		= フレームバッファ
	//					  "tempbuffer"		= 仮想バッファ
	//					  "cache:xxxx"		= キャッシュバッファ(xxxxは任意の名前)
	// target_num		: 読み書き先の画像リソースの数
	// resource_list	: 参照する画像リソース名のリストへのポインタ ※nullptrの場合は設定無し
	//					  Direct3Dのシェーダーリソース(t0〜)に設定されます ※UnorderedAccessリソースと同じリソースは利用出来ません
	//					  "object"			= 現在のオブジェクト ※nullptrの指定でも現在のオブジェクトになります
	//					  "resource:xxxx"	= 標準リソース(xxxxは任意の名前)
	//					  "tempbuffer"		= 仮想バッファ
	//					  "cache:xxxx"		= キャッシュバッファ(xxxxは任意の名前)
	//					  "image:xxxx"		= 画像ファイル(xxxxは画像ファイルパス) ※画像はVRAMにキャッシュされます
	//					  "random"			= 乱数バッファ(0.0〜1.0の乱数値の256x256の領域) ※DXGI_FORMAT_R32_FLOAT(r値のみ)になります
	// resource_num		: 参照する画像リソースの数
	// constant			: 定数バッファへのポインタ ※nullptrの場合は定数バッファの設定無し
	//					  Direct3Dの定数バッファ(b0)に設定します
	// constant_size	: 定数バッファのサイズ
	// count_x			: X軸スレッドグループ数
	// count_y			: Y軸スレッドグループ数
	// count_z			: Z軸スレッドグループ数
	// sampler_state	: Direct3DのSamplerState(s0)を設定します ※nullptrの場合は設定無し
	// 戻り値			: 失敗した場合はfalse (画像リソース名が不正な場合等)
	bool (*exec_computeshader_data)(const BYTE* data, int data_size, LPCWSTR* target_list, int target_num, LPCWSTR* resource_list, int resource_num, void* constant, int constant_size, int count_x, int count_y, int count_z, ID3D11SamplerState* sampler_state);

	// 指定の画像リソースのサイズを取得する
	// resource		: 画像リソース名
	//				  "object"			= 現在のオブジェクト ※nullptrの指定でも現在のオブジェクトになります
	//				  "resource:xxxx"	= 標準リソース(xxxxは任意の名前)
	//				  "tempbuffer"		= 仮想バッファ
	//				  "cache:xxxx"		= キャッシュバッファ(xxxxは任意の名前)
	//				  "image:xxxx"		= 画像ファイル(xxxxは画像ファイルパス) ※画像はVRAMにキャッシュされます
	// width,height	: 画像サイズの格納先へのポインタ
	// 戻り値		: 失敗した場合はfalse (画像リソース名が不正な場合等)
	bool (*get_image_resource_size)(LPCWSTR resource, int* width, int* height);

	// 画像リソースから指定フォーマットの画像データを取得する (VRAMからデータを取得します) 
	// resource		: 画像リソース名
	//				  "object"			= 現在のオブジェクト ※nullptrの指定でも現在のオブジェクトになります
	//				  "resource:xxxx"	= 標準リソース(xxxxは任意の名前)
	// buffer		: 画像データの格納先へのポインタ
	// width,height	: 画像データの格納先のサイズ ※画像リソースとサイズが一致する場合のみ取得出来ます
	// pitch		: 画像データの格納先の横1ラインのバイト数
	// format		: 取得する画像データのピクセルフォーマット
	// 戻り値		: 失敗した場合はfalse (画像リソース名が不正な場合等)
	bool (*get_image_resource_data)(LPCWSTR resource, void* buffer, int width, int height, int pitch, OUTPUT_PIXEL_FORMAT format);

	// 画像リソースに指定フォーマットの画像データを設定する (VRAMへデータを書き込みます)
	// ※存在しない画像リソース名を指定した場合は新規作成します
	// resource		: 作成する画像リソース名
	//				  "object"			= 現在のオブジェクト ※nullptrの指定でも現在のオブジェクトになります
	//				  "resource:xxxx"	= 標準リソース(xxxxは任意の名前) ※フィルタ処理後に破棄されます
	// buffer		: 画像データへのポインタ
	// width,height	: 画像サイズ
	// pitch		: 画像データの横1ラインのバイト数
	// format		: 画像データのピクセルフォーマット
	// 戻り値		: 失敗した場合はfalse (画像リソース名が不正な場合等)
	bool (*set_image_resource_data)(LPCWSTR resource, const void* buffer, int width, int height, int pitch, INPUT_PIXEL_FORMAT format);

};

//----------------------------------------------------------------------------------

// 音声フィルタ処理用構造体
struct FILTER_PROC_AUDIO {
	// シーン情報
	const SCENE_INFO* scene;

	// オブジェクト情報
	const OBJECT_INFO* object;

	// 現在のオブジェクトの音声データを取得する
	// buffer		: 音声データの格納先へのポインタ ※音声データはPCM(float)32bit
	// channel		: 音声データのチャンネル ( 0 = 左チャンネル / 1 = 右チャンネル )
	void (*get_sample_data)(float* buffer, int channel);

	// 現在のオブジェクトの音声データを設定する
	// buffer		: 音声データへのポインタ ※音声データはPCM(float)32bit
	// channel		: 音声データのチャンネル ( 0 = 左チャンネル / 1 = 右チャンネル )
	void (*set_sample_data)(const float* buffer, int channel);

	// 編集セクション関数
	// フィルタ処理中は参照系の関数が利用出来ます
	EDIT_SECTION* edit;

	// 現在のオブジェクトの音声パラメータ情報
	// パラメータを直接変更することが出来ます
	// ※このパラメータは音声出力項目のパラメータからの相対設定になります
	OBJECT_AUDIO_PARAM* param;

	// 指定オブジェクトの音声出力項目のパラメータを取得する
	// object		: 対象のオブジェクトのハンドル (nullptrを指定すると現在のオブジェクトが対象)
	// offset		: 取得時間のオフセット(秒) (0なら現時間)
	// param		: パラメータの格納先へのポインタ
	// param_size	: パラメータの格納先のサイズ ※サイズ分のみ取得されます
	// 戻り値		: 取得出来ない場合はfalse (音声オブジェクト以外が指定された場合)
	bool (*get_output_audio_param)(OBJECT_HANDLE object, double offset, OBJECT_AUDIO_PARAM* param, int param_size);

	// 指定のレイヤー位置にある音声オブジェクトを取得します
	// layer	: 対象のレイヤー番号
	// offset	: 取得時間のオフセット(秒) (0なら現時間)
	// 戻り値	: 取得したオブジェクトのハンドル (存在しない場合はnullptrを返却)
	OBJECT_HANDLE (*get_audio_object)(int layer, double offset);

};

//----------------------------------------------------------------------------------

// フィルタプラグイン構造体
struct FILTER_PLUGIN_TABLE {
	int flag;								// フラグ
	static constexpr int FLAG_VIDEO = 1;	// 画像フィルタをサポートする
	static constexpr int FLAG_AUDIO = 2;	// 音声フィルタをサポートする
											// 画像と音声のフィルタ処理は別々のスレッドで処理されます
	static constexpr int FLAG_INPUT = 4;	// メディアオブジェクトの初期入力をする (メディアオブジェクトにする場合)
	static constexpr int FLAG_FILTER = 8;	// フィルタオブジェクトをサポートする (フィルタオブジェクトに対応する場合)
											// フィルタオブジェクトの場合は画像サイズの変更が出来ません
	LPCWSTR name;				// プラグインの名前
	LPCWSTR label;				// ラベルの初期値 (nullptrならデフォルトのラベルになります)
	LPCWSTR information;		// プラグインの情報
	void** items;				// 設定項目の定義 (FILTER_ITEM_XXXポインタを列挙してnull終端したリストへのポインタ)

	// 画像フィルタ処理関数へのポインタ (FLAG_VIDEOが有効の時のみ呼ばれます)
	// 戻り値	: falseを返却すると以降のフィルタや出力処理が中断されます
	bool (*func_proc_video)(FILTER_PROC_VIDEO* video);

	// 音声フィルタ処理関数へのポインタ (FLAG_AUDIOが有効の時のみ呼ばれます)
	// 戻り値	: falseを返却すると以降のフィルタや出力処理が中断されます
	bool (*func_proc_audio)(FILTER_PROC_AUDIO* audio);

};
