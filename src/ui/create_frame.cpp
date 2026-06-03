#include "mainframe.hpp"
#include "ownerdraw.hpp"

extern LOG_HANDLE*    logger;
extern CONFIG_HANDLE* config;

constexpr const wchar_t* track_method[] = { L"MIL", L"KCF", L"CSRT", L"DaSiamRPN", L"Nano", L"Vit" };
constexpr int METHOD_N = sizeof(track_method) / sizeof(track_method[0]);

void MainFrame::CreateControls() {
    // 色情報の取得
    m_colors.background       = m_colors.background;
    m_colors.buttonBody       = config->get_color_code(config, "ButtonBody");
    m_colors.buttonBodyPress  = config->get_color_code(config, "ButtonBodyPress");
    m_colors.buttonBodyHover  = config->get_color_code(config, "ButtonBodyHover");
    m_colors.buttonBodyDisable = config->get_color_code(config, "ButtonBodyDisable");
    m_colors.text             = config->get_color_code(config, "Text");
    m_colors.textDisable      = config->get_color_code(config, "TextDisable");

    // フォント情報の取得とフォント作成
    FONT_INFO* font_info = config->get_font_info(config, "Control");
    LOGFONT logfont = {};
    logfont.lfHeight = -static_cast<int>(font_info->size * 96 / 72);
    logfont.lfCharSet = DEFAULT_CHARSET;
    logfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    logfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    logfont.lfQuality = DEFAULT_QUALITY;
    logfont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    wcscpy_s(logfont.lfFaceName, LF_FACESIZE, font_info->name);
    HFONT hfont = CreateFontIndirect(&logfont);

    int item_height = config->get_layout_size(config, "SettingItemHeight");
    int y_pos = 10;

    // Tracking Method ラベルを作成
    HWND label_track = CreateWindowEx(
        0,
        WC_STATIC,
        config->translate(config, L"Method"),
        WS_VISIBLE | WS_CHILD,
        10, y_pos, 100, item_height,
        m_hwnd,
        (HMENU)-1,
        m_hInst,
        nullptr);
    SendMessage(label_track, WM_SETFONT, (WPARAM)hfont, TRUE);

    // Tracking Method コンボボックスを作成
    HWND combo_track = CreateWindowEx(
        0,
        WC_COMBOBOX,
        nullptr,
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS,
        75, y_pos, 195, 200, // ドロップダウンが開くように高さを大きめに確保
        m_hwnd,
        (HMENU)IDC_Button::TrackingMethodCombo,
        m_hInst,
        nullptr);
    SendMessage(combo_track, WM_SETFONT, (WPARAM)hfont, TRUE);
    for (int i = 0; i < METHOD_N; i++) {
        SendMessage(combo_track, CB_ADDSTRING, 0, (LPARAM)track_method[i]);
    }
    SendMessage(combo_track, CB_SETCURSEL, 2, 0); // Default to CSRT

    y_pos += item_height + 5;

    // Hueラベルを作成
    HWND label_hue = CreateWindowEx(
        0,
        WC_STATIC,
        config->translate(config, L"Hue"),
        WS_VISIBLE | WS_CHILD,
        10, y_pos, 60, item_height,
        m_hwnd,
        (HMENU)-1,
        m_hInst,
        nullptr);
    SendMessage(label_hue, WM_SETFONT, (WPARAM)hfont, TRUE);

    // Hueトラックバーを作成
    HWND trackbar_hue = CreateWindowEx(
        0,
        TRACKBAR_CLASS,
        L"Hue",
        WS_VISIBLE | WS_CHILD,
        75, y_pos, 205, item_height,
        m_hwnd,
        (HMENU)IDC_Button::HueTrackbar,
        m_hInst,
        nullptr);
    SendMessage(trackbar_hue, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 359));
    SendMessage(trackbar_hue, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)m_hueValue);
    SendMessage(trackbar_hue, WM_SETFONT, (WPARAM)hfont, TRUE);

    // Hue数値表示を作成
    HWND hue_value_display = CreateWindowEx(
        0,
        WC_STATIC,
        L"180",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        285, y_pos, 25, item_height,
        m_hwnd,
        (HMENU)IDC_Button::HueValue,
        m_hInst,
        nullptr);
    SendMessage(hue_value_display, WM_SETFONT, (WPARAM)hfont, TRUE);

    y_pos += item_height + 5;

    // Select Object ボタンを作成
    HWND button0 = CreateWindowEx(
        0,
        WC_BUTTON,
        config->translate(config, L"Select Object"),
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        10, y_pos, 300, item_height,
        m_hwnd,
        (HMENU)IDC_Button::SelectObject,
        m_hInst,
        nullptr);
    SendMessage(button0, WM_SETFONT, (WPARAM)hfont, TRUE);

    y_pos += item_height + 5;

    // Analyze ボタンを作成
    HWND button1 = CreateWindowEx(
        0,
        WC_BUTTON,
        config->translate(config, L"Analyze"),
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        10, y_pos, 300, item_height,
        m_hwnd,
        (HMENU)IDC_Button::Analyze,
        m_hInst,
        nullptr);
    SendMessage(button1, WM_SETFONT, (WPARAM)hfont, TRUE);

    y_pos += item_height + 5;

    // Save EXO ボタンを作成
    HWND button_save = CreateWindowEx(
        0,
        WC_BUTTON,
        config->translate(config, L"Insert Object"),
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        10, y_pos, 300, item_height,
        m_hwnd,
        (HMENU)IDC_Button::InsertObject,
        m_hInst,
        nullptr);
    SendMessage(button_save, WM_SETFONT, (WPARAM)hfont, TRUE);

    y_pos += item_height + 5;

    // Clear Result ボタンを作成
    HWND button2 = CreateWindowEx(
        0,
        WC_BUTTON,
        config->translate(config, L"Clear Result"),
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        10, y_pos, 300, item_height,
        m_hwnd,
        (HMENU)IDC_Button::ClearResult,
        m_hInst,
        nullptr);
    SendMessage(button2, WM_SETFONT, (WPARAM)hfont, TRUE);

    y_pos += item_height + 5;

    // As Sub-filter/部分フィルター? チェックボックスを作成
    HWND check_sub_filter = CreateWindowEx(
        0,
        WC_BUTTON,
        config->translate(config, L"As Sub-filter/部分フィルター?"),
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        10, y_pos, 300, item_height,
        m_hwnd,
        (HMENU)IDC_Button::AsSubFilter,
        m_hInst,
        nullptr);
    SendMessage(check_sub_filter, WM_SETFONT, (WPARAM)hfont, TRUE);

    y_pos += item_height + 5;

    // Invert Position チェックボックスを作成
    HWND check_invert = CreateWindowEx(
        0,
        WC_BUTTON,
        config->translate(config, L"Invert Position"),
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        10, y_pos, 300, item_height,
        m_hwnd,
        (HMENU)IDC_Button::InvertPosition,
        m_hInst,
        nullptr);
    SendMessage(check_invert, WM_SETFONT, (WPARAM)hfont, TRUE);

    y_pos += item_height + 5;

    // Ignore Aspect Ratio チェックボックスを作成
    HWND check_ignore_aspect = CreateWindowEx(
        0,
        WC_BUTTON,
        config->translate(config, L"Ignore Aspect Ratio"),
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        10, y_pos, 300, item_height,
        m_hwnd,
        (HMENU)IDC_Button::IgnoreAspectRatio,
        m_hInst,
        nullptr);
    SetWindowLongPtr(check_ignore_aspect, GWLP_USERDATA, 1);
    SendMessage(check_ignore_aspect, WM_SETFONT, (WPARAM)hfont, TRUE);

    y_pos += item_height + 5;

    // Quick Blur チェックボックスを作成
    HWND check_quick_blur = CreateWindowEx(
        0,
        WC_BUTTON,
        config->translate(config, L"Quick Blur"),
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        10, y_pos, 300, item_height,
        m_hwnd,
        (HMENU)IDC_Button::QuickBlur,
        m_hInst,
        nullptr);
    SendMessage(check_quick_blur, WM_SETFONT, (WPARAM)hfont, TRUE);

    y_pos += item_height + 5;

    // Easy Privacy チェックボックスを作成
    HWND check_easy_privacy = CreateWindowEx(
        0,
        WC_BUTTON,
        config->translate(config, L"Easy Privacy"),
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        10, y_pos, 300, item_height,
        m_hwnd,
        (HMENU)IDC_Button::EasyPrivacy,
        m_hInst,
        nullptr);
    SendMessage(check_easy_privacy, WM_SETFONT, (WPARAM)hfont, TRUE);
}
