#include "ui/feedback.h"
#include "ui/motion.h"
#include "ui/redraw.h"
#include "ui/state.h"
#include "settings/state.h"
#include "animation/scheduler.h"
#include "localization/strings.h"

namespace attendance {
namespace {
struct Operation {
    OperationId id = 0; HWND button = nullptr;
    OperationPhase phase = OperationPhase::Working;
    uint64_t start = 0, finish = 0;bool buttonRestored=false;
    std::wstring label, message;
};
std::map<OperationId,Operation> operations;
OperationId sequence = 0;
HWND notice = nullptr;
constexpr wchar_t ButtonOwner[] = L"Attendance.Operation";
bool OwnsButton(const Operation& op) {
    return op.button && IsWindow(op.button) && (OperationId)(UINT_PTR)GetPropW(op.button,ButtonOwner)==op.id;
}
void Stroke(HDC dc, POINT a, POINT b, double fraction) {
    MoveToEx(dc,a.x,a.y,nullptr);
    LineTo(dc,a.x+(int)std::lround((b.x-a.x)*std::clamp(fraction,0.0,1.0)),
        a.y+(int)std::lround((b.y-a.y)*std::clamp(fraction,0.0,1.0)));
}
void Symbol(HDC dc, int cx, int cy, const Operation& op, uint64_t now) {
    HPEN pen=CreatePen(PS_SOLID,2,op.phase==OperationPhase::Failure?Ui().COLOR_DANGER:Ui().COLOR_TEXT);
    HGDIOBJ old=SelectObject(dc,pen);
    if(op.phase==OperationPhase::Working){
        if(now-op.start>=150){
            double turn=Settings().animationLevel==AnimationLevel::Off?0:now/200.0;
            POINT previous{};
            for(int i=0;i<=18;++i){double a=turn+i*.21;POINT point{cx+(int)std::lround(std::cos(a)*9),cy+(int)std::lround(std::sin(a)*9)};
                if(i)Stroke(dc,previous,point,1);previous=point;}
        }else {MoveToEx(dc,cx-5,cy,nullptr);LineTo(dc,cx+5,cy);}
    }else if(op.phase==OperationPhase::Success){
        double t=Settings().animationLevel==AnimationLevel::Off?1:std::clamp(((double)(now-op.finish)-80)/220,0.0,1.0);
        Stroke(dc,{cx-7,cy},{cx-2,cy+5},t*2.4);
        if(t>.42)Stroke(dc,{cx-2,cy+5},{cx+8,cy-6},(t-.42)/.58);
        if(Settings().particlesEnabled && Settings().animationLevel==AnimationLevel::Advanced){
            double burst=std::clamp(((double)(now-op.finish)-180)/380,0.0,1.0);
            int count=Settings().particleLevel==ParticleLevel::Low?8:Settings().particleLevel==ParticleLevel::High?16:12;
            if(burst>0 && burst<1)for(int i=0;i<count;++i){
                double angle=i*6.28318530718/count,radius=13+burst*11;
                int x=cx+(int)std::lround(std::cos(angle)*radius),y=cy+(int)std::lround(std::sin(angle)*radius);
                SetPixel(dc,x,y,BlendColor(Ui().COLOR_ACCENT,Ui().COLOR_BG,burst));
            }
        }
    }else {MoveToEx(dc,cx,cy-7,nullptr);LineTo(dc,cx,cy+2);SetPixel(dc,cx,cy+6,Ui().COLOR_TEXT);}
    SelectObject(dc,old);DeleteObject(pen);
}
void PositionNotice() {
    if(!notice || !Ui().hwnd)return;
    RECT main{};GetWindowRect(Ui().hwnd,&main);
    MONITORINFO monitor{sizeof(monitor)};GetMonitorInfoW(MonitorFromWindow(Ui().hwnd,MONITOR_DEFAULTTONEAREST),&monitor);
    int width=std::min(520,(int)(monitor.rcWork.right-monitor.rcWork.left)-32);
    int x=std::clamp((int)main.right-width-24,(int)monitor.rcWork.left+8,std::max((int)monitor.rcWork.left+8,(int)monitor.rcWork.right-width-8));
    int height=100;
    int y=std::clamp((int)main.bottom-height-32,(int)monitor.rcWork.top+8,std::max((int)monitor.rcWork.top+8,(int)monitor.rcWork.bottom-height-8));
    SetWindowPos(notice,HWND_TOP,x,y,width,height,SWP_NOACTIVATE);
}
LRESULT CALLBACK NoticeProc(HWND window,UINT message,WPARAM w,LPARAM l) {
    if(message==WM_NCHITTEST)return HTTRANSPARENT;
    if(message==WM_TIMER){
        uint64_t now=AnimationNowMs();bool hasVisible=false;
        for(auto it=operations.begin();it!=operations.end();){
            auto& op=it->second;
            if(OwnsButton(op) && (op.phase==OperationPhase::Working || now-op.finish<850 || !op.buttonRestored)){
                InvalidateRect(op.button,nullptr,FALSE);
                if(op.phase!=OperationPhase::Working && now-op.finish>=850)op.buttonRestored=true;
            }
            if(op.phase!=OperationPhase::Working && now-op.finish>6000){
                if(OwnsButton(op))RemovePropW(op.button,ButtonOwner);
                it=operations.erase(it);
            }else{
                hasVisible=hasVisible || (op.phase!=OperationPhase::Cancelled && (op.phase!=OperationPhase::Working || now-op.start>=150));++it;
            }
        }
        if(operations.empty()){KillTimer(window,1);ShowWindow(window,SW_HIDE);}
        else {SetTimer(window,1,Settings().animationLevel==AnimationLevel::Off?150:16,nullptr);PositionNotice();ShowWindow(window,hasVisible && Ui().hwnd && IsWindowVisible(Ui().hwnd) && !IsIconic(Ui().hwnd)?SW_SHOWNOACTIVATE:SW_HIDE);InvalidateRect(window,nullptr,FALSE);}
        return 0;
    }
    if(message==WM_PAINT){
        PAINTSTRUCT ps{};HDC target=BeginPaint(window,&ps);RECT rect{};GetClientRect(window,&rect);
        BufferedPaint buffer(target,rect);HDC dc=buffer.DC();FillRect(dc,&rect,Ui().bgBrush);
        PaintSurface(dc,rect,Ui().COLOR_PANEL);
        auto it=std::find_if(operations.rbegin(),operations.rend(),[](const auto& pair){return pair.second.phase!=OperationPhase::Cancelled;});
        if(it!=operations.rend()){
            const auto& op=it->second;Symbol(dc,28,48,op,AnimationNowMs());
            HGDIOBJ old=SelectObject(dc,Ui().font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,Ui().COLOR_TEXT);
            RECT text{56,12,rect.right-16,rect.bottom-10};
            std::wstring content=op.label+L"\n"+(op.message.empty()?Tr(L"Working...",L"正在处理…"):op.message);
            DrawTextW(dc,content.c_str(),-1,&text,DT_LEFT|DT_WORDBREAK|DT_NOPREFIX);
            SelectObject(dc,old);
        }
        buffer.Commit();EndPaint(window,&ps);return 0;
    }
    if(message==WM_ERASEBKGND)return 1;
    return DefWindowProcW(window,message,w,l);
}
void EnsureNotice() {
    if(notice && IsWindow(notice))return;
    WNDCLASSW wc{};wc.hInstance=GetModuleHandleW(nullptr);wc.lpfnWndProc=NoticeProc;wc.lpszClassName=L"AttendanceOperationNotice";
    RegisterClassW(&wc);
    notice=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE|WS_EX_TRANSPARENT,wc.lpszClassName,L"",WS_POPUP,
        0,0,520,100,Ui().hwnd,nullptr,wc.hInstance,nullptr);
    if(notice)ClipPaperWindow(notice,1,false);
}
}
OperationId BeginOperation(HWND button,const std::wstring& label) {
    EnsureNotice();Operation op;op.id=++sequence;op.start=AnimationNowMs();op.button=button;op.label=label;
    if(button && IsWindow(button))SetPropW(button,ButtonOwner,(HANDLE)(UINT_PTR)op.id);
    operations.emplace(op.id,op);if(notice)SetTimer(notice,1,16,nullptr);
    if(button)InvalidateRect(button,nullptr,FALSE);
    return op.id;
}
void FinishOperation(OperationId id,OperationPhase phase,const std::wstring& message) {
    auto it=operations.find(id);if(it==operations.end() || it->second.phase!=OperationPhase::Working)return;
    auto& op=it->second;op.phase=phase;op.finish=AnimationNowMs();op.message=message;
    if(OwnsButton(op))InvalidateRect(op.button,nullptr,FALSE);
    if(notice){SetWindowTextW(notice,(op.label+L": "+message).c_str());NotifyWinEvent(EVENT_OBJECT_NAMECHANGE,notice,OBJID_CLIENT,CHILDID_SELF);}
}
bool OperationButtonBusy(HWND button) {
    for(const auto& pair:operations)if(pair.second.button==button && OwnsButton(pair.second) && pair.second.phase==OperationPhase::Working)return true;
    return false;
}
OperationPhase OperationStatus(OperationId id) { auto it=operations.find(id);return it==operations.end()?OperationPhase::Cancelled:it->second.phase; }
bool DrawOperationButton(const DRAWITEMSTRUCT* draw) {
    const Operation* found=nullptr;
    for(const auto& pair:operations)if(pair.second.button==draw->hwndItem && OwnsButton(pair.second))found=&pair.second;
    if(!found || found->phase==OperationPhase::Cancelled)return false;
    uint64_t now=AnimationNowMs();
    if(found->phase!=OperationPhase::Working && now-found->finish>850)return false;
    if(found->phase!=OperationPhase::Working && found->phase!=OperationPhase::Success)return false;
    RECT rect=draw->rcItem;FillRect(draw->hDC,&rect,Ui().bgBrush);
    double shrink=Settings().animationLevel==AnimationLevel::Off?0:CubicBezierEase((now-found->start)/100.0,.22,1,.36,1);
    if(found->phase==OperationPhase::Success && now-found->finish>560)shrink*=1-CubicBezierEase((now-found->finish-560)/200.0,.22,1,.36,1);
    int cx=(rect.left+rect.right)/2,cy=(rect.top+rect.bottom)/2;
    int half=(int)std::lround((rect.right-rect.left)/2.0*(1-shrink)+18*shrink);
    RECT surface{cx-half,rect.top+2,cx+half,rect.bottom-2};
    PaintSurface(draw->hDC,surface,Ui().COLOR_INPUT,(int)std::lround(Visual::ControlRadius*(1-shrink)+18*shrink));
    Symbol(draw->hDC,cx,cy,*found,now);
    return true;
}
void StopOperationFeedback(){
    for(auto& pair:operations)if(OwnsButton(pair.second)){RemovePropW(pair.second.button,ButtonOwner);InvalidateRect(pair.second.button,nullptr,FALSE);}
    operations.clear();if(notice&&IsWindow(notice))DestroyWindow(notice);notice=nullptr;
}
} // namespace attendance
