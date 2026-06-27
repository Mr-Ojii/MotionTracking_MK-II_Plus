#pragma once
#include <windows.h>
#include "aviutl2_sdk/config2.h"

// AviUtl2 の 0x00RRGGBB → Win32 COLORREF (0x00BBGGRR) に変換
inline COLORREF AviUtl2ColorToColorRef(int colorCode) {
    BYTE r = (colorCode >> 16) & 0xFF;
    BYTE g = (colorCode >>  8) & 0xFF;
    BYTE b = (colorCode      ) & 0xFF;
    return RGB(r, g, b);
}

// config->get_color_code で取得した値を格納しておく (0x00RRGGBB 形式)
struct SystemColors {
    int background        = 0;
    int buttonBody        = 0;
    int buttonBodyPress   = 0;
    int buttonBodyHover   = 0;
    int buttonBodyDisable = 0;
    int text              = 0;
    int textDisable       = 0;

    void Load(CONFIG_HANDLE* config) {
        if (!config) return;
        background        = config->get_color_code(config, "Background");
        buttonBody        = config->get_color_code(config, "ButtonBody");
        buttonBodyPress   = config->get_color_code(config, "ButtonBodyPress");
        buttonBodyHover   = config->get_color_code(config, "ButtonBodyHover");
        buttonBodyDisable = config->get_color_code(config, "ButtonBody");
        text              = config->get_color_code(config, "Text");
        textDisable       = config->get_color_code(config, "TextDisable");
    }
};

namespace ownerdraw {
    LRESULT OnCtlColor(WPARAM wparam, const SystemColors& colors);
    LRESULT OnDrawItem(LPARAM lparam, const SystemColors& colors);
}
