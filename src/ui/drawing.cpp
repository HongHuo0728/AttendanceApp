#include "ui/drawing.h"
#include "settings/toggles.h"
#include "ui/common.h"
#include "animation/scheduler.h"
#include "ui/theme.h"
#include "animation/particles.h"
#include "ui/state.h"
#include "core/constants.h"

namespace attendance {

void PaintHeaderControl(HWND hwnd, HDC hdc);
LRESULT CALLBACK HeaderPaintProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR);
LRESULT CALLBACK StatsPaintProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR);

void DrawSoftDivider(HDC hdc, int left, int right, int y){
    if (right <= left) return;
    int mid = left + (right - left) / 2;

    TRIVERTEX leftVerts[2] = {
        {(LONG)left, (LONG)y, 0, 0, 0, 0},
        {(LONG)mid, (LONG)(y + 1), 0x3f00, 0x3f00, 0x3f00, 0}
    };
    GRADIENT_RECT leftRect{0, 1};
    GradientFill(hdc, leftVerts, 2, &leftRect, 1, GRADIENT_FILL_RECT_H);

    TRIVERTEX rightVerts[2] = {
        {(LONG)mid, (LONG)y, 0x3f00, 0x3f00, 0x3f00, 0},
        {(LONG)right, (LONG)(y + 1), 0, 0, 0, 0}
    };
    GRADIENT_RECT rightRect{0, 1};
    GradientFill(hdc, rightVerts, 2, &rightRect, 1, GRADIENT_FILL_RECT_H);
}

bool DrawParticleToggleItem(const DRAWITEMSTRUCT* draw){
    if (!draw || draw->CtlType != ODT_BUTTON) return false;
    bool* value = FindSettingToggleValue(draw->CtlID);
    if (!value) return false;

    wchar_t text[160]{};
    GetWindowTextW(draw->hwndItem, text, 160);

    RECT rc = draw->rcItem;
    FillRect(draw->hDC, &rc, Ui().bgBrush ? Ui().bgBrush : (HBRUSH)GetStockObject(BLACK_BRUSH));

    bool checked = *value;
    double hover = GetAnimationValue(draw->hwndItem, AnimChannel::Hover, 0.0);
    bool focused = (draw->itemState & ODS_FOCUS) != 0;
    COLORREF border = BlendColor(Ui().COLOR_BORDER, Ui().COLOR_ACCENT, focused ? 1.0 : hover);

    int box = 18;
    int boxTop = (int)rc.top + std::max(0, ((int)(rc.bottom - rc.top) - box) / 2);
    RECT boxRc{
        rc.left + 2,
        boxTop,
        rc.left + 2 + box,
        boxTop + box
    };

    HGDIOBJ oldBrush = SelectObject(draw->hDC, GetStockObject(DC_BRUSH));
    HGDIOBJ oldPen = SelectObject(draw->hDC, GetStockObject(DC_PEN));
    SetDCBrushColor(draw->hDC, Ui().COLOR_CHECK_FILL);
    SetDCPenColor(draw->hDC, border);
    Rectangle(draw->hDC, boxRc.left, boxRc.top, boxRc.right, boxRc.bottom);
    SelectObject(draw->hDC, oldPen);
    SelectObject(draw->hDC, oldBrush);

    if (checked) {
        HPEN checkPen = CreatePen(PS_SOLID, 2, Ui().COLOR_TEXT);
        oldPen = SelectObject(draw->hDC, checkPen);
        MoveToEx(draw->hDC, boxRc.left + 4, boxRc.top + 9, nullptr);
        LineTo(draw->hDC, boxRc.left + 8, boxRc.top + 13);
        LineTo(draw->hDC, boxRc.left + 15, boxRc.top + 5);
        SelectObject(draw->hDC, oldPen);
        DeleteObject(checkPen);
    }

    RECT textRc = rc;
    textRc.left = boxRc.right + 10;
    textRc.right -= 4;
    SetBkMode(draw->hDC, TRANSPARENT);
    SetTextColor(draw->hDC, Ui().COLOR_TEXT);
    HGDIOBJ oldFont = SelectObject(draw->hDC, Ui().font);
    DrawTextW(draw->hDC, text, -1, &textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    SelectObject(draw->hDC, oldFont);
    return true;
}

bool DrawButtonItem(const DRAWITEMSTRUCT* draw){
    if (!draw || draw->CtlType != ODT_BUTTON) return false;

    wchar_t text[128]{};
    GetWindowTextW(draw->hwndItem, text, 128);
    bool pressedState = (draw->itemState & ODS_SELECTED) != 0;
    bool focused = (draw->itemState & ODS_FOCUS) != 0;
    double hover = GetAnimationValue(draw->hwndItem, AnimChannel::Hover, focused ? 1.0 : 0.0);
    double press = std::max(pressedState ? 1.0 : 0.0, GetAnimationValue(draw->hwndItem, AnimChannel::Press, 0.0));

    COLORREF fill = BlendColor(Ui().COLOR_INPUT, Ui().COLOR_HOVER, hover);
    COLORREF border = BlendColor(Ui().COLOR_BORDER, Ui().COLOR_ACCENT, focused ? 1.0 : hover);
    COLORREF buttonText = BlendColor(Ui().COLOR_MUTED, Ui().COLOR_TEXT, std::clamp(0.45 + hover * 0.55, 0.0, 1.0));

    bool isDanger = draw->CtlID == IDC_DELETE || draw->CtlID == IDC_SETTINGS_RESET;
    bool isGreen = draw->CtlID == IDC_PRESENT || draw->CtlID == IDC_SAVE || draw->CtlID == IDC_ALL_PRESENT || draw->CtlID == IDC_SETTINGS_APPLY;
    bool isBlue = draw->CtlID == IDC_IMPORT || draw->CtlID == IDC_NEW || draw->CtlID == IDC_EXPORT_CSV;

    if (isDanger) {
        COLORREF dangerBase = BlendColor(Ui().COLOR_INPUT, Ui().COLOR_DANGER, IsRenderedPaletteDark() ? 0.12 : 0.08);
        fill = BlendColor(dangerBase, Ui().COLOR_DANGER, hover * 0.18);
        border = BlendColor(Ui().COLOR_BORDER, Ui().COLOR_DANGER, focused ? 1.0 : 0.45 + hover * 0.55);
        buttonText = Ui().COLOR_TEXT;
    } else if (isGreen) {
        COLORREF success = IsRenderedPaletteDark() ? RGB(146, 166, 156) : RGB(74, 112, 92);
        fill = BlendColor(Ui().COLOR_INPUT, success, 0.08 + hover * 0.12);
        border = BlendColor(Ui().COLOR_BORDER, success, focused ? 1.0 : 0.35 + hover * 0.65);
    } else if (isBlue) {
        fill = BlendColor(Ui().COLOR_INPUT, Ui().COLOR_ACCENT, 0.04 + hover * 0.10);
        border = BlendColor(Ui().COLOR_BORDER, Ui().COLOR_ACCENT, focused ? 1.0 : hover);
    }

    fill = BlendColor(fill, Ui().COLOR_SELECTED, press * 0.45);

    FillRect(draw->hDC, &draw->rcItem,
        Ui().bgBrush ? Ui().bgBrush : (HBRUSH)GetStockObject(BLACK_BRUSH));

    RECT buttonRc = draw->rcItem;
    int inset = (int)std::lround(press * 2.0);
    InflateRect(&buttonRc, -inset, -inset);
    OffsetRect(&buttonRc, 0, (int)std::lround(press * 1.0));
    HGDIOBJ oldPen = SelectObject(draw->hDC, GetStockObject(DC_PEN));
    HGDIOBJ oldBrush = SelectObject(draw->hDC, GetStockObject(DC_BRUSH));
    SetDCPenColor(draw->hDC, border);
    SetDCBrushColor(draw->hDC, fill);
    RoundRect(draw->hDC, buttonRc.left, buttonRc.top, buttonRc.right, buttonRc.bottom, 6, 6);
    SelectObject(draw->hDC, oldBrush);
    SelectObject(draw->hDC, oldPen);

    int r = std::max(buttonRc.left, buttonRc.right - 1);
    int b = std::max(buttonRc.top, buttonRc.bottom - 1);
    SetPixel(draw->hDC, buttonRc.left, buttonRc.top, border);
    SetPixel(draw->hDC, r, buttonRc.top, border);
    SetPixel(draw->hDC, buttonRc.left, b, border);
    SetPixel(draw->hDC, r, b, border);

    DrawButtonEffects(draw, buttonRc, fill);

    SetBkMode(draw->hDC, TRANSPARENT);
    SetTextColor(draw->hDC, buttonText);
    HGDIOBJ oldFont = Ui().font ? SelectObject(draw->hDC, Ui().font) : nullptr;
    DrawTextW(draw->hDC, text, -1, &buttonRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    if (oldFont) SelectObject(draw->hDC, oldFont);
    return true;
}

bool DrawComboItem(const DRAWITEMSTRUCT* draw){
    if (!draw || draw->CtlType != ODT_COMBOBOX) return false;

    RECT rc = draw->rcItem;
    bool comboEdit = (draw->itemState & ODS_COMBOBOXEDIT) != 0;
    if (comboEdit) {
        PaintComboClosed(draw->hwndItem, draw->hDC);
        return true;
    }
    bool selected = (draw->itemState & ODS_SELECTED) != 0;
    double open = GetAnimationValue(draw->hwndItem, AnimChannel::ComboOpen, 1.0);
    double itemProgress = ElasticOut(std::clamp((open - (double)std::min<UINT>(draw->itemID, 10) * 0.075) / 0.55, 0.0, 1.0));
    COLORREF rowFill = selected ? Ui().COLOR_SELECTED : Ui().COLOR_INPUT;
    COLORREF fill = BlendColor(Ui().COLOR_BG, rowFill, itemProgress);
    COLORREF textColor = BlendColor(Ui().COLOR_MUTED, Ui().COLOR_TEXT, itemProgress);

    HBRUSH brush = CreateSolidBrush(fill);
    FillRect(draw->hDC, &rc, brush);
    DeleteObject(brush);

    std::wstring text;
    if (draw->itemID != (UINT)-1) {
        int len = (int)SendMessageW(draw->hwndItem, CB_GETLBTEXTLEN, draw->itemID, 0);
        if (len >= 0) {
            text.resize(len + 1);
            SendMessageW(draw->hwndItem, CB_GETLBTEXT, draw->itemID, (LPARAM)text.data());
            text.resize(len);
        }
    } else {
        text = GetText(draw->hwndItem);
    }

    rc.left += 10 + (int)std::lround((1.0 - itemProgress) * 12.0);
    rc.right -= 8;
    SetBkMode(draw->hDC, TRANSPARENT);
    SetTextColor(draw->hDC, textColor);
    HGDIOBJ oldFont = Ui().font ? SelectObject(draw->hDC, Ui().font) : nullptr;
    DrawTextW(draw->hDC, text.c_str(), -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    if (oldFont) SelectObject(draw->hDC, oldFont);
    return true;
}

void PaintComboClosed(HWND hwnd, HDC hdc){
    RECT rc{};
    GetClientRect(hwnd, &rc);
    bool enabled = IsWindowEnabled(hwnd) != FALSE;
    bool focused = GetFocus() == hwnd;
    double hover = GetAnimationValue(hwnd, AnimChannel::Hover, focused ? 1.0 : 0.0);
    double open = GetAnimationValue(hwnd, AnimChannel::ComboOpen, 0.0);

    double active = std::clamp(std::max(hover, open), 0.0, 1.0);
    COLORREF fill = enabled ? BlendColor(Ui().COLOR_INPUT, Ui().COLOR_HOVER, active) : Ui().COLOR_PANEL;
    COLORREF border = BlendColor(Ui().COLOR_BORDER, Ui().COLOR_ACCENT, focused ? 1.0 : active);
    COLORREF arrowFill = BlendColor(Ui().COLOR_PANEL, Ui().COLOR_SELECTED, active);
    COLORREF textColor = enabled ? Ui().COLOR_TEXT : Ui().COLOR_MUTED;

    HBRUSH bg = CreateSolidBrush(fill);
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);

    RECT arrowRc = rc;
    arrowRc.left = std::max(arrowRc.left, arrowRc.right - 34);
    HBRUSH arrowBg = CreateSolidBrush(arrowFill);
    FillRect(hdc, &arrowRc, arrowBg);
    DeleteObject(arrowBg);

    HPEN borderPen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ oldPen = SelectObject(hdc, borderPen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
    MoveToEx(hdc, arrowRc.left, arrowRc.top + 1, nullptr);
    LineTo(hdc, arrowRc.left, arrowRc.bottom - 1);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(borderPen);

    int selected = (int)SendMessageW(hwnd, CB_GETCURSEL, 0, 0);
    std::wstring text;
    if (selected >= 0) {
        int len = (int)SendMessageW(hwnd, CB_GETLBTEXTLEN, selected, 0);
        if (len >= 0) {
            text.resize(len + 1);
            SendMessageW(hwnd, CB_GETLBTEXT, selected, (LPARAM)text.data());
            text.resize(len);
        }
    } else {
        text = GetText(hwnd);
    }

    RECT textRc = rc;
    textRc.left += 10;
    textRc.right = arrowRc.left - 8;
    HGDIOBJ oldFont = SelectObject(hdc, Ui().font);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, textColor);
    DrawTextW(hdc, text.c_str(), -1, &textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    SelectObject(hdc, oldFont);

    int cx = arrowRc.left + (arrowRc.right - arrowRc.left) / 2;
    int cy = arrowRc.top + (arrowRc.bottom - arrowRc.top) / 2 + 1;
    double angle = open * 3.14159265358979323846;
    double pulse = 1.0 + std::sin(open * 3.14159265358979323846) * 0.16;
    POINT base[3] = {{-5, -3}, {5, -3}, {0, 4}};
    POINT pts[3]{};
    double c = std::cos(angle);
    double s = std::sin(angle);
    for (int i = 0; i < 3; ++i) {
        double x = base[i].x * pulse;
        double y = base[i].y * pulse;
        pts[i] = {
            cx + (int)std::lround(x * c - y * s),
            cy + (int)std::lround(x * s + y * c)
        };
    }
    HBRUSH arrowBrush = CreateSolidBrush(textColor);
    HPEN arrowPen = CreatePen(PS_SOLID, 1, textColor);
    oldBrush = SelectObject(hdc, arrowBrush);
    oldPen = SelectObject(hdc, arrowPen);
    Polygon(hdc, pts, 3);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(arrowPen);
    DeleteObject(arrowBrush);
}

void PaintHeaderControl(HWND hwnd, HDC hdc){
    RECT full{};
    GetClientRect(hwnd, &full);
    COLORREF fill = Ui().COLOR_PANEL;
    COLORREF border = Ui().COLOR_BORDER;
    HBRUSH bg = CreateSolidBrush(fill);
    FillRect(hdc, &full, bg);
    DeleteObject(bg);

    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, Ui().COLOR_TEXT);
    HGDIOBJ oldFont = Ui().font ? SelectObject(hdc, Ui().font) : nullptr;

    int count = Header_GetItemCount(hwnd);
    for (int i = 0; i < count; ++i) {
        RECT rc{};
        Header_GetItemRect(hwnd, i, &rc);

        DrawSoftDivider(hdc, rc.left, rc.right, rc.bottom - 1);
        MoveToEx(hdc, rc.right - 1, rc.top, nullptr);
        LineTo(hdc, rc.right - 1, rc.bottom);

        wchar_t text[128]{};
        HDITEMW item{};
        item.mask = HDI_TEXT;
        item.pszText = text;
        item.cchTextMax = 128;
        Header_GetItem(hwnd, i, &item);

        rc.left += 10;
        rc.right -= 8;
        DrawTextW(hdc, text, -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }

    if (oldFont) SelectObject(hdc, oldFont);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

LRESULT CALLBACK HeaderPaintProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR){
    switch (msg) {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, HeaderPaintProc, 3);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        PaintHeaderControl(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void EnableHeaderPaint(HWND header){
    if (header) SetWindowSubclass(header, HeaderPaintProc, 3, 0);
}

LRESULT CALLBACK StatsPaintProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR){
    switch (msg) {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, StatsPaintProc, 4);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc{};
        GetClientRect(hwnd, &rc);
        wchar_t text[1024]{};
        GetWindowTextW(hwnd, text, 1024);
        int id = GetDlgCtrlID(hwnd);
        bool card = id == IDC_STAT_TOTAL || id == IDC_STAT_ATTENDANCE || id == IDC_STAT_ISSUES || id == IDC_STAT_VISIBLE;

        if (card) {
            HBRUSH outer = CreateSolidBrush(Ui().COLOR_BG);
            FillRect(hdc, &rc, outer);
            DeleteObject(outer);

            RECT cardRc = rc;
            InflateRect(&cardRc, -1, -1);
            HBRUSH fill = CreateSolidBrush(Ui().COLOR_CARD);
            HPEN pen = CreatePen(PS_SOLID, 1, Ui().COLOR_CARD);
            HGDIOBJ oldBrush = SelectObject(hdc, fill);
            HGDIOBJ oldPen = SelectObject(hdc, pen);
            RoundRect(hdc, cardRc.left, cardRc.top, cardRc.right, cardRc.bottom, 8, 8);
            SelectObject(hdc, oldPen);
            SelectObject(hdc, oldBrush);
            DeleteObject(pen);
            DeleteObject(fill);

            for (int glow = 0; glow < 8; ++glow) {
                double amount = 0.050 * (1.0 - glow / 8.0);
                COLORREF glowTarget = IsRenderedPaletteDark() ? RGB(255, 255, 255) : RGB(0, 0, 0);
                COLORREF glowColor = BlendColor(Ui().COLOR_CARD, glowTarget, amount);
                HPEN glowPen = CreatePen(PS_SOLID, 1, glowColor);
                HGDIOBJ oldGlowPen = SelectObject(hdc, glowPen);
                HGDIOBJ oldGlowBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
                RECT glowRc = cardRc;
                InflateRect(&glowRc, -glow - 1, -glow - 1);
                RoundRect(hdc, glowRc.left, glowRc.top, glowRc.right, glowRc.bottom, 8, 8);
                SelectObject(hdc, oldGlowBrush);
                SelectObject(hdc, oldGlowPen);
                DeleteObject(glowPen);
            }

            std::wstring content = text;
            size_t split = content.find(L'\n');
            std::wstring label = split == std::wstring::npos ? content : content.substr(0, split);
            std::wstring value = split == std::wstring::npos ? L"" : content.substr(split + 1);

            HGDIOBJ oldFont = SelectObject(hdc, Ui().smallFont ? Ui().smallFont : Ui().font);
            SetBkMode(hdc, TRANSPARENT);
            RECT labelRc = cardRc;
            labelRc.left += 18;
            labelRc.top += 14;
            labelRc.right -= 18;
            labelRc.bottom = labelRc.top + 24;
            SetTextColor(hdc, Ui().COLOR_MUTED);
            DrawTextW(hdc, label.c_str(), -1, &labelRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

            RECT valueRc = cardRc;
            valueRc.left += 18;
            valueRc.top += 40;
            valueRc.right -= 18;
            valueRc.bottom -= 10;
            SetTextColor(hdc, Ui().COLOR_TEXT);
            DrawTextW(hdc, value.c_str(), -1, &valueRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
            SelectObject(hdc, oldFont);
            EndPaint(hwnd, &ps);
            return 0;
        }

        PaintAppBackgroundSlice(hwnd, hdc);
        rc.left += 2;
        rc.right -= 4;

        HGDIOBJ oldFont = SelectObject(hdc, Ui().smallFont ? Ui().smallFont : Ui().font);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, Ui().COLOR_ACCENT);
        UINT align = GetDlgCtrlID(hwnd) == IDC_STATS ? DT_RIGHT : DT_LEFT;
        DrawTextW(hdc, text, -1, &rc, align | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(hdc, oldFont);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void EnableStatsPaint(HWND stats){
    if (stats) SetWindowSubclass(stats, StatsPaintProc, 4, 0);
}

} // namespace attendance
