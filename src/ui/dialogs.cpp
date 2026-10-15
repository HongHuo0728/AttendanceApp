#include "ui/dialogs.h"
#include "ui/common.h"
#include "animation/scheduler.h"
#include "ui/title_bar.h"
#include "localization/strings.h"
#include "ui/theme.h"
#include "ui/controls.h"
#include "ui/drawing.h"
#include "model/state.h"
#include "ui/state.h"
#include "core/constants.h"

namespace attendance {

int MeasureLongestTextLinePx(HWND owner, const std::wstring& text);
SIZE MeasureWrappedTextPx(HWND owner, const std::wstring& text, int width);
SIZE CalculateMessageDialogWindowSize(HWND owner, const std::wstring& message);
void ResizeMessageDialog(HWND hwnd);
LRESULT CALLBACK MessageDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK InputDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int MeasureLongestTextLinePx(HWND owner, const std::wstring& text){
    HDC hdc = GetDC(owner ? owner : GetDesktopWindow());
    HGDIOBJ oldFont = Ui().font ? SelectObject(hdc, Ui().font) : nullptr;
    int longest = 0;
    size_t start = 0;
    while (start <= text.size()) {
        size_t end = text.find(L'\n', start);
        std::wstring line = text.substr(start, end == std::wstring::npos ? std::wstring::npos : end - start);
        if (!line.empty() && line.back() == L'\r') line.pop_back();
        SIZE size{};
        GetTextExtentPoint32W(hdc, line.c_str(), (int)line.size(), &size);
        longest = std::max(longest, (int)size.cx);
        if (end == std::wstring::npos) break;
        start = end + 1;
    }
    if (oldFont) SelectObject(hdc, oldFont);
    ReleaseDC(owner ? owner : GetDesktopWindow(), hdc);
    return longest;
}

SIZE MeasureWrappedTextPx(HWND owner, const std::wstring& text, int width){
    HDC hdc = GetDC(owner ? owner : GetDesktopWindow());
    HGDIOBJ oldFont = Ui().font ? SelectObject(hdc, Ui().font) : nullptr;
    RECT rc{0, 0, width, 0};
    DrawTextW(hdc, text.c_str(), -1, &rc, DT_LEFT | DT_NOPREFIX | DT_WORDBREAK | DT_CALCRECT);
    if (oldFont) SelectObject(hdc, oldFont);
    ReleaseDC(owner ? owner : GetDesktopWindow(), hdc);
    return SIZE{std::max(1, (int)(rc.right - rc.left)), std::max(1, (int)(rc.bottom - rc.top))};
}

SIZE CalculateMessageDialogWindowSize(HWND owner, const std::wstring& message){
    int longest = MeasureLongestTextLinePx(owner, message);
    int textW = std::clamp(longest + 12, 360, 760);
    SIZE textSize = MeasureWrappedTextPx(owner, message, textW);
    int clientW = std::clamp(textW + 48, 470, 840);
    int clientH = std::clamp((int)textSize.cy + 108, 230, 620);

    RECT windowRc{0, 0, clientW, clientH};
    AdjustWindowRectEx(&windowRc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE, WS_EX_CONTROLPARENT);
    return SIZE{(LONG)(windowRc.right - windowRc.left), (LONG)(windowRc.bottom - windowRc.top)};
}

void ResizeMessageDialog(HWND hwnd){
    RECT rc{};
    GetClientRect(hwnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    int pad = 24;
    MoveWindow(GetDlgItem(hwnd, IDC_MESSAGE_TEXT), pad, pad, std::max(120, w - pad * 2), std::max(48, h - 96), TRUE);

    HWND ok = GetDlgItem(hwnd, IDC_MESSAGE_OK);
    HWND yes = GetDlgItem(hwnd, IDC_MESSAGE_YES);
    HWND no = GetDlgItem(hwnd, IDC_MESSAGE_NO);
    int buttonW = 94;
    int buttonH = 36;
    int y = h - pad - buttonH;
    if (ok) {
        MoveWindow(ok, w - pad - buttonW, y, buttonW, buttonH, TRUE);
    } else {
        MoveWindow(no, w - pad - buttonW, y, buttonW, buttonH, TRUE);
        MoveWindow(yes, w - pad - buttonW * 2 - 12, y, buttonW, buttonH, TRUE);
    }
}

LRESULT CALLBACK MessageDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    auto* state = reinterpret_cast<MessageDialogState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE: {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        state = reinterpret_cast<MessageDialogState*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)state);
        MakeSettingsControl(hwnd, L"STATIC", state->message.c_str(), SS_LEFT | SS_NOPREFIX | SS_EDITCONTROL, IDC_MESSAGE_TEXT);
        if (state->yesNo) {
            MakeSettingsControl(hwnd, L"BUTTON", Tr(L"Yes", L"\u662f").c_str(), BS_PUSHBUTTON | BS_OWNERDRAW | WS_TABSTOP, IDC_MESSAGE_YES);
            MakeSettingsControl(hwnd, L"BUTTON", Tr(L"No", L"\u5426").c_str(), BS_PUSHBUTTON | BS_OWNERDRAW | WS_TABSTOP, IDC_MESSAGE_NO);
        } else {
            MakeSettingsControl(hwnd, L"BUTTON", Tr(L"OK", L"\u786e\u5b9a").c_str(), BS_PUSHBUTTON | BS_OWNERDRAW | WS_TABSTOP, IDC_MESSAGE_OK);
        }
        ApplyThemedControls(hwnd);
        ResizeMessageDialog(hwnd);
        SetFocus(GetDlgItem(hwnd, state->yesNo ? IDC_MESSAGE_NO : IDC_MESSAGE_OK));
        return 0;
    }
    case WM_SIZE:
        ResizeMessageDialog(hwnd);
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_MESSAGE_YES:
            state->result = IDYES;
            BeginAnimatedClose(hwnd);
            return 0;
        case IDC_MESSAGE_NO:
            state->result = IDNO;
            BeginAnimatedClose(hwnd);
            return 0;
        case IDC_MESSAGE_OK:
            state->result = IDOK;
            BeginAnimatedClose(hwnd);
            return 0;
        }
        break;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            state->result = state->yesNo ? IDNO : IDOK;
            BeginAnimatedClose(hwnd);
            return 0;
        }
        if (wParam == VK_RETURN) {
            state->result = state->yesNo ? IDYES : IDOK;
            BeginAnimatedClose(hwnd);
            return 0;
        }
        break;
    case WM_DRAWITEM: {
        auto* draw = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (DrawButtonItem(draw)) return TRUE;
        break;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, Ui().COLOR_TEXT);
        SetBkMode(hdc, OPAQUE);
        SetBkColor(hdc, Ui().COLOR_BG);
        return (LRESULT)Ui().bgBrush;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        PaintAppBackground(hwnd, hdc);
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
        state->result = state->yesNo ? IDNO : IDOK;
        BeginAnimatedClose(hwnd);
        return 0;
    case WM_DESTROY:
        RemoveAnimationState(hwnd);
        if (state) state->done = true;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int ThemedMessageBox(HWND owner, const std::wstring& message, const std::wstring& title, bool yesNo){
    HINSTANCE instance = GetModuleHandleW(nullptr);
    const wchar_t* className = L"AttendanceMessageDialog";
    static bool registered = false;
    if (!registered) {
        WNDCLASSW wc{};
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = MessageDialogProc;
        wc.hInstance = instance;
        wc.lpszClassName = className;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON));
        wc.hbrBackground = nullptr;
        RegisterClassW(&wc);
        registered = true;
    }

    MessageDialogState state{title, message, yesNo, yesNo ? IDNO : IDOK, false};
    HWND parent = owner ? owner : Ui().hwnd;
    if (parent) EnableWindow(parent, FALSE);
    SIZE dialogSize = CalculateMessageDialogWindowSize(parent, message);
    HWND dialog = CreateWindowExW(
        WS_EX_CONTROLPARENT,
        className,
        title.c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, dialogSize.cx, dialogSize.cy,
        parent, nullptr, instance, &state
    );
    if (!dialog) {
        if (parent) EnableWindow(parent, TRUE);
        return state.result;
    }

    BOOL dark = IsRenderedPaletteDark() ? TRUE : FALSE;
    DwmSetWindowAttribute(dialog, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
    ApplyGlassTitleBar(dialog);
    CenterOwnedWindow(dialog, parent);
    ShowWindow(dialog, SW_SHOW);
    UpdateWindow(dialog);
    SetFocus(GetDlgItem(dialog, state.yesNo ? IDC_MESSAGE_NO : IDC_MESSAGE_OK));

    MSG msg{};
    while (!state.done && IsWindow(dialog)) {
        int messageResult = GetMessageW(&msg, nullptr, 0, 0);
        if (messageResult == 0) {
            PostQuitMessage((int)msg.wParam);
            break;
        }
        if (messageResult == -1) break;
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            SendMessageW(dialog, WM_CLOSE, 0, 0);
            continue;
        }
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN) {
            HWND focused = GetFocus();
            int id = focused && IsChild(dialog, focused) ? GetDlgCtrlID(focused) : 0;
            if (id != IDC_MESSAGE_YES && id != IDC_MESSAGE_NO && id != IDC_MESSAGE_OK) {
                id = state.yesNo ? IDC_MESSAGE_NO : IDC_MESSAGE_OK;
            }
            SendMessageW(dialog, WM_COMMAND, MAKEWPARAM(id, BN_CLICKED), (LPARAM)GetDlgItem(dialog, id));
            continue;
        }
        if (!IsDialogMessageW(dialog, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    if (IsWindow(dialog)) DestroyWindow(dialog);
    if (parent) {
        EnableWindow(parent, TRUE);
        SetForegroundWindow(parent);
    }
    return state.result;
}

bool ConfirmDiscardUnsaved(const std::wstring& actionText){
    if (!Document().dirty) return true;
    std::wstring msg = Tr(L"You have unsaved changes. Continue and discard them?", L"\u5f53\u524d\u6709\u672a\u4fdd\u5b58\u7684\u4fee\u6539\u3002\u662f\u5426\u7ee7\u7eed\u5e76\u653e\u5f03\u8fd9\u4e9b\u4fee\u6539\uff1f");
    if (!actionText.empty()) msg += L"\n\n" + actionText;
    std::wstring title = Tr(L"Unsaved Changes", L"\u672a\u4fdd\u5b58\u7684\u4fee\u6539");
    return ThemedMessageBox(Ui().hwnd, msg, title, true) == IDYES;
}

LRESULT CALLBACK InputDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    auto* state = reinterpret_cast<InputDialogState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE: {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        state = reinterpret_cast<InputDialogState*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)state);
        MakeSettingsControl(hwnd, L"STATIC", state->prompt.c_str(), 0, 0);
        HWND edit = MakeSettingsControl(hwnd, L"EDIT", state->value.c_str(), WS_TABSTOP | ES_AUTOHSCROLL | WS_BORDER, IDC_INPUT_EDIT);
        SetAccessibleName(edit, state->prompt);
        std::wstring okText = Tr(L"OK", L"\u786e\u5b9a");
        std::wstring cancelText = Tr(L"Cancel", L"\u53d6\u6d88");
        MakeSettingsControl(hwnd, L"BUTTON", okText.c_str(), BS_PUSHBUTTON | BS_OWNERDRAW | WS_TABSTOP, IDC_INPUT_OK);
        MakeSettingsControl(hwnd, L"BUTTON", cancelText.c_str(), BS_PUSHBUTTON | BS_OWNERDRAW | WS_TABSTOP, IDC_INPUT_CANCEL);
        ApplyThemedControls(hwnd);
        SendMessageW(GetDlgItem(hwnd, IDC_INPUT_EDIT), EM_SETSEL, 0, -1);
        SetFocus(GetDlgItem(hwnd, IDC_INPUT_EDIT));
        return 0;
    }
    case WM_SIZE: {
        RECT rc{};
        GetClientRect(hwnd, &rc);
        int w = rc.right - rc.left;
        HWND prompt = FindWindowExW(hwnd, nullptr, L"STATIC", nullptr);
        MoveWindow(prompt, 20, 18, w - 40, 24, TRUE);
        MoveWindow(GetDlgItem(hwnd, IDC_INPUT_EDIT), 20, 52, w - 40, 32, TRUE);
        MoveWindow(GetDlgItem(hwnd, IDC_INPUT_OK), w - 190, 102, 80, 34, TRUE);
        MoveWindow(GetDlgItem(hwnd, IDC_INPUT_CANCEL), w - 100, 102, 80, 34, TRUE);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_INPUT_OK) {
            state->value = GetText(GetDlgItem(hwnd, IDC_INPUT_EDIT));
            state->accepted = true;
            BeginAnimatedClose(hwnd);
            return 0;
        }
        if (LOWORD(wParam) == IDC_INPUT_CANCEL) {
            BeginAnimatedClose(hwnd);
            return 0;
        }
        break;
    case WM_KEYDOWN:
        if (wParam == VK_RETURN) {
            SendMessageW(hwnd, WM_COMMAND, IDC_INPUT_OK, 0);
            return 0;
        }
        if (wParam == VK_ESCAPE) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        PaintAppBackground(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_DRAWITEM: {
        auto* draw = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (DrawButtonItem(draw)) return TRUE;
        break;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, Ui().COLOR_TEXT);
        SetBkMode(hdc, OPAQUE);
        SetBkColor(hdc, Ui().COLOR_BG);
        return (LRESULT)Ui().bgBrush;
    }
    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, Ui().COLOR_TEXT);
        SetBkColor(hdc, Ui().COLOR_INPUT);
        return (LRESULT)Ui().inputBrush;
    }
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
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool PromptText(const std::wstring& title, const std::wstring& prompt, std::wstring& value){
    HINSTANCE instance = GetModuleHandleW(nullptr);
    const wchar_t* className = L"AttendanceInputDialog";
    static bool registered = false;
    if (!registered) {
        WNDCLASSW wc{};
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = InputDialogProc;
        wc.hInstance = instance;
        wc.lpszClassName = className;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON));
        RegisterClassW(&wc);
        registered = true;
    }

    InputDialogState state{title, prompt, value, false};
    EnableWindow(Ui().hwnd, FALSE);
    HWND dialog = CreateWindowExW(
        WS_EX_CONTROLPARENT,
        className,
        title.c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 430, 190,
        Ui().hwnd, nullptr, instance, &state
    );
    if (!dialog) {
        EnableWindow(Ui().hwnd, TRUE);
        return false;
    }
    ApplyGlassTitleBar(dialog);
    ApplyThemedControls(dialog);
    CenterOwnedWindow(dialog, Ui().hwnd);
    ShowWindow(dialog, SW_SHOW);
    UpdateWindow(dialog);
    HWND edit = GetDlgItem(dialog, IDC_INPUT_EDIT);
    SetFocus(edit);
    SendMessageW(edit, EM_SETSEL, 0, -1);

    MSG msg{};
    while (IsWindow(dialog)) {
        int messageResult = GetMessageW(&msg, nullptr, 0, 0);
        if (messageResult == 0) {
            PostQuitMessage((int)msg.wParam);
            break;
        }
        if (messageResult == -1) break;
        if (msg.message == WM_KEYDOWN && (msg.hwnd == dialog || IsChild(dialog, msg.hwnd))) {
            if (msg.wParam == VK_RETURN || msg.wParam == VK_ESCAPE) {
                SendMessageW(dialog, WM_KEYDOWN, msg.wParam, msg.lParam);
                continue;
            }
        }
        if (!IsDialogMessageW(dialog, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    EnableWindow(Ui().hwnd, TRUE);
    SetForegroundWindow(Ui().hwnd);
    if (state.accepted) value = state.value;
    return state.accepted;
}

void ShowMessage(const std::wstring& text, const std::wstring& title){
    ThemedMessageBox(Ui().hwnd, text, title, false);
}

} // namespace attendance
