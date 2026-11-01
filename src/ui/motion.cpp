#include "core/constants.h"
#include "ui/motion.h"
#include "animation/scheduler.h"
#include "settings/state.h"
#include "ui/state.h"
#include "ui/common.h"
#include "ui/redraw.h"

namespace attendance {
double ItemEntrance(double progress, int visibleIndex) {
    if (progress >= 1.0 || Settings().animationLevel != AnimationLevel::Advanced) return progress;
    return CubicBezierEase(std::clamp((progress*360-std::min(visibleIndex,6)*20)/240,0.0,1.0),.22,1,.36,1);
}
RECT PaperViewport(int width, int height, double progress, bool upward) {
    int visible = std::clamp((int)std::lround(height * std::clamp(progress, 0.0, 1.0)), 0, height);
    return RECT{0, upward ? height - visible : 0, width, upward ? height : visible};
}
void ClipPaperWindow(HWND window, double progress, bool upward) {
    RECT bounds{}; GetWindowRect(window, &bounds);
    int width = bounds.right - bounds.left, height = bounds.bottom - bounds.top;
    RECT clip = PaperViewport(width, height, progress, upward);
    auto signature=(UINT_PTR)(clip.top+clip.bottom+1) | (upward?(UINT_PTR)1<<30:0);
    if((UINT_PTR)GetPropW(window,L"Attendance.PaperClip")==signature && progress>0 && progress<1)return;
    SetPropW(window,L"Attendance.PaperClip",(HANDLE)signature);
    HRGN shape = CreateRoundRectRgn(0, 0, width + 1, height + 1, Visual::PopupRadius * 2, Visual::PopupRadius * 2);
    HRGN visible = CreateRectRgnIndirect(&clip);
    if (shape && visible) CombineRgn(shape, shape, visible, RGN_AND);
    if (visible) DeleteObject(visible);
    if (shape && !SetWindowRgn(window, shape, FALSE)) DeleteObject(shape);
}
void PaintSurface(HDC dc, RECT rect, COLORREF fill, int radius) {
    HGDIOBJ pen = SelectObject(dc, GetStockObject(DC_PEN));
    HGDIOBJ brush = SelectObject(dc, GetStockObject(DC_BRUSH));
    for (int inset = 0; inset < 3; ++inset) {
        SetDCBrushColor(dc, BlendColor(Ui().COLOR_BG, Ui().COLOR_BORDER, .05 + inset * .04));
        SetDCPenColor(dc, GetDCBrushColor(dc));
        RoundRect(dc, rect.left + inset, rect.top + inset + 1, rect.right - inset, rect.bottom - inset,
                  radius * 2, radius * 2);
    }
    InflateRect(&rect, -3, -3);
    SetDCBrushColor(dc, fill); SetDCPenColor(dc, fill);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, std::max(2, radius - 3) * 2, std::max(2, radius - 3) * 2);
    SelectObject(dc, brush); SelectObject(dc, pen);
}
int MeasuredTextHeight(HWND control) {
    HDC dc = GetDC(control); if (!dc) return 24;
    HFONT font = (HFONT)SendMessageW(control, WM_GETFONT, 0, 0);
    HGDIOBJ old = font ? SelectObject(dc, font) : nullptr;
    TEXTMETRICW metrics{}; GetTextMetricsW(dc, &metrics);
    if (old) SelectObject(dc, old); ReleaseDC(control, dc);
    return std::max(18, (int)(metrics.tmHeight + metrics.tmExternalLeading));
}
int MeasuredTextWidth(HWND control) {
    HDC dc = GetDC(control); if (!dc) return 160;
    HFONT font = (HFONT)SendMessageW(control, WM_GETFONT, 0, 0);
    HGDIOBJ old = font ? SelectObject(dc, font) : nullptr;
    auto text = GetText(control); SIZE size{}; GetTextExtentPoint32W(dc, text.c_str(), (int)text.size(), &size);
    if (old) SelectObject(dc, old); ReleaseDC(control, dc);
    return size.cx;
}
int WrappedTextHeight(HWND control,int width){
    HDC dc=GetDC(control);if(!dc)return 24;
    HGDIOBJ old=SelectObject(dc,(HFONT)SendMessageW(control,WM_GETFONT,0,0));
    RECT rect{0,0,std::max(16,width),0};auto text=GetText(control);
    DrawTextW(dc,text.c_str(),-1,&rect,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);
    SelectObject(dc,old);ReleaseDC(control,dc);return std::max(18,(int)rect.bottom);
}
void DrawStableDigits(HDC dc, const std::wstring& text, RECT rect, UINT alignment) {
    SIZE size{}; int digitWidth = 0, total = 0;
    for (wchar_t c = L'0'; c <= L'9'; ++c) { GetTextExtentPoint32W(dc, &c, 1, &size); digitWidth = std::max(digitWidth, (int)size.cx); }
    for (wchar_t c : text) { GetTextExtentPoint32W(dc, &c, 1, &size); total += c >= L'0' && c <= L'9' ? digitWidth : size.cx; }
    int x = alignment & DT_RIGHT ? rect.right - total : alignment & DT_CENTER ? (rect.left + rect.right - total) / 2 : rect.left;
    for (wchar_t c : text) {
        GetTextExtentPoint32W(dc, &c, 1, &size);
        int width = c >= L'0' && c <= L'9' ? digitWidth : size.cx;
        RECT cell{x, rect.top, x + width, rect.bottom};
        DrawTextW(dc, &c, 1, &cell, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        x += width;
    }
}
namespace {
LRESULT CALLBACK StableDigitsProc(HWND window, UINT message, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR) {
    if (message == WM_NCDESTROY) RemoveWindowSubclass(window, StableDigitsProc, 31);
    if (message != WM_PAINT) return DefSubclassProc(window, message, w, l);
    PAINTSTRUCT ps{}; HDC dc = BeginPaint(window, &ps); RECT rect{}; GetClientRect(window, &rect);
    BufferedPaint buffer(dc, rect); FillRect(buffer.DC(), &rect, Ui().bgBrush);
    HGDIOBJ old = SelectObject(buffer.DC(), (HFONT)SendMessageW(window, WM_GETFONT, 0, 0));
    SetBkMode(buffer.DC(), TRANSPARENT); SetTextColor(buffer.DC(), Ui().COLOR_TEXT);
    DrawStableDigits(buffer.DC(), GetText(window), rect, DT_RIGHT);
    SelectObject(buffer.DC(), old); buffer.Commit(); EndPaint(window, &ps); return 0;
}
LRESULT CALLBACK BalancedTitleProc(HWND window, UINT message, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR) {
    if(message==WM_NCDESTROY)RemoveWindowSubclass(window,BalancedTitleProc,32);
    if(message!=WM_PAINT)return DefSubclassProc(window,message,w,l);
    PAINTSTRUCT ps{};HDC dc=BeginPaint(window,&ps);RECT rect{};GetClientRect(window,&rect);
    BufferedPaint buffer(dc,rect);HDC paint=buffer.DC();FillRect(paint,&rect,Ui().bgBrush);
    HGDIOBJ old=SelectObject(paint,(HFONT)SendMessageW(window,WM_GETFONT,0,0));
    auto text=GetText(window);SIZE full{};GetTextExtentPoint32W(paint,text.c_str(),(int)text.size(),&full);
    if(full.cx>rect.right && text.find(L' ')!=std::wstring::npos){
        int best=INT_MAX;size_t split=std::wstring::npos;
        for(size_t index=0;index<text.size();++index)if(text[index]==L' '){
            SIZE left{},right{};GetTextExtentPoint32W(paint,text.data(),(int)index,&left);
            GetTextExtentPoint32W(paint,text.data()+index+1,(int)(text.size()-index-1),&right);
            if(left.cx>rect.right||right.cx>rect.right)continue;
            int score=std::abs(left.cx-right.cx)+(text.find(L' ',index+1)==std::wstring::npos?10000:0);
            if(score<best){best=score;split=index;}
        }
        if(split!=std::wstring::npos)text[split]=L'\n';
    }
    SetBkMode(paint,TRANSPARENT);SetTextColor(paint,GetDlgCtrlID(window)==IDC_SUBTITLE?Ui().COLOR_MUTED:Ui().COLOR_TEXT);
    DrawTextW(paint,text.c_str(),-1,&rect,DT_LEFT|DT_WORDBREAK|DT_NOPREFIX);
    SelectObject(paint,old);buffer.Commit();EndPaint(window,&ps);return 0;
}
}
void EnableStableDigits(HWND control) { if (control) SetWindowSubclass(control, StableDigitsProc, 31, 0); }
void EnableBalancedTitle(HWND control) { if(control)SetWindowSubclass(control,BalancedTitleProc,32,0); }
} // namespace attendance
