#pragma once
#include <windows.h>

namespace constants {
    constexpr const wchar_t* WindowName = L"MotionTracking MK-II Plus for AviUtl2";
    constexpr const wchar_t* version = L"1";
    constexpr const wchar_t* APIerr = L"AviUtl2 API Error";
}

enum class IDC_Menu : UINT {
    ExportCSV    = 3001,
    ExportObject = 3002,
};

enum class IDC_Toolbar : UINT {
    Bar     = 4000,
    File    = 4001,
    Options = 4002,
    Info    = 4003,
};

// ProgressDlg がバックグラウンド解析の完了を検知したときに、本体ウィンドウへ通知するメッセージ
constexpr UINT WM_APP_ANALYZE_DONE = WM_APP + 1;
