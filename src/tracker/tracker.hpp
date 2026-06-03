#pragma once

#include <string>
#include <vector>
#include <windows.h>
#include "opencv2/tracking.hpp"
#include "opencv2/video/tracking.hpp"
#include "aviutl2_sdk/plugin2.h"

class Tracker {
public:
    Tracker() = default;

    void SetModelDir(const std::string& dir);
    void SetBox(cv::Rect2d box);
    bool Run(EDIT_HANDLE* edit, OBJECT_LAYER_FRAME olf, int method);
    void Clear();
    bool SelectObject(EDIT_HANDLE* edit);

    const std::vector<cv::Rect2d>& Results()   const { return m_trackResult; }
    const std::vector<bool>&       Found()     const { return m_trackFound; }
    bool HasResult() const { return !m_trackResult.empty(); }

private:
    static void OnMouse(int event, int x, int y, int flags, void* userdata);
    cv::Ptr<cv::Tracker> CreateTracker(int method);
    cv::Mat RenderFrame(EDIT_HANDLE* edit, int frame);

    std::string             m_modelDir;
    cv::Rect2d              m_boundingBox;
    std::vector<cv::Rect2d> m_trackResult;
    std::vector<bool>       m_trackFound;
};
