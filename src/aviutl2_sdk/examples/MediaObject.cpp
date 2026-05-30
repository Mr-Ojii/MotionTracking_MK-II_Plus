//----------------------------------------------------------------------------------
//	サンプルフィルタプラグイン(メディアオブジェクト) for AviUtl ExEdit2
//----------------------------------------------------------------------------------
#include <windows.h>
#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <d3d11.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#include <DirectXMath.h>
using namespace DirectX;

#include "filter2.h"

bool func_proc_video(FILTER_PROC_VIDEO* video);
bool func_proc_audio(FILTER_PROC_AUDIO* audio);

//---------------------------------------------------------------------
//	フィルタ設定項目定義
//---------------------------------------------------------------------
auto separator_image = FILTER_ITEM_SEPARATOR(L"画像");
auto width = FILTER_ITEM_TRACK(L"横", 100, 1, 1000);
auto height = FILTER_ITEM_TRACK(L"縦", 100, 1, 1000);
auto color = FILTER_ITEM_COLOR(L"色", 0xffffff);
auto file = FILTER_ITEM_FILE(L"画像ファイル", L"", L"ImageFile (*.bmp;*.jpg;*.png)\0*.bmp;*.jpg;*.png\0");
FILTER_ITEM_SELECT::ITEM sample_list[] = {
	{ L"draw_image()", 0 },
	{ L"draw_poly()", 1 },
	{ L"exec_pixelshader()", 2 },
	{ L"get_image_texture2d()", 3 },
	{ nullptr } };
auto sample_type = FILTER_ITEM_SELECT(L"サンプル種類", 0, sample_list);
auto separator_audio = FILTER_ITEM_SEPARATOR(L"音声");
auto frequency = FILTER_ITEM_TRACK(L"周波数", 1000, 1, 24000);

void* items[] = {
	&separator_image, &width, &height, &color, &file, &sample_type,
	&separator_audio, &frequency,
	nullptr };

//---------------------------------------------------------------------
//	フィルタプラグイン構造体定義
//---------------------------------------------------------------------
FILTER_PLUGIN_TABLE filter_plugin_table = {
	FILTER_PLUGIN_TABLE::FLAG_VIDEO | FILTER_PLUGIN_TABLE::FLAG_AUDIO | FILTER_PLUGIN_TABLE::FLAG_INPUT, //	フラグ
	L"MediaObject(sample)",							// プラグインの名前
	L"サンプル",									// ラベルの初期値 (nullptrならデフォルトのラベルになります)
	L"Sample MediaObject version 2.00 By ＫＥＮくん",	// プラグインの情報
	items,											// 設定項目の定義 (FILTER_ITEM_XXXポインタを列挙してnull終端したリストへのポインタ)
	func_proc_video,								// 画像フィルタ処理関数へのポインタ (FLAG_VIDEOが有効の時のみ呼ばれます)
	func_proc_audio									// 音声フィルタ処理関数へのポインタ (FLAG_AUDIOが有効の時のみ呼ばれます)
};

//---------------------------------------------------------------------
//	プラグインDLL初期化関数 (未定義なら呼ばれません)
//---------------------------------------------------------------------
EXTERN_C __declspec(dllexport) bool InitializePlugin(DWORD version) { // versionは本体のバージョン番号
	return true;
}

//---------------------------------------------------------------------
//	プラグインDLL解放関数 (未定義なら呼ばれません)
//---------------------------------------------------------------------
EXTERN_C __declspec(dllexport) void UninitializePlugin() {
}

//---------------------------------------------------------------------
//	フィルタ構造体のポインタを渡す関数
//---------------------------------------------------------------------
EXTERN_C __declspec(dllexport) FILTER_PLUGIN_TABLE* GetFilterPluginTable(void) {
	return &filter_plugin_table;
}

//---------------------------------------------------------------------
//	画像フィルタ処理
//---------------------------------------------------------------------
bool func_proc_video(FILTER_PROC_VIDEO* video) {
	auto w = (int)width.value;
	auto h = (int)height.value;
	if (w <= 0 || h <= 0) return false;

	// 複数の簡単なサンプル処理をリスト選択で切り替えています
	switch (sample_type.value) {

		//-------------------------------------------------------------
		// 画像ファイルから画像を取得して自前で描画するサンプル
		//-------------------------------------------------------------
		case 0:
		{
			// 画像ファイルのリソース名を作成
			if (!*file.value) return false;
			auto resource = std::wstring(L"image:") + file.value;

			// 自前で画像を描画する
			for (int i = 0; i < 16; i++) {
				auto rz = 360.0f * i / 16.0f;
				auto rad = XMConvertToRadians(rz);
				auto x = (float)width.value * cosf(rad);
				auto y = (float)height.value * sinf(rad);
				video->draw_image(resource.c_str(), x, y, 0, 0, 0, rz, 1, 1, 1, 1);
			}

			video->set_default_anchor(0, 0); // 自前でアンカー枠を表示
			return false; // 以降の処理を中断する
		}

		//-------------------------------------------------------------
		// 画像ファイルから画像を取得して球体に描画するサンプル
		//-------------------------------------------------------------
		case 1:
		{
			// 画像ファイルのリソース名を作成
			if (!*file.value) return false;
			auto resource = std::wstring(L"image:") + file.value;

			// 画像リソースを球体に描画する
			int num = 20;
			std::vector<VERTEX_TEXTURE_NORM> vertex;
			vertex.reserve(num * num * 4);
			for (int y = 0; y < num; y++) {
				auto y0 = -(float)height.value * cosf(XM_PI * y / num);
				auto r0 = +(float)width.value * sinf(XM_PI * y / num);
				auto y1 = -(float)height.value * cosf(XM_PI * (y + 1) / num);
				auto r1 = +(float)width.value * sinf(XM_PI * (y + 1) / num);
				auto v0 = (float)y / num;
				auto v1 = (float)(y + 1) / num;
				for (int x = 0; x < num; x++) {
					auto x0 = +r0 * sinf(XM_PI * 2 * x / num);
					auto x1 = +r0 * sinf(XM_PI * 2 * (x + 1) / num);
					auto x2 = +r1 * sinf(XM_PI * 2 * (x + 1) / num);
					auto x3 = +r1 * sinf(XM_PI * 2 * x / num);
					auto z0 = -r0 * cosf(XM_PI * 2 * x / num);
					auto z1 = -r0 * cosf(XM_PI * 2 * (x + 1) / num);
					auto z2 = -r1 * cosf(XM_PI * 2 * (x + 1) / num);
					auto z3 = -r1 * cosf(XM_PI * 2 * x / num);
					auto u0 = (float)x / num;
					auto u1 = (float)(x + 1) / num;
					// 4頂点のデータを追加していく
					vertex.push_back({ x0, y0, z0, u0, v0, 1, x0, y0, z0 });
					vertex.push_back({ x1, y0, z1, u1, v0, 1, x1, y0, z1 });
					vertex.push_back({ x2, y1, z2, u1, v1, 1, x2, y1, z2 });
					vertex.push_back({ x3, y1, z3, u0, v1, 1, x3, y1, z3 });
				}
			}
			// 実際にポリゴンを描画する
			video->set_material_shine(0.5f);
			video->draw_poly(VERTEX_TYPE::QUAD_TEXTURE_NORM, vertex.data(), (int)vertex.size(), resource.c_str());

			video->set_default_anchor((int)width.value * 2, (int)height.value * 2); // 自前でアンカー枠を表示
			return false; // 以降の処理を中断する
		}

		//-------------------------------------------------------------
		// 画像ファイルから画像を取得して色を乗算して出力するサンプル
		//-------------------------------------------------------------
		case 2:
		{
			// オブジェクトを指定サイズに変更
			video->set_image_data(nullptr, w, h);

			// 画像ファイルのリソース名を作成
			if (!*file.value) return false;
			auto image = std::wstring(L"image:") + file.value;

			// 色設定を取得して定数バッファのデータ作成
			struct CONSTANT {
				float r, g, b, a;
			};
			CONSTANT constant = { color.value.r / 255.0f, color.value.g / 255.0f, color.value.b / 255.0f, 1.0f };

			// ピクセルシェーダーを利用してオブジェクトの画像を作成
			// 画像リソースに色を乗算してオブジェクトに出力する
			LPCWSTR resources[] = { image.c_str() };
			video->exec_pixelshader_file(L"MediaObject.cso", nullptr, resources, 1, &constant, sizeof(constant), nullptr, nullptr);
			return true;
		}

		//-------------------------------------------------------------
		// D3Dを直接操作する特殊なサンプル ※D3Dに詳しい人向け
		//-------------------------------------------------------------
		case 3:
		{
			// 指定サイズの画像を設定してTexture2Dを取得
			video->set_image_data(nullptr, w, h);
			auto texture = video->get_image_texture2d();

			// D3DのDevice,DeviceContextを取得
			ComPtr<ID3D11Device> device;
			texture->GetDevice(&device);
			ComPtr<ID3D11DeviceContext> context;
			device->GetImmediateContext(&context);

			// Texture2DのRTVを取得
			D3D11_TEXTURE2D_DESC desc{};
			texture->GetDesc(&desc);
			D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
			rtvDesc.Format = desc.Format;
			rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			ComPtr<ID3D11RenderTargetView> rtv;
			if (FAILED(device->CreateRenderTargetView(texture, &rtvDesc, &rtv))) {
				return false;
			}

			// 指定の色で塗りつぶす
			auto col = color.value;
			const float color[4] = { col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, 1.0f }; // 乗算済みアルファ
			context->ClearRenderTargetView(rtv.Get(), color);
			return true;
		}

	}
	return false;
}

//---------------------------------------------------------------------
//	音声フィルタ処理
//---------------------------------------------------------------------
bool func_proc_audio(FILTER_PROC_AUDIO* audio) {
	auto sample_index = audio->object->sample_index;
	auto sample_num = audio->object->sample_num;
	auto channel_num = audio->object->channel_num;

	// 指定周波数のサイン波の音声データを作成
	auto step = (3.141592653589793 * 2.0) * frequency.value / audio->scene->sample_rate;
	auto buffer = std::make_unique<float[]>(sample_num);
	auto p = buffer.get();
	for (int i = 0; i < sample_num; i++) {
		*p++ = (float)sin(sample_index++ * step);
	}

	for (int i = 0; i < channel_num; i++) {
		audio->set_sample_data(buffer.get(), i);
	}
	return true;
}
