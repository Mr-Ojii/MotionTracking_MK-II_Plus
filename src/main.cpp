#include "main.hpp"
#include "config.h"

HINSTANCE hModuleDLL = nullptr;
MainFrame* g_frame   = nullptr;
EDIT_HANDLE* edit_handle = nullptr;

#ifdef __AVX__
static wchar_t verstr[] = L"MotionTracker_M AVX r" GIT_REV L" by Mr-Ojii";
#else
static wchar_t verstr[] = L"MotionTracker_M SSE2 r" GIT_REV L" by Mr-Ojii";
#endif

// いつもの外部公開
COMMON_PLUGIN_TABLE common_plugin_table = {
    constants::WindowName,
    verstr,
};

EXTERN_C __declspec(dllexport) COMMON_PLUGIN_TABLE* GetCommonPluginTable(void) {
    return &common_plugin_table;
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
