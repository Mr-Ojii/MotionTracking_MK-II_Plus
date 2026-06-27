#pragma once

#include <string>
#include <vector>
#include <atomic>
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

// 画像格納場所
struct RenderParam {
    cv::Mat* image;
};

enum class TrackingMethod {
    MIL       = 0,
    KCF       = 1,
    CSRT      = 2,
    DaSiamRPN = 3,
    Nano      = 4,
    Vit       = 5,
};

// Clear Result の処理分岐用
// デフォルトはFull設定
enum class ClearMode {
    Full,    // Clear Result ボタンが押されたとき、バウンディングボックス含めリセット
    Partial, // ダイアログの x からリセットされたとき、バウンディングボックスは保持
};


class Tracker {
public:
    Tracker() = default;

    void SetModelDir(const std::string& dir);
    void SetBox(cv::Rect2d box);
    bool Run(EDIT_HANDLE* edit, OBJECT_LAYER_FRAME olf, TrackingMethod method);
    bool Analyze(EDIT_HANDLE* edit, TrackingMethod method);
    bool Analyze2(EDIT_HANDLE* edit, TrackingMethod method);
    void Clear(ClearMode mode = ClearMode::Full);
    bool SelectObject(EDIT_HANDLE* edit, int hueValue);

    const std::vector<cv::Rect2d>& Results()    const { return m_track_result; }
    const std::vector<bool>&       Found()      const { return m_track_found; }
    bool HasResult()  const { return !m_track_result.empty(); }
    int  RangeStart() const { return m_range.start; }

    // 解析情報をプログレスバーに渡すため
    std::atomic<int>    m_progress_current = 0;
    std::atomic<int>    m_progress_total   = 0;
    std::atomic<double> m_progress_fps     = 0.0;
    std::atomic<bool>   m_analyzing        = false;
    std::atomic<bool> m_cancel = false; // ダイアログをxしたらtrueにして、解析処理中止
private:
    // 引数固定のため、static
    static void OnMouse(int event, int x, int y, int flags, void* userdata);
    RangeResult m_range;
    // モデル選択
    cv::Ptr<cv::Tracker> CreateTracker(TrackingMethod method);
    cv::Mat RenderFrame(EDIT_HANDLE* edit, int frame);
    // 選択範囲描画関数
    void UpdateObjectSelectionWindow(int x1, int y1, int x2, int y2);

    std::string             m_modelDir;
    std::vector<cv::Rect2d> m_track_result;

    // 追跡結果 true or false
    std::vector<bool>       m_track_found;
    // 状態
    int        m_hueValue  = 180;
    cv::Mat    m_image;
    cv::Rect2d m_boundingBox;
    bool       m_selectObj = false;
    bool       m_startSel  = false;
};
