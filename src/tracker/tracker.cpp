#include "tracker.hpp"
#include "aviutl2_sdk/config2.h"
#include "aviutl2_sdk/plugin2.h"
#include "constants.hpp"
#include "opencv2/highgui.hpp"
#include "ui/mainframe.hpp"
#include <windows.h>

extern LOG_HANDLE* logger;
extern CONFIG_HANDLE* config;

void Tracker::SetModelDir(const std::string& dir) {
    m_modelDir = dir;
}

void Tracker::SetBox(cv::Rect2d box) {
    m_boundingBox = box;
}

cv::Mat Tracker::RenderFrame(EDIT_HANDLE* edit, int frame) {
    // TODO: rendering_scene_video + wait_rendering_task で実装
    return cv::Mat{};
}

bool Tracker::SelectObject(EDIT_HANDLE* edit_handle, int hueValue) {

    // 色相をメンバ変数に
    m_hueValue = hueValue;
    // 前回の状態を保存
    cv::Rect2d prevBoundingBox = m_boundingBox;
    bool prevSelectObj = m_selectObj;


    // フレーム選択範囲を取得
    // call_read_section_param を使うとなぜかクラッシュする
    edit_handle->call_edit_section_param(&m_range, [](void* param, EDIT_SECTION* edit) {
        auto* r = static_cast<RangeResult*>(param);
        r->start = edit->info->select_range_start;
        r->end = edit->info->select_range_end;

        // 範囲が選択されていないとき、0~最大フレームを取得
        if (r->start == -1 && r->end == -1) {
            r->start = 0;
            r->end = edit->info->frame_max;
        }
    });

    logger->info(logger, std::format(L"range: start={}, end={}", m_range.start, m_range.end).c_str());

    RenderParam rp{ &m_image };

    // レンダリング結果取得
    bool renderIsOk = edit_handle->rendering_scene_video(
    m_range.start, // 取得するフレーム番号
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

namespace {
    // ShowResultWindow のトラックバーコールバック用
    struct ViewResultParam {
        Tracker*     tracker;
        EDIT_HANDLE* edit;
    };
}

void Tracker::ShowResultWindow(EDIT_HANDLE* edit) {
    if (m_track_result.empty()) {
        MessageBoxA(nullptr, "No tracking result found", "Operational Error", MB_OK);
        return;
    }

    cv::namedWindow("Tracking Result", cv::WINDOW_AUTOSIZE);

    int pos_val = 0;
    int max_pos = (int)m_track_result.size() - 1;

    ViewResultParam param{ this, edit };
    cv::createTrackbar("Frame", "Tracking Result", &pos_val, max_pos, OnResultTrackbarChange, &param);
    OnResultTrackbarChange(pos_val, &param);

    // × で閉じるまで待機 (SelectObjectと同じパターン)
    while (true) {
        cv::waitKey(10);
        if (!static_cast<bool>(cv::getWindowProperty("Tracking Result", cv::WND_PROP_VISIBLE))) {
            break;
        }
    }
}

void Tracker::OnResultTrackbarChange(int pos, void* userdata) {
    auto* param = static_cast<ViewResultParam*>(userdata);
    Tracker*     tracker = param->tracker;
    EDIT_HANDLE* edit    = param->edit;

    if (pos < 0 || (size_t)pos >= tracker->m_track_result.size()) return;

    int frame = tracker->m_range.start + pos;

    cv::Mat image;
    edit->rendering_scene_video(frame, &image,
        [](void* p, int, const void* buffer, int w, int h, int pitch) {
            auto* img = static_cast<cv::Mat*>(p);
            cv::Mat rgba(h, w, CV_8UC4, const_cast<void*>(buffer), (size_t)pitch);
            cv::cvtColor(rgba, *img, cv::COLOR_RGBA2BGR);
        });
    edit->wait_rendering_task();

    if (image.empty()) return;

    if (tracker->m_track_found[pos]) {
        cv::rectangle(image, tracker->m_track_result[pos], utils::hue_to_scalar(tracker->m_hueValue), 2, 1);
        cv::putText(image, "OK", cv::Point(0, 50), cv::FONT_HERSHEY_PLAIN, 2.0, cv::Scalar(0, 255, 0), 2);
    } else {
        cv::putText(image, "ERROR", cv::Point(0, 50), cv::FONT_HERSHEY_PLAIN, 2.0, cv::Scalar(0, 0, 255), 2);
    }

    cv::imshow("Tracking Result", image);
}

bool Tracker::Analyze(EDIT_HANDLE* edit, TrackingMethod method) {
    if (!m_selectObj)
    {
        MessageBoxA(NULL, "Nothing selected", "Operation Error", MB_OK);
        return false;
    }

    m_track_result.clear();
    m_track_found.clear();
    //Correct for out-of-bound box
    if (m_boundingBox.br().x > m_image.cols)
    {
        m_boundingBox.width = m_image.cols - m_boundingBox.x;
    }
    if (m_boundingBox.br().y > m_image.rows)
    {
        m_boundingBox.height = m_image.rows - m_boundingBox.y;
    }
    // Create Tracker
    cv::Ptr<cv::Tracker> tracker = CreateTracker(method);

    m_progress_current = 0;
    m_progress_total   = m_range.end - m_range.start + 1;
    m_analyzing        = true;

    // 応答なしバグ対応
    // mutable で、書き換え可能に
    std::thread([this, edit, tracker]() mutable{
        cv::Mat image;
        cv::Rect2i box = m_boundingBox;
        bool track_init = false;
        int64 start_time = cv::getTickCount();
        int64 prev_stamp = start_time;

        // 最初の1枚目は、SelectObject のレンダリング結果を代入
        image = m_image;

        for (int frame = m_range.start; frame <= m_range.end; frame++) {
            // キャンセルフラグがたったら、結果クリアしてbreak
            if (m_cancel) {
                Clear(ClearMode::Partial);
                break;
            }
            if (frame + 1 <= m_range.end) {
                edit->rendering_scene_video(frame + 1, &image,
                    [](void* param, int, const void* buffer, int w, int h, int pitch) {
                        auto* img = static_cast<cv::Mat*>(param);
                        cv::Mat rgba(h, w, CV_8UC4, const_cast<void*>(buffer), (size_t)pitch);
                        // BGRAではトラッキングができないものがあるため、RGBに変換（CSRTなど）
                        cv::cvtColor(rgba, *img, cv::COLOR_RGBA2BGR);
                });
            }

            edit->wait_rendering_task();

            if (!track_init) {
                // 追跡対象登録
                tracker->init(image, box);
                // 初回は必ず成功
                track_init = true;
                m_track_found.push_back(true);
            } else {
                if (tracker->update(image, box)) {
                    m_track_found.push_back(true);
                } else {
                    m_track_found.push_back(false);
                }
            }

            m_track_result.push_back(box);
            m_progress_current = frame - m_range.start + 1;
            int64 new_stamp = cv::getTickCount();
            if (new_stamp != prev_stamp)
                m_progress_fps = 1.0 / ((new_stamp - prev_stamp) / cv::getTickFrequency());
            prev_stamp = new_stamp;
            logger->info(logger, std::format(L"Analyzing: {}/{}", m_progress_current.load(), m_progress_total.load()).c_str());
        }

    int64 end_time = cv::getTickCount();
    double run_time = (end_time - start_time) / cv::getTickFrequency();
    m_analyzing = false;

    char msg[64];
    sprintf_s(msg, "Tracking Completed!\nAverage %.2f fps",
              (m_range.end - m_range.start) / run_time);

    if (m_cancel) {
        m_cancel = false;
        // xする前に、ダイアログウィンドウをクリックしたり移動させたりすると後ろの方に行くので、MB_TOPMOSTしてます
        MessageBoxA(nullptr, "Tracking Canceled", "INFO", MB_OK | MB_TOPMOST);
    } else {
        m_cancel = false;
        MessageBoxA(nullptr, msg, "Tracking Completed!", MB_OK | MB_TOPMOST);
    }

    }).detach();
    return true;
}

// 処理最適化版。要検証
bool Tracker::Analyze2(EDIT_HANDLE* edit, TrackingMethod method) {
    if (!m_selectObj) {
        MessageBoxA(NULL, "Nothing selected", "Operation Error", MB_OK);
        return false;
    }

    m_track_result.clear();
    m_track_found.clear();

    if (m_boundingBox.br().x > m_image.cols)
        m_boundingBox.width = m_image.cols - m_boundingBox.x;
    if (m_boundingBox.br().y > m_image.rows)
        m_boundingBox.height = m_image.rows - m_boundingBox.y;

    cv::Ptr<cv::Tracker> tracker = CreateTracker(method);

    std::thread([this, edit, tracker]() mutable {
        const int total = m_range.end - m_range.start + 1;

        // 全フレーム分バッファ＋完了フラグ
        std::vector<cv::Mat> frame_cache(total);
        std::vector<std::atomic<bool>> ready(total);
        for (auto& r : ready) r.store(false);

        // コールバック用パラメータ（newなし、配列で管理）
        struct RenderParam {
            cv::Mat* img;
            std::atomic<bool>* ready;
        };
        std::vector<RenderParam> params(total);
        for (int i = 0; i < total; i++) {
            params[i] = { &frame_cache[i], &ready[i] };
        }

        auto callback = [](void* param, int, const void* buffer,
                           int w, int h, int pitch) {
            auto* p = static_cast<RenderParam*>(param);
            cv::Mat rgba(h, w, CV_8UC4,
                         const_cast<void*>(buffer), (size_t)pitch);
            cv::cvtColor(rgba, *p->img, cv::COLOR_RGBA2BGR);
            p->ready->store(true, std::memory_order_release);
        };

        auto kickRender = [&](int idx) {
            if (idx >= total) return;
            int frame = m_range.start + idx;
            edit->rendering_scene_video(frame, &params[idx], callback);
        };

        // 最初のフレームは SelectObject の結果をそのまま使用
        frame_cache[0] = m_image;
        ready[0].store(true, std::memory_order_release);

        // 先行レンダリングキック（PREFETCH枚）
        const int PREFETCH = 16;
        int render_kicked = 1;
        for (int i = 0; i < PREFETCH; i++) {
            kickRender(render_kicked++);
        }

        cv::Rect2i box = m_boundingBox;
        bool track_init = false;
        int64 start_time = cv::getTickCount();
        double total_render_wait = 0.0;
        double total_track = 0.0;

        for (int idx = 0; idx < total; idx++) {
            // レンダリング完了待ち（先行済みならほぼ即抜ける）
            auto t0 = cv::getTickCount();
            while (!ready[idx].load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            auto t1 = cv::getTickCount();
            total_render_wait += (t1 - t0) / cv::getTickFrequency() * 1000.0;

            // トラッキング（この間に次フレームのレンダリングが並走）
            if (!track_init) {
                tracker->init(frame_cache[idx], box);
                track_init = true;
                m_track_found.push_back(true);
            } else {
                if (tracker->update(frame_cache[idx], box)) {
                    m_track_found.push_back(true);
                } else {
                    m_track_found.push_back(false);
                }
            }
            m_track_result.push_back(box);

            auto t2 = cv::getTickCount();
            total_track += (t2 - t1) / cv::getTickFrequency() * 1000.0;

            logger->info(logger, std::format(L"Analyzing: {}/{}",
                idx + 1, total).c_str());

            // 次フレームをキック（トラッキング中にレンダリング並走）
            kickRender(render_kicked++);
        }

        int64 end_time = cv::getTickCount();
        double run_time = (end_time - start_time) / cv::getTickFrequency();

        char msg[256];
        sprintf_s(msg,
            "Tracking Completed!\n"
            "Average %.2f fps\n"
            "Render wait: %.1f ms/frame\n"
            "Track:       %.1f ms/frame",
            (total - 1) / run_time,
            total_render_wait / total,
            total_track / total);
        MessageBoxA(nullptr, msg, "Tracking Completed!", MB_OK);
        logger->info(logger, std::format(L"OpenCV threads: {}",
        cv::getNumThreads()).c_str());

    }).detach();

    return true;
}

void Tracker::Clear(ClearMode mode) {
    m_track_result.clear();
    m_track_found.clear();
    if (mode == ClearMode::Full) { // ClearResult が押されたとき
        m_boundingBox = {};
        m_selectObj   = false;
        m_image.release();
    }
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

cv::Ptr<cv::Tracker> Tracker::CreateTracker(TrackingMethod method) {
    cv::Ptr<cv::Tracker> tracker;
    try
    {
        switch (method) {
            case TrackingMethod::MIL:
            tracker = cv::TrackerMIL::create();
            break;
            case TrackingMethod::KCF:
            // KCFはOpenCVのextra modulesに移動されたため、環境によっては利用できない可能性があります
            tracker = cv::TrackerKCF::create();
            break;
            case TrackingMethod::CSRT:
            tracker = cv::TrackerCSRT::create();
            break;
            case TrackingMethod::DaSiamRPN:
        {
            auto params = cv::TrackerDaSiamRPN::Params();
            params.model = m_modelDir +  "dasiamrpn_model.onnx";
            params.kernel_r1 = m_modelDir + "dasiamrpn_kernel_r1.onnx";
            params.kernel_cls1 = m_modelDir + "dasiamrpn_kernel_cls1.onnx";
            tracker = cv::TrackerDaSiamRPN::create(params);
            break;
        }
            case TrackingMethod::Nano:
        {
            auto params = cv::TrackerNano::Params();
            params.backbone = m_modelDir + "nanotrack_backbone_sim.onnx";
            params.neckhead = m_modelDir + "nanotrack_head_sim.onnx";
            tracker = cv::TrackerNano::create(params);
            break;
        }
            case TrackingMethod::Vit:
        {
            //なんか2つモデルがあるが、上のほうが良い？
            //https://github.com/opencv/opencv_extra/blob/4.x/testdata/dnn/onnx/models/vitTracker.onnx
            //https://github.com/opencv/opencv_zoo/blob/main/models/object_tracking_vittrack/object_tracking_vittrack_2023sep.onnx

            auto params = cv::TrackerVit::Params();
            params.net = m_modelDir + "vitTracker.onnx";
            tracker = cv::TrackerVit::create(params);
            break;
        }
        default:
            // 選択されていない、または不正なインデックス
            MessageBox(nullptr, config->translate(config, L"Please select a tracking method."), constants::WindowName, MB_OK | MB_ICONERROR);
            return {};
        }
    }
    catch (cv::Exception e)
    {
        MessageBoxA(NULL, e.what(), "OpenCV3 Error", MB_OK);
        return {};
    }
    catch (...)
    {
        //nullptr
        tracker = cv::Ptr<cv::Tracker>();
    }
    if (!tracker)
    {
        MessageBoxA(NULL, "Error when creating tracker", "OpenCV3 Error", MB_OK);
        return {};
    }
    return tracker;
}
