#pragma once
//----------------------------------------------------------------------------------
//	キャッシュ関連機能 ヘッダーファイル for AviUtl ExEdit2
//	By ＫＥＮくん
//----------------------------------------------------------------------------------

//	各種プラグインで下記の関数を外部公開すると呼び出されます
//	※入力プラグインではファイルからのキャッシュ取得関数が非推奨になります
// 
//	キャッシュ関連機能初期化関数
//		void InitializeCache(CACHE_HANDLE* cache)
//		※InitializePlugin()より先に呼ばれます

//----------------------------------------------------------------------------------

// filter2.hに定義されています
struct PIXEL_RGBA;
enum class INPUT_PIXEL_FORMAT : int;

// キャッシュデータ参照の基底クラス
// クラスオブジェクトが生存している間はキャッシュ参照が有効となるように制御されます
class CACHE_REFERENCE {
	CACHE_REFERENCE(const CACHE_REFERENCE&) = delete;
	CACHE_REFERENCE& operator=(const CACHE_REFERENCE&) = delete;
	CACHE_REFERENCE(CACHE_REFERENCE&&) = delete;
	CACHE_REFERENCE& operator=(CACHE_REFERENCE&&) = delete;
	void (* const func_release)(void* instance);
	void* const cache_instance;
public:
	CACHE_REFERENCE() : func_release(nullptr), cache_instance(nullptr) {}
	CACHE_REFERENCE(void (*func_release)(void*), void* cache_instance) : func_release(func_release), cache_instance(cache_instance) {}
	~CACHE_REFERENCE() { if (func_release && cache_instance) func_release(cache_instance); }
};

// 画像キャッシュデータ構造体
struct CACHE_IMAGE : CACHE_REFERENCE {
	// 画像キャッシュデータへのポインタ (取得失敗時はnullptr)
	// ※画像データはPIXEL_RGBA
	PIXEL_RGBA* const buffer;

	// 画像キャッシュの画像サイズ
	const int width, height;

	// 画像キャッシュデータが正しく取得されたか？
	// 戻り値	: データが無い場合はfalse
	operator bool() const { return buffer; }
};

// 音声キャッシュデータ構造体
struct CACHE_AUDIO : CACHE_REFERENCE {
	// 音声キャッシュデータ(左チャンネル)へのポインタ (取得失敗時はnullptr)
	// ※音声データはPCM(float)32bit
	float* const buffer0;

	// 音声キャッシュデータ(右チャンネル)へのポインタ (取得失敗時はnullptr)
	// ※音声データはPCM(float)32bit
	float* const buffer1;

	// 音声キャッシュのサンプル数
	const int sample_num;

	// 音声キャッシュのチャンネル数 ( 1 = モノラル / 2 = ステレオ )
	// チャンネル数が1の場合は buffer0 のみ利用出来ます
	const int channel_num;

	// 音声キャッシュデータへのポインタを配列指定で取得
	float* const operator[](size_t index) {
		if (index == 0) return buffer0;
		if (index == 1) return buffer1;
		return nullptr;
	}

	// 音声キャッシュデータが正しく取得されたか？
	// 戻り値	: データが無い場合はfalse
	operator bool() const { return buffer0; }
};

// メディアファイルの画像キャッシュデータ構造体
struct CACHE_FILE_IMAGE : CACHE_REFERENCE {
	// 画像キャッシュデータへのポインタ (取得失敗時はnullptr)
	// ※画像データはINPUT_PIXEL_FORMATのいずれかになります
	const void* buffer;

	// 画像キャッシュの画像サイズ
	const int width, height;

	// 画像キャッシュデータの横1ラインのバイト数
	const int pitch;

	// 画像キャッシュのピクセルフォーマット
	const INPUT_PIXEL_FORMAT format;

	// 画像キャッシュデータが正しく取得されたか？
	// 戻り値	: データが無い場合はfalse
	operator bool() const { return buffer; }
};

//----------------------------------------------------------------------------------

// ビデオ情報構造体
struct VIDEO_INFO {
	double total_time;	// 総時間
	int frame_num;		// 総フレーム数
	int track_num;		// トラック数
	int width, height;	// 解像度
	int rate, scale;	// フレームレート
};

// オーディオ情報構造体
struct AUDIO_INFO {
	double total_time;	// 総時間
	int64_t sample_num;	// 総サンプル数
	int track_num;		// トラック数
	int rate;			// サンプリングレート
	int channel;		// チャンネル数
};

//----------------------------------------------------------------------------------

// キャッシュハンドル
// アプリケーションの共用のキャッシュ領域に各種キャッシュデータを作成することが出来ます
// ※スクリプトのキャッシュバッファ(cache:xxxx)とは異なりメインメモリに確保されます
struct CACHE_HANDLE {
	// 画像キャッシュデータを取得する
	// identifier	: キャッシュ識別のポインタ ※任意の静的なポインタを指定する(CACHE_HANDLEやFILTER_PLUGIN_TABLE等)
	// name			: キャッシュ識別の名前 ※任意の名前を付けることが出来る
	// 戻り値		: 画像キャッシュデータ
	//				  取得出来ない場合は返却オブジェクトがfalseとなる
	CACHE_IMAGE (*get_image_cache)(void* identifier, LPCWSTR name);

	// 画像キャッシュデータを作成する
	// identifier	: キャッシュ識別のポインタ ※任意の静的なポインタを指定する(CACHE_HANDLEやFILTER_PLUGIN_TABLE等)
	// name			: キャッシュ識別の名前 ※任意の名前を付けることが出来る
	// width,height	: 作成するキャッシュの画像サイズ
	// 戻り値		: 画像キャッシュデータ
	//				  返却されたキャッシュに画像データを書き込むことが出来る
	CACHE_IMAGE (*create_image_cache)(void* identifier, LPCWSTR name, int width, int height);

	// 音声キャッシュデータを取得する
	// identifier	: キャッシュ識別のポインタ ※任意の静的なポインタを指定する(CACHE_HANDLEやFILTER_PLUGIN_TABLE等)
	// name			: キャッシュ識別の名前 ※任意の名前を付けることが出来る
	// 戻り値		: 音声キャッシュデータ
	//				  取得出来ない場合は返却オブジェクトがfalseとなる
	CACHE_AUDIO (*get_audio_cache)(void* identifier, LPCWSTR name);

	// 音声キャッシュデータを作成する
	// identifier	: キャッシュ識別のポインタ ※任意の静的なポインタを指定する(CACHE_HANDLEやFILTER_PLUGIN_TABLE等)
	// name			: キャッシュ識別の名前 ※任意の名前を付けることが出来る
	// sample_num	: 作成する音声キャッシュのサンプル数
	// channel_num	: 作成する音声キャッシュのチャンネル数 ( 1 = モノラル / 2 = ステレオ )
	// 戻り値		: 音声キャッシュデータ
	//				  返却されたキャッシュに音声データを書き込むことが出来る
	CACHE_AUDIO (*create_audio_cache)(void* identifier, LPCWSTR name, int sample_num, int channel_num);

	// 新しい関数に差し替えるので廃止します
	CACHE_IMAGE (*deprecated_get_image_file_cache)(LPCWSTR file);

	// メディアファイルのビデオ情報を取得する
	// file			: メディアファイルのパス
	// info			: ビデオ情報の格納先へのポインタ
	// info_size	: ビデオ情報の格納先のサイズ ※VIDEO_INFOと異なる場合はサイズ分のみ取得されます
	// 戻り値		: 取得出来た場合はtrue
	bool (*get_video_file_info)(LPCWSTR file, VIDEO_INFO* info, int info_size);

	// メディアファイルのオーディオ情報を取得する
	// file			: メディアファイルのパス
	// info			: オーディオ情報の格納先へのポインタ
	// info_size	: オーディオ情報の格納先のサイズ ※AUDIO_INFOと異なる場合はサイズ分のみ取得されます
	// 戻り値		: 取得出来た場合はtrue
	bool (*get_audio_file_info)(LPCWSTR file, AUDIO_INFO* info, int info_size);

	// 画像ファイルから画像データをキャッシュ経由で取得する
	// file			: 画像ファイルのパス
	// 戻り値		: 画像キャッシュデータ
	//				  取得出来ない場合は返却オブジェクトがfalseとなる
	CACHE_FILE_IMAGE (*get_image_file_cache)(LPCWSTR file);

	// メディアファイルから画像データをキャッシュ経由で取得する
	// file			: メディアファイルのパス
	// track		: トラック番号
	// frame		: 取得するフレーム番号
	// 戻り値		: 画像キャッシュデータ
	//				  取得出来ない場合は返却オブジェクトがfalseとなる
	CACHE_FILE_IMAGE (*get_video_file_cache)(LPCWSTR file, int track, int frame);

	// メディアファイルから画像データをキャッシュ経由で取得する
	// file			: メディアファイルのパス
	// track		: ビデオトラック番号
	// time			: 取得するフレームの時間
	// 戻り値		: 画像キャッシュデータ
	//				  取得出来ない場合は返却オブジェクトがfalseとなる
	CACHE_FILE_IMAGE (*get_video_file_cache_by_time)(LPCWSTR file, int track, double time);

	// メディアファイルから音声データをキャッシュ経由で取得する
	// ※音声データはPCM(float)32bit2ch
	// file			: メディアファイルのパス
	// track		: オーディオトラック番号
	// sample_index	: 取得するサンプル位置
	// sample_num	: 取得するサンプル数
	// buffer0		: サンプル(左チャンネル)取得先のバッファへのポインタ
	// buffer1		: サンプル(右チャンネル)取得先のバッファへのポインタ
	// 戻り値		: 実際に取得したサンプル数
	int (*get_audio_file_data)(LPCWSTR file, int track, int64_t sample_index, int sample_num, float* buffer0, float* buffer1);

};

//----------------------------------------------------------------------------------
// キャッシュ処理のサンプルコード 
//----------------------------------------------------------------------------------
/*
	...
	// キャッシュの識別名を作成 ※下記の場合はエフェクト毎、フレーム番号毎のキャッシュとなる
	std::wstring cache_name = std::to_wstring(video->object->effect_id) + L',' + std::to_wstring(video->object->frame);

	// キャッシュが存在する場合はキャッシュデータから設定
	{
		auto image = cache_handle->get_image_cache(cache_handle, cache_name.c_str());
		if (image && image.width == width && image.height == height) {
			video->set_image_data(image.buffer, image.width, image.height);
			return true;
		}
	}

	// 今回のフレーム番号の画像データを作成
	auto buffer = std::make_unique<PIXEL_RGBA[]>(width * height);
	...
	video->set_image_data(buffer.get(), width, height);

	// キャッシュを作成して今回の画像データを設定
	{
		auto image = cache_handle->create_image_cache(cache_handle, cache_name.c_str(), width, height);
		memcpy(image.buffer, buffer.get(), width * height * sizeof(PIXEL_RGBA));
	}
	return true;
*/
