#include "tracker.hpp"
#include "aviutl2_sdk/config2.h"
#include "constants.hpp"
#include "opencv2/highgui.hpp"

extern LOG_HANDLE* logger;
extern CONFIG_HANDLE* config;

void Tracker::SetModelDir(const std::string& dir) {
    m_modelDir = dir;
}

void Tracker::SetBox(cv::Rect2d box) {
    m_boundingBox = box;
}

void Tracker::Clear() {
    m_trackResult.clear();
    m_trackFound.clear();
    m_boundingBox = {};
}

cv::Mat Tracker::RenderFrame(EDIT_HANDLE* edit, int frame) {
    // TODO: rendering_scene_video + wait_rendering_task で実装
    return cv::Mat{};
}

cv::Ptr<cv::Tracker> Tracker::CreateTracker(int method) {
    // TODO: 旧 OnAnalyze の switch から移植
    return nullptr;
}

bool Tracker::SelectObject(EDIT_HANDLE* edit_handle) {

    // 前回の状態を保存
    cv::Rect2d prevBoundingBox = m_boundingBox;
    bool prevSelectObj = m_selectObj;

    RangeResult range;

    // フレーム選択範囲を取得
    // call_read_section_param を使うとなぜかクラッシュする
    edit_handle->call_edit_section_param(&range, [](void* param, EDIT_SECTION* edit) {
        auto* r = static_cast<RangeResult*>(param);
        r->start = edit->info->select_range_start;
        r->end = edit->info->select_range_end;

        // 範囲が選択されていないとき、0~最大フレームを取得
        if (r->start == -1 && r->end == -1) {
            r->start = 0;
            r->end = edit->info->frame_max;
        }
    });

    logger->info(logger, std::format(L"range: start={}, end={}", range.start, range.end).c_str());

    RenderParam rp{ &m_image };

    // レンダリング結果取得
    bool renderIsOk = edit_handle->rendering_scene_video(
    range.start, // 取得するフレーム番号
    &rp, // m_image のアドレス
    [](void* param, int frame, const void* buffer, int width, int height, int pitch) {
        // param を RenderParam に変換
        auto* rp = static_cast<RenderParam*>(param);

        // 画像の行列に、結果書き込み
        // CV_8UC4: 8bit (0~255) の RGBA
        // const_cast: const void* の const を外す
        // pitch: 次の行位置を教えてくれるやつ
        cv::Mat rgba(height, width, CV_8UC4, const_cast<void*>(buffer), static_cast<size_t>(pitch));
        // 入力元，出力先，RGBA -> BGR に変換
        cv::cvtColor(rgba, *rp->image, cv::COLOR_RGBA2BGR);
    });


    if (!renderIsOk) {
        MessageBox(nullptr, config->translate(config, L"Cannot get image"), constants::APIerr, MB_OK | MB_ICONERROR);
        return false;
    }

    edit_handle->wait_rendering_task();
    logger->info(logger, L"SelectObject: rendering complete");

    if (m_image.empty()) {
        MessageBox(nullptr, config->translate(config, L"Failed to get image from AviUtl. Please make sure AviUtl is in a state where it can provide images."), constants::WindowName, MB_OK | MB_ICONERROR);
        return false;
    }

    logger->info(logger, L"SelectObject: window opened");
    cv::namedWindow("Object Selection", cv::WINDOW_KEEPRATIO);
    cv::setMouseCallback("Object Selection", OnMouse, this);
    cv::resizeWindow("Object Selection", m_image.cols, m_image.rows);
    cv::imshow("Object Selection", m_image);

    // 前回の選択範囲があれば表示
    if (m_selectObj) {
        UpdateObjectSelectionWindow(
            m_boundingBox.x,
            m_boundingBox.y,
            m_boundingBox.x + m_boundingBox.width,
            m_boundingBox.y + m_boundingBox.height
        );
    }

    SetFocus(nullptr);

    // 解析場所を選択するまで待機 (モーダルみたいなの)
    while (true) {
        // 10ms 周期で観測
        int key = cv::waitKey(10);

        // ESC でキャンセル
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { // OS の ESC を監視
            HWND hwnd = FindWindowA(nullptr, "Object Selection");
            // Object Selection ウィンドウが選択状態の時のみ発動させる
            if (hwnd && GetForegroundWindow() == hwnd) {
                // 前回の状態を復元
                m_boundingBox = prevBoundingBox;
                m_selectObj   = prevSelectObj;
                PostMessage(hwnd, WM_CLOSE, 0, 0);
                return false;
            }
        }

        // F3 で確定 (x ボタンまで遠いので、キーボード操作できるように)
        if (GetAsyncKeyState(VK_F3) & 0x8000) {
            HWND hwnd = FindWindowA(nullptr, "Object Selection");
            if (hwnd && GetForegroundWindow() == hwnd) {
                PostMessage(hwnd, WM_CLOSE, 0, 0);
                return m_selectObj;
            }
        }

        // × で結果格納
        if (!static_cast<bool>(cv::getWindowProperty("Object Selection", cv::WND_PROP_VISIBLE))) {
            return m_selectObj;  // 選択していれば true、していなければ false
        }
    }
}

bool Tracker::Run(EDIT_HANDLE* edit, OBJECT_LAYER_FRAME olf, int method) {
    // TODO
    return false;
}

void Tracker::OnMouse(int event, int x, int y, int, void* userdata) {
    // static において、Tracker を使えるようにするため
    auto* tracker = static_cast<Tracker*>(userdata);

    switch (event)
    {
    case cv::EVENT_LBUTTONDOWN:
        //set origin of the bounding box
        tracker->m_startSel = true;
        tracker->m_selectObj = false;
        tracker->m_boundingBox.x = x;
        tracker->m_boundingBox.y = y;
        logger->info(logger, std::format(L"OnMouse: LBUTTONDOWN x={}, y={}", x, y).c_str());
        break;
    case cv::EVENT_LBUTTONUP:
        //set with and height of the bounding box
        tracker->m_boundingBox.width = std::abs(x - tracker->m_boundingBox.x);
        tracker->m_boundingBox.height = std::abs(y - tracker->m_boundingBox.y);
        tracker->m_boundingBox.x = std::clamp(static_cast<double>(x), 0.0, tracker->m_boundingBox.x);
        tracker->m_boundingBox.y = std::clamp(static_cast<double>(y), 0.0, tracker->m_boundingBox.y);
        tracker->m_selectObj = true;
        tracker->m_startSel = false;
        logger->info(logger, std::format(L"OnMouse: LBUTTONUP x={}, y={}, box=({}, {}, {}, {})",
            x, y,
            tracker->m_boundingBox.x,
            tracker->m_boundingBox.y,
            tracker->m_boundingBox.width,
            tracker->m_boundingBox.height).c_str());
        break;
    case cv::EVENT_MOUSEMOVE:

        if (tracker->m_startSel && !tracker->m_selectObj)
        {
            tracker->UpdateObjectSelectionWindow(tracker->m_boundingBox.x, tracker->m_boundingBox.y, x, y);
        }
        break;
    }
}

void Tracker::UpdateObjectSelectionWindow(int x1, int y1, int x2, int y2) {
    //update only if visible
    if(!static_cast<bool>(cv::getWindowProperty("Object Selection", cv::WND_PROP_VISIBLE)))
        return;

    x1 = std::clamp(x1, 0, m_image.cols);
    y1 = std::clamp(y1, 0, m_image.rows);
    x2 = std::clamp(x2, 0, m_image.cols);
    y2 = std::clamp(y2, 0, m_image.rows);

    //draw the bounding box
    auto displayFrame = m_image.clone();
    cv::Rect2i rect(std::min(x1, x2), std::min(y1, y2), std::abs(x1 - x2), std::abs(y1 - y2));
    cv::Mat renderFrame;
    if (rect.area() > 0) {
        renderFrame = displayFrame(rect);
        renderFrame /= 2;
        renderFrame += utils::hue_to_scalar(m_hueValue) / 2;
    }
    cv::imshow("Object Selection", displayFrame);
}
