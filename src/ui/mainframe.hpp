#pragma once

#define NOMINMAX

#include <windows.h>
#include "constants.hpp"
#include <commctrl.h>
#include <mutex>
#include <unordered_map>
#include "aviutl2_sdk/plugin2.h"
#include "aviutl2_sdk/logger2.h"
#include "aviutl2_sdk/config2.h"
#include "opencv2/core/utility.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/tracking.hpp"
#include "opencv2/objdetect.hpp"
#include "opencv2/video.hpp"
#include "opencv2/video/tracking.hpp"
#include "utils.hpp"
//#include "TrackedData.hpp"
#include "config.h"
#include "tracker/tracker.hpp"
#include "ownerdraw.hpp"


enum class IDC_Button : int {
    SelectObject = 1001,
    Analyze,
    ClearResult,
    InsertObject,
    TrackingMethodCombo,
    HueTrackbar,
    HueValue,
    ViewResult,
    AsSubFilter,
    InvertPosition,
    IgnoreAspectRatio,
    QuickBlur,
    EasyPrivacy,
};

extern HINSTANCE    hModuleDLL;
extern EDIT_HANDLE* edit_handle; // main.cpp で定義。グローバルように参照。後で消す。

struct HOST_APP_TABLE;

class MainFrame {
public:
    MainFrame(HINSTANCE hInst, HOST_APP_TABLE* host, EDIT_HANDLE* edit_handle);
    HWND hwnd() const { return m_hwnd; }

private:
    // this を渡したらクラッシュするため

    // lpfnWndProc に渡せる関数は4引数固定
    // 非 static メンバ関数はコンパイラが this を第1引数に追加するため形が合わない
    // -> static にして this を消す
    static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    void OnSelectObject();
    void OnAnalyze();
    void OnInsertObject();
    void OnClearResult();
    void OnCheckboxClick(HWND hBtn);

    HWND            m_hwnd       = nullptr;
    HINSTANCE       m_hInst      = nullptr;
    HOST_APP_TABLE* m_host       = nullptr;
    EDIT_HANDLE*    m_edit_handle = nullptr;
    LOG_HANDLE*     m_logger     = nullptr;
    CONFIG_HANDLE*  m_config     = nullptr;
    std::string     m_modelDir;

    // Tracker インスタンス
    Tracker m_tracker;

    // システムカラー
    SystemColors m_colors;
};
