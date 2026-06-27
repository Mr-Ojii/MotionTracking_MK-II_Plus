#include "progress_dlg.hpp"
#include <commctrl.h>
#include <format>

static constexpr wchar_t CLASS_NAME[] = L"MotionTracker_ProgressDlg";
static constexpr UINT    TIMER_ID     = 1;
static constexpr UINT    TIMER_MS     = 10;

bool ProgressDlg::s_registered = false;

LRESULT CALLBACK ProgressDlg::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    ProgressDlg* self = reinterpret_cast<ProgressDlg*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_NCCREATE:
        self = reinterpret_cast<ProgressDlg*>(
            reinterpret_cast<CREATESTRUCT*>(lp)->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->m_hwnd = hwnd;
        return TRUE;

    case WM_CREATE:
        SetTimer(hwnd, TIMER_ID, TIMER_MS, nullptr);
        return 0;

    case WM_TIMER: {
        int cur   = self->m_tracker->m_progress_current.load();
        int total = self->m_tracker->m_progress_total.load();

        if (total > 0) {
            SendMessage(self->m_progress, PBM_SETRANGE32, 0, total);
            SendMessage(self->m_progress, PBM_SETPOS, cur, 0);
            double fps = self->m_tracker->m_progress_fps.load();
            auto text = std::format(L"{} processing frame {}/{} @{:.2f} fps",
                self->m_methodName, cur, total, fps);
            SetWindowText(self->m_label, text.c_str());
        }

        if (!self->m_tracker->m_analyzing.load()) {
            KillTimer(hwnd, TIMER_ID);
            DestroyWindow(hwnd);
        }
        return 0;
    }

    case WM_CLOSE:
        // 解析中は × で閉じない
        return 0;

    case WM_DESTROY:
        delete self;
        return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

ProgressDlg* ProgressDlg::Create(HWND parent, Tracker* tracker, HINSTANCE hInst, const wchar_t* methodName) {
    if (!s_registered) {
        WNDCLASSEXW wcex    = {};
        wcex.cbSize         = sizeof(wcex);
        wcex.lpszClassName  = CLASS_NAME;
        wcex.lpfnWndProc    = WndProc;
        wcex.hInstance      = hInst;
        wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground  = (HBRUSH)(COLOR_BTNFACE + 1);
        RegisterClassExW(&wcex);
        s_registered = true;
    }

    auto* dlg = new ProgressDlg();
    dlg->m_tracker    = tracker;
    dlg->m_methodName = methodName;

    HWND hwnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_APPWINDOW,
        CLASS_NAME,
        L"Analyzing...",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 360, 300,
        parent, nullptr, hInst, dlg);

    HFONT hfont = CreateFontW(
        -12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Yu Gothic UI");

    dlg->m_label = CreateWindowExW(
        0, WC_STATIC, L"Initializing...",
        WS_VISIBLE | WS_CHILD,
        10, 12, 330, 20,
        hwnd, nullptr, hInst, nullptr);
    SendMessage(dlg->m_label, WM_SETFONT, (WPARAM)hfont, TRUE);

    dlg->m_progress = CreateWindowExW(
        0, PROGRESS_CLASS, nullptr,
        WS_VISIBLE | WS_CHILD | PBS_SMOOTH,
        10, 40, 330, 18,
        hwnd, nullptr, hInst, nullptr);

    // 親ウィンドウの中央に配置
    RECT pr, wr;
    GetWindowRect(parent, &pr);
    GetWindowRect(hwnd, &wr);
    int w = wr.right - wr.left;
    int h = wr.bottom - wr.top;
    SetWindowPos(hwnd, HWND_TOP,
        pr.left + (pr.right - pr.left - w) / 2,
        pr.top  + (pr.bottom - pr.top  - h) / 2,
        0, 0, SWP_NOSIZE);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    return dlg;
}
