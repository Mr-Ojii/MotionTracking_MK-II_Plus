#pragma once
#include <windows.h>

// config->get_color_code で取得した値を格納しておく
struct SystemColors {
    int background        = 0;
    int buttonBody        = 0;
    int buttonBodyPress   = 0;
    int buttonBodyHover   = 0;
    int buttonBodyDisable = 0;
    int text              = 0;
    int textDisable       = 0;
};

namespace ownerdraw {
    LRESULT OnCtlColor(WPARAM wparam, const SystemColors& colors);
    LRESULT OnDrawItem(LPARAM lparam, const SystemColors& colors);
}
