#include "mainframe.hpp"
#include "aviutl2_sdk/plugin2.h"

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

// config->get_color_code で取得した値を格納しておく
struct SystemColors {
    int background;
    int buttonBody;
    int buttonBodyPress;
    int buttonBodyHover;
    int buttonBodyDisable;
    int text;
    int textDisable;
} systemColors = {};

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

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
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
        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wparam;
            SetTextColor(hdc, (COLORREF)systemColors.text);
            SetBkColor(hdc, (COLORREF)systemColors.background);
            return (LRESULT)CreateSolidBrush((COLORREF)systemColors.background);
        }
        case WM_CTLCOLORLISTBOX: {
            HDC hdc = (HDC)wparam;
            SetTextColor(hdc, (COLORREF)systemColors.text);
            SetBkColor(hdc, (COLORREF)systemColors.background);
            return (LRESULT)CreateSolidBrush((COLORREF)systemColors.background);
        }
        case WM_CTLCOLOREDIT: {
            HDC hdc = (HDC)wparam;
            SetTextColor(hdc, (COLORREF)systemColors.text);
            SetBkColor(hdc, (COLORREF)systemColors.background);
            return (LRESULT)CreateSolidBrush((COLORREF)systemColors.background);
        }
        case WM_CTLCOLORBTN: {
            HDC hdc = (HDC)wparam;
            SetTextColor(hdc, (COLORREF)systemColors.text);
            SetBkColor(hdc, (COLORREF)systemColors.background);
            return (LRESULT)CreateSolidBrush((COLORREF)systemColors.background);
        }
        case WM_DRAWITEM: {
            // 背景色や文字色を指定のでdrawするためのOWNERDRAW
            DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lparam;

            if (dis->CtlType == ODT_BUTTON) {
                int id = dis->CtlID;
                bool isCheckbox = (id == (int)IDC_Button::ViewResult || id == (int)IDC_Button::AsSubFilter ||
                                   id == (int)IDC_Button::InvertPosition || id == (int)IDC_Button::IgnoreAspectRatio ||
                                   id == (int)IDC_Button::QuickBlur || id == (int)IDC_Button::EasyPrivacy);

                if (isCheckbox) {
                    HBRUSH hbrBackground = CreateSolidBrush((COLORREF)systemColors.background);
                    FillRect(dis->hDC, &dis->rcItem, hbrBackground);

                    COLORREF textColor = (dis->itemState & ODS_DISABLED) ? (COLORREF)systemColors.textDisable : (COLORREF)systemColors.text;
                    int state = (int)GetWindowLongPtr(dis->hwndItem, GWLP_USERDATA);

                    RECT rcCheck = dis->rcItem;
                    int checkSize = 15;
                    rcCheck.left += 2;
                    rcCheck.right = rcCheck.left + checkSize;
                    rcCheck.top = rcCheck.top + (rcCheck.bottom - rcCheck.top - checkSize) / 2;
                    rcCheck.bottom = rcCheck.top + checkSize;

                    UINT uState = DFCS_BUTTONCHECK;
                    if (state) uState |= DFCS_CHECKED;
                    if (dis->itemState & ODS_DISABLED) uState |= DFCS_INACTIVE;
                    if (dis->itemState & ODS_SELECTED) uState |= DFCS_PUSHED;

                    DrawFrameControl(dis->hDC, &rcCheck, DFC_BUTTON, uState);

                    RECT rcText = dis->rcItem;
                    rcText.left = rcCheck.right + 5;

                    SetTextColor(dis->hDC, textColor);
                    SetBkMode(dis->hDC, TRANSPARENT);

                    WCHAR text[256];
                    GetWindowText(dis->hwndItem, text, sizeof(text) / sizeof(text[0]));
                    DrawText(dis->hDC, text, -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                    DeleteObject(hbrBackground);
                } else {
                    HBRUSH hbrBackground, hbrBorder;
                    COLORREF textColor;

                    if (dis->itemState & ODS_DISABLED) {
                        hbrBackground = CreateSolidBrush((COLORREF)systemColors.buttonBodyDisable);
                        textColor = (COLORREF)systemColors.textDisable;
                    } else if (dis->itemState & ODS_SELECTED) {
                        hbrBackground = CreateSolidBrush((COLORREF)systemColors.buttonBodyPress);
                        textColor = (COLORREF)systemColors.text;
                    } else {
                        hbrBackground = CreateSolidBrush((COLORREF)systemColors.buttonBody);
                        textColor = (COLORREF)systemColors.text;
                    }

                    FillRect(dis->hDC, &dis->rcItem, hbrBackground);

                    if ((dis->itemState & ODS_FOCUS) == 0) {
                        hbrBorder = CreateSolidBrush(RGB(128, 128, 128));
                    } else {
                        hbrBorder = CreateSolidBrush(RGB(64, 64, 64));
                    }
                    FrameRect(dis->hDC, &dis->rcItem, hbrBorder);

                    SetTextColor(dis->hDC, textColor);
                    SetBkMode(dis->hDC, TRANSPARENT);

                    WCHAR text[256];
                    GetWindowText(dis->hwndItem, text, sizeof(text) / sizeof(text[0]));

                    DrawText(dis->hDC, text, -1, &dis->rcItem,
                             DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                    DeleteObject(hbrBackground);
                    DeleteObject(hbrBorder);
                }
                return TRUE;
            }
            else if (dis->CtlType == ODT_COMBOBOX) {
                HBRUSH hbrBackground;
                COLORREF textColor = (COLORREF)systemColors.text;
                COLORREF bgColor = (COLORREF)systemColors.background;

                if (dis->itemState & ODS_DISABLED) {
                    textColor = (COLORREF)systemColors.textDisable;
                } else if (dis->itemState & ODS_SELECTED) {
                    bgColor = (COLORREF)systemColors.buttonBodyPress;
                }

                hbrBackground = CreateSolidBrush(bgColor);
                FillRect(dis->hDC, &dis->rcItem, hbrBackground);

                SetTextColor(dis->hDC, textColor);
                SetBkColor(dis->hDC, bgColor);
                SetBkMode(dis->hDC, OPAQUE);

                if (dis->itemID != (UINT)-1) {
                    WCHAR text[256];
                    SendMessage(dis->hwndItem, CB_GETLBTEXT, dis->itemID, (LPARAM)text);
                    DrawText(dis->hDC, text, -1, &dis->rcItem, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                }

                DeleteObject(hbrBackground);
                return TRUE;
            }
            break;
        }
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
                    OBJECT_LAYER_FRAME olf = {};
                    if (!get_effected_object_layer_frame(edit_handle, &olf)) {
                        if (!create_alias_object_and_set_olf(edit_handle, &olf)) {
                            MessageBox(hwnd, config->translate(config, L"Please select an object with MotionTracker_M Filter effect or create an alias object."), constants::WindowName, MB_OK | MB_ICONINFORMATION);
                            return 0;
                        }
                    }

                    finishedFilter = false;
                    getImageFromAUX = true;

                    edit_handle->call_edit_section_param(&olf.start, [](void* message, EDIT_SECTION* edit) {
                        int* frame = (int*)message;
                        edit->set_cursor_layer_frame(0, *frame);
                    });
                    // 描画待ち
                    std::unique_lock<std::mutex> lock(mtx);
                    cov.wait(lock, [] { return finishedFilter; });

                    getImageFromAUX = false;
                    if (ocvImage.empty()) {
                        MessageBox(hwnd, config->translate(config, L"Failed to get image from AviUtl. Please make sure AviUtl is in a state where it can provide images."), constants::WindowName, MB_OK | MB_ICONERROR);
                        return 0;
                    }
                    cv::namedWindow("Object Selection", cv::WINDOW_KEEPRATIO);
                    cv::setMouseCallback("Object Selection", onMouse, nullptr); // TODO: nullptrの代わりにuserdata
                    cv::resizeWindow("Object Selection", ocvImage.cols, ocvImage.rows);
                    cv::imshow("Object Selection", ocvImage);

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

MainFrame::MainFrame(HINSTANCE hInst, HOST_APP_TABLE* host, EDIT_HANDLE* edit_handle)
    : m_hInst(hInst)
    , m_host(host)
{
    // メンバ変数にハンドル渡す
    m_edit_handle = edit_handle;

    // モデルファイルのパスを設定
    char path[MAX_PATH * 2];
    if (GetModuleFileNameA(m_hInst, path, sizeof(path)))
    {
        char* p = strrchr(path, '\\');
        if (p) {
            *(p + 1) = '\0';
            modelDir = std::string(path);
            modelDir += "MotionTracking_model\\";
        }
    }

    // 自身のウィンドウを作成
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpszClassName = constants::WindowName;
    wcex.lpfnWndProc = wnd_proc;
    wcex.hInstance = m_hInst;
    wcex.hbrBackground = CreateSolidBrush((COLORREF)systemColors.background);
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
        nullptr);
    if (!m_hwnd) {
        return;
    }
    // 色情報の取得
    systemColors.background = systemColors.background;
    systemColors.buttonBody = config->get_color_code(config, "ButtonBody");
    systemColors.buttonBodyPress = config->get_color_code(config, "ButtonBodyPress");
    systemColors.buttonBodyHover = config->get_color_code(config, "ButtonBodyHover");
    systemColors.buttonBodyDisable = config->get_color_code(config, "ButtonBodyDisable");
    systemColors.text = config->get_color_code(config, "Text");
    systemColors.textDisable = config->get_color_code(config, "TextDisable");

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
    SendMessage(trackbar_hue, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)hueValue);
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

    // View Result チェックボックスを作成
    HWND check_view_result = CreateWindowEx(
        0,
        WC_BUTTON,
        config->translate(config, L"View Result"),
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        10, y_pos, 300, item_height,
        m_hwnd,
        (HMENU)IDC_Button::ViewResult,
        m_hInst,
        nullptr);
    SendMessage(check_view_result, WM_SETFONT, (WPARAM)hfont, TRUE);

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

// RegisterPlugin は main.cpp に移動しました

// typedef struct{
//     UINT32 frame;
//     int cx;
//     int cy;
//     int width;
//     int height;
//     float scale;
//     bool error;
// }FRMFIX;

// typedef struct{
//     int start; //0-based frame index
//     int end;
//     int vi_start; //vector index
//     int vi_end;
// }FRMGROUP;

// //Find single-frame error to be interpolate
// //RETURN: a std::vector<UINT32> containing relevant index -> out_list
// //RETURN: no. of inter-frame ->func return int
// static int find_inter_frame(std::vector<bool> &err_list, std::vector<UINT32> &out_list)
// {
//     //TODO
//     int loop_last_index = err_list.size() - 3;
//     int interfrm_count = 0;
//     if (err_list.size() < 3)
//     {
//         return FALSE;
//     }
//     out_list.clear();
//     for (int i = 0; i <= loop_last_index; i++)
//     {
//         bool S, M, E;
//         S = err_list[i];
//         M = err_list[i + 1];
//         E = err_list[i + 2];
//         if ((S && E) && !M)
//         {
//             interfrm_count++;
//             out_list.push_back((UINT32)i + 1);
//         }
//     }
//     return interfrm_count;
// }

// //Get center coordinate from Rect2d
// static cv::Point getCenter(cv::Rect2d &box)
// {
//     cv::Point buf(0, 0);
//     buf.x = ((int)box.tl().x + (int)box.br().x) / 2;
//     buf.y = ((int)box.tl().y + (int)box.br().y) / 2;
//     return buf;
// }
// static cv::Point getCenter(cv::Rect &box) //overload for Rect2i
// {
//     cv::Point buf(0, 0);
//     buf.x = (box.tl().x + box.br().x) / 2;
//     buf.y = (box.tl().y + box.br().y) / 2;
//     return buf;
// }

// //Interpolate frames, and transform to AviUtl coordinate
// static void fix_frame(std::vector<cv::Rect2d> &rect_list, std::vector<bool> &err_list, std::vector<UINT32> &inter_list, std::vector<FRMFIX> &out, int frm_w, int frm_h)
// {
//     //TODO
//     //Interpolation phase
//     if (inter_list.size() > 0)
//     {
//         for (size_t f = 0; f < inter_list.size(); f++)
//         {
//             int v_idx = inter_list[f];
//             int now_cx, now_cy, now_tlx, now_tly;
//             int prevW, nowW, nextW;
//             int prevH, nowH, nextH;

//             cv::Point prevC(getCenter(rect_list[v_idx - 1]));
//             prevW = (int)rect_list[v_idx - 1].width;
//             prevH = (int)rect_list[v_idx - 1].height;

//             cv::Point nextC(getCenter(rect_list[v_idx + 1]));
//             nextW = (int)rect_list[v_idx + 1].width;
//             nextH = (int)rect_list[v_idx + 1].height;

//             nowW = (prevW + nextW) / 2;
//             nowH = (prevH + nextH) / 2;

//             now_cx = (prevC.x + nextC.x) / 2;
//             now_cy = (prevC.y + nextC.y) / 2;

//             now_tlx = now_cx - (nowW / 2);
//             now_tly = now_cy - (nowH / 2);
//             //Update box data
//             rect_list[v_idx].x = now_tlx;
//             rect_list[v_idx].y = now_tly;
//             rect_list[v_idx].width = nowW;
//             rect_list[v_idx].height = nowH;
//             //Update error state
//             err_list[v_idx] = true;
//         }

//     }
//     //Transform to AviUtl coordiante
//     int dX = frm_w / -2;
//     int dY = frm_h / -2;
//     for (size_t i = 0; i < rect_list.size(); i++)
//     {
//         FRMFIX buf;
//         cv::Point center(getCenter(rect_list[i]));
//         buf.cx = center.x + dX;
//         buf.cy = center.y + dY;
//         buf.width = (int)rect_list[i].width;
//         buf.height = (int)rect_list[i].height;
//         buf.scale = std::max(rect_list[i].width, rect_list[i].height);
//         buf.frame = i + selA + 1;
//         buf.error = err_list[i];
//         out.push_back(buf); //store to output vector
//     }
// }

// //Group into objects
// static void groupObject(std::vector<FRMFIX> &fixedframes, std::vector<FRMGROUP> &out)
// {
//     //TODO
//     std::vector<int> startpos;
//     std::vector<int> endpos;
//     bool prevstate = false;
//     for (size_t i = 0; i < fixedframes.size(); i++)
//     {
//         bool currentstate = fixedframes[i].error;
//         if (prevstate != currentstate) // a state change marking obj boundary
//         {
//             if (currentstate) //F->T = start
//             {
//                 startpos.push_back(i);
//             }
//             else //T->F = end (prev frame)
//             {
//                 endpos.push_back(i - 1);
//             }

//         }
//         prevstate = currentstate;
//     }
//     //If endpos has 1 less item than startpos, add the last item back
//     if (endpos.size() < startpos.size())
//     {
//         endpos.push_back(fixedframes.size() - 1);
//     }
//     //set output
//     out.clear();
//     if (startpos.size() > 0) //if there is at least 1 object
//     {
//         for (size_t i = 0; i < startpos.size(); i++)
//         {
//             FRMGROUP buf;
//             buf.vi_start = startpos[i];
//             buf.vi_end = endpos[i];
//             buf.start = buf.vi_start + selA;
//             buf.end = buf.vi_end + selA;
//             out.push_back(buf);
//         }
//     }
// }
// //BOOL func_init(FILTER *fp);

// //BOOL func_exit(FILTER *fp);
// BOOL func_update(FILTER *fp, int status)
// {
//     if (status == FILTER_UPDATE_STATUS_TRACK + 1)
//     {
//         if (selectObj)
//             update_object_selection_window(boundingBox.x, boundingBox.y, boundingBox.x + boundingBox.width, boundingBox.y + boundingBox.height, fp);

//         // View Result
//         if (fp->check[2])
//             return TRUE;
//     }
//     return FALSE;
// }
// BOOL func_WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, void *editp, FILTER *fp)
// {
//     if (!fp->exfunc->is_filter_active(fp) || !fp->exfunc->is_editing(editp))
//         return FALSE;

//     switch (message)
//     {
//     case WM_COMMAND:
//     {
//         switch (wparam)
//         {
//         case MID_FILTER_BUTTON: //Object selection
//         {
//             int srcw, srch;
//             if (!fp->exfunc->get_select_frame(editp, &selA, &selB))
//             {
//                 MessageBox(NULL, "Cannot get selected frame number", "AviUtl API Error", MB_OK);
//                 return FALSE;
//             }
//             if (!fp->exfunc->get_pixel_filtered(editp, selA, NULL, &srcw, &srch))
//             {
//                 MessageBox(NULL, "Cannot get frame size", "AviUtl API Error", MB_OK);
//                 return FALSE;
//             }
//             const int step = ((srcw + 1) * 3) & ~3;

//             std::unique_ptr<uint8_t[]> aubgr = std::make_unique<uint8_t[]>(step * srch);
//             if (!fp->exfunc->get_pixel_filtered(editp, selA, aubgr.get(), NULL, NULL))
//             {
//                 MessageBox(NULL, "Cannot get image", "AviUtl API Error", MB_OK);
//                 return FALSE;
//             }

//             cv::Mat cvBuffer(srch, srcw, CV_8UC3, aubgr.get(), step);
//             cv::flip(cvBuffer, cvBuffer, 0);
//             cvBuffer.copyTo(ocvImage);

//             cv::namedWindow("Object Selection", cv::WINDOW_KEEPRATIO);
//             cv::setMouseCallback("Object Selection", onMouse, fp);
//             cv::resizeWindow("Object Selection", srcw, srch);
//             cv::imshow("Object Selection", ocvImage);
//             return TRUE;
//             break;
//         }
//         case MID_FILTER_BUTTON + 1: //Analyze
//         {
//             if (!selectObj)
//             {
//                 MessageBox(NULL, "Nothing selected", "Operation Error", MB_OK);
//                 return FALSE;
//             }
//             int srcw, srch;
//             if (!fp->exfunc->get_frame_size(editp, &srcw, &srch))
//             {
//                 MessageBox(NULL, "Cannot get original frame size", "AviUtl API Error", MB_OK);
//                 return FALSE;
//             }
//             int frmw, frmh;
//             if (!fp->exfunc->get_pixel_filtered(editp, selA, NULL, &frmw, &frmh))
//             {
//                 MessageBox(NULL, "Cannot get frame size", "AviUtl API Error", MB_OK);
//                 return FALSE;
//             }

//             if (srcw != frmw || srch != frmh)
//             {
//                 int res = MessageBoxA(NULL,
//                     "EN\n"
//                     "Resizing has been detected.\n"
//                     "You may not be able to get normal results.\n"
//                     "It is recommended that you disable resizing.\n"
//                     "Do you want to continue the analyze?\n"
//                     "JA\n"
//                     "動画のリサイズが検出されました。\n"
//                     "正常な結果が得られない可能性があります。\n"
//                     "リサイズを無効化することをお勧めします。\n"
//                     "Analyzeを続行しますか？"
//                     , "MotionTracking MK-II Plus", MB_ICONWARNING | MB_YESNO);
//                 if (res == IDNO) {
//                     return FALSE;
//                 }
//             }

//             track_result.clear();
//             track_found.clear();
//             cv::Rect2i box = boundingBox;
//             //Correct for out-of-bound box
//             if (box.br().x > frmw)
//             {
//                 box.width = frmw - box.x;
//             }
//             if (box.br().y > frmh)
//             {
//                 box.height = frmh - box.y;
//             }
//             int64 start_time = cv::getTickCount();
//             // Create Tracker
//             cv::Ptr<cv::Tracker> tracker;
//             try
//             {
//                 switch (fp->track[0] - 1) {
//                 case 0:
//                     tracker = cv::TrackerMIL::create();
//                     break;
//                 case 1:
//                     tracker = cv::TrackerKCF::create();
//                     break;
//                 case 2:
//                     tracker = cv::TrackerCSRT::create();
//                     break;
//                 case 3:
//                 {
//                     auto params = cv::TrackerDaSiamRPN::Params();
//                     params.model = modelDir +  "dasiamrpn_model.onnx";
//                     params.kernel_r1 = modelDir + "dasiamrpn_kernel_r1.onnx";
//                     params.kernel_cls1 = modelDir + "dasiamrpn_kernel_cls1.onnx";
//                     tracker = cv::TrackerDaSiamRPN::create(params);
//                     break;
//                 }
//                 case 4:
//                 {
//                     auto params = cv::TrackerNano::Params();
//                     params.backbone = modelDir + "nanotrack_backbone_sim.onnx";
//                     params.neckhead = modelDir + "nanotrack_head_sim.onnx";
//                     tracker = cv::TrackerNano::create(params);
//                     break;
//                 }
//                 default:
//                 {
//                     //なんか2つモデルがあるが、上のほうが良い？
//                     //https://github.com/opencv/opencv_extra/blob/4.x/testdata/dnn/onnx/models/vitTracker.onnx
//                     //https://github.com/opencv/opencv_zoo/blob/main/models/object_tracking_vittrack/object_tracking_vittrack_2023sep.onnx

//                     auto params = cv::TrackerVit::Params();
//                     params.net = modelDir + "vitTracker.onnx";
//                     tracker = cv::TrackerVit::create(params);
//                     break;
//                 }
//                 }
//             }
//             catch (cv::Exception e)
//             {
//                 MessageBox(NULL, e.what(), "OpenCV3 Error", MB_OK);
//                 return FALSE;
//             }
//             catch (...)
//             {
//                 //nullptr
//                 tracker = cv::Ptr<cv::Tracker>();
//             }
//             if (!tracker)
//             {
//                 MessageBox(NULL, "Error when creating tracker", "OpenCV3 Error", MB_OK);
//                 return FALSE;
//             }

//             bool track_init = false;
//             track_result.clear();
//             track_found.clear();
//             //Loop through selected frames for analysis
//             TCHAR shortmsg[64] = { 0 };
//             int64 prev_stamp, new_stamp;
//             const int step = ((frmw + 1) * 3) & ~3;
//             std::unique_ptr<uint8_t[]> nextau = std::make_unique<uint8_t[]>(step * frmh);
//             for (int f = selA; f <= selB; f++)
//             {
//                 //Set next frame
//                 if (!fp->exfunc->get_pixel_filtered(editp, f, nextau.get(), NULL, NULL))
//                 {
//                     MessageBox(NULL, "Cannot get next image", "AviUtl API Error", MB_OK);
//                     return FALSE;
//                 }

//                 cv::Mat cvNext(frmh, frmw, CV_8UC3, nextau.get(), step);
//                 cv::flip(cvNext, cvNext, 0);

//                 if (!track_init)
//                 {
//                     //TODO
//                     SecureZeroMemory(shortmsg, sizeof(TCHAR[64]));
//                     sprintf_s(shortmsg, "%s tracker initializing...", track_method[fp->track[0] - 1]);
//                     SetWindowText(fp->hwnd, shortmsg);
//                     try
//                     {
//                         tracker->init(cvNext, boundingBox);
//                     }
//                     catch (...)
//                     {
//                         MessageBox(NULL, "Error initializing tracker", "OpenCV3 Error", MB_OK);
//                         return FALSE;
//                     }
//                     track_init = true;
//                     track_found.push_back(true);

//                     prev_stamp = cv::getTickCount();
//                 }
//                 else
//                 {
//                     new_stamp = cv::getTickCount();
//                     double fps = 1.0 / ((new_stamp - prev_stamp) / cv::getTickFrequency());
//                     SecureZeroMemory(shortmsg, sizeof(TCHAR[64]));
//                     sprintf_s(shortmsg, "%s processing frame %d/%d @%.2f fps", track_method[fp->track[0] - 1], f + 1, selB + 1, fps);
//                     SetWindowText(fp->hwnd, shortmsg);
//                     prev_stamp = cv::getTickCount();

//                     try {
//                         if (tracker->update(cvNext, box))
//                         {
//                             track_found.push_back(true);
//                         }
//                         else
//                         {
//                             track_found.push_back(false);
//                         }
//                     }
//                     catch (...)
//                     {
//                         MessageBox(NULL, "Obscure tracker error", "Tracker update exception", MB_OK);
//                         return FALSE;
//                     }
//                 }
//                 if (box.area() <= 0)
//                 {
//                     box.width = 20;
//                     box.height = 20;
//                 }
//                 if (box.x < 0) {
//                     box.width += box.x;
//                     box.x = 0;
//                 }
//                 if (box.y < 0) {
//                     box.height += box.y;
//                     box.y = 0;
//                 }
//                 if (box.x + box.width > frmw)
//                 {
//                     box.width = frmw - box.x;
//                 }
//                 if (box.y + box.height > frmh)
//                 {
//                     box.height = frmh - box.y;
//                 }
//                 track_result.push_back(box);
//             }
//             int64 end_time = cv::getTickCount();
//             double run_time = (end_time - start_time) / cv::getTickFrequency();
//             SecureZeroMemory(shortmsg, sizeof(TCHAR[64]));
//             sprintf_s(shortmsg, "%d frames tracked in %.2f sec", selB - selA, run_time);
//             SetWindowText(fp->hwnd, shortmsg);
//             SecureZeroMemory(shortmsg, sizeof(TCHAR[64]));
//             sprintf_s(shortmsg, "Tracking Completed!\nAverage %.2f fps", (selB - selA) / run_time);
//             MessageBox(NULL, shortmsg, "Tracking Completed!", MB_OK);
//             return TRUE;

//             break;
//         }
//         case MID_FILTER_BUTTON + 3: //Clear data
//         {
//             selA = 0;
//             selB = 0;
//             boundingBox.x = 0;
//             boundingBox.y = 0;
//             boundingBox.width = 0;
//             boundingBox.height = 0;
//             track_result.clear();
//             track_found.clear();
//             startSel = false;
//             selectObj = false;
//             ocvImage.empty();
//             SetWindowText(fp->hwnd, "MotionTracking MK-II Plus");
//             MessageBox(NULL, "Selection states, results and image cache reseted", "INFO", MB_OK);
//             return TRUE;
//             break;
//         }
//         case MID_FILTER_BUTTON + 7: //Save EXO
//         {
//             //TODO
//             if (track_result.size() <= 0)
//             {
//                 MessageBox(fp->hwnd, "No track data to save!", "Operation Error", MB_OK);
//                 return FALSE;
//             }

//             CHAR filename[MAX_PATH] = { 0 };
//             if (!fp->exfunc->dlg_get_save_name(filename, "ExEdit Object File(*.exo)\0*.exo;\0", "tracking.exo"))
//                 return TRUE;

//             // Starts doing the real work
//             std::ostringstream strbuf;
//             TCHAR boilerplate[2048] = { 0 };
//             TCHAR fmtstr[2048] = { 0 };
//             /* Common Project Data*/
//             int width, height;
//             FILE_INFO fi;
//             fp->exfunc->get_pixel_filtered(editp, selA, NULL, &width, &height);
//             fp->exfunc->get_file_info(editp, &fi);
//             /* End common prj data*/
//             /* Format and write to buffer*/
//             LoadString(fp->dll_hinst, IDS_PRJHEADER, boilerplate, 2048);
//             sprintf_s(fmtstr, sizeof(TCHAR[2048]), boilerplate, width, height, fi.video_rate, fi.video_scale, fi.frame_n, fi.audio_rate);
//             strbuf << fmtstr;
//             SecureZeroMemory(boilerplate, sizeof(TCHAR[2048]));
//             SecureZeroMemory(fmtstr, sizeof(TCHAR[2048]));
//             //* END OF COMMON PROJECT HEADER*//
//             //Starts object processing //
//             /* vector storage for bounding-box interpolation and various checks*/
//             std::vector<UINT32> frmInterpolate; //frames to be interpolate;
//             std::vector<FRMFIX> fixedFrm; //interpolated and use AviUtl coordinate
//             std::vector<FRMGROUP> gpinfo; //strats and ends of objects
//             //
//             find_inter_frame(track_found, frmInterpolate);
//             fix_frame(track_result, track_found, frmInterpolate, fixedFrm, width, height);
//             groupObject(fixedFrm, gpinfo);
//             // Should now have all info we need...
//             int object_id = 0; //keep track of how many segments/objects have been added
//             for (size_t o = 0; o < gpinfo.size(); o++) //loop through each object
//             {
//                 int oStart, oEnd;
//                 oStart = gpinfo[o].vi_start;
//                 oEnd = gpinfo[o].vi_end;
//                 bool firstframe = true;
//                 for (int f = oStart; f <= oEnd; f++) //each frames in this object
//                 {
//                     //Section Num
//                     char head_num[32];
//                     sprintf_s(head_num, sizeof(char[32]), "[%d]\n\0", object_id);
//                     strbuf << head_num;
//                     //Common obj param
//                     LoadString(fp->dll_hinst, IDS_OBJPARAM, boilerplate, 2048);
//                     sprintf_s(fmtstr, sizeof(TCHAR[2048]), boilerplate, fixedFrm[f].frame, fixedFrm[f].frame, 1);//st, ed, layer
//                     strbuf << fmtstr;
//                     SecureZeroMemory(boilerplate, sizeof(TCHAR[2048]));
//                     SecureZeroMemory(fmtstr, sizeof(TCHAR[2048]));
//                     //2 more param for Figure obj
//                     if (!fp->check[4])
//                     {
//                         LoadString(fp->dll_hinst, IDS_FIGUREPARAMA, boilerplate, 2048);
//                         strbuf << boilerplate; //verbatim copy
//                         SecureZeroMemory(boilerplate, sizeof(TCHAR[2048]));
//                     }
//                     if (!firstframe)
//                     {
//                         LoadString(fp->dll_hinst, IDS_OBJCHAIN, boilerplate, 2048);
//                         strbuf << boilerplate; //verbatim copy
//                         SecureZeroMemory(boilerplate, sizeof(TCHAR[2048]));
//                     }
//                     //Section [*.0] Graphics/Sub-filter
//                     sprintf_s(head_num, sizeof(char[32]), "[%d.0]\n\0", object_id);
//                     strbuf << head_num;
//                     //
//                     if (fp->check[4]) //Sub-filter
//                     {
//                         LoadString(fp->dll_hinst, IDS_SFPARAM_JP, boilerplate, 2048);//TODO: Set JP text
//                         int Xi, Xf, Yi, Yf, size_st, size_ed;
//                         double  rAsp_st, rAsp_ed;
//                         Xi = fixedFrm[f].cx;
//                         Yi = fixedFrm[f].cy;
//                         if (fixedFrm[f].width > fixedFrm[f].height) //-ve rAsp, const width
//                         {
//                             size_st = fixedFrm[f].width;
//                             rAsp_st = -100.0 * (1.0 - ((double)fixedFrm[f].height / (double)fixedFrm[f].width));
//                         }
//                         else if (fixedFrm[f].width < fixedFrm[f].height) // +ve rAsp, const height
//                         {
//                             size_st = fixedFrm[f].height;
//                             rAsp_st = 100.0 * (1.0 - ((double)fixedFrm[f].width / (double)fixedFrm[f].height));
//                         }
//                         else // rAsp=0, square
//                         {
//                             size_st = fixedFrm[f].width;
//                             rAsp_st = 0.0;
//                         }
//                         if ((size_t)f >= (fixedFrm.size() - 1))//special handling for last frame
//                         {
//                             Xf = Xi;
//                             Yf = Yi;
//                             size_ed = size_st;
//                             rAsp_ed = rAsp_st;
//                         }
//                         else
//                         {
//                             Xf = fixedFrm[f + 1].cx;
//                             Yf = fixedFrm[f + 1].cy;
//                             if (fixedFrm[f + 1].width > fixedFrm[f + 1].height) //-ve rAsp, const width
//                             {
//                                 size_ed = fixedFrm[f + 1].width;
//                                 rAsp_ed = -100.0 * (1.0 - ((double)fixedFrm[f + 1].height / (double)fixedFrm[f + 1].width));
//                             }
//                             else if (fixedFrm[f + 1].width < fixedFrm[f + 1].height) // +ve rAsp, const height
//                             {
//                                 size_ed = fixedFrm[f + 1].height;
//                                 rAsp_ed = 100.0 * (1.0 - ((double)fixedFrm[f + 1].width / (double)fixedFrm[f + 1].height));
//                             }
//                             else // rAsp=0, square
//                             {
//                                 size_ed = fixedFrm[f + 1].width;
//                                 rAsp_ed = 0.0;
//                             }
//                         }
//                         if (fp->check[6]) // Ignore Aspect Ratio
//                         {
//                             rAsp_st = 0.0;
//                             rAsp_ed = 0.0;
//                         }

//                         if (fp->check[5]) //Invert Position
//                         {
//                             Xi = -Xi;
//                             Xf = -Xf;
//                             Yi = -Yi;
//                             Yf = -Yf;
//                         }

//                         sprintf_s(fmtstr, sizeof(TCHAR[2048]), boilerplate, (double)Xi, (double)Xf, (double)Yi, (double)Yf, size_st, size_ed, rAsp_st, rAsp_ed);//X-st, X-ed, Y-st, Y-ed, size_start, size_end, rAsp-st, rAsp-ed
//                         strbuf << fmtstr;
//                         SecureZeroMemory(boilerplate, sizeof(TCHAR[2048]));
//                         SecureZeroMemory(fmtstr, sizeof(TCHAR[2048]));
//                     }
//                     else //Graphics
//                     {
//                         LoadString(fp->dll_hinst, IDS_FIGUREPARAMB_JP, boilerplate, 2048);//TODO: Set JP text
//                         strbuf << boilerplate;
//                         SecureZeroMemory(boilerplate, sizeof(TCHAR[2048]));
//                     }
//                     //Section [*.1] Resize FX or Mono FX
//                     sprintf_s(head_num, sizeof(char[32]), "[%d.1]\n\0", object_id);
//                     strbuf << head_num;
//                     //
//                     if (fp->check[4])// Mono FX for Sub-filter
//                     {
//                         LoadString(fp->dll_hinst, IDS_FXMONO_JP, boilerplate, 2048);//TODO: Set JP text
//                         strbuf << boilerplate;
//                         SecureZeroMemory(boilerplate, sizeof(TCHAR[2048]));
//                     }
//                     else //Resize FX for Graphics
//                     {
//                         int Wi, Wf, Hi, Hf;
//                         float Si, Sf;

//                         Wi = fixedFrm[f].width;
//                         Hi = fixedFrm[f].height;
//                         Si = fixedFrm[f].scale;

//                         if ((size_t)f >= (fixedFrm.size() - 1)) //Last frame
//                         {
//                             Wf = Wi;
//                             Hf = Hi;
//                             Sf = Si;
//                         }
//                         else // Normal
//                         {
//                             Wf = fixedFrm[f + 1].width;
//                             Hf = fixedFrm[f + 1].height;
//                             Sf = fixedFrm[f + 1].scale;
//                         }

//                         LoadString(fp->dll_hinst, IDS_FXRESIZE_JP, boilerplate, 2048);//TODO: Set JP text
//                         if (fp->check[6]) // Ignore Aspect Ratio
//                         {
//                             sprintf_s(fmtstr, sizeof(TCHAR[2048]), boilerplate, (double)Si, Sf, 100.0, 100.0, 100.0, 100.0, !fp->check[6]);
//                         }
//                         else
//                         {
//                             sprintf_s(fmtstr, sizeof(TCHAR[2048]), boilerplate, 100.0, 100.0, (double)Wi, (double)Wf, (double)Hi, (double)Hf, !fp->check[6]);
//                         }
//                         strbuf << fmtstr;
//                         SecureZeroMemory(boilerplate, sizeof(TCHAR[2048]));
//                         SecureZeroMemory(fmtstr, sizeof(TCHAR[2048]));
//                     }
//                     //Section [*.2] Std Drawing for Graphics
//                     //Coordinate animation part for Graphics
//                     if (!fp->check[4]) //only for graphics
//                     {
//                         sprintf_s(head_num, sizeof(char[32]), "[%d.2]\n\0", object_id);
//                         strbuf << head_num;
//                         //
//                         int Xi, Xf, Yi, Yf;
//                         Xi = fixedFrm[f].cx;
//                         Yi = fixedFrm[f].cy;
//                         if ((size_t)f >= (fixedFrm.size() - 1))//last frame
//                         {
//                             Xf = Xi;
//                             Yf = Yi;
//                         }
//                         else //Normal
//                         {
//                             Xf = fixedFrm[f + 1].cx;
//                             Yf = fixedFrm[f + 1].cy;
//                         }
//                         LoadString(fp->dll_hinst, IDS_STDDRAW_JP, boilerplate, 2048);//TODO: Set JP text
//                         if (fp->check[5]) // invert position
//                         {
//                             Xi = -Xi;
//                             Xf = -Xf;
//                             Yi = -Yi;
//                             Yf = -Yf;
//                         }
//                         sprintf_s(fmtstr, sizeof(TCHAR[2048]), boilerplate, (double)Xi, (double)Xf, (double)Yi, (double)Yf);
//                         strbuf << fmtstr;
//                         SecureZeroMemory(boilerplate, sizeof(TCHAR[2048]));
//                         SecureZeroMemory(fmtstr, sizeof(TCHAR[2048]));
//                     }
//                     //
//                     object_id++;
//                     firstframe = false;
//                 }
//             }
//             //Write to file
//             std::ofstream fhandle(filename, std::ofstream::out | std::ofstream::trunc);
//             if (fhandle.is_open())
//             {
//                 fhandle << strbuf.str();
//                 fhandle.flush();
//                 fhandle.close();
//                 MessageBox(fp->hwnd, "DONE", "Finished", MB_OK);
//             }
//             else
//             {
//                 MessageBox(fp->hwnd, "Cannot write to file!", "File I/O ERROR", MB_OK);
//                 return FALSE;
//             }

//             return TRUE;
//             break;
//         }
//         default:
//         {
//             //TODO
//             return FALSE;
//         }
//         }
//     }
//     }

//     return FALSE;
// }

// BOOL func_proc(FILTER *fp, FILTER_PROC_INFO *fpip)
// {
//     bool isFilterActive, isEditing, isFrameInRng, hasResult, redraw, isSaving;
//     redraw = false;
//     isFilterActive = (fp->exfunc->is_filter_active(fp) == TRUE);
//     isEditing = (fp->exfunc->is_editing(fpip->editp) == TRUE);
//     hasResult = track_result.size() > fpip->frame - selA;
//     isFrameInRng = (fpip->frame >= selA) && (fpip->frame <= selB);
//     isSaving = (fp->exfunc->is_saving(fpip->editp) == TRUE);

//     if (isFilterActive && isEditing && fp->check[2] && hasResult && isFrameInRng && !isSaving)
//     {
//         int frmsize = fpip->w * fpip->h;
//         std::unique_ptr<PIXEL[]> aubuf = std::make_unique<PIXEL[]>(frmsize);
//         std::unique_ptr<PIXEL_YC[]> yc_src = std::make_unique<PIXEL_YC[]>(frmsize);
//         byte* p1 = (byte*)fpip->ycp_edit;
//         byte* p2 = (byte*)yc_src.get();
//         size_t yc_linesize = sizeof(PIXEL_YC) * fpip->w;
//         size_t yc_maxlinesize = sizeof(PIXEL_YC) * fpip->max_w;
//         for (int line = 0; line < fpip->h; line++)
//         {
//             memcpy_s(p2, yc_linesize, p1, yc_linesize);
//             p1 += yc_maxlinesize;
//             p2 += yc_linesize;
//         }
//         fp->exfunc->yc2rgb(aubuf.get(), yc_src.get(), frmsize);
//         cv::Mat disp(fpip->h, fpip->w, CV_8UC3, aubuf.get());

//         if (track_found[fpip->frame - selA])
//         {
//             cv::rectangle(disp, track_result[fpip->frame - selA], hue_to_scalar(fp->track[1]), 2, 1);
//             cv::putText(disp, "OK", cv::Point(0, 50), cv::FONT_HERSHEY_PLAIN, 2.0, cv::Scalar(0, 255, 0), 2);
//         }
//         else
//         {
//             cv::putText(disp, "ERROR", cv::Point(0, 50), cv::FONT_HERSHEY_PLAIN, 2.0, cv::Scalar(0, 0, 255), 2);
//         }

//         PIXEL* aubuf2 = (PIXEL*)disp.data;
//         std::unique_ptr<PIXEL_YC[]> ycbuf = std::make_unique<PIXEL_YC[]>(frmsize);
//         fp->exfunc->rgb2yc(ycbuf.get(), aubuf2, frmsize);
//         size_t linesize = sizeof(PIXEL_YC) * fpip->w;
//         size_t canvas_linesize = sizeof(PIXEL_YC) * fpip->max_w;
//         byte* ptemp = (byte*)fpip->ycp_temp;
//         byte* psrc = (byte*)ycbuf.get();
//         for (int line = 0; line < fpip->h; line++)
//         {
//             memcpy_s(ptemp, canvas_linesize, psrc, linesize);
//             ptemp += canvas_linesize;
//             psrc += linesize;
//         }
//         //swap ptr
//         PIXEL_YC* ycswap = fpip->ycp_temp;
//         fpip->ycp_temp = fpip->ycp_edit;
//         fpip->ycp_edit = ycswap;
//     }

//     if (isFilterActive && isEditing && fp->check[8] && hasResult && isFrameInRng)
//     {
//         if (track_found[fpip->frame - selA])
//         {
//             size_t frmsize = fpip->w* fpip->h;
//             std::unique_ptr<PIXEL[]> aubuf = std::make_unique<PIXEL[]>(frmsize);
//             std::unique_ptr<PIXEL_YC[]> yc_src = std::make_unique<PIXEL_YC[]>(frmsize);
//             byte* p1 = (byte*)fpip->ycp_edit;
//             byte* p2 = (byte*)yc_src.get();
//             size_t yc_linesize = sizeof(PIXEL_YC)*fpip->w;
//             size_t yc_maxlinesize = sizeof(PIXEL_YC)*fpip->max_w;
//             for (int line = 0; line < fpip->h; line++)
//             {
//                 memcpy_s(p2, yc_linesize, p1, yc_linesize);
//                 p1 += yc_maxlinesize;
//                 p2 += yc_linesize;
//             }
//             fp->exfunc->yc2rgb(aubuf.get(), yc_src.get(), frmsize);

//             cv::Mat ocvbuf(fpip->h, fpip->w, CV_8UC3, aubuf.get());
//             //cv::flip(ocvbuf, ocvbuf, 0);
//             cv::Mat blurArea(ocvbuf, track_result[fpip->frame - selA]);
//             cv::blur(blurArea, blurArea, cv::Size(21, 21));
//             PIXEL* aubuf2 = (PIXEL*)ocvbuf.data;
//             std::unique_ptr<PIXEL_YC[]> ycbuf = std::make_unique<PIXEL_YC[]>(frmsize);
//             fp->exfunc->rgb2yc(ycbuf.get(), aubuf2, frmsize);
//             size_t linesize = sizeof(PIXEL_YC)*fpip->w;
//             size_t canvas_linesize = sizeof(PIXEL_YC)*fpip->max_w;
//             byte* ptemp = (byte*)fpip->ycp_temp;
//             byte* psrc = (byte*)ycbuf.get();
//             for (int line = 0; line < fpip->h; line++)
//             {
//                 memcpy_s(ptemp, canvas_linesize, psrc, linesize);
//                 ptemp += canvas_linesize;
//                 psrc += linesize;
//             }
//             //swap ptr
//             PIXEL_YC* ycswap = fpip->ycp_temp;
//             fpip->ycp_temp = fpip->ycp_edit;
//             fpip->ycp_edit = ycswap;
//             //Cleanup

//             redraw = true;
//         }
//     }
//     if (fp->check[9] && isFilterActive && isEditing)
//     {
//         //AviUtl -> OCV
//         size_t frmsize = fpip->w* fpip->h;
//         std::unique_ptr<PIXEL[]> aubuf = std::make_unique<PIXEL[]>(frmsize);
//         std::unique_ptr<PIXEL_YC[]> yc_src = std::make_unique<PIXEL_YC[]>(frmsize);
//         byte* p1 = (byte*)fpip->ycp_edit;
//         byte* p2 = (byte*)yc_src.get();
//         size_t yc_linesize = sizeof(PIXEL_YC)*fpip->w;
//         size_t yc_maxlinesize = sizeof(PIXEL_YC)*fpip->max_w;
//         for (int line = 0; line < fpip->h; line++)
//         {
//             memcpy_s(p2, yc_linesize, p1, yc_linesize);
//             p1 += yc_maxlinesize;
//             p2 += yc_linesize;
//         }
//         fp->exfunc->yc2rgb(aubuf.get(), yc_src.get(), frmsize);
//         cv::Mat ocvbuf(fpip->h, fpip->w, CV_8UC3, aubuf.get());
//         //
//         //Convert to greyscale
//         cv::Mat ocvGrey;
//         cv::cvtColor(ocvbuf, ocvGrey, cv::COLOR_BGR2GRAY, 1);
//         //Haar part
//         //Check if XML exists in root or plugin folder
//         cv::CascadeClassifier frontface = cv::CascadeClassifier::CascadeClassifier();
//         cv::CascadeClassifier profileface = cv::CascadeClassifier::CascadeClassifier();
//         frontface.load(modelDir + "haarcascade_frontalface_default.xml");
//         profileface.load(modelDir + "haarcascade_profileface.xml");
//         //if XML loaded, proceed
//         if (!frontface.empty() && !profileface.empty())
//         {
//             std::vector<cv::Rect> frontface_list;
//             std::vector<cv::Rect> profileface_list;
//             frontface.detectMultiScale(ocvGrey, frontface_list, 1.1);
//             profileface.detectMultiScale(ocvGrey, profileface_list, 1.1);
//             //Loop through each result and apply blur
//             if (frontface_list.size() > 0)
//             {
//                 for (size_t i = 0; i < frontface_list.size(); i++)
//                 {
//                     cv::Mat toblur(ocvbuf, frontface_list[i]);
//                     int kwidth = frontface_list[i].width / 4;
//                     int kheight = frontface_list[i].height / 4;
//                     cv::blur(toblur, toblur, cv::Size(kwidth, kheight));
//                 }
//             }
//             if (profileface_list.size()>0)
//             {
//                 for (size_t i = 0; i < profileface_list.size(); i++)
//                 {
//                     int kwidth = profileface_list[i].width / 4;
//                     int kheight = profileface_list[i].height / 4;
//                     cv::Mat toblur(ocvbuf, profileface_list[i]);
//                     cv::blur(toblur, toblur, cv::Size(kwidth, kheight));
//                 }
//             }
//             //Send back to aviutl if detected any face
//             if (frontface_list.size() || profileface_list.size())
//             {
//                 PIXEL* aubuf2 = (PIXEL*)ocvbuf.data;
//                 std::unique_ptr<PIXEL_YC[]> ycbuf = std::make_unique<PIXEL_YC[]>(frmsize);
//                 fp->exfunc->rgb2yc(ycbuf.get(), aubuf2, frmsize);
//                 size_t linesize = sizeof(PIXEL_YC)*fpip->w;
//                 size_t canvas_linesize = sizeof(PIXEL_YC)*fpip->max_w;
//                 byte* ptemp = (byte*)fpip->ycp_temp;
//                 byte* psrc = (byte*)ycbuf.get();
//                 for (int line = 0; line < fpip->h; line++)
//                 {
//                     memcpy_s(ptemp, canvas_linesize, psrc, linesize);
//                     ptemp += canvas_linesize;
//                     psrc += linesize;
//                 }
//                 //swap ptr
//                 PIXEL_YC* ycswap = fpip->ycp_temp;
//                 fpip->ycp_temp = fpip->ycp_edit;
//                 fpip->ycp_edit = ycswap;
//             }

//             //Clean up

//             redraw = true;
//         }
//     }
//     return redraw;
// }
