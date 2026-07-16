#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include "opencv2/core.hpp"
#include "aviutl2_sdk/plugin2.h"

struct FRMFIX {
    int   frame;
    int   cx;
    int   cy;
    int   width;
    int   height;
    float scale;
    bool  found;
};

struct FRMGROUP {
    int start;
    int end;
    int vi_start;
    int vi_end;
};

class InsertObject {
public:
    static std::string make_alias(const std::vector<FRMFIX>& fixedFrm, int vi_start, int vi_end);
    static bool Insert(
        const std::vector<cv::Rect2d>& results,
        const std::vector<bool>& found,
        int rangeStart,
        EDIT_HANDLE* edit
    );
    // タイムラインに挿入せず、生成したaliasテキストをファイルに書き出す(確認用)
    static bool ExportToFile(
        const std::vector<cv::Rect2d>& results,
        const std::vector<bool>& found,
        int rangeStart,
        EDIT_HANDLE* edit,
        const std::wstring& filepath
    );
private:
    InsertObject() = delete;
    static cv::Point getCenter(const cv::Rect2d& box);
    static int  find_inter_frame(std::vector<bool> &err_list, std::vector<UINT32> &out_list);
    static void fix_frame(std::vector<cv::Rect2d> &rect_list, std::vector<bool> &err_list, std::vector<UINT32> &inter_list, std::vector<FRMFIX> &out, int frm_w, int frm_h, int rangeStart);
    static void groupObject(std::vector<FRMFIX> &fixedframes, std::vector<FRMGROUP> &out, int rangeStart);
    static std::string make_alias(int cx, int cy, int size, double rAsp);

};
