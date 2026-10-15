#include "ui/statistics.h"
#include "animation/scheduler.h"
#include "ui/title_bar.h"
#include "localization/strings.h"
#include "model/query.h"
#include "model/state.h"
#include "ui/state.h"
#include "core/constants.h"

namespace attendance {

void PaintStatsChart(HWND hwnd, HDC hdc);
LRESULT CALLBACK ChartProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void PaintStatsChart(HWND hwnd, HDC hdc){
    RECT rc{};
    GetClientRect(hwnd, &rc);
    FillRect(hdc, &rc, Ui().bgBrush);

    int present, absent, late, other;
    CountStatuses(present, absent, late, other);
    int values[] = {present, absent, late, other};
    const wchar_t* labels[] = {L"Present", L"Absent", L"Late", L"Other"};
    COLORREF colors[] = {RGB(126, 154, 142), RGB(170, 112, 112), RGB(164, 146, 104), RGB(126, 126, 126)};
    int maxValue = std::max(1, std::max(std::max(present, absent), std::max(late, other)));
    double reveal = GetAnimationValue(hwnd, AnimChannel::Reveal, 1.0);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, Ui().COLOR_TEXT);
    SelectObject(hdc, Ui().titleFont);
    std::wstring title = Tr(L"Statistics Chart", L"\u7edf\u8ba1\u56fe\u8868") + L" - " + Document().sheets[Document().activeSheet].name;
    TextOutW(hdc, 24, 20, title.c_str(), (int)title.size());

    SelectObject(hdc, Ui().font);
    int labelX = 34;
    int barX = 150;
    int y = 86;
    int barMaxW = std::max(120, (int)(rc.right - barX - 90));
    for (int i = 0; i < 4; ++i) {
        std::wstring label = Tr(labels[i], labels[i]);
        TextOutW(hdc, labelX, y + 6, label.c_str(), (int)label.size());
        RECT barBg{barX, y, barX + barMaxW, y + 30};
        HBRUSH bg = CreateSolidBrush(Ui().COLOR_PANEL);
        FillRect(hdc, &barBg, bg);
        DeleteObject(bg);

        double rowProgress = std::clamp(reveal * 1.25 - i * 0.1, 0.0, 1.0);
        int width = (int)(barMaxW * (values[i] / (double)maxValue) * rowProgress);
        RECT bar{barX, y, barX + width, y + 30};
        HBRUSH fill = CreateSolidBrush(colors[i]);
        FillRect(hdc, &bar, fill);
        DeleteObject(fill);

        std::wstring count = std::to_wstring(values[i]);
        TextOutW(hdc, barX + barMaxW + 14, y + 6, count.c_str(), (int)count.size());
        y += 56;
    }
}

LRESULT CALLBACK ChartProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc{};
        GetClientRect(hwnd, &rc);
        int width = std::max(1, (int)(rc.right - rc.left));
        int height = std::max(1, (int)(rc.bottom - rc.top));
        HDC memDc = CreateCompatibleDC(hdc);
        HBITMAP bitmap = CreateCompatibleBitmap(hdc, width, height);
        HGDIOBJ oldBitmap = SelectObject(memDc, bitmap);
        PaintStatsChart(hwnd, memDc);
        BitBlt(hdc, 0, 0, width, height, memDc, 0, 0, SRCCOPY);
        SelectObject(memDc, oldBitmap);
        DeleteObject(bitmap);
        DeleteDC(memDc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_APP_ANIMATION_TICK:
        AcknowledgeAnimationTick(hwnd);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_APP_ANIMATION_DONE:
        DestroyWindow(hwnd);
        return 0;
    case WM_CLOSE:
        BeginAnimatedClose(hwnd);
        return 0;
    case WM_DESTROY:
        RemoveAnimationState(hwnd);
        Ui().chartWindow = nullptr;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void ShowStatsChart(){
    if (Ui().chartWindow && IsWindow(Ui().chartWindow)) {
        InvalidateRect(Ui().chartWindow, nullptr, FALSE);
        SetForegroundWindow(Ui().chartWindow);
        return;
    }

    HINSTANCE instance = GetModuleHandleW(nullptr);
    const wchar_t* className = L"AttendanceStatsChart";
    static bool registered = false;
    if (!registered) {
        WNDCLASSW wc{};
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = ChartProc;
        wc.hInstance = instance;
        wc.lpszClassName = className;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON));
        RegisterClassW(&wc);
        registered = true;
    }
    std::wstring chartTitle = Tr(L"Statistics Chart", L"\u7edf\u8ba1\u56fe\u8868");
    Ui().chartWindow = CreateWindowExW(
        0, className, chartTitle.c_str(),
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 760, 420,
        Ui().hwnd, nullptr, instance, nullptr
    );
    ApplyGlassTitleBar(Ui().chartWindow);
    CenterOwnedWindow(Ui().chartWindow, Ui().hwnd);
    StartWindowReveal(Ui().chartWindow, 190);
    ShowWindow(Ui().chartWindow, SW_SHOW);
    UpdateWindow(Ui().chartWindow);
}

} // namespace attendance
