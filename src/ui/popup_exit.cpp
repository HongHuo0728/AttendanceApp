#include "core/constants.h"
#include "ui/popup_exit.h"
#include "ui/motion.h"
#include "ui/state.h"
#include "animation/scheduler.h"
#include "settings/state.h"
#include <memory>

namespace attendance {
namespace {
struct ExitFrame {
    HDC dc=nullptr; HBITMAP bitmap=nullptr; HGDIOBJ previous=nullptr;
    int width=0,height=0; bool upward=false; bool* accepted=nullptr;
    ~ExitFrame(){if(dc&&previous)SelectObject(dc,previous);if(bitmap)DeleteObject(bitmap);if(dc)DeleteDC(dc);}
};
HWND exiting=nullptr;
LRESULT CALLBACK ExitProc(HWND window,UINT message,WPARAM w,LPARAM l){
    auto* frame=(ExitFrame*)GetWindowLongPtrW(window,GWLP_USERDATA);
    switch(message){
    case WM_NCCREATE:{auto* created=(ExitFrame*)((CREATESTRUCTW*)l)->lpCreateParams;SetWindowLongPtrW(window,GWLP_USERDATA,(LONG_PTR)created);if(created&&created->accepted)*created->accepted=true;return TRUE;}
    case WM_NCHITTEST:return HTTRANSPARENT;
    case WM_MOUSEACTIVATE:return MA_NOACTIVATEANDEAT;
    case WM_ERASEBKGND:return 1;
    case WM_PAINT:{PAINTSTRUCT ps{};HDC dc=BeginPaint(window,&ps);if(frame)BitBlt(dc,0,0,frame->width,frame->height,frame->dc,0,0,SRCCOPY);EndPaint(window,&ps);return 0;}
    case WM_APP_ANIMATION_TICK:
        AcknowledgeAnimationTick(window);
        if(frame){double progress=1-GetAnimationValue(window,AnimChannel::Close,0);ClipPaperWindow(window,progress,frame->upward);SetLayeredWindowAttributes(window,0,(BYTE)std::lround(progress*255),LWA_ALPHA);}
        return 0;
    case WM_APP_ANIMATION_DONE:DestroyWindow(window);return 0;
    case WM_NCDESTROY:
        RemoveAnimationState(window);SetWindowLongPtrW(window,GWLP_USERDATA,0);delete frame;
        if(exiting==window)exiting=nullptr;return DefWindowProcW(window,message,w,l);
    }
    return DefWindowProcW(window,message,w,l);
}
}
void StopPopupExit(){if(exiting && IsWindow(exiting))DestroyWindow(exiting);exiting=nullptr;}
void ShowPopupExit(HWND source,HWND owner,HDC frame,int width,int height,bool upward){
    StopPopupExit();
    if(!frame || !owner || !IsWindowVisible(owner) || Settings().animationLevel==AnimationLevel::Off || width<=0 || height<=0)return;
    auto state=std::make_unique<ExitFrame>();state->width=width;state->height=height;state->upward=upward;
    state->dc=CreateCompatibleDC(frame);if(!state->dc)return;
    state->bitmap=CreateCompatibleBitmap(frame,width,height);if(!state->bitmap)return;
    state->previous=SelectObject(state->dc,state->bitmap);BitBlt(state->dc,0,0,width,height,frame,0,0,SRCCOPY);
    WNDCLASSW wc{};wc.hInstance=GetModuleHandleW(nullptr);wc.lpfnWndProc=ExitProc;wc.lpszClassName=L"AttendancePopupExit";RegisterClassW(&wc);
    RECT rect{};GetWindowRect(source,&rect);
    bool accepted=false;state->accepted=&accepted;
    auto* owned=state.release();
    HWND window=CreateWindowExW(WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW|WS_EX_TRANSPARENT|WS_EX_LAYERED,wc.lpszClassName,L"",WS_POPUP,
        rect.left,rect.top,width,height,owner,nullptr,wc.hInstance,owned);
    if(!window){if(!accepted)delete owned;return;}
    owned->accepted=nullptr;
    exiting=window;SetLayeredWindowAttributes(window,0,255,LWA_ALPHA);ClipPaperWindow(window,1,upward);
    ShowWindow(window,SW_SHOWNOACTIVATE);StartAnimation(window,AnimChannel::Close,1,Visual::CloseMs,true);
}
} // namespace attendance
