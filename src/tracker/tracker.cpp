#include "tracker.hpp"

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

bool Tracker::SelectObject(EDIT_HANDLE* edit) {
    // TODO
    return false;
}

bool Tracker::Run(EDIT_HANDLE* edit, OBJECT_LAYER_FRAME olf, int method) {
    // TODO
    return false;
}

void Tracker::OnMouse(int event, int x, int y, int, void* userdata) {
    // TODO
}
