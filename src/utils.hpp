#pragma once
#include <windows.h>
#include "opencv2/core/utility.hpp"

namespace utils {
    cv::Scalar hue_to_scalar(int hue);
    // モデルのファイルパス処理
    std::string get_model_dir(HINSTANCE hInst);
}
