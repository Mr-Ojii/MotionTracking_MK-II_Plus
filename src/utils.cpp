#include "utils.hpp"

namespace utils {

cv::Scalar hue_to_scalar(int hue) {
    hue = hue % 360;

    if (hue < 60)
        return cv::Scalar(255, hue * 255 / 60, 0);
    else if (hue < 120)
        return cv::Scalar((120-hue) * 255 / 60, 255, 0);
    else if (hue < 180)
        return cv::Scalar(0, 255, (hue - 120) * 255 / 60);
    else if (hue < 240)
        return cv::Scalar(0, (240 - hue) * 255 / 60, 255);
    else if (hue < 300)
        return cv::Scalar((hue - 240) * 255 / 60, 0, 255);
    else
        return cv::Scalar(255, 0, (360 - hue) * 255 / 60);
}

}
