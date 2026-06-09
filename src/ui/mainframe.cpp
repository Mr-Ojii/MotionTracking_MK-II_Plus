#include "mainframe.hpp"
#include "ownerdraw.hpp"
#include "aviutl2_sdk/plugin2.h"
#include <windows.h>

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


EXTERN_C __declspec(dllexport) DWORD RequiredVersion() {
    return 2003300;
}

EXTERN_C __declspec(dllexport) void InitializeLogger(LOG_HANDLE* handle) {
    logger = handle;
}

EXTERN_C __declspec(dllexport) void InitializeConfig(CONFIG_HANDLE* handle) {
    config = handle;
}

EXTERN_C __declspec(dllexport) bool InitializePlugin(DWORD version) {
    return true;
}

EXTERN_C __declspec(dllexport) void UninitializePlugin() {
}


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

    // モデルファイルのパスを設定
    m_tracker.SetModelDir(utils::get_model_dir(m_hInst));

    // 自身のウィンドウを作成
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpszClassName = constants::WindowName;
    wcex.lpfnWndProc = wnd_proc;
    wcex.hInstance = m_hInst;
    wcex.hbrBackground = CreateSolidBrush((COLORREF)m_colors.background);
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
        case WM_COMMAND:
            switch (static_cast<IDC_Button>(LOWORD(wparam))) {
                case IDC_Button::ViewResult:
                case IDC_Button::AsSubFilter:
                case IDC_Button::InvertPosition:
                case IDC_Button::IgnoreAspectRatio:
                case IDC_Button::QuickBlur:
                case IDC_Button::EasyPrivacy: {
                    HWND hBtn = (HWND)lparam;
                    int state = (int)GetWindowLongPtr(hBtn, GWLP_USERDATA);
                    SetWindowLongPtr(hBtn, GWLP_USERDATA, (LONG_PTR)!state);
                    InvalidateRect(hBtn, nullptr, TRUE);
                    SetFocus(nullptr);
                    return 0;
                }
                case IDC_Button::SelectObject:
                {
                    logger->info(logger, L"SelectObject: start");
                    self->m_tracker.SelectObject(self->m_edit_handle);
                    SetFocus(nullptr);
                    return 0;
                }
                case IDC_Button::Analyze:
                {
                    int sel = SendMessage(GetDlgItem(hwnd, (int)IDC_Button::TrackingMethodCombo),
                         CB_GETCURSEL, 0, 0);
                    auto method = static_cast<TrackingMethod>(sel);
                    self->m_tracker.Analyze(self->m_edit_handle, method);
                }
                case IDC_Button::InsertObject:
                    SetFocus(nullptr);

                    // //TODO
                    // if (track_result.size() <= 0)
                    // {
                    //     MessageBox(fp->hwnd, "No track data to save!", "Operation Error", MB_OK);
                    //     return FALSE;
                    // }

                    // CHAR filename[MAX_PATH] = "D:\\tracking.exo";

                    // // Starts doing the real work
                    // std::ostringstream strbuf;
                    // TCHAR boilerplate[2048] = { 0 };
                    // TCHAR fmtstr[2048] = { 0 };
                    // /* Common Project Data*/
                    // int width, height;
                    // FILE_INFO fi;
                    // fp->exfunc->get_pixel_filtered(editp, selA, NULL, &width, &height);
                    // fp->exfunc->get_file_info(editp, &fi);
                    // /* End common prj data*/
                    // /* Format and write to buffer*/
                    // LoadString(fp->dll_hinst, IDS_PRJHEADER, boilerplate, 2048);
                    // sprintf_s(fmtstr, sizeof(TCHAR[2048]), boilerplate, width, height, fi.video_rate, fi.video_scale, fi.frame_n, fi.audio_rate);
                    // strbuf << fmtstr;
                    // SecureZeroMemory(boilerplate, sizeof(TCHAR[2048]));
                    // SecureZeroMemory(fmtstr, sizeof(TCHAR[2048]));
                    // //* END OF COMMON PROJECT HEADER*//
                    // //Starts object processing //
                    // /* vector storage for bounding-box interpolation and various checks*/
                    // std::vector<UINT32> frmInterpolate; //frames to be interpolate;
                    // std::vector<FRMFIX> fixedFrm; //interpolated and use AviUtl coordinate
                    // std::vector<FRMGROUP> gpinfo; //strats and ends of objects
                    // //
                    // find_inter_frame(track_found, frmInterpolate);
                    // fix_frame(track_result, track_found, frmInterpolate, fixedFrm, width, height);
                    // groupObject(fixedFrm, gpinfo);
                    // // Should now have all info we need...
                    // int object_id = 0; //keep track of how many segments/objects have been added
                    // for (size_t o = 0; o < gpinfo.size(); o++) //loop through each object
                    // {
                    //     int oStart, oEnd;
                    //     oStart = gpinfo[o].vi_start;
                    //     oEnd = gpinfo[o].vi_end;
                    //     bool firstframe = true;
                    //     for (int f = oStart; f <= oEnd; f++) //each frames in this object
                    //     {
                    //         //Section Num
                    //         char head_num[32];
                    //         sprintf_s(head_num, sizeof(char[32]), "[%d]\n\0", object_id);
                    //         strbuf << head_num;
                    //         //Common obj param
                    //         LoadString(fp->dll_hinst, IDS_OBJPARAM, boilerplate, 2048);
                    //         sprintf_s(fmtstr, sizeof(TCHAR[2048]), boilerplate, fixedFrm[f].frame, fixedFrm[f].frame, 1);//st, ed, layer
                    //         strbuf << fmtstr;
                    //         SecureZeroMemory(boilerplate, sizeof(TCHAR[2048]));
                    //         SecureZeroMemory(fmtstr, sizeof(TCHAR[2048]));
                    //         //2 more param for Figure obj
                    //         if (!fp->check[4])
                    //         {
                    //             LoadString(fp->dll_hinst, IDS_FIGUREPARAMA, boilerplate, 2048);
                    //             strbuf << boilerplate; //verbatim copy
                    //             SecureZeroMemory(boilerplate, sizeof(TCHAR[2048]));
                    //         }
                    //         if (!firstframe)
                    //         {
                    //             LoadString(fp->dll_hinst, IDS_OBJCHAIN, boilerplate, 2048);
                    //             strbuf << boilerplate; //verbatim copy
                    //             SecureZeroMemory(boilerplate, sizeof(TCHAR[2048]));
                    //         }
                    //         //Section [*.0] Graphics/Sub-filter
                    //         sprintf_s(head_num, sizeof(char[32]), "[%d.0]\n\0", object_id);
                    //         strbuf << head_num;
                    //         //
                    //         if (fp->check[4]) //Sub-filter
                    //         {
                    //             LoadString(fp->dll_hinst, IDS_SFPARAM_JP, boilerplate, 2048);//TODO: Set JP text
                    //             int Xi, Xf, Yi, Yf, size_st, size_ed;
                    //             double  rAsp_st, rAsp_ed;
                    //             Xi = fixedFrm[f].cx;
                    //             Yi = fixedFrm[f].cy;
                    //             if (fixedFrm[f].width > fixedFrm[f].height) //-ve rAsp, const width
                    //             {
                    //                 size_st = fixedFrm[f].width;
                    //                 rAsp_st = -100.0 * (1.0 - ((double)fixedFrm[f].height / (double)fixedFrm[f].width));
                    //             }
                    //             else if (fixedFrm[f].width < fixedFrm[f].height) // +ve rAsp, const height
                    //             {
                    //                 size_st = fixedFrm[f].height;
                    //                 rAsp_st = 100.0 * (1.0 - ((double)fixedFrm[f].width / (double)fixedFrm[f].height));
                    //             }
                    //             else // rAsp=0, square
                    //             {
                    //                 size_st = fixedFrm[f].width;
                    //                 rAsp_st = 0.0;
                    //             }
                    //             if ((size_t)f >= (fixedFrm.size() - 1))//special handling for last frame
                    //             {
                    //                 Xf = Xi;
                    //                 Yf = Yi;
                    //                 size_ed = size_st;
                    //                 rAsp_ed = rAsp_st;
                    //             }
                    //             else
                    //             {
                    //                 Xf = fixedFrm[f + 1].cx;
                    //                 Yf = fixedFrm[f + 1].cy;
                    //                 if (fixedFrm[f + 1].width > fixedFrm[f + 1].height) //-ve rAsp, const width
                    //                 {
                    //                     size_ed = fixedFrm[f + 1].width;
                    //                     rAsp_ed = -100.0 * (1.0 - ((double)fixedFrm[f + 1].height / (double)fixedFrm[f + 1].width));
                    //                 }
                    //                 else if (fixedFrm[f + 1].width < fixedFrm[f + 1].height) // +ve rAsp, const height
                    //                 {
                    //                     size_ed = fixedFrm[f + 1].height;
                    //                     rAsp_ed = 100.0 * (1.0 - ((double)fixedFrm[f + 1].width / (double)fixedFrm[f + 1].height));
                    //                 }
                    //                 else // rAsp=0, square
                    //                 {
                    //                     size_ed = fixedFrm[f + 1].width;
                    //                     rAsp_ed = 0.0;
                    //                 }
                    //             }
                    //             if (fp->check[6]) // Ignore Aspect Ratio
                    //             {
                    //                 rAsp_st = 0.0;
                    //                 rAsp_ed = 0.0;
                    //             }

                    //             if (fp->check[5]) //Invert Position
                    //             {
                    //                 Xi = -Xi;
                    //                 Xf = -Xf;
                    //                 Yi = -Yi;
                    //                 Yf = -Yf;
                    //             }

                    //             sprintf_s(fmtstr, sizeof(TCHAR[2048]), boilerplate, (double)Xi, (double)Xf, (double)Yi, (double)Yf, size_st, size_ed, rAsp_st, rAsp_ed);//X-st, X-ed, Y-st, Y-ed, size_start, size_end, rAsp-st, rAsp-ed
                    //             strbuf << fmtstr;
                    //             SecureZeroMemory(boilerplate, sizeof(TCHAR[2048]));
                    //             SecureZeroMemory(fmtstr, sizeof(TCHAR[2048]));
                    //         }
                    //         else //Graphics
                    //         {
                    //             LoadString(fp->dll_hinst, IDS_FIGUREPARAMB_JP, boilerplate, 2048);//TODO: Set JP text
                    //             strbuf << boilerplate;
                    //             SecureZeroMemory(boilerplate, sizeof(TCHAR[2048]));
                    //         }
                    //         //Section [*.1] Resize FX or Mono FX
                    //         sprintf_s(head_num, sizeof(char[32]), "[%d.1]\n\0", object_id);
                    //         strbuf << head_num;
                    //         //
                    //         if (fp->check[4])// Mono FX for Sub-filter
                    //         {
                    //             LoadString(fp->dll_hinst, IDS_FXMONO_JP, boilerplate, 2048);//TODO: Set JP text
                    //             strbuf << boilerplate;
                    //             SecureZeroMemory(boilerplate, sizeof(TCHAR[2048]));
                    //         }
                    //         else //Resize FX for Graphics
                    //         {
                    //             int Wi, Wf, Hi, Hf;
                    //             float Si, Sf;

                    //             Wi = fixedFrm[f].width;
                    //             Hi = fixedFrm[f].height;
                    //             Si = fixedFrm[f].scale;

                    //             if ((size_t)f >= (fixedFrm.size() - 1)) //Last frame
                    //             {
                    //                 Wf = Wi;
                    //                 Hf = Hi;
                    //                 Sf = Si;
                    //             }
                    //             else // Normal
                    //             {
                    //                 Wf = fixedFrm[f + 1].width;
                    //                 Hf = fixedFrm[f + 1].height;
                    //                 Sf = fixedFrm[f + 1].scale;
                    //             }

                    //             LoadString(fp->dll_hinst, IDS_FXRESIZE_JP, boilerplate, 2048);//TODO: Set JP text
                    //             if (fp->check[6]) // Ignore Aspect Ratio
                    //             {
                    //                 sprintf_s(fmtstr, sizeof(TCHAR[2048]), boilerplate, (double)Si, Sf, 100.0, 100.0, 100.0, 100.0, !fp->check[6]);
                    //             }
                    //             else
                    //             {
                    //                 sprintf_s(fmtstr, sizeof(TCHAR[2048]), boilerplate, 100.0, 100.0, (double)Wi, (double)Wf, (double)Hi, (double)Hf, !fp->check[6]);
                    //             }
                    //             strbuf << fmtstr;
                    //             SecureZeroMemory(boilerplate, sizeof(TCHAR[2048]));
                    //             SecureZeroMemory(fmtstr, sizeof(TCHAR[2048]));
                    //         }
                    //         //Section [*.2] Std Drawing for Graphics
                    //         //Coordinate animation part for Graphics
                    //         if (!fp->check[4]) //only for graphics
                    //         {
                    //             sprintf_s(head_num, sizeof(char[32]), "[%d.2]\n\0", object_id);
                    //             strbuf << head_num;
                    //             //
                    //             int Xi, Xf, Yi, Yf;
                    //             Xi = fixedFrm[f].cx;
                    //             Yi = fixedFrm[f].cy;
                    //             if ((size_t)f >= (fixedFrm.size() - 1))//last frame
                    //             {
                    //                 Xf = Xi;
                    //                 Yf = Yi;
                    //             }
                    //             else //Normal
                    //             {
                    //                 Xf = fixedFrm[f + 1].cx;
                    //                 Yf = fixedFrm[f + 1].cy;
                    //             }
                    //             LoadString(fp->dll_hinst, IDS_STDDRAW_JP, boilerplate, 2048);//TODO: Set JP text
                    //             if (fp->check[5]) // invert position
                    //             {
                    //                 Xi = -Xi;
                    //                 Xf = -Xf;
                    //                 Yi = -Yi;
                    //                 Yf = -Yf;
                    //             }
                    //             sprintf_s(fmtstr, sizeof(TCHAR[2048]), boilerplate, (double)Xi, (double)Xf, (double)Yi, (double)Yf);
                    //             strbuf << fmtstr;
                    //             SecureZeroMemory(boilerplate, sizeof(TCHAR[2048]));
                    //             SecureZeroMemory(fmtstr, sizeof(TCHAR[2048]));
                    //         }
                    //         //
                    //         object_id++;
                    //         firstframe = false;
                    //     }
                    // }
                    // //Write to file
                    // std::ofstream fhandle(filename, std::ofstream::out | std::ofstream::trunc);
                    // if (fhandle.is_open())
                    // {
                    //     fhandle << strbuf.str();
                    //     fhandle.flush();
                    //     fhandle.close();
                    //     MessageBox(fp->hwnd, "DONE", "Finished", MB_OK);
                    // }
                    // else
                    // {
                    //     MessageBox(fp->hwnd, "Cannot write to file!", "File I/O ERROR", MB_OK);
                    //     return FALSE;
                    // }

                    return 0;
            }
            break;
    }
    return DefWindowProc(hwnd, message, wparam, lparam);
}
