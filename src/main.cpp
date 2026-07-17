#include "main.hpp"
#include "config.h"
#include "aviutl2_sdk/filter2.h"
#include "constants.hpp"
#include <string>

extern FILTER_PLUGIN_TABLE filter; // filter.cpp で定義

HINSTANCE hModuleDLL = nullptr;
MainFrame* g_frame   = nullptr;
EDIT_HANDLE* edit_handle = nullptr;

static std::wstring verstr = std::wstring(constants::WindowName) + L" " + constants::version + L" by nullru";

// beta48 or later
constexpr DWORD REQUIRED_VERSION = 2004800;

// いつもの外部公開
COMMON_PLUGIN_TABLE common_plugin_table = {
    constants::WindowName,
    verstr.c_str(),
};

EXTERN_C __declspec(dllexport) COMMON_PLUGIN_TABLE* GetCommonPluginTable(void) {
    return &common_plugin_table;
}

// 必要バージョンを本体に宣言
EXTERN_C __declspec(dllexport) DWORD RequiredVersion() {
    return REQUIRED_VERSION;
}

EXTERN_C __declspec(dllexport) bool InitializePlugin(DWORD version) {
    if (version < REQUIRED_VERSION) {
        MessageBoxW(nullptr,
            L"AviUtl ExEdit2 version 2.0beta48 or later is required.",
            constants::WindowName, MB_OK | MB_ICONERROR);
        return false;
    }
    return true;
}

EXTERN_C __declspec(dllexport) void RegisterPlugin(HOST_APP_TABLE* host)
{
    // 編集ハンドルを作成
    edit_handle = host->create_edit_handle();

    // フィルターを登録
    // host->register_filter_plugin(&filter);

    g_frame = new MainFrame(hModuleDLL, host, edit_handle); // ウィンドウ作成

    // ウィンドウを登録
    host->register_window_client(
        constants::WindowName,
        g_frame->hwnd() // 作成されたウィンドウハンドルを返す
    );

}

// DllMain
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        hModuleDLL = hinstDLL;
        break;
    }
    return TRUE;
}
