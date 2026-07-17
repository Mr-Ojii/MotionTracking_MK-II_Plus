#include "mainframe.hpp"
#include "constants.hpp"
#include "ownerdraw.hpp"
#include "progress_dlg/progress_dlg.hpp"
#include "aviutl2_sdk/plugin2.h"
#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include <shellapi.h>

extern FILTER_PLUGIN_TABLE filter;

static std::string modelDir;

LOG_HANDLE* logger = nullptr;
CONFIG_HANDLE* config = nullptr;

// 画像をとってくるときに使う変数で、向こうではexternで配置されているはず
bool getImageFromAUX = false;
bool finishedFilter = false;
std::mutex mtx;
std::condition_variable cov;
cv::Mat ocvImage;
int hueValue = 180;
// --

// Obj Selection
cv::Rect2d boundingBox;
bool selectObj = false;
bool startSel = false;
// Analyze
std::vector<bool> track_found;
std::vector<cv::Rect2d> track_result;


constexpr const wchar_t* track_method[] = { L"MIL", L"KCF", L"CSRT", L"DaSiamRPN", L"Nano", L"Vit"};
constexpr int METHOD_N = sizeof(track_method) / sizeof(track_method[0]);


// RequiredVersion / InitializePlugin は main.cpp に移設 (バージョン判定を実装)

EXTERN_C __declspec(dllexport) void InitializeLogger(LOG_HANDLE* handle) {
    logger = handle;
}

EXTERN_C __declspec(dllexport) void InitializeConfig(CONFIG_HANDLE* handle) {
    config = handle;
}

/*
 * 動作せず
EXTERN_C __declspec(dllexport) void UninitializePlugin() {
    cv::destroyAllWindows();
}
*/


// --

const char alias[] = u8R"(
[Object]
[Object.0]
effect.name=フィルタオブジェクト
[Object.1]
effect.name=MotionTracker_M Filter
)";

// フィルタがつけられているタイムラインオブジェクトを探し、返す関数
bool get_effected_object_layer_frame(EDIT_HANDLE* edit_handle, OBJECT_LAYER_FRAME* olf) {
    edit_handle->call_edit_section_param(olf, [](void* message, EDIT_SECTION* edit) {
        OBJECT_LAYER_FRAME* olf = (OBJECT_LAYER_FRAME*)message;
        OBJECT_HANDLE found_obj = nullptr;
        for (int layer = 0; layer <= edit->info->layer_max && !found_obj; layer++) {
            for (int frame = 0; frame <= edit->info->frame_max; frame++) {
                OBJECT_HANDLE obj = edit->find_object(layer, frame);
                if (obj && edit->count_object_effect(obj, L"MotionTracker_M Filter") > 0) {
                    found_obj = obj;
                    break;
                }
            }
        }

        if (!found_obj) {
            olf->layer = -1;
            olf->start = -1;
            olf->end = -1;
            return;
        }

        *olf = edit->get_object_layer_frame(found_obj);
    });

    return olf->layer != -1;
}

// フィルタオブジェクトを現在のレイヤー・フレームに配置し、返す関数
bool create_alias_object_and_set_olf(EDIT_HANDLE* edit_handle, OBJECT_LAYER_FRAME* olf) {
    edit_handle->call_edit_section_param(olf, [](void* message, EDIT_SECTION* edit) {
        OBJECT_LAYER_FRAME* olf = (OBJECT_LAYER_FRAME*)message;
        if (edit->create_object_from_alias(alias, edit->info->layer, edit->info->frame, 10)) {
            logger->log(logger, L"create alias object");
            OBJECT_HANDLE obj = edit->find_object(edit->info->layer, edit->info->frame);
            if (obj) {
                *olf = edit->get_object_layer_frame(obj);
            } else {
                logger->warn(logger, L"object not found after create");
                olf->layer = -1;
                olf->start = -1;
                olf->end = -1;
            }
        } else {
            logger->warn(logger, L"create alias failed");
            olf->layer = -1;
            olf->start = -1;
            olf->end = -1;
        }
    });

    return olf->layer != -1;
}

static std::mutex g_mutex;

// ユーザーの手でトラッキング領域が変更されたときに呼び出し、描画しなおす
static void update_object_selection_window(int x1, int y1, int x2, int y2)
{
    std::lock_guard<std::mutex> lg(g_mutex);

    //update only if visible
    if(!static_cast<bool>(cv::getWindowProperty("Object Selection", cv::WND_PROP_VISIBLE)))
        return;

    x1 = std::clamp(x1, 0, ocvImage.cols);
    y1 = std::clamp(y1, 0, ocvImage.rows);
    x2 = std::clamp(x2, 0, ocvImage.cols);
    y2 = std::clamp(y2, 0, ocvImage.rows);

    //draw the bounding box
    auto displayFrame = ocvImage.clone();
    cv::Rect2i rect(std::min(x1, x2), std::min(y1, y2), std::abs(x1 - x2), std::abs(y1 - y2));
    cv::Mat renderFrame;
    if (rect.area() > 0) {
        renderFrame = displayFrame(rect);
        renderFrame /= 2;
        renderFrame += utils::hue_to_scalar(hueValue) / 2;
    }
    cv::imshow("Object Selection", displayFrame);
}


// Mouse callback function for object selection
static void onMouse(int event, int x, int y, int, void* fp_v)
{
    switch (event)
    {
    case cv::EVENT_LBUTTONDOWN:
        //set origin of the bounding box
        startSel = true;
        selectObj = false;
        boundingBox.x = x;
        boundingBox.y = y;
        break;
    case cv::EVENT_LBUTTONUP:
        //set with and height of the bounding box
        boundingBox.width = std::abs(x - boundingBox.x);
        boundingBox.height = std::abs(y - boundingBox.y);
        boundingBox.x = std::clamp(static_cast<double>(x), 0.0, boundingBox.x);
        boundingBox.y = std::clamp(static_cast<double>(y), 0.0, boundingBox.y);
        selectObj = true;
        startSel = false;
        break;
    case cv::EVENT_MOUSEMOVE:

        if (startSel && !selectObj)
        {
            update_object_selection_window(boundingBox.x, boundingBox.y, x, y);
        }
        break;
    }
}

MainFrame::MainFrame(HINSTANCE hInst, HOST_APP_TABLE* host, EDIT_HANDLE* edit_handle)
    : m_hInst(hInst)
    , m_host(host)
{
    // メンバ変数にハンドル渡す
    m_edit_handle = edit_handle;

    // AviUtl2 のテーマカラーを読み込む (InitializeConfig より後に呼ばれる保証あり)
    m_colors.Load(config);

    // モデルファイルのパスを設定
    m_tracker.SetModelDir(utils::get_model_dir(m_hInst));

    // 自身のウィンドウを作成
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpszClassName = constants::WindowName;
    wcex.lpfnWndProc = wnd_proc;
    wcex.hInstance = m_hInst;
    wcex.hbrBackground = CreateSolidBrush(AviUtl2ColorToColorRef(m_colors.background));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    if (!RegisterClassEx(&wcex)) {
        return;
    }
    m_hwnd = CreateWindowEx(
        0,
        constants::WindowName,
        constants::WindowName,
        WS_POPUP, // 親ウィンドウの指定無しでWS_CHILDが作れないので一旦WS_POPUPで作成しています
        CW_USEDEFAULT, CW_USEDEFAULT, 320, CW_USEDEFAULT,
        nullptr,
        nullptr,
        hInst,
        this); // wnd_proc で、this をWM_NCCREATE で回収して保存する
    if (!m_hwnd) {
        return;
    }
    // UI 構築
    CreateControls();
}

// 操作系ボタンをまとめて有効/無効化する
// (m_during_operation のbool判定だけでは、cv::waitKey等のメッセージポンプ経由の再入を
//  防ぎきれないケースがあるため、OSレベルでクリックイベント自体を発生させないようにする)
static void EnableOperationButtons(HWND hwnd, BOOL enable) {
    static const IDC_Button targets[] = {
        IDC_Button::SelectObject,
        IDC_Button::Analyze,
        IDC_Button::ViewResult,
        IDC_Button::ClearResult,
        IDC_Button::InsertObject,
    };
    for (auto id : targets) {
        EnableWindow(GetDlgItem(hwnd, (int)id), enable);
    }
    // Export Object File 等のポップアップメニューを出す File ボタン、Info ボタンも一緒に無効化
    EnableWindow(GetDlgItem(hwnd, (int)IDC_Toolbar::File), enable);
    EnableWindow(GetDlgItem(hwnd, (int)IDC_Toolbar::Info), enable);
}

LRESULT CALLBACK MainFrame::wnd_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    MainFrame* self = nullptr;

    // WM_NCCREATE は lparam に this が入ってくる一番最初のメッセージ
    if (message == WM_NCCREATE) {

        // CREATESTRUCT::lpCreateParams が CreateWindowEx の最後の引数 = this
        // WM_NCCREATE 時、lparam は CREATESTRUCT 構造体へのポインタとなっている。
        // そのメンバ lpCreateParams（void*型）に、CreateWindowEx の第12引数（this）が入っているため、MainFrame* にキャストして取り出す。
        self = static_cast<MainFrame*>(reinterpret_cast<CREATESTRUCT*>(lparam)->lpCreateParams);

        // HWND に this を紐付けて this を GWLP_USERDATA に格納
        // 第3引数はLONG_PTRのため、cast
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));

        self->m_hwnd = hwnd; // この時点でまだ m_hwnd に入っていないので手動でセット

    } else {
        // WM_NCCREATE 以降は保存した値を取り出すだけ
        self = reinterpret_cast<MainFrame*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    // WM_NCCREATE より前のメッセージ（WM_GETMINMAXINFO等）は
    // まだ保存していないので self が nullptr になる -> DefWindowProc に流す
    if (!self) return DefWindowProc(hwnd, message, wparam, lparam);

    switch (message) {
        case WM_HSCROLL:
        case WM_VSCROLL: {
            if ((HWND)lparam == GetDlgItem(hwnd, (int)IDC_Button::HueTrackbar)) {
                hueValue = SendMessage((HWND)lparam, TBM_GETPOS, 0, 0);
                wchar_t buffer[16];
                swprintf_s(buffer, L"%d", hueValue);
                SetWindowText(GetDlgItem(hwnd, (int)IDC_Button::HueValue), buffer);
            }
            break;
        }
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORBTN:
            return ownerdraw::OnCtlColor(wparam, self->m_colors);
        case WM_DRAWITEM:
            return ownerdraw::OnDrawItem(lparam, self->m_colors);
        case WM_APP_ANALYZE_DONE:
            // ProgressDlgからの、バックグラウンド解析完了通知
            logger->info(logger, L"MainFrame: WM_APP_ANALYZE_DONE received");
            self->m_during_operation = false;
            EnableOperationButtons(hwnd, TRUE);
            return 0;
        case WM_COMMAND:
            // ツールバー・メニューからのコマンド
            switch (static_cast<IDC_Menu>(LOWORD(wparam))) {
                case IDC_Menu::ExportCSV:
                    // TODO: CSV エクスポート
                    return 0;
                case IDC_Menu::ExportObject:
                {
                    if (self->m_during_operation || self->m_tracker.m_analyzing) {
                        MessageBoxW(hwnd, config->translate(config, L"Another operation is in progress."), L"Operation Error", MB_OK | MB_ICONWARNING);
                        return 0;
                    }
                    self->m_during_operation = true;
                    EnableOperationButtons(hwnd, FALSE);
                    if (!self->m_tracker.HasResult()) {
                        MessageBoxW(hwnd, config->translate(config, L"No track data to save!"), L"Operation Error", MB_OK | MB_ICONWARNING);
                        self->m_during_operation = false;
                        EnableOperationButtons(hwnd, TRUE);
                        return 0;
                    }

                    wchar_t filepath[MAX_PATH] = L"tracking.object";
                    OPENFILENAMEW ofn = {};
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hwnd;
                    ofn.lpstrFilter = L"ObjectFile (*.object)\0*.object\0All Files (*.*)\0*.*\0";
                    ofn.lpstrFile = filepath;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.lpstrDefExt = L"object";
                    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
                    if (!GetSaveFileNameW(&ofn)) {
                        self->m_during_operation = false;
                        EnableOperationButtons(hwnd, TRUE);
                        return 0;
                    }

                    bool ignoreAspectRatio = (bool)GetWindowLongPtr(GetDlgItem(hwnd, (int)IDC_Button::IgnoreAspectRatio), GWLP_USERDATA);
                    bool invertPosition = (bool)GetWindowLongPtr(GetDlgItem(hwnd, (int)IDC_Button::InvertPosition), GWLP_USERDATA);
                    bool asSubFilter = (bool)GetWindowLongPtr(GetDlgItem(hwnd, (int)IDC_Button::AsSubFilter), GWLP_USERDATA);
                    bool ok = InsertObject::ExportToFile(
                        self->m_tracker.Results(),
                        self->m_tracker.Found(),
                        self->m_tracker.RangeStart(),
                        self->m_edit_handle,
                        filepath,
                        ignoreAspectRatio,
                        invertPosition,
                        asSubFilter
                    );
                    if (!ok) {
                        MessageBox(hwnd, TEXT("Failed to save Alias"), TEXT("Error"), MB_OK);
                    }
                    self->m_during_operation = false;
                    EnableOperationButtons(hwnd, TRUE);
                    return 0;
                }
                default:
                    self->m_during_operation = false;
                    break;
            }
            // File ボタン -> ボタン直下にポップアップメニューを表示
            if (LOWORD(wparam) == (UINT)IDC_Toolbar::File) {
                HWND hBtn = (HWND)lparam;
                RECT rc;
                GetWindowRect(hBtn, &rc);
                HMENU hPopup = CreatePopupMenu();
                // AppendMenuW(hPopup, MF_STRING, (UINT_PTR)IDC_Menu::ExportCSV,    L"Export CSV...");
                AppendMenuW(hPopup, MF_STRING, (UINT_PTR)IDC_Menu::ExportObject, L"Export Object File");
                TrackPopupMenu(hPopup, TPM_LEFTALIGN | TPM_TOPALIGN, rc.left, rc.bottom, 0, hwnd, nullptr);
                DestroyMenu(hPopup);
                SetFocus(nullptr);
                return 0;
            }
            // Info
            if (LOWORD(wparam) == (UINT)IDC_Toolbar::Info) {
                if (self->m_during_operation || self->m_tracker.m_analyzing) {
                    MessageBoxW(hwnd, config->translate(config, L"Another operation is in progress."), L"Operation Error", MB_OK | MB_ICONWARNING);
                    SetFocus(nullptr);
                    return 0;
                }
                self->m_during_operation = true;
                EnableOperationButtons(hwnd, FALSE);

                std::wstring content =
                    std::wstring(L"Version: ") + constants::version +
                    L"\n"
                    L"Developer: MaverickTse, Mr-Ojii, nullru"
                    L"\n\n"
                    L"Source Code:"
                    L"\n"
                    L"<A HREF=\"https://github.com/nullruptr/MotionTracking_MK-II_Plus_for_AviUtl2\">nullruptr/MotionTracking_MK-II_Plus_for_AviUtl2</A>"
                    L"\n\n"
                    L"LICENSE: "
                    L"<A HREF=\"https://github.com/nullruptr/MotionTracking_MK-II_Plus_for_AviUtl2/blob/master/LICENSE\">MIT</A>";


                TASKDIALOGCONFIG tdc = {};
                tdc.cbSize = sizeof(tdc);
                tdc.hwndParent = hwnd;
                tdc.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_ENABLE_HYPERLINKS;
                tdc.pszWindowTitle = L"Info";
                tdc.pszMainIcon = TD_INFORMATION_ICON;
                tdc.pszMainInstruction = constants::WindowName;
                tdc.pszContent = content.c_str();
                tdc.pfCallback = [](HWND hDlg, UINT msg, WPARAM wp, LPARAM lp, LONG_PTR) -> HRESULT {
                    if (msg == TDN_HYPERLINK_CLICKED) {
                        ShellExecuteW(hDlg, L"open", (LPCWSTR)lp, nullptr, nullptr, SW_SHOWNORMAL);
                    }
                    return S_OK;
                };
                TaskDialogIndirect(&tdc, nullptr, nullptr, nullptr);

                SetFocus(nullptr);
                self->m_during_operation = false;
                EnableOperationButtons(hwnd, TRUE);
                return 0;
            }
            // ウィンドウコントロールからのコマンド
            switch (static_cast<IDC_Button>(LOWORD(wparam))) {
                case IDC_Button::AsSubFilter:
                case IDC_Button::InvertPosition:
                case IDC_Button::IgnoreAspectRatio: {
                    HWND hBtn = (HWND)lparam;
                    int state = (int)GetWindowLongPtr(hBtn, GWLP_USERDATA);
                    SetWindowLongPtr(hBtn, GWLP_USERDATA, (LONG_PTR)!state);
                    InvalidateRect(hBtn, nullptr, TRUE);
                    SetFocus(nullptr);
                    return 0;
                }
                case IDC_Button::ViewResult:
                {
                    if (self->m_during_operation || self->m_tracker.m_analyzing) {
                        MessageBoxW(hwnd, config->translate(config, L"Another operation is in progress."), L"Operation Error", MB_OK | MB_ICONWARNING);
                        SetFocus(nullptr);
                        return 0;
                    }
                    self->m_during_operation = true;
                    EnableOperationButtons(hwnd, FALSE);
                    if (!self->m_tracker.HasResult()) {
                        MessageBoxA(nullptr, "No tracking result found", "Operational Error", MB_OK);
                        SetFocus(nullptr);
                        self->m_during_operation = false;
                        EnableOperationButtons(hwnd, TRUE);
                        return 0;
                    }
                    self->m_tracker.ShowResultWindow(self->m_edit_handle);
                    SetFocus(nullptr);
                    self->m_during_operation = false;
                    EnableOperationButtons(hwnd, TRUE);
                    return 0;
                }
                case IDC_Button::SelectObject:
                {
                    if (self->m_during_operation || self->m_tracker.m_analyzing) {
                        MessageBoxW(hwnd, config->translate(config, L"Another operation is in progress."), L"Operation Error", MB_OK | MB_ICONWARNING);
                        SetFocus(nullptr);
                        return 0;
                    }
                    self->m_during_operation = true;
                    EnableOperationButtons(hwnd, FALSE);
                    logger->info(logger, L"SelectObject: start");
                    self->m_tracker.SelectObject(self->m_edit_handle, hueValue);
                    SetFocus(nullptr);
                    self->m_during_operation = false;
                    EnableOperationButtons(hwnd, TRUE);
                    return 0;
                }
                case IDC_Button::Analyze:
                {
                    if (self->m_during_operation || self->m_tracker.m_analyzing) {
                        MessageBoxW(hwnd, config->translate(config, L"Another operation is in progress."), L"Operation Error", MB_OK | MB_ICONWARNING);
                        SetFocus(nullptr);
                        return 0;
                    }
                    self->m_during_operation = true;
                    EnableOperationButtons(hwnd, FALSE);
                    int sel = SendMessage(GetDlgItem(hwnd, (int)IDC_Button::TrackingMethodCombo),
                         CB_GETCURSEL, 0, 0);
                    auto method = static_cast<TrackingMethod>(sel);
                    if (self->m_tracker.Analyze(self->m_edit_handle, method)) {
                        // 解析中プログレスバー
                        ProgressDlg::Create(hwnd, &self->m_tracker, self->m_hInst, track_method[sel]);
                    }
                    SetFocus(nullptr);
                    self->m_during_operation = false;
                    // Analyzeはバックグラウンドスレッドで継続するため、m_analyzingが解除されるまではボタンを戻さない
                    if (!self->m_tracker.m_analyzing) {
                        EnableOperationButtons(hwnd, TRUE);
                    }
                    return 0;
                }
                case IDC_Button::InsertObject:
                {
                    if (self->m_during_operation || self->m_tracker.m_analyzing) {
                        MessageBoxW(hwnd, config->translate(config, L"Another operation is in progress."), L"Operation Error", MB_OK | MB_ICONWARNING);
                        SetFocus(nullptr);
                        return 0;
                    }
                    self->m_during_operation = true;
                    EnableOperationButtons(hwnd, FALSE);
                    if (!self->m_tracker.HasResult()) {
                        MessageBoxW(hwnd, config->translate(config, L"No track data."), constants::WindowName, MB_OK | MB_ICONWARNING);
                        SetFocus(nullptr);
                        self->m_during_operation = false;
                        EnableOperationButtons(hwnd, TRUE);
                        return 0;
                    }
                    bool ignoreAspectRatio = (bool)GetWindowLongPtr(GetDlgItem(hwnd, (int)IDC_Button::IgnoreAspectRatio), GWLP_USERDATA);
                    bool invertPosition = (bool)GetWindowLongPtr(GetDlgItem(hwnd, (int)IDC_Button::InvertPosition), GWLP_USERDATA);
                    bool asSubFilter = (bool)GetWindowLongPtr(GetDlgItem(hwnd, (int)IDC_Button::AsSubFilter), GWLP_USERDATA);
                    bool ok = InsertObject::Insert(
                        self->m_tracker.Results(),
                        self->m_tracker.Found(),
                        self->m_tracker.RangeStart(),
                        self->m_edit_handle,
                        ignoreAspectRatio,
                        invertPosition,
                        asSubFilter
                    );
                    if (!ok)
                        MessageBoxW(hwnd,
                            L"Failed to insert object.\n"
                            "An object may already exist on this layer.\n"
                            "Please select a different layer and try again.",
                            L"Insert failed", MB_OK | MB_ICONERROR);
                    SetFocus(nullptr);
                    self->m_during_operation = false;
                    EnableOperationButtons(hwnd, TRUE);
                    return 0;
                }
                case IDC_Button::ClearResult:
                {
                    if (self->m_during_operation || self->m_tracker.m_analyzing) {
                        MessageBoxW(hwnd, config->translate(config, L"Another operation is in progress."), L"Operation Error", MB_OK | MB_ICONWARNING);
                        SetFocus(nullptr);
                        return 0;
                    }
                    self->m_during_operation = true;
                    EnableOperationButtons(hwnd, FALSE);
                    self->m_tracker.Clear();
                    MessageBoxW(hwnd, L"Selection states, results and image cache reseted", L"INFO", MB_OK);
                    self->m_during_operation = false;
                    EnableOperationButtons(hwnd, TRUE);
                    return 0;
                }
                default:
                    break;
            }
            break;
    }
    return DefWindowProc(hwnd, message, wparam, lparam);
}
