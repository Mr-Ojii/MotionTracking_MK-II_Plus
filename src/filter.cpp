#include <windows.h>
#include <cstdint>
#include <format>
#include <mutex>
#include <vector>
#include "aviutl2_sdk/filter2.h"
#include "aviutl2_sdk/logger2.h"
#include "opencv2/video.hpp"
#include "utils.hpp"

extern LOG_HANDLE* logger;
extern bool getImageFromAUX;
extern cv::Mat ocvImage;
extern int hueValue;
extern bool finishedFilter;
extern std::mutex mtx;
extern std::condition_variable cov;
extern std::vector<cv::Rect2d> track_result;
extern std::vector<bool> track_found;

// フィルターに関するほう
static void* filter_items[] = { nullptr };

bool func_proc_video(FILTER_PROC_VIDEO* video) {
    cv::Mat image(video->object->height, video->object->width, CV_8UC4);
    video->get_image_data((PIXEL_RGBA*)image.data);
    cv::cvtColor(image, image, cv::COLOR_RGBA2BGRA);
    if (getImageFromAUX) {
        ocvImage = image.clone();
    } else {
        logger->info(logger, std::format(L"track_result.size()={}, frame_total={}", track_result.size(), video->object->frame_total).c_str());
        bool hasResult = track_result.size() >= video->object->frame_total;
        if (hasResult) {
            if (track_found[video->object->frame])
            {
                cv::Scalar s = utils::hue_to_scalar(hueValue) / 2;
                s[3] = 255;
                cv::rectangle(image, track_result[video->object->frame], s, 2, 1);
                cv::putText(image, "OK", cv::Point(0, 50), cv::FONT_HERSHEY_PLAIN, 2.0, cv::Scalar(0, 255, 0, 255), 2);
            }
            else
            {
                cv::putText(image, "ERROR", cv::Point(0, 50), cv::FONT_HERSHEY_PLAIN, 2.0, cv::Scalar(0, 0, 255, 255), 2);
            }
            cv::cvtColor(image, image, cv::COLOR_BGRA2RGBA);
            video->set_image_data((PIXEL_RGBA*)image.data, video->object->width, video->object->height);
        }
    }

    {
        std::lock_guard<std::mutex> lock(mtx);
        finishedFilter = true;
    }

    cov.notify_one();
    logger->info(logger, std::format(L"func_proc_video called: layer={}, frame={}, width={}, height={}", video->object->layer, video->object->frame, video->object->width, video->object->height).c_str());
    return true;
}

FILTER_PLUGIN_TABLE filter = {
    FILTER_PLUGIN_TABLE::FLAG_VIDEO | FILTER_PLUGIN_TABLE::FLAG_FILTER,
    L"MotionTracker_M Filter",
    nullptr,
    L"MotionTracker_M Filter Effect",
    filter_items,
    func_proc_video,
    nullptr
};


