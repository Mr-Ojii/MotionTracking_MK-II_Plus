#pragma once

#include <string>
#include <vector>
#include <windows.h>
#include "opencv2/tracking.hpp"
#include "opencv2/video/tracking.hpp"
#include "aviutl2_sdk/plugin2.h"
#include "aviutl2_sdk/logger2.h"
#include "opencv2/core/utility.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/tracking.hpp"
#include "opencv2/objdetect.hpp"
#include "opencv2/video.hpp"
#include "opencv2/video/tracking.hpp"
#include "utils.hpp"
#include "constants.hpp"

// フレーム範囲選択のフレーム位置
struct RangeResult {
    int start = -1;
    int end   = -1;
};

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
    std::vector<cv::Rect2d> m_trackResult;

    std::vector<bool>       m_trackFound;
    // 状態
    int        m_hueValue  = 180;
    cv::Mat    m_image;
    cv::Rect2d m_boundingBox;
    bool       m_selectObj = false;
    bool       m_startSel  = false;
};
