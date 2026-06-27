#pragma once
#include <windows.h>

namespace constants {
    constexpr const wchar_t* WindowName = L"MotionTracking MK-II Plus for AviUtl2";
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
};
