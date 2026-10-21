#include "version.h"
#include "app/application.h"
#include "reports/jobs.h"
#include "ui/title_bar.h"
#include "core/text.h"
#include "settings/preferences.h"
#include "tests/parser_regression.h"
#include "ui/appearance.h"
#include "app/recovery.h"
#include "app/documents.h"
#include "settings/shortcuts.h"
#include "app/main_window.h"
#include "settings/state.h"
#include "ui/state.h"
#include "core/constants.h"

namespace attendance {



int RunApplication(
    _In_ HINSTANCE instance,
    _In_opt_ HINSTANCE,
    _In_ PWSTR commandLine,
    _In_ int showCmd){
    if (commandLine && TrimWide(commandLine) == L"--verify-parser") {
        return RunParserRegressionChecks() ? 0 : 2;
    }
    HRESULT comInitialization = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    const bool uninitializeCom = SUCCEEDED(comInitialization);
    LoadSettings();

    INITCOMMONCONTROLSEX icc{};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icc);

    const wchar_t* className = L"AttendanceAppWindow";
    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = className;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON));
    wc.hbrBackground = nullptr;
    RegisterClassW(&wc);

    // Buffer individual controls. Whole-window composition leaves native main
    // controls blank on some Windows configurations (verified on the test host).
    HWND hwnd = CreateWindowExW(
        0,
        className,
        ATTENDANCE_TITLE,
        WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 1600, 900,
        nullptr, nullptr, instance, nullptr
    );

    if (!hwnd) {
        if (uninitializeCom) CoUninitialize();
        return 1;
    }
    SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON)));
    SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON)));
    ApplyGlassTitleBar(hwnd);
    ShowWindow(hwnd, showCmd);
    RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_NOERASE | RDW_UPDATENOW);
    // Accessibility annotations are applied once more after creation so the
    // native providers can observe fully initialized child windows.
    ApplyMainLanguage();

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(commandLine, &argc);
    bool loadedFromArg = false;
    if (argv && argc > 0 && LowerText(std::filesystem::path(argv[0]).extension().wstring()) == L".attd") {
        loadedFromArg = LoadAttendanceFile(argv[0], false, AttendanceLoadIntent::CommandLine);
    }
    if (argv) LocalFree(argv);
    if (!loadedFromArg) PromptRestoreAutosave();
    PostMessageW(hwnd, WM_APP_PREWARM_SETTINGS, 0, 0);

    MSG msg{};
    int exitCode = 0;
    while (true) {
        int result = GetMessageW(&msg, nullptr, 0, 0);
        if (result == 0) {
            exitCode = static_cast<int>(msg.wParam);
            break;
        }
        if (result == -1) {
            exitCode = 1;
            break;
        }
        if (HandleMainShortcutMessage(msg)) continue;
        if (Ui().quickRollCallWindow && IsWindow(Ui().quickRollCallWindow)
            && (msg.hwnd == Ui().quickRollCallWindow || IsChild(Ui().quickRollCallWindow, msg.hwnd))) {
            if (msg.message == WM_KEYDOWN) {
                HWND notes = GetDlgItem(Ui().quickRollCallWindow, IDC_QUICK_NOTES);
                bool editingNotes = GetFocus() == notes;
                bool globalKey = msg.wParam == VK_ESCAPE;
                bool rollCallKey = msg.wParam == L'1' || msg.wParam == L'2' || msg.wParam == L'3' || msg.wParam == L'4' || msg.wParam == L'5' || msg.wParam == L'6'
                    || msg.wParam == VK_NUMPAD1 || msg.wParam == VK_NUMPAD2 || msg.wParam == VK_NUMPAD3 || msg.wParam == VK_NUMPAD4 || msg.wParam == VK_NUMPAD5 || msg.wParam == VK_NUMPAD6
                    || msg.wParam == VK_LEFT || msg.wParam == VK_RIGHT;
                if (globalKey || (!editingNotes && rollCallKey)) {
                    SendMessageW(Ui().quickRollCallWindow, WM_KEYDOWN, msg.wParam, msg.lParam);
                    continue;
                }
            }
            if (IsDialogMessageW(Ui().quickRollCallWindow, &msg)) continue;
        }
        if (Ui().settingsWindow && IsWindow(Ui().settingsWindow)
            && (msg.hwnd == Ui().settingsWindow || IsChild(Ui().settingsWindow, msg.hwnd))) {
            if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN
                && GetFocus() == GetDlgItem(Ui().settingsWindow, IDC_SETTINGS_SCROLL_FPS)) {
                SendMessageW(Ui().settingsWindow, WM_COMMAND,
                    MAKEWPARAM(IDC_SETTINGS_SCROLL_FPS, EN_KILLFOCUS),
                    (LPARAM)GetFocus());
                continue;
            }
            if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
                SendMessageW(Ui().settingsWindow, WM_KEYDOWN, msg.wParam, msg.lParam);
                continue;
            }
            if (IsDialogMessageW(Ui().settingsWindow, &msg)) continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        DispatchReportCompletions();
    }
    if (IsWindow(hwnd)) {
        if (exitCode != 0) AutoSaveNow();
        DestroyWindow(hwnd);
    }
    if (uninitializeCom) CoUninitialize();
    return exitCode;
}

} // namespace attendance
