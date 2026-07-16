#include "ownerdraw.hpp"
#include "ui/mainframe.hpp"

namespace ownerdraw {

LRESULT OnCtlColor(WPARAM wparam, const SystemColors& colors) {
    HDC hdc = (HDC)wparam;
    SetTextColor(hdc, AviUtl2ColorToColorRef(colors.text));
    SetBkColor(hdc, AviUtl2ColorToColorRef(colors.background));
    return (LRESULT)CreateSolidBrush(AviUtl2ColorToColorRef(colors.background));
}

LRESULT OnDrawItem(LPARAM lparam, const SystemColors& colors) {
    // 背景色や文字色を指定のでdrawするためのOWNERDRAW
    DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lparam;

    if (dis->CtlType == ODT_BUTTON) {
        int id = dis->CtlID;
        bool isCheckbox = (id == (int)IDC_Button::AsSubFilter ||
                           id == (int)IDC_Button::InvertPosition || id == (int)IDC_Button::IgnoreAspectRatio);

        if (isCheckbox) {
            HBRUSH hbrBackground = CreateSolidBrush(AviUtl2ColorToColorRef(colors.background));
            FillRect(dis->hDC, &dis->rcItem, hbrBackground);

            COLORREF textColor = (dis->itemState & ODS_DISABLED) ? AviUtl2ColorToColorRef(colors.textDisable) : AviUtl2ColorToColorRef(colors.text);
            int state = (int)GetWindowLongPtr(dis->hwndItem, GWLP_USERDATA);

            RECT rcCheck = dis->rcItem;
            int checkSize = 15;
            rcCheck.left += 2;
            rcCheck.right = rcCheck.left + checkSize;
            rcCheck.top = rcCheck.top + (rcCheck.bottom - rcCheck.top - checkSize) / 2;
            rcCheck.bottom = rcCheck.top + checkSize;

            UINT uState = DFCS_BUTTONCHECK;
            if (state) uState |= DFCS_CHECKED;
            if (dis->itemState & ODS_DISABLED) uState |= DFCS_INACTIVE;
            if (dis->itemState & ODS_SELECTED) uState |= DFCS_PUSHED;

            DrawFrameControl(dis->hDC, &rcCheck, DFC_BUTTON, uState);

            RECT rcText = dis->rcItem;
            rcText.left = rcCheck.right + 5;

            SetTextColor(dis->hDC, textColor);
            SetBkMode(dis->hDC, TRANSPARENT);

            WCHAR text[256];
            GetWindowText(dis->hwndItem, text, sizeof(text) / sizeof(text[0]));
            DrawText(dis->hDC, text, -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            DeleteObject(hbrBackground);
        } else {
            HBRUSH hbrBackground, hbrBorder;
            COLORREF textColor;

            if (dis->itemState & ODS_DISABLED) {
                hbrBackground = CreateSolidBrush(AviUtl2ColorToColorRef(colors.buttonBodyDisable));
                textColor = AviUtl2ColorToColorRef(colors.textDisable);
            } else if (dis->itemState & ODS_SELECTED) {
                hbrBackground = CreateSolidBrush(AviUtl2ColorToColorRef(colors.buttonBodyPress));
                textColor = AviUtl2ColorToColorRef(colors.text);
            } else {
                hbrBackground = CreateSolidBrush(AviUtl2ColorToColorRef(colors.buttonBody));
                textColor = AviUtl2ColorToColorRef(colors.text);
            }

            FillRect(dis->hDC, &dis->rcItem, hbrBackground);

            if ((dis->itemState & ODS_FOCUS) == 0) {
                hbrBorder = CreateSolidBrush(RGB(128, 128, 128));
            } else {
                hbrBorder = CreateSolidBrush(RGB(64, 64, 64));
            }
            FrameRect(dis->hDC, &dis->rcItem, hbrBorder);

            SetTextColor(dis->hDC, textColor);
            SetBkMode(dis->hDC, TRANSPARENT);

            WCHAR text[256];
            GetWindowText(dis->hwndItem, text, sizeof(text) / sizeof(text[0]));

            DrawText(dis->hDC, text, -1, &dis->rcItem,
                     DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            DeleteObject(hbrBackground);
            DeleteObject(hbrBorder);
        }
        return TRUE;
    }
    else if (dis->CtlType == ODT_COMBOBOX) {
        HBRUSH hbrBackground;
        COLORREF textColor = AviUtl2ColorToColorRef(colors.text);
        COLORREF bgColor = AviUtl2ColorToColorRef(colors.background);

        if (dis->itemState & ODS_DISABLED) {
            textColor = AviUtl2ColorToColorRef(colors.textDisable);
        } else if (dis->itemState & ODS_SELECTED) {
            bgColor = AviUtl2ColorToColorRef(colors.buttonBodyPress);
        }

        hbrBackground = CreateSolidBrush(bgColor);
        FillRect(dis->hDC, &dis->rcItem, hbrBackground);

        SetTextColor(dis->hDC, textColor);
        SetBkColor(dis->hDC, bgColor);
        SetBkMode(dis->hDC, OPAQUE);

        if (dis->itemID != (UINT)-1) {
            WCHAR text[256];
            SendMessage(dis->hwndItem, CB_GETLBTEXT, dis->itemID, (LPARAM)text);
            DrawText(dis->hDC, text, -1, &dis->rcItem, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }

        DeleteObject(hbrBackground);
        return TRUE;
    }

    return FALSE;
}

} // namespace ownerdraw
