#include "tracker.hpp"
#include "aviutl2_sdk/config2.h"

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


    if (m_image.empty()) {
        MessageBox(nullptr, config->translate(config, L"Failed to get image from AviUtl. Please make sure AviUtl is in a state where it can provide images."), constants::WindowName, MB_OK | MB_ICONERROR);
        return 0;
    }
    cv::namedWindow("Object Selection", cv::WINDOW_KEEPRATIO);
    cv::setMouseCallback("Object Selection", OnMouse, nullptr); // TODO: nullptrの代わりにuserdata
    cv::resizeWindow("Object Selection", m_image.cols, m_image.rows);
    cv::imshow("Object Selection", m_image);

    SetFocus(nullptr);
    return false;
}

bool Tracker::Run(EDIT_HANDLE* edit, OBJECT_LAYER_FRAME olf, int method) {
    // TODO
    return false;
}

void Tracker::OnMouse(int event, int x, int y, int, void* userdata) {
    // TODO
}
