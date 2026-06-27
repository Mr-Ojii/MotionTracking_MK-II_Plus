#pragma once
#include <windows.h>
#include <string>
#include "tracker/tracker.hpp"

class ProgressDlg {
public:
    static ProgressDlg* Create(HWND parent, Tracker* tracker, HINSTANCE hInst, const wchar_t* methodName);

private:
    HWND         m_hwnd       = nullptr;
    HWND         m_progress   = nullptr;
    HWND         m_label      = nullptr;
    Tracker*     m_tracker    = nullptr;
    std::wstring m_methodName;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    static bool s_registered;
};
