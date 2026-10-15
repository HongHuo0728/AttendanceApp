#include "ui/common.h"
#include "core/constants.h"

namespace attendance {



std::wstring GetText(HWND hwnd){
    int len = GetWindowTextLengthW(hwnd);
    std::wstring text(len + 1, L'\0');
    GetWindowTextW(hwnd, text.data(), len + 1);
    text.resize(len);
    return text;
}

void SetText(HWND hwnd, const std::wstring& text){
    SetWindowTextW(hwnd, text.c_str());
}

void SetAccessibleName(HWND hwnd, const std::wstring& name){
    if (!hwnd || name.empty()) return;
    constexpr CLSID kAccPropServicesClass = {
        0xb5f8350b, 0x0548, 0x48b1, {0xa6, 0xee, 0x88, 0xbd, 0x00, 0xb4, 0xa5, 0xe7}};
    constexpr MSAAPROPID kAccessibleNameProperty = {
        0x608d3df8, 0x8128, 0x4aa7, {0xa4, 0x28, 0xf5, 0x5e, 0x49, 0x26, 0x72, 0x91}};
    IAccPropServices* services = nullptr;
    if (FAILED(CoCreateInstance(
            kAccPropServicesClass, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&services)))
        || !services) {
        return;
    }
    HRESULT annotationResult = services->SetHwndPropStr(
        hwnd,
        static_cast<DWORD>(OBJID_CLIENT),
        static_cast<DWORD>(CHILDID_SELF),
        kAccessibleNameProperty,
        name.c_str());
    services->Release();
    if (SUCCEEDED(annotationResult)) {
        NotifyWinEvent(EVENT_OBJECT_NAMECHANGE, hwnd, OBJID_CLIENT, CHILDID_SELF);
    }
}

void ConfigureEditMetrics(HWND hwnd){
    if (!hwnd) return;
    SendMessageW(hwnd, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(10, 10));

    RECT rc{};
    GetClientRect(hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    if (width <= 0 || height <= 0) return;

    HDC hdc = GetDC(hwnd);
    HFONT font = (HFONT)SendMessageW(hwnd, WM_GETFONT, 0, 0);
    HGDIOBJ oldFont = font ? SelectObject(hdc, font) : nullptr;
    TEXTMETRICW tm{};
    GetTextMetricsW(hdc, &tm);
    if (oldFont) SelectObject(hdc, oldFont);
    ReleaseDC(hwnd, hdc);

    int textH = std::max(1, (int)(tm.tmHeight + tm.tmExternalLeading));
    int top = std::max(2, (height - textH) / 2);
    RECT textRc{10, top, std::max(11, width - 10), std::min(height - 2, top + textH + 2)};
    SendMessageW(hwnd, EM_SETRECTNP, 0, (LPARAM)&textRc);
}

} // namespace attendance
