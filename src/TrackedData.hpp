#pragma once

class TrackedData {
public:
    // Obj Selection
    cv::Rect2d boundingBox;
    bool selectObj = false;
    bool startSel = false;
    // Analyze
    std::vector<bool> track_found;
    std::vector<cv::Rect2d> track_result;
};
