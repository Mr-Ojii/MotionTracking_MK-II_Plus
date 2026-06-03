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
                    OBJECT_LAYER_FRAME olf = {};
                    if (!get_effected_object_layer_frame(edit_handle, &olf)) {
                        if (!create_alias_object_and_set_olf(edit_handle, &olf)) {
                            MessageBox(hwnd, config->translate(config, L"Please select an object with MotionTracker_M Filter effect or create an alias object."), constants::WindowName, MB_OK | MB_ICONINFORMATION);
                            return 0;
                        }
                    }

                    if (!selectObj)
                    {
                        MessageBoxA(NULL, "Nothing selected", "Operation Error", MB_OK);
                        return 0;
                    }


                    track_result.clear();
                    track_found.clear();
                    cv::Rect2i box = boundingBox;
                    //Correct for out-of-bound box
                    if (box.br().x > ocvImage.cols)
                    {
                        box.width = ocvImage.cols - box.x;
                    }
                    if (box.br().y > ocvImage.rows)
                    {
                        box.height = ocvImage.rows - box.y;
                    }
                    int64 start_time = cv::getTickCount();
                    // Create Tracker
                    cv::Ptr<cv::Tracker> tracker;
                    try
                    {
                        int selected_index = SendMessage(GetDlgItem(hwnd, (int)IDC_Button::TrackingMethodCombo), CB_GETCURSEL, 0, 0);

                        switch (selected_index) {
                        case 0:
                            tracker = cv::TrackerMIL::create();
                            break;
                        case 1:
                            // KCFはOpenCVのextra modulesに移動されたため、環境によっては利用できない可能性があります
                            tracker = cv::TrackerKCF::create();
                            break;
                        case 2:
                            tracker = cv::TrackerCSRT::create();
                            break;
                        case 3:
                        {
                            auto params = cv::TrackerDaSiamRPN::Params();
                            params.model = modelDir +  "dasiamrpn_model.onnx";
                            params.kernel_r1 = modelDir + "dasiamrpn_kernel_r1.onnx";
                            params.kernel_cls1 = modelDir + "dasiamrpn_kernel_cls1.onnx";
                            tracker = cv::TrackerDaSiamRPN::create(params);
                            break;
                        }
                        case 4:
                        {
                            auto params = cv::TrackerNano::Params();
                            params.backbone = modelDir + "nanotrack_backbone_sim.onnx";
                            params.neckhead = modelDir + "nanotrack_head_sim.onnx";
                            tracker = cv::TrackerNano::create(params);
                            break;
                        }
                        case 5:
                        {
                            //なんか2つモデルがあるが、上のほうが良い？
                            //https://github.com/opencv/opencv_extra/blob/4.x/testdata/dnn/onnx/models/vitTracker.onnx
                            //https://github.com/opencv/opencv_zoo/blob/main/models/object_tracking_vittrack/object_tracking_vittrack_2023sep.onnx

                            auto params = cv::TrackerVit::Params();
                            params.net = modelDir + "vitTracker.onnx";
                            tracker = cv::TrackerVit::create(params);
                            break;
                        }
                        default:
                            // 選択されていない、または不正なインデックス
                            MessageBox(hwnd, config->translate(config, L"Please select a tracking method."), constants::WindowName, MB_OK | MB_ICONERROR);
                            return 0;
                        }
                    }
                    catch (cv::Exception e)
                    {
                        MessageBoxA(NULL, e.what(), "OpenCV3 Error", MB_OK);
                        return FALSE;
                    }
                    catch (...)
                    {
                        //nullptr
                        tracker = cv::Ptr<cv::Tracker>();
                    }
                    if (!tracker)
                    {
                        MessageBoxA(NULL, "Error when creating tracker", "OpenCV3 Error", MB_OK);
                        return FALSE;
                    }

                    bool track_init = false;
                    track_result.clear();
                    track_found.clear();
                    //Loop through selected frames for analysis
                    char shortmsg[64] = { 0 };
                    int64 prev_stamp, new_stamp;


                    finishedFilter = false;
                    getImageFromAUX = true;;
                    cv::Mat image;
                    for (int frame = olf.start; frame <= olf.end; frame++) {
                        finishedFilter = false; // ループの開始時にフラグをリセット
                        edit_handle->call_edit_section_param(&frame, [](void* message, EDIT_SECTION* edit) {
                            int* frame = (int*)message;
                            edit->set_cursor_layer_frame(0, *frame);
                        });

                        std::unique_lock<std::mutex> lock(mtx);
                        cov.wait(lock, [] { return finishedFilter; });

                        // BGRAではトラッキングができないものがあるため、RGBに変換（CSRTなど）
                        cv::cvtColor(ocvImage, image, cv::COLOR_BGRA2RGB);

                        if (image.empty()) {
                            MessageBoxA(NULL, "Failed to get image for tracking.", "Error", MB_OK);
                            getImageFromAUX = false;
                            return 0;
                        }

                        if (!track_init)
                        {
                            try
                            {
                                tracker->init(image, boundingBox);
                            }
                            catch (std::exception e)
                            {
                                MessageBoxA(NULL, e.what(), "OpenCV3 Error", MB_OK);
                            }
                            catch (...)
                            {
                                MessageBoxA(NULL, "Error initializing tracker", "OpenCV3 Error", MB_OK);
                                return FALSE;
                            }
                            track_init = true;
                            track_found.push_back(true);

                            prev_stamp = cv::getTickCount();
                        }
                        else
                        {
                            new_stamp = cv::getTickCount();
                            double fps = 1.0 / ((new_stamp - prev_stamp) / cv::getTickFrequency());
                            prev_stamp = cv::getTickCount();

                            try {
                                if (tracker->update(image, box))
                                {
                                    track_found.push_back(true);
                                }
                                else
                                {
                                    track_found.push_back(false);
                                }
                            }
                            catch (...)
                            {
                                MessageBoxA(NULL, "Obscure tracker error", "Tracker update exception", MB_OK);
                                return FALSE;
                            }
                        }
                        if (box.area() <= 0)
                        {
                            box.width = 20;
                            box.height = 20;
                        }
                        if (box.x < 0) {
                            box.width += box.x;
                            box.x = 0;
                        }
                        if (box.y < 0) {
                            box.height += box.y;
                            box.y = 0;
                        }
                        if (box.x + box.width > image.cols)
                        {
                            box.width = image.cols - box.x;
                        }
                        if (box.y + box.height > image.rows)
                        {
                            box.height = image.rows - box.y;
                        }
                        track_result.push_back(box);


                        MSG msg;
                        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                            // マウス入力とキーボード入力のメッセージを破棄して操作を無効化する
                            if ((msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST) ||
                                (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST)) {
                                continue;
                            }
                            TranslateMessage(&msg);
                            DispatchMessage(&msg);
                        }
                    }
                    getImageFromAUX = false;

                    int64 end_time = cv::getTickCount();
                    double run_time = (end_time - start_time) / cv::getTickFrequency();
                    SecureZeroMemory(shortmsg, sizeof(char[64]));
                    sprintf_s(shortmsg, "Tracking Completed!\nAverage %.2f fps", (olf.end - olf.start) / run_time);
                    MessageBoxA(NULL, shortmsg, "Tracking Completed!", MB_OK);

                    SetFocus(nullptr);
                    return 0;
                }
                case IDC_Button::ClearResult:
                    boundingBox.x = 0;
                    boundingBox.y = 0;
                    boundingBox.width = 0;
                    boundingBox.height = 0;
                    track_result.clear();
                    track_found.clear();
                    startSel = false;
                    selectObj = false;
                    ocvImage.empty();
                    MessageBox(NULL, L"Selection states, results and image cache reseted", L"INFO", MB_OK);
                    SetFocus(nullptr);
                    return 0;
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
