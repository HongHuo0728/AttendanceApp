#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <shlobj.h>
#include <uxtheme.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <map>
#include <mutex>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "../include/resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "msimg32.lib")

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_CAPTION_COLOR
#define DWMWA_CAPTION_COLOR 35
#endif
#ifndef DWMWA_TEXT_COLOR
#define DWMWA_TEXT_COLOR 36
#endif
#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif

struct AttendanceRecord {
    std::wstring dateTime;
    std::wstring name;
    std::wstring status;
    std::wstring other;
};

struct AttendanceSheet {
    std::wstring name;
    std::wstring teacher;
    std::wstring location;
    std::wstring notes;
    std::vector<std::wstring> students;
    std::vector<AttendanceRecord> records;
};

static constexpr int IDC_DATE = 1001;
static constexpr int IDC_NAME = 1002;
static constexpr int IDC_OTHER = 1003;
static constexpr int IDC_LIST = 1004;
static constexpr int IDC_PRESENT = 1005;
static constexpr int IDC_ABSENT = 1006;
static constexpr int IDC_LATE = 1007;
static constexpr int IDC_OTHER_STATUS = 1008;
static constexpr int IDC_SAVE = 1009;
static constexpr int IDC_IMPORT = 1010;
static constexpr int IDC_NEW = 1011;
static constexpr int IDC_DELETE = 1012;
static constexpr int IDC_ADD_UPDATE = 1013;
static constexpr int IDC_EDIT_SELECTED = 1014;
static constexpr int IDC_EXPORT_CSV = 1015;
static constexpr int IDC_ALL_PRESENT = 1016;
static constexpr int IDC_SETTINGS = 1017;
static constexpr int IDC_COURSE_COMBO = 1018;
static constexpr int IDC_COURSE_OPTIONS = 1019;
static constexpr int IDC_TOOLS = 1020;
static constexpr int IDC_FILTER = 1021;
static constexpr int IDC_CLEAR_FILTER = 1022;
static constexpr int IDC_TITLE = 2004;
static constexpr int IDC_SUBTITLE = 2005;
static constexpr int IDC_STATS = 2006;
static constexpr int IDC_HINT = 2007;
static constexpr int IDC_FILTER_LABEL = 2008;
static constexpr int IDC_STAT_TOTAL = 2009;
static constexpr int IDC_STAT_ATTENDANCE = 2010;
static constexpr int IDC_STAT_ISSUES = 2011;
static constexpr int IDC_STAT_VISIBLE = 2012;

static constexpr int IDM_DELETE_SELECTED = 3001;
static constexpr int IDM_DELETE_ABSENT = 3002;
static constexpr int IDM_DELETE_LATE = 3003;
static constexpr int IDM_CLEAR_ALL = 3004;
static constexpr int IDM_COURSE_ADD = 3010;
static constexpr int IDM_COURSE_DELETE = 3011;
static constexpr int IDM_COURSE_RENAME = 3012;
static constexpr int IDM_IMPORT_ROSTER = 3020;
static constexpr int IDM_PRINT_HTML = 3021;
static constexpr int IDM_STATS_CHART = 3022;
static constexpr int IDM_UNDO = 3023;
static constexpr int IDM_REDO = 3024;
static constexpr int IDM_SHORTCUTS = 3025;
static constexpr int IDM_EXPORT_DB = 3026;
static constexpr int IDM_OPEN_AUTOSAVE = 3027;
static constexpr int IDM_EXPORT_PPTX = 3028;
static constexpr int IDM_STUDENTS_ADD = 3029;
static constexpr int IDM_STUDENTS_REMOVE = 3030;
static constexpr int IDM_STUDENTS_GENERATE = 3031;
static constexpr int IDM_COURSE_DETAILS = 3032;
static constexpr int IDM_STATS_SUMMARY = 3033;
static constexpr int IDM_BACKUP_NOW = 3034;
static constexpr int IDM_RESTORE_BACKUP = 3035;
static constexpr int IDM_OPEN_RECENT = 3036;
static constexpr int IDM_SET_SAVE_DIR = 3037;

static constexpr int IDC_SETTINGS_LANGUAGE = 4001;
static constexpr int IDC_SETTINGS_THEME = 4002;
static constexpr int IDC_SETTINGS_FONT = 4003;
static constexpr int IDC_SETTINGS_APPLY = 4004;
static constexpr int IDC_SETTINGS_CLOSE = 4005;
static constexpr int IDC_SETTINGS_LANG_LABEL = 4006;
static constexpr int IDC_SETTINGS_THEME_LABEL = 4007;
static constexpr int IDC_SETTINGS_FONT_LABEL = 4008;
static constexpr int IDC_SETTINGS_TITLE = 4009;
static constexpr int IDC_SETTINGS_RESET = 4010;
static constexpr int IDC_SETTINGS_PARTICLES = 4011;
static constexpr int IDC_INPUT_EDIT = 5001;
static constexpr int IDC_INPUT_OK = 5002;
static constexpr int IDC_INPUT_CANCEL = 5003;
static constexpr int IDC_MESSAGE_TEXT = 6001;
static constexpr int IDC_MESSAGE_YES = 6002;
static constexpr int IDC_MESSAGE_NO = 6003;
static constexpr int IDC_MESSAGE_OK = 6004;

static HWND g_hwnd = nullptr;
static HWND g_dateEdit = nullptr;
static HWND g_nameEdit = nullptr;
static HWND g_otherEdit = nullptr;
static HWND g_list = nullptr;
static HWND g_courseCombo = nullptr;
static HWND g_filterEdit = nullptr;
static HWND g_settingsWindow = nullptr;
static HWND g_chartWindow = nullptr;
static HFONT g_font = nullptr;
static HFONT g_titleFont = nullptr;
static HFONT g_smallFont = nullptr;
static HBRUSH g_bgBrush = nullptr;
static HBRUSH g_inputBrush = nullptr;
static HBRUSH g_panelBrush = nullptr;
static std::vector<AttendanceRecord> g_records;
static std::vector<AttendanceSheet> g_sheets;
static std::vector<int> g_visibleRows;
static int g_activeSheet = 0;
static int g_loadedActiveSheet = 0;
static std::vector<std::string> g_undoStack;
static std::vector<std::string> g_redoStack;
static bool g_allowAutosaveOverwrite = true;
static bool g_dirty = false;
static bool g_fullscreen = false;
static bool g_liveResizing = false;
static bool g_particlesEnabled = true;
static WINDOWPLACEMENT g_previousPlacement{sizeof(g_previousPlacement)};
static DWORD g_previousStyle = 0;
static int g_scrollX = 0;
static int g_scrollY = 0;
static int g_contentW = 0;
static int g_contentH = 0;
static std::wstring g_filterText;
static constexpr int MIN_LAYOUT_W = 1180;
static constexpr int MIN_LAYOUT_H = 820;
static constexpr UINT WM_APP_ANIMATION_TICK = WM_APP + 41;
static constexpr UINT WM_APP_ANIMATION_DONE = WM_APP + 42;

struct InputDialogState {
    std::wstring title;
    std::wstring prompt;
    std::wstring value;
    bool accepted = false;
};

struct MessageDialogState {
    std::wstring title;
    std::wstring message;
    bool yesNo = false;
    int result = IDOK;
    bool done = false;
};

enum class UiLanguage {
    English,
    ChineseSimplified,
    Maltese,
    Japanese,
    French,
    German,
    Russian,
    ChineseTraditional,
    Spanish,
    Italian,
    Mongolian,
    Esperanto,
    ClassicalChinese,
    Thai,
    Filipino,
    Turkish,
    Lithuanian,
    Norwegian,
    Vietnamese,
    ChineseTraditionalHongKong
};
enum class UiTheme { Dark };

static UiLanguage g_language = UiLanguage::English;
static UiTheme g_theme = UiTheme::Dark;
static std::wstring g_fontFamily = L"Segoe UI";
static std::wstring g_defaultSaveDir;
static std::vector<std::wstring> g_availableFonts;

static COLORREF COLOR_BG = RGB(0, 0, 0);
static COLORREF COLOR_PANEL = RGB(18, 18, 18);
static COLORREF COLOR_INPUT = RGB(26, 26, 26);
static COLORREF COLOR_TEXT = RGB(255, 255, 255);
static COLORREF COLOR_MUTED = RGB(224, 224, 224);
static COLORREF COLOR_ACCENT = RGB(188, 188, 188);
static COLORREF COLOR_DANGER = RGB(176, 112, 112);

enum class AnimChannel : int {
    Hover = 1,
    Press = 2,
    ComboOpen = 3,
    Reveal = 4,
    Close = 5,
    ListReveal = 6,
    Effect = 7
};

struct AnimValue {
    double current = 0.0;
    double start = 0.0;
    double target = 0.0;
    uint64_t startMs = 0;
    uint32_t durationMs = 160;
    bool active = false;
    bool closeOnDone = false;
};

static std::mutex g_animMutex;
static std::map<HWND, std::map<int, AnimValue>> g_animations;
static std::atomic_bool g_animationShutdown{false};
static std::thread g_animationThread;

struct ButtonParticle {
    double x = 0.0;
    double y = 0.0;
    double vx = 0.0;
    double vy = 0.0;
    double radius = 2.0;
    uint32_t lifetimeMs = 1000;
    COLORREF color = RGB(255, 255, 255);
    bool star = false;
};

struct ButtonEffectState {
    POINT origin{};
    uint64_t startMs = 0;
    std::vector<ButtonParticle> particles;
};

static std::map<HWND, ButtonEffectState> g_buttonEffects;
static std::mt19937 g_effectRng{std::random_device{}()};

void ResizeLayout(HWND hwnd);
void ScrollMainWindow(HWND hwnd, int bar, int code, int wheelDelta = 0);
void ApplyVisualSettings();
void ApplyDarkMode(HWND hwnd);
void ApplyThemedControls(HWND root);
void PaintAppBackground(HWND hwnd, HDC hdc);
bool DrawButtonItem(const DRAWITEMSTRUCT* draw);
bool DrawComboItem(const DRAWITEMSTRUCT* draw);
void PaintComboClosed(HWND hwnd, HDC hdc);
void EnableHeaderPaint(HWND header);
void EnableComboPaint(HWND combo);
void ApplyComboDropDownTheme(HWND combo);
std::string WideToUtf8(const std::wstring& input);
std::wstring Utf8ToWide(const std::string& input);
bool LoadAttendanceFile(const std::wstring& path, bool showSuccess);
void PushUndo();
HWND MakeSettingsControl(HWND parent, const wchar_t* cls, const wchar_t* text, DWORD style, int id);
std::wstring Tr(const wchar_t* english, const wchar_t* chinese);
void EnableEditShortcuts(HWND hwnd);
void EnableMouseWheelForward(HWND hwnd);
void CountStatuses(int& present, int& absent, int& late, int& other);
void EnableInteractiveAnimation(HWND hwnd);
void StartAnimation(HWND hwnd, AnimChannel channel, double target, uint32_t durationMs = 160, bool closeOnDone = false);
void RestartAnimation(HWND hwnd, AnimChannel channel, double target, uint32_t durationMs = 220);
double GetAnimationValue(HWND hwnd, AnimChannel channel, double fallback = 0.0);
void RemoveAnimationState(HWND hwnd);
void StartWindowReveal(HWND hwnd, uint32_t durationMs = 180);
void BeginAnimatedClose(HWND hwnd, uint32_t durationMs = 140);
COLORREF BlendColor(COLORREF from, COLORREF to, double amount);
double EaseInOut(double value);
double EaseOut(double value);
double ElasticOut(double value);
void StartListTransition();
LRESULT HandleListCustomDraw(LPARAM lParam);
void StopAnimationThread();
void ApplyGlassTitleBar(HWND hwnd);
void CancelMainWindowClose();
void FillInstalledFontFamilies();
std::wstring GetComboSelectedText(HWND combo);
void CommitComboSelectionNow(HWND combo);
void ConfigureEditMetrics(HWND hwnd);
void TriggerButtonFeedback(HWND hwnd, POINT origin);
void DrawButtonEffects(const DRAWITEMSTRUCT* draw, const RECT& buttonRc, COLORREF baseFill);
void DrawSoftDivider(HDC hdc, int left, int right, int y);
int ThemedMessageBox(HWND owner, const std::wstring& message, const std::wstring& title, bool yesNo);
bool DrawParticleToggleItem(const DRAWITEMSTRUCT* draw);

struct ThemedMenuItem {
    int command = 0;
    std::wstring text;
    bool separator = false;
};

struct ThemedMenuState {
    std::vector<ThemedMenuItem> items;
    int hover = -1;
    int selected = 0;
    int finalX = 0;
    int finalY = 0;
    int width = 0;
    int height = 0;
    bool done = false;
    bool closing = false;
};

static constexpr int THEMED_MENU_PAD_Y = 8;
static constexpr int THEMED_MENU_ITEM_H = 34;
static constexpr int THEMED_MENU_SEPARATOR_H = 13;
static constexpr int THEMED_MENU_SLIDE_PX = 5;

int ShowThemedPopupMenu(HWND button, const std::vector<ThemedMenuItem>& items);

void MarkDirty() {
    g_dirty = true;
    g_allowAutosaveOverwrite = true;
}

std::wstring GetText(HWND hwnd) {
    int len = GetWindowTextLengthW(hwnd);
    std::wstring text(len + 1, L'\0');
    GetWindowTextW(hwnd, text.data(), len + 1);
    text.resize(len);
    return text;
}

void SetText(HWND hwnd, const std::wstring& text) {
    SetWindowTextW(hwnd, text.c_str());
}

void ConfigureEditMetrics(HWND hwnd) {
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

uint64_t AnimationNowMs() {
    using namespace std::chrono;
    return (uint64_t)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

double EaseInOut(double value) {
    value = std::clamp(value, 0.0, 1.0);
    return value < 0.5
        ? 4.0 * value * value * value
        : 1.0 - std::pow(-2.0 * value + 2.0, 3.0) / 2.0;
}

double EaseOut(double value) {
    value = std::clamp(value, 0.0, 1.0);
    return 1.0 - std::pow(1.0 - value, 3.0);
}

double ElasticOut(double value) {
    value = std::clamp(value, 0.0, 1.0);
    if (value <= 0.0 || value >= 1.0) return value;
    constexpr double c4 = (2.0 * 3.14159265358979323846) / 3.0;
    return std::pow(2.0, -10.0 * value) * std::sin((value * 10.0 - 0.75) * c4) + 1.0;
}

COLORREF BlendColor(COLORREF from, COLORREF to, double amount) {
    amount = std::clamp(amount, 0.0, 1.0);
    auto mix = [&](int a, int b) {
        return (int)std::lround(a + (b - a) * amount);
    };
    return RGB(
        mix(GetRValue(from), GetRValue(to)),
        mix(GetGValue(from), GetGValue(to)),
        mix(GetBValue(from), GetBValue(to)));
}

void EnsureAnimationThread() {
    if (g_animationThread.joinable()) return;
    g_animationShutdown = false;
    g_animationThread = std::thread([] {
        while (!g_animationShutdown.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));

            std::vector<HWND> tickWindows;
            std::vector<HWND> doneWindows;
            uint64_t now = AnimationNowMs();
            {
                std::lock_guard<std::mutex> lock(g_animMutex);
                for (auto& hwndEntry : g_animations) {
                    bool changed = false;
                    for (auto& channelEntry : hwndEntry.second) {
                        AnimValue& anim = channelEntry.second;
                        if (!anim.active) continue;
                        double t = anim.durationMs == 0
                            ? 1.0
                            : (double)(now - anim.startMs) / (double)anim.durationMs;
                        if (t >= 1.0) {
                            anim.current = anim.target;
                            anim.active = false;
                            changed = true;
                            if (anim.closeOnDone) doneWindows.push_back(hwndEntry.first);
                        } else {
                            double eased = (channelEntry.first == (int)AnimChannel::Reveal || channelEntry.first == (int)AnimChannel::Close)
                                ? EaseOut(t)
                                : EaseInOut(t);
                            anim.current = anim.start + (anim.target - anim.start) * eased;
                            changed = true;
                        }
                    }
                    if (changed) tickWindows.push_back(hwndEntry.first);
                }
            }

            for (HWND hwnd : tickWindows) {
                PostMessageW(hwnd, WM_APP_ANIMATION_TICK, 0, 0);
            }
            for (HWND hwnd : doneWindows) {
                PostMessageW(hwnd, WM_APP_ANIMATION_DONE, 0, 0);
            }
        }
    });
}

void StopAnimationThread() {
    g_animationShutdown = true;
    if (g_animationThread.joinable()) g_animationThread.join();
}

void StartAnimation(HWND hwnd, AnimChannel channel, double target, uint32_t durationMs, bool closeOnDone) {
    if (!hwnd) return;
    EnsureAnimationThread();
    std::lock_guard<std::mutex> lock(g_animMutex);
    AnimValue& anim = g_animations[hwnd][(int)channel];
    target = std::clamp(target, 0.0, 1.0);
    if (std::abs(anim.target - target) <= 0.001 && (anim.active || std::abs(anim.current - target) <= 0.001)) {
        return;
    }
    anim.start = anim.current;
    anim.target = target;
    anim.startMs = AnimationNowMs();
    anim.durationMs = durationMs;
    anim.active = durationMs > 0 && std::abs(anim.target - anim.start) > 0.001;
    anim.closeOnDone = closeOnDone;
    if (!anim.active) anim.current = anim.target;
}

void RestartAnimation(HWND hwnd, AnimChannel channel, double target, uint32_t durationMs) {
    if (!hwnd) return;
    EnsureAnimationThread();
    std::lock_guard<std::mutex> lock(g_animMutex);
    AnimValue& anim = g_animations[hwnd][(int)channel];
    anim.current = 0.0;
    anim.start = 0.0;
    anim.target = std::clamp(target, 0.0, 1.0);
    anim.startMs = AnimationNowMs();
    anim.durationMs = durationMs;
    anim.active = durationMs > 0;
    anim.closeOnDone = false;
}

double GetAnimationValue(HWND hwnd, AnimChannel channel, double fallback) {
    if (!hwnd) return fallback;
    std::lock_guard<std::mutex> lock(g_animMutex);
    auto hwndIt = g_animations.find(hwnd);
    if (hwndIt == g_animations.end()) return fallback;
    auto channelIt = hwndIt->second.find((int)channel);
    if (channelIt == hwndIt->second.end()) return fallback;
    return channelIt->second.current;
}

void RemoveAnimationState(HWND hwnd) {
    if (!hwnd) return;
    std::lock_guard<std::mutex> lock(g_animMutex);
    g_animations.erase(hwnd);
}

void StartWindowReveal(HWND hwnd, uint32_t durationMs) {
    if (!hwnd) return;
    RestartAnimation(hwnd, AnimChannel::Reveal, 1.0, durationMs);
}

void BeginAnimatedClose(HWND hwnd, uint32_t durationMs) {
    (void)durationMs;
    if (!hwnd) return;
    DestroyWindow(hwnd);
}

void CancelMainWindowClose() {
}

void ApplyGlassTitleBar(HWND hwnd) {
    if (!hwnd) return;
    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

    COLORREF caption = RGB(0, 0, 0);
    COLORREF text = RGB(255, 255, 255);
    DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &caption, sizeof(caption));
    DwmSetWindowAttribute(hwnd, DWMWA_TEXT_COLOR, &text, sizeof(text));

    int backdrop = 2; // DWMSBT_MAINWINDOW / Mica on supported Windows builds.
    DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop, sizeof(backdrop));

    MARGINS margins{0, 0, 1, 0};
    DwmExtendFrameIntoClientArea(hwnd, &margins);
}

void StartListTransition() {
    if (!g_list) return;
    RestartAnimation(g_list, AnimChannel::ListReveal, 1.0, 260);
    RedrawWindow(g_list, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME);
}

LRESULT HandleListCustomDraw(LPARAM lParam) {
    auto* draw = reinterpret_cast<NMLVCUSTOMDRAW*>(lParam);
    if (!draw) return CDRF_DODEFAULT;

    if (draw->nmcd.dwDrawStage == CDDS_PREPAINT) {
        return CDRF_NOTIFYITEMDRAW;
    }
    if (draw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
        int item = (int)draw->nmcd.dwItemSpec;
        bool selected = (ListView_GetItemState(g_list, item, LVIS_SELECTED) & LVIS_SELECTED) != 0;
        double reveal = GetAnimationValue(g_list, AnimChannel::ListReveal, 1.0);
        double rowProgress = std::clamp(reveal * 1.25 - item * 0.035, 0.0, 1.0);

        COLORREF rowBase = selected
            ? RGB(42, 42, 42)
            : (item % 2 == 0 ? COLOR_PANEL : RGB(14, 14, 14));
        draw->clrTextBk = BlendColor(COLOR_BG, rowBase, rowProgress);
        draw->clrText = BlendColor(COLOR_MUTED, COLOR_TEXT, rowProgress);
        return CDRF_NEWFONT;
    }
    return CDRF_DODEFAULT;
}

void SetStaticTextClean(HWND hwnd, const std::wstring& text) {
    if (!hwnd) return;
    HWND parent = GetParent(hwnd);
    RECT rc{};
    if (parent && GetWindowRect(hwnd, &rc)) {
        MapWindowPoints(nullptr, parent, reinterpret_cast<POINT*>(&rc), 2);
        InvalidateRect(parent, &rc, TRUE);
    }
    SetWindowTextW(hwnd, text.c_str());
    InvalidateRect(hwnd, nullptr, TRUE);
    UpdateWindow(hwnd);
}

std::wstring LowerText(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t ch) {
        return (wchar_t)towlower(ch);
    });
    return value;
}

bool ContainsText(const std::wstring& haystack, const std::wstring& needle) {
    return LowerText(haystack).find(needle) != std::wstring::npos;
}

bool RecordMatchesFilter(const AttendanceRecord& record) {
    if (g_filterText.empty()) return true;
    return ContainsText(record.dateTime, g_filterText)
        || ContainsText(record.name, g_filterText)
        || ContainsText(record.status, g_filterText)
        || ContainsText(record.other, g_filterText);
}

int VisibleToRecordIndex(int visibleIndex) {
    if (visibleIndex < 0 || visibleIndex >= (int)g_visibleRows.size()) return -1;
    int recordIndex = g_visibleRows[visibleIndex];
    return recordIndex >= 0 && recordIndex < (int)g_records.size() ? recordIndex : -1;
}

int RecordToVisibleIndex(int recordIndex) {
    for (int i = 0; i < (int)g_visibleRows.size(); ++i) {
        if (g_visibleRows[i] == recordIndex) return i;
    }
    return -1;
}

LRESULT CALLBACK EditShortcutProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR) {
    if (msg == WM_NCDESTROY) {
        RemoveWindowSubclass(hwnd, EditShortcutProc, 1);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }
    if (msg == WM_KEYDOWN && wParam == 'A' && (GetKeyState(VK_CONTROL) & 0x8000)) {
        SendMessageW(hwnd, EM_SETSEL, 0, -1);
        return 0;
    }
    if (msg == WM_CHAR && wParam == VK_RETURN) {
        return 0;
    }
    if (msg == WM_CHAR && wParam == 1) {
        return 0;
    }
    if (msg == WM_SIZE || msg == WM_WINDOWPOSCHANGED) {
        LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
        ConfigureEditMetrics(hwnd);
        return result;
    }
    if (msg == WM_SETFONT) {
        LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
        ConfigureEditMetrics(hwnd);
        return result;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void EnableEditShortcuts(HWND hwnd) {
    SetWindowSubclass(hwnd, EditShortcutProc, 1, 0);
    ConfigureEditMetrics(hwnd);
}

LRESULT CALLBACK WheelForwardProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR) {
    if (msg == WM_NCDESTROY) {
        RemoveWindowSubclass(hwnd, WheelForwardProc, 2);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }
    if (msg == WM_MOUSEWHEEL && g_hwnd) {
        ScrollMainWindow(g_hwnd, SB_VERT, 0, GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;
    }
    if (msg == WM_MOUSEHWHEEL && g_hwnd) {
        ScrollMainWindow(g_hwnd, SB_HORZ, 0, GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void EnableMouseWheelForward(HWND hwnd) {
    SetWindowSubclass(hwnd, WheelForwardProc, 2, 0);
}

void RedrawAnimatedControl(HWND hwnd) {
    RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_UPDATENOW);
}

void DrawAnimatedEditFrame(HWND hwnd) {
    wchar_t className[32]{};
    GetClassNameW(hwnd, className, 32);
    if (lstrcmpiW(className, L"Edit") != 0) return;

    HDC hdc = GetWindowDC(hwnd);
    if (!hdc) return;
    RECT rc{};
    GetWindowRect(hwnd, &rc);
    OffsetRect(&rc, -rc.left, -rc.top);

    double hover = GetAnimationValue(hwnd, AnimChannel::Hover, GetFocus() == hwnd ? 1.0 : 0.0);
    COLORREF border = BlendColor(RGB(58, 58, 58), COLOR_ACCENT, hover);
    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
    ReleaseDC(hwnd, hdc);
}

LRESULT CALLBACK InteractiveControlProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR) {
    switch (msg) {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, InteractiveControlProc, 7);
        RemoveAnimationState(hwnd);
        g_buttonEffects.erase(hwnd);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    case WM_MOUSEMOVE: {
        TRACKMOUSEEVENT track{};
        track.cbSize = sizeof(track);
        track.dwFlags = TME_LEAVE;
        track.hwndTrack = hwnd;
        TrackMouseEvent(&track);
        StartAnimation(hwnd, AnimChannel::Hover, 1.0, 140);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }
    case WM_MOUSELEAVE:
        StartAnimation(hwnd, AnimChannel::Hover, GetFocus() == hwnd ? 1.0 : 0.0, 180);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    case WM_LBUTTONDOWN: {
        CancelMainWindowClose();
        POINT origin{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        TriggerButtonFeedback(hwnd, origin);
        StartAnimation(hwnd, AnimChannel::Press, 1.0, 70);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }
    case WM_LBUTTONUP:
    case WM_CANCELMODE:
    case WM_CAPTURECHANGED:
        StartAnimation(hwnd, AnimChannel::Press, 0.0, 180);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    case WM_SETFOCUS:
        StartAnimation(hwnd, AnimChannel::Hover, 1.0, 150);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    case WM_KILLFOCUS:
        StartAnimation(hwnd, AnimChannel::Hover, 0.0, 180);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    case WM_NCPAINT: {
        LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
        DrawAnimatedEditFrame(hwnd);
        return result;
    }
    case WM_APP_ANIMATION_TICK:
        RedrawAnimatedControl(hwnd);
        return 0;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void EnableInteractiveAnimation(HWND hwnd) {
    if (!hwnd) return;
    SetWindowSubclass(hwnd, InteractiveControlProc, 7, 0);
}

int MeasureLongestTextLinePx(HWND owner, const std::wstring& text) {
    HDC hdc = GetDC(owner ? owner : GetDesktopWindow());
    HGDIOBJ oldFont = g_font ? SelectObject(hdc, g_font) : nullptr;
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

SIZE MeasureWrappedTextPx(HWND owner, const std::wstring& text, int width) {
    HDC hdc = GetDC(owner ? owner : GetDesktopWindow());
    HGDIOBJ oldFont = g_font ? SelectObject(hdc, g_font) : nullptr;
    RECT rc{0, 0, width, 0};
    DrawTextW(hdc, text.c_str(), -1, &rc, DT_LEFT | DT_NOPREFIX | DT_WORDBREAK | DT_CALCRECT);
    if (oldFont) SelectObject(hdc, oldFont);
    ReleaseDC(owner ? owner : GetDesktopWindow(), hdc);
    return SIZE{std::max(1, (int)(rc.right - rc.left)), std::max(1, (int)(rc.bottom - rc.top))};
}

SIZE CalculateMessageDialogWindowSize(HWND owner, const std::wstring& message) {
    int longest = MeasureLongestTextLinePx(owner, message);
    int textW = std::clamp(longest + 12, 360, 760);
    SIZE textSize = MeasureWrappedTextPx(owner, message, textW);
    int clientW = std::clamp(textW + 48, 470, 840);
    int clientH = std::clamp((int)textSize.cy + 108, 230, 620);

    RECT windowRc{0, 0, clientW, clientH};
    AdjustWindowRectEx(&windowRc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE, WS_EX_DLGMODALFRAME);
    return SIZE{(LONG)(windowRc.right - windowRc.left), (LONG)(windowRc.bottom - windowRc.top)};
}

void ResizeMessageDialog(HWND hwnd) {
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

LRESULT CALLBACK MessageDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
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
        SetTextColor(hdc, COLOR_TEXT);
        SetBkMode(hdc, TRANSPARENT);
        return (LRESULT)GetStockObject(HOLLOW_BRUSH);
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

int ThemedMessageBox(HWND owner, const std::wstring& message, const std::wstring& title, bool yesNo) {
    HINSTANCE instance = GetModuleHandleW(nullptr);
    const wchar_t* className = L"AttendanceMessageDialog";
    static bool registered = false;
    if (!registered) {
        WNDCLASSW wc{};
        wc.style = CS_DROPSHADOW;
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
    HWND parent = owner ? owner : g_hwnd;
    if (parent) EnableWindow(parent, FALSE);
    SIZE dialogSize = CalculateMessageDialogWindowSize(parent, message);
    HWND dialog = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        className,
        title.c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, dialogSize.cx, dialogSize.cy,
        parent, nullptr, instance, &state
    );
    if (!dialog) {
        if (parent) EnableWindow(parent, TRUE);
        return state.result;
    }

    BOOL dark = TRUE;
    DwmSetWindowAttribute(dialog, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
    ApplyGlassTitleBar(dialog);
    ShowWindow(dialog, SW_SHOW);
    UpdateWindow(dialog);

    MSG msg{};
    while (!state.done && IsWindow(dialog) && GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    if (IsWindow(dialog)) DestroyWindow(dialog);
    if (parent) {
        EnableWindow(parent, TRUE);
        SetForegroundWindow(parent);
    }
    return state.result;
}

bool ConfirmDiscardUnsaved(const std::wstring& actionText) {
    if (!g_dirty) return true;
    std::wstring msg = Tr(L"You have unsaved changes. Continue and discard them?", L"\u5f53\u524d\u6709\u672a\u4fdd\u5b58\u7684\u4fee\u6539\u3002\u662f\u5426\u7ee7\u7eed\u5e76\u653e\u5f03\u8fd9\u4e9b\u4fee\u6539\uff1f");
    if (!actionText.empty()) msg += L"\n\n" + actionText;
    std::wstring title = Tr(L"Unsaved Changes", L"\u672a\u4fdd\u5b58\u7684\u4fee\u6539");
    return ThemedMessageBox(g_hwnd, msg, title, true) == IDYES;
}

LRESULT CALLBACK InputDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* state = reinterpret_cast<InputDialogState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE: {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        state = reinterpret_cast<InputDialogState*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)state);
        MakeSettingsControl(hwnd, L"STATIC", state->prompt.c_str(), 0, 0);
        MakeSettingsControl(hwnd, L"EDIT", state->value.c_str(), WS_TABSTOP | ES_AUTOHSCROLL | WS_BORDER, IDC_INPUT_EDIT);
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
        SetTextColor(hdc, COLOR_TEXT);
        SetBkMode(hdc, TRANSPARENT);
        return (LRESULT)GetStockObject(HOLLOW_BRUSH);
    }
    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, COLOR_TEXT);
        SetBkColor(hdc, COLOR_INPUT);
        return (LRESULT)g_inputBrush;
    }
    case WM_APP_ANIMATION_TICK:
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

bool PromptText(const std::wstring& title, const std::wstring& prompt, std::wstring& value) {
    HINSTANCE instance = GetModuleHandleW(nullptr);
    const wchar_t* className = L"AttendanceInputDialog";
    static bool registered = false;
    if (!registered) {
        WNDCLASSW wc{};
        wc.style = CS_DROPSHADOW;
        wc.lpfnWndProc = InputDialogProc;
        wc.hInstance = instance;
        wc.lpszClassName = className;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON));
        RegisterClassW(&wc);
        registered = true;
    }

    InputDialogState state{title, prompt, value, false};
    EnableWindow(g_hwnd, FALSE);
    HWND dialog = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        className,
        title.c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 430, 190,
        g_hwnd, nullptr, instance, &state
    );
    ShowWindow(dialog, SW_SHOW);
    UpdateWindow(dialog);

    MSG msg{};
    while (IsWindow(dialog) && GetMessageW(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessageW(dialog, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    EnableWindow(g_hwnd, TRUE);
    SetForegroundWindow(g_hwnd);
    if (state.accepted) value = state.value;
    return state.accepted;
}

const wchar_t* LanguageName(UiLanguage language) {
    switch (language) {
    case UiLanguage::ChineseSimplified: return L"\u7b80\u4f53\u4e2d\u6587";
    case UiLanguage::Maltese: return L"Malti";
    case UiLanguage::Japanese: return L"\u65e5\u672c\u8a9e";
    case UiLanguage::French: return L"Fran\u00e7ais";
    case UiLanguage::German: return L"Deutsch";
    case UiLanguage::Russian: return L"\u0420\u0443\u0441\u0441\u043a\u0438\u0439";
    case UiLanguage::ChineseTraditional: return L"\u7e41\u9ad4\u4e2d\u6587\uff08\u53f0\u7063\uff09";
    case UiLanguage::Spanish: return L"Espa\u00f1ol";
    case UiLanguage::Italian: return L"Italiano";
    case UiLanguage::Mongolian: return L"\u041c\u043e\u043d\u0433\u043e\u043b";
    case UiLanguage::Esperanto: return L"Esperanto";
    case UiLanguage::ClassicalChinese: return L"\u6587\u8a00\u6587";
    case UiLanguage::Thai: return L"\u0e44\u0e17\u0e22";
    case UiLanguage::Filipino: return L"Filipino";
    case UiLanguage::Turkish: return L"T\u00fcrk\u00e7e";
    case UiLanguage::Lithuanian: return L"Lietuvi\u0173";
    case UiLanguage::Norwegian: return L"Norsk";
    case UiLanguage::Vietnamese: return L"Ti\u1ebfng Vi\u1ec7t";
    case UiLanguage::ChineseTraditionalHongKong: return L"\u7e41\u9ad4\u4e2d\u6587\uff08\u9999\u6e2f\u7cb5\u8a9e\uff09";
    default: return L"English";
    }
}

bool TranslateAdditionalLanguage(const std::wstring& key, std::wstring& out) {
    if (g_language == UiLanguage::Norwegian || g_language == UiLanguage::Vietnamese || g_language == UiLanguage::ChineseTraditionalHongKong) {
        struct V12Entry {
            const wchar_t* key;
            const wchar_t* no;
            const wchar_t* vi;
            const wchar_t* zhhk;
        };
        static const V12Entry entries[] = {
            {L"AttendanceApp - .attd Roll Call Manager", L"AttendanceApp - .attd oppropsbehandler", L"AttendanceApp - Tr\u00ecnh qu\u1ea3n l\u00fd \u0111i\u1ec3m danh .attd", L"AttendanceApp - .attd \u9ede\u540d\u7ba1\u7406"},
            {L"Attendance Manager", L"Frav\u00e6rsbehandler", L"Qu\u1ea3n l\u00fd \u0111i\u1ec3m danh", L"\u9ede\u540d\u7ba1\u7406"},
            {L"Create, edit, export, save, import, and batch clean .attd roll calls.", L"Opprett, rediger, eksporter, lagre, importer og rydd .attd-opprop.", L"T\u1ea1o, s\u1eeda, xu\u1ea5t, l\u01b0u, nh\u1eadp v\u00e0 d\u1ecdn d\u1eb9p \u0111i\u1ec3m danh .attd.", L"\u5efa\u7acb\u3001\u7de8\u8f2f\u3001\u532f\u51fa\u3001\u5132\u5b58\u3001\u532f\u5165\u540c\u6279\u91cf\u6e05\u7406 .attd \u9ede\u540d\u8a18\u9304\u3002"},
            {L"Date/Time", L"Dato/tid", L"Ng\u00e0y/gi\u1edd", L"\u65e5\u671f/\u6642\u9593"},
            {L"Date / Time", L"Dato / tid", L"Ng\u00e0y / gi\u1edd", L"\u65e5\u671f / \u6642\u9593"},
            {L"Name", L"Navn", L"T\u00ean", L"\u59d3\u540d"},
            {L"Status", L"Status", L"Tr\u1ea1ng th\u00e1i", L"\u72c0\u614b"},
            {L"Other", L"Annet", L"Kh\u00e1c", L"\u5176\u4ed6"},
            {L"Present", L"Tilstede", L"C\u00f3 m\u1eb7t", L"\u51fa\u5e2d"},
            {L"Absent", L"Frav\u00e6rende", L"V\u1eafng", L"\u7f3a\u5e2d"},
            {L"Late", L"Sen", L"\u0110i tr\u1ec5", L"\u9072\u5230"},
            {L"Search", L"S\u00f8k", L"T\u00ecm ki\u1ebfm", L"\u641c\u5c0b"},
            {L"Clear Filter", L"T\u00f8m filter", L"X\u00f3a b\u1ed9 l\u1ecdc", L"\u6e05\u9664\u7be9\u9078"},
            {L"Total", L"Totalt", L"T\u1ed5ng", L"\u7e3d\u6578"},
            {L"Attendance", L"Oppm\u00f8te", L"Chuy\u00ean c\u1ea7n", L"\u51fa\u52e4"},
            {L"Absent/Late", L"Frav\u00e6r/sen", L"V\u1eafng/tr\u1ec5", L"\u7f3a\u5e2d/\u9072\u5230"},
            {L"Save .attd", L"Lagre .attd", L"L\u01b0u .attd", L"\u5132\u5b58 .attd"},
            {L"Import .attd", L"Importer .attd", L"Nh\u1eadp .attd", L"\u532f\u5165 .attd"},
            {L"Export CSV", L"Eksporter CSV", L"Xu\u1ea5t CSV", L"\u532f\u51fa CSV"},
            {L"Tools", L"Verkt\u00f8y", L"C\u00f4ng c\u1ee5", L"\u5de5\u5177"},
            {L"Courses", L"Kurs", L"Kh\u00f3a/l\u1edbp", L"\u8ab2\u7a0b"},
            {L"Settings", L"Innstillinger", L"C\u00e0i \u0111\u1eb7t", L"\u8a2d\u5b9a"},
            {L"Language", L"Spr\u00e5k", L"Ng\u00f4n ng\u1eef", L"\u8a9e\u8a00"},
            {L"Style", L"Stil", L"Giao di\u1ec7n", L"\u98a8\u683c"},
            {L"Interface Font", L"Grensesnittskrift", L"Ph\u00f4ng giao di\u1ec7n", L"\u4ecb\u9762\u5b57\u578b"},
            {L"Apply", L"Bruk", L"\u00c1p d\u1ee5ng", L"\u5957\u7528"},
            {L"Close", L"Lukk", L"\u0110\u00f3ng", L"\u95dc\u9589"},
            {L"OK", L"OK", L"OK", L"\u78ba\u5b9a"},
            {L"Cancel", L"Avbryt", L"H\u1ee7y", L"\u53d6\u6d88"},
            {L"Import student roster (CSV)", L"Importer elevliste (CSV)", L"Nh\u1eadp danh s\u00e1ch h\u1ecdc sinh (CSV)", L"\u532f\u5165\u5b78\u751f\u540d\u55ae (CSV)"},
            {L"Manage students", L"Administrer elever", L"Qu\u1ea3n l\u00fd h\u1ecdc sinh", L"\u7ba1\u7406\u5b78\u751f"},
            {L"Add student to roster", L"Legg elev til liste", L"Th\u00eam h\u1ecdc sinh v\u00e0o danh s\u00e1ch", L"\u52a0\u5b78\u751f\u5165\u540d\u55ae"},
            {L"Remove student from roster", L"Fjern elev fra liste", L"X\u00f3a h\u1ecdc sinh kh\u1ecfi danh s\u00e1ch", L"\u5f9e\u540d\u55ae\u79fb\u9664\u5b78\u751f"},
            {L"Create records from roster", L"Opprett poster fra liste", L"T\u1ea1o b\u1ea3n ghi t\u1eeb danh s\u00e1ch", L"\u7528\u540d\u55ae\u5efa\u7acb\u9ede\u540d\u8a18\u9304"},
            {L"Course details", L"Kursdetaljer", L"Chi ti\u1ebft kh\u00f3a/l\u1edbp", L"\u8ab2\u7a0b\u8cc7\u6599"},
            {L"Teacher/owner:", L"L\u00e6rer/ansvarlig:", L"Gi\u00e1o vi\u00ean/ph\u1ee5 tr\u00e1ch:", L"\u8001\u5e2b/\u8ca0\u8cac\u4eba\uff1a"},
            {L"Location:", L"Sted:", L"\u0110\u1ecba \u0111i\u1ec3m:", L"\u5730\u9ede\uff1a"},
            {L"Course notes:", L"Kursnotater:", L"Ghi ch\u00fa kh\u00f3a/l\u1edbp:", L"\u8ab2\u7a0b\u5099\u8a3b\uff1a"},
            {L"Student name:", L"Elevnavn:", L"T\u00ean h\u1ecdc sinh:", L"\u5b78\u751f\u59d3\u540d\uff1a"},
            {L"Student added.", L"Elev lagt til.", L"\u0110\u00e3 th\u00eam h\u1ecdc sinh.", L"\u5df2\u52a0\u5165\u5b78\u751f\u3002"},
            {L"Student removed.", L"Elev fjernet.", L"\u0110\u00e3 x\u00f3a h\u1ecdc sinh.", L"\u5df2\u79fb\u9664\u5b78\u751f\u3002"},
            {L"No students in the roster.", L"Ingen elever i listen.", L"Ch\u01b0a c\u00f3 h\u1ecdc sinh trong danh s\u00e1ch.", L"\u540d\u55ae\u672a\u6709\u5b78\u751f\u3002"},
            {L"Roster records created.", L"Listeposter opprettet.", L"\u0110\u00e3 t\u1ea1o b\u1ea3n ghi t\u1eeb danh s\u00e1ch.", L"\u5df2\u7528\u540d\u55ae\u5efa\u7acb\u8a18\u9304\u3002"},
            {L"Statistics summary", L"Statistikksammendrag", L"T\u00f3m t\u1eaft th\u1ed1ng k\u00ea", L"\u7d71\u8a08\u6458\u8981"},
            {L"Top absent/late students", L"Flest frav\u00e6r/sene elever", L"H\u1ecdc sinh v\u1eafng/tr\u1ec5 nhi\u1ec1u nh\u1ea5t", L"\u7f3a\u5e2d/\u9072\u5230\u6700\u591a\u5b78\u751f"},
            {L"Backup now", L"Sikkerhetskopier n\u00e5", L"Sao l\u01b0u ngay", L"\u7acb\u5373\u5099\u4efd"},
            {L"Restore latest backup", L"Gjenopprett siste kopi", L"Kh\u00f4i ph\u1ee5c b\u1ea3n sao l\u01b0u m\u1edbi nh\u1ea5t", L"\u9084\u539f\u6700\u65b0\u5099\u4efd"},
            {L"Open recent file", L"\u00c5pne nylig fil", L"M\u1edf t\u1ec7p g\u1ea7n \u0111\u00e2y", L"\u958b\u555f\u6700\u8fd1\u6a94\u6848"},
            {L"Backup created:", L"Sikkerhetskopi opprettet:", L"\u0110\u00e3 t\u1ea1o sao l\u01b0u:", L"\u5df2\u5efa\u7acb\u5099\u4efd\uff1a"},
            {L"No backup file was found.", L"Ingen sikkerhetskopi funnet.", L"Kh\u00f4ng t\u00ecm th\u1ea5y b\u1ea3n sao l\u01b0u.", L"\u627e\u4e0d\u5230\u5099\u4efd\u6a94\u3002"},
            {L"No recent file was found.", L"Ingen nylig fil funnet.", L"Kh\u00f4ng t\u00ecm th\u1ea5y t\u1ec7p g\u1ea7n \u0111\u00e2y.", L"\u627e\u4e0d\u5230\u6700\u8fd1\u6a94\u6848\u3002"},
            {L"Set default save folder", L"Velg standard lagringsmappe", L"\u0110\u1eb7t th\u01b0 m\u1ee5c l\u01b0u m\u1eb7c \u0111\u1ecbnh", L"\u8a2d\u5b9a\u9810\u8a2d\u5132\u5b58\u8cc7\u6599\u593e"},
            {L"Default save folder updated:", L"Standard lagringsmappe oppdatert:", L"\u0110\u00e3 c\u1eadp nh\u1eadt th\u01b0 m\u1ee5c l\u01b0u m\u1eb7c \u0111\u1ecbnh:", L"\u9810\u8a2d\u5132\u5b58\u8cc7\u6599\u593e\u5df2\u66f4\u65b0\uff1a"}
        };
        for (const auto& entry : entries) {
            if (key == entry.key) {
                if (g_language == UiLanguage::Norwegian) out = entry.no;
                else if (g_language == UiLanguage::Vietnamese) out = entry.vi;
                else out = entry.zhhk;
                return true;
            }
        }
        if (g_language == UiLanguage::ChineseTraditionalHongKong) {
            UiLanguage saved = g_language;
            g_language = UiLanguage::ChineseTraditional;
            out = Tr(key.c_str(), L"");
            g_language = saved;
            return out != key;
        }
        return false;
    }

    if (g_language < UiLanguage::Italian) return false;

    if (g_language == UiLanguage::ClassicalChinese) {
        struct ClassicalEntry {
            const wchar_t* key;
            const wchar_t* text;
        };
        static const ClassicalEntry classical[] = {
            {L"AttendanceApp - .attd Roll Call Manager", L"AttendanceApp \u00b7 \u9ede\u540d\u7c3f"},
            {L"Attendance Manager", L"\u9ede\u540d\u53f8"},
            {L"Create, edit, export, save, import, and batch clean .attd roll calls.", L"\u9020\u7c3f\u3001\u6821\u9304\u3001\u532f\u51fa\u3001\u85cf\u6a94\u3001\u8f09\u5165\uff0c\u7686\u53ef\u70ba\u4e5f\u3002"},
            {L"Tip: double-click a row to edit. Ctrl/Shift supports multi-select.", L"\u6309\u884c\u518d\u4e09\uff0c\u5247\u53ef\u4fee\uff1bCtrl/Shift\uff0c\u53ef\u517c\u64c7\u3002"},
            {L"Date/Time", L"\u65e5\u8fb0"},
            {L"Date / Time", L"\u65e5 / \u6642"},
            {L"Name", L"\u59d3\u540d"},
            {L"Other", L"\u5225\u8a18"},
            {L"Status", L"\u72c0"},
            {L"Search", L"\u8993"},
            {L"Clear Filter", L"\u53bb\u7be9"},
            {L"Present", L"\u5728"},
            {L"Absent", L"\u95d5"},
            {L"Late", L"\u9072"},
            {L"Total", L"\u51e1"},
            {L"Showing", L"\u898b"},
            {L"Attendance", L"\u51fa\u52e4"},
            {L"Absent/Late", L"\u95d5/\u9072"},
            {L"Attendance Sheet", L"\u9ede\u540d\u7c3f"},
            {L"Update Selected", L"\u66f4\u6240\u64c7"},
            {L"Edit Selected", L"\u4fee\u6240\u64c7"},
            {L"Mark All Present", L"\u54b8\u8a18\u5176\u5728"},
            {L"Create New", L"\u65b0\u7acb"},
            {L"Delete Options", L"\u522a\u6cd5"},
            {L"Delete", L"\u522a"},
            {L"selected record(s)?", L"\u689d\u6240\u64c7\u4e4b\u9304\u4e4e\uff1f"},
            {L"Delete Selected", L"\u522a\u6240\u64c7"},
            {L"Delete selected records", L"\u522a\u6240\u64c7\u4e4b\u9304"},
            {L"Delete all Absent records", L"\u522a\u8af8\u95d5\u9304"},
            {L"Delete all Late records", L"\u522a\u8af8\u9072\u9304"},
            {L"Clear all records", L"\u6e05\u8af8\u9304"},
            {L"Delete all", L"\u76e1\u522a"},
            {L"records?", L"\u9304\u4e4e\uff1f"},
            {L"Batch Delete", L"\u6279\u522a"},
            {L"Clear every attendance record in this sheet?", L"\u6b32\u76e1\u6e05\u6b64\u7c3f\u4e4b\u9ede\u540d\u9304\u4e4e\uff1f"},
            {L"Clear All", L"\u76e1\u6e05"},
            {L"Save .attd", L"\u85cf .attd"},
            {L"Import .attd", L"\u8f09 .attd"},
            {L"Export CSV", L"\u532f\u51fa CSV"},
            {L"Settings", L"\u8a2d\u5b9a"},
            {L"Courses", L"\u8ab2\u7a0b"},
            {L"Tools", L"\u5668\u7528"},
            {L"OK", L"\u53ef"},
            {L"Cancel", L"\u7f77"},
            {L"Add course/class", L"\u589e\u8ab2/\u73ed"},
            {L"Rename current course/class", L"\u6613\u6b64\u8ab2/\u73ed\u540d"},
            {L"Delete current course/class", L"\u522a\u6b64\u8ab2/\u73ed"},
            {L"Add Course/Class", L"\u589e\u8ab2/\u73ed"},
            {L"Course or class name:", L"\u8ab2\u6216\u73ed\u4e4b\u540d\uff1a"},
            {L"Rename Course/Class", L"\u6613\u8ab2/\u73ed\u540d"},
            {L"New course or class name:", L"\u65b0\u8ab2\u6216\u73ed\u4e4b\u540d\uff1a"},
            {L"Import student roster (CSV)", L"\u8f09\u5f1f\u5b50\u540d\u518a (CSV)"},
            {L"Print / export PDF", L"\u5370 / \u532f\u51fa PDF"},
            {L"Export PowerPoint (.pptx)", L"\u532f\u51fa PowerPoint (.pptx)"},
            {L"Statistics chart", L"\u8a08\u6578\u5716"},
            {L"Undo", L"\u5fa9\u524d"},
            {L"Redo", L"\u518d\u884c"},
            {L"Keyboard shortcuts", L"\u6377\u9375"},
            {L"Export database mirror", L"\u532f\u51fa\u5eab\u93e1"},
            {L"Open autosave", L"\u555f\u81ea\u85cf"},
            {L"At least one course must remain.", L"\u81f3\u5c11\u7576\u7559\u4e00\u8ab2\u3002"},
            {L"Delete the current course and its records?", L"\u6b32\u522a\u6b64\u8ab2\u53ca\u5176\u9304\u4e4e\uff1f"},
            {L"Delete Course", L"\u522a\u8ab2"},
            {L"Nothing to undo.", L"\u7121\u53ef\u5fa9\u8005\u3002"},
            {L"Nothing to redo.", L"\u7121\u53ef\u518d\u884c\u8005\u3002"},
            {L"Keyboard Shortcuts", L"\u6377\u9375"},
            {L"Statistics Chart", L"\u8a08\u6578\u5716"},
            {L"Print / Save as PDF", L"\u5370 / \u53e6\u85cf PDF"},
            {L"New attendance sheet name:", L"\u65b0\u9ede\u540d\u7c3f\u4e4b\u540d\uff1a"},
            {L"New Attendance", L"\u65b0\u9ede\u540d"},
            {L"Interface Settings", L"\u4ecb\u9762\u8af8\u8a2d"},
            {L"Language", L"\u8a9e"},
            {L"Style", L"\u8c8c"},
            {L"Interface Font", L"\u5b57\u9ad4"},
            {L"Apply", L"\u884c\u4e4b"},
            {L"Close", L"\u9589"},
            {L"Reset All Settings", L"\u76e1\u5fa9\u521d\u8a2d"},
            {L"Imported successfully.", L"\u5999\u54c9\uff0c\u8f09\u5165\u5df2\u6210\uff01"},
            {L"Saved successfully.", L"\u5584\u54c9\uff0c\u6a94\u5df2\u85cf\uff01"},
            {L"CSV exported successfully.", L"\u5999\u54c9\uff0cCSV \u5df2\u532f\u51fa\uff01"},
            {L"PowerPoint exported successfully.", L"\u5999\u54c9\uff0c\u6f14\u793a\u7a3f\u5df2\u6210\uff01"},
            {L"Could not export the PowerPoint file.", L"\u566b\uff0c\u6f14\u793a\u7a3f\u672a\u80fd\u532f\u51fa\u3002"},
            {L"Please enter a date and time.", L"\u8acb\u66f8\u65e5\u8fb0\u3002"},
            {L"Please enter a name.", L"\u8acb\u66f8\u59d3\u540d\u3002"},
            {L"Please fill the Other field.", L"\u8acb\u88dc\u5225\u8a18\u4e4b\u6b04\u3002"},
            {L"Please select a record to edit.", L"\u8acb\u64c7\u4e00\u9304\u800c\u4fee\u4e4b\u3002"},
            {L"There are no records to mark.", L"\u7121\u9304\u53ef\u8a18\u3002"},
            {L"Mark every record as Present?", L"\u6b32\u4ee5\u8af8\u9304\u54b8\u8a18\u70ba\u5728\u4e4e\uff1f"},
            {L"All Present", L"\u54b8\u5728"},
            {L"Please select one or more records to delete.", L"\u8acb\u64c7\u4e00\u9304\u6216\u6578\u9304\u800c\u522a\u4e4b\u3002"},
            {L"No matching records found.", L"\u89d3\u4e4b\u4e0d\u5f97\u3002"},
            {L"There are no records to clear.", L"\u7121\u9304\u53ef\u6e05\u3002"},
            {L"There are no records to export.", L"\u7121\u9304\u53ef\u532f\u51fa\u3002"},
            {L"Could not export the CSV file.", L"\u566b\uff0cCSV \u672a\u80fd\u532f\u51fa\u3002"},
            {L"Could not save the file.", L"\u566b\uff0c\u6a94\u672a\u80fd\u85cf\u3002"},
            {L"Could not open the file.", L"\u566b\uff0c\u6a94\u672a\u80fd\u555f\u3002"},
            {L"This .attd file could not be decoded.", L"\u566b\uff0c\u6b64 .attd \u4e0d\u53ef\u89e3\u3002"}
        };

        for (const auto& entry : classical) {
            if (key == entry.key) {
                out = entry.text;
                return true;
            }
        }
    }

    struct Entry {
        const wchar_t* key;
        const wchar_t* it;
        const wchar_t* mn;
        const wchar_t* eo;
        const wchar_t* lzh;
        const wchar_t* th;
        const wchar_t* fil;
        const wchar_t* tr;
        const wchar_t* lt;
    };

    static const Entry entries[] = {
        {L"AttendanceApp - .attd Roll Call Manager", L"AttendanceApp - Gestore appello .attd", L"AttendanceApp - .attd \u0438\u0440\u0446\u0438\u0439\u043d \u043c\u0435\u043d\u0435\u0436\u0435\u0440", L"AttendanceApp - .attd \u0109eestadministrilo", L"AttendanceApp - \u9ede\u540d\u7c3f", L"AttendanceApp - \u0e15\u0e31\u0e27\u0e08\u0e31\u0e14\u0e01\u0e32\u0e23\u0e40\u0e0a\u0e47\u0e01\u0e0a\u0e37\u0e48\u0e2d .attd", L"AttendanceApp - tagapamahala ng roll call .attd", L"AttendanceApp - .attd yoklama y\u00f6neticisi", L"AttendanceApp - .attd lankomumo valdiklis"},
        {L"Attendance Manager", L"Gestore presenze", L"\u0418\u0440\u0446\u0438\u0439\u043d \u043c\u0435\u043d\u0435\u0436\u0435\u0440", L"\u0108eestadministrilo", L"\u9ede\u540d\u7ba1\u7406", L"\u0e08\u0e31\u0e14\u0e01\u0e32\u0e23\u0e01\u0e32\u0e23\u0e40\u0e0a\u0e47\u0e01\u0e0a\u0e37\u0e48\u0e2d", L"Tagapamahala ng attendance", L"Yoklama y\u00f6neticisi", L"Lankomumo valdiklis"},
        {L"Create, edit, export, save, import, and batch clean .attd roll calls.", L"Crea, modifica, esporta, salva, importa e pulisci appelli .attd.", L".attd \u0438\u0440\u0446\u0438\u0439\u0433 \u04af\u04af\u0441\u0433\u044d\u0445, \u0437\u0430\u0441\u0430\u0445, \u044d\u043a\u0441\u043f\u043e\u0440\u0442, \u0445\u0430\u0434\u0433\u0430\u043b\u0430\u0445, \u0438\u043c\u043f\u043e\u0440\u0442 \u0445\u0438\u0439\u0445.", L"Kreu, redaktu, eksportu, konservu kaj importu .attd \u0109eestojn.", L"\u5efa\u7c3f\u3001\u7de8\u8f2f\u3001\u532f\u51fa\u3001\u5132\u5b58\u3001\u532f\u5165\u3001\u6279\u6e05 .attd \u9ede\u540d\u3002", L"\u0e2a\u0e23\u0e49\u0e32\u0e07 \u0e41\u0e01\u0e49\u0e44\u0e02 \u0e2a\u0e48\u0e07\u0e2d\u0e2d\u0e01 \u0e1a\u0e31\u0e19\u0e17\u0e36\u0e01 \u0e19\u0e33\u0e40\u0e02\u0e49\u0e32 \u0e41\u0e25\u0e30\u0e25\u0e49\u0e32\u0e07 .attd", L"Gumawa, mag-edit, mag-export, mag-save, mag-import, at maglinis ng .attd.", L".attd yoklamalar\u0131 olu\u015ftur, d\u00fczenle, d\u0131\u015fa aktar, kaydet ve i\u00e7e aktar.", L"Kurkite, taisykite, eksportuokite, saugokite ir importuokite .attd lankomum\u0105."},
        {L"Tip: double-click a row to edit. Ctrl/Shift supports multi-select.", L"Suggerimento: doppio clic per modificare. Ctrl/Maiusc seleziona pi\u00f9 righe.", L"\u0417\u04e9\u0432\u043b\u04e9\u0433\u04e9\u04e9: \u043c\u04e9\u0440\u0438\u0439\u0433 \u0434\u0430\u0432\u0445\u0430\u0440 \u0434\u0430\u0440\u0436 \u0437\u0430\u0441\u043d\u0430. Ctrl/Shift \u043e\u043b\u043e\u043d \u0441\u043e\u043d\u0433\u043e\u043d\u043e.", L"Konsilo: duoble alklaku vicon por redakti. Ctrl/Shift elektas plurajn.", L"\u63d0\u793a\uff1a\u96d9\u64ca\u884c\u53ef\u7de8\uff0cCtrl/Shift \u53ef\u591a\u9078\u3002", L"\u0e40\u0e04\u0e25\u0e47\u0e14\u0e25\u0e31\u0e1a: \u0e14\u0e31\u0e1a\u0e40\u0e1a\u0e34\u0e25\u0e04\u0e25\u0e34\u0e01\u0e41\u0e16\u0e27\u0e40\u0e1e\u0e37\u0e48\u0e2d\u0e41\u0e01\u0e49\u0e44\u0e02 Ctrl/Shift \u0e40\u0e25\u0e37\u0e2d\u0e01\u0e2b\u0e25\u0e32\u0e22\u0e41\u0e16\u0e27", L"Tip: i-double click ang row para mag-edit. Ctrl/Shift para multi-select.", L"\u0130pucu: d\u00fczenlemek i\u00e7in sat\u0131ra \u00e7ift t\u0131kla. Ctrl/Shift \u00e7oklu se\u00e7er.", L"Patarimas: dukart spustel\u0117kite eilut\u0119 redagavimui. Ctrl/Shift kelioms eilut\u0117ms."},
        {L"Date/Time", L"Data/Ora", L"\u041e\u0433\u043d\u043e\u043e/\u0446\u0430\u0433", L"Dato/tempo", L"\u65e5\u6642", L"\u0e27\u0e31\u0e19/\u0e40\u0e27\u0e25\u0e32", L"Petsa/Oras", L"Tarih/Saat", L"Data/laikas"},
        {L"Date / Time", L"Data / Ora", L"\u041e\u0433\u043d\u043e\u043e / \u0446\u0430\u0433", L"Dato / tempo", L"\u65e5 / \u6642", L"\u0e27\u0e31\u0e19 / \u0e40\u0e27\u0e25\u0e32", L"Petsa / Oras", L"Tarih / Saat", L"Data / laikas"},
        {L"Name", L"Nome", L"\u041d\u044d\u0440", L"Nomo", L"\u59d3\u540d", L"\u0e0a\u0e37\u0e48\u0e2d", L"Pangalan", L"Ad", L"Vardas"},
        {L"Other", L"Altro", L"\u0411\u0443\u0441\u0430\u0434", L"Alia", L"\u5176\u4ed6", L"\u0e2d\u0e37\u0e48\u0e19\u0e46", L"Iba pa", L"Di\u011fer", L"Kita"},
        {L"Status", L"Stato", L"\u0422\u04e9\u043b\u04e9\u0432", L"Stato", L"\u72c0\u614b", L"\u0e2a\u0e16\u0e32\u0e19\u0e30", L"Katayuan", L"Durum", L"B\u016bsena"},
        {L"Search", L"Cerca", L"\u0425\u0430\u0439\u0445", L"Ser\u0109i", L"\u5c0b", L"\u0e04\u0e49\u0e19\u0e2b\u0e32", L"Hanapin", L"Ara", L"Ie\u0161koti"},
        {L"Clear Filter", L"Cancella filtro", L"\u0428\u04af\u04af\u043b\u0442\u04af\u04af\u0440 \u0446\u044d\u0432\u044d\u0440\u043b\u044d\u0445", L"Malplenigi filtrilon", L"\u6e05\u7be9", L"\u0e25\u0e49\u0e32\u0e07\u0e15\u0e31\u0e27\u0e01\u0e23\u0e2d\u0e07", L"I-clear ang filter", L"Filtreyi temizle", L"I\u0161valyti filtr\u0105"},
        {L"Present", L"Presente", L"\u0418\u0440\u0441\u044d\u043d", L"\u0108eestas", L"\u51fa\u5e2d", L"\u0e21\u0e32", L"Present", L"Var", L"Dalyvauja"},
        {L"Absent", L"Assente", L"\u0422\u0430\u0441\u0430\u043b\u0441\u0430\u043d", L"Forestas", L"\u7f3a\u5e2d", L"\u0e02\u0e32\u0e14", L"Absent", L"Yok", L"Nedalyvauja"},
        {L"Late", L"In ritardo", L"\u0425\u043e\u0446\u043e\u0440\u0441\u043e\u043d", L"Malfrua", L"\u9072\u5230", L"\u0e2a\u0e32\u0e22", L"Late", L"Ge\u00e7", L"V\u0117luoja"},
        {L"Total", L"Totale", L"\u041d\u0438\u0439\u0442", L"Sumo", L"\u7e3d", L"\u0e23\u0e27\u0e21", L"Kabuuan", L"Toplam", L"I\u0161 viso"},
        {L"Showing", L"Mostra", L"\u0425\u0430\u0440\u0443\u0443\u043b\u0436 \u0431\u0430\u0439\u043d\u0430", L"Montras", L"\u986f\u793a", L"\u0e41\u0e2a\u0e14\u0e07", L"Ipinapakita", L"G\u00f6steriliyor", L"Rodoma"},
        {L"Attendance", L"Presenza", L"\u0418\u0440\u0446", L"\u0108eesto", L"\u51fa\u52e4", L"\u0e01\u0e32\u0e23\u0e21\u0e32\u0e40\u0e23\u0e35\u0e22\u0e19", L"Attendance", L"Kat\u0131l\u0131m", L"Lankomumas"},
        {L"Absent/Late", L"Assente/Ritardo", L"\u0422\u0430\u0441\u0430\u043b\u0441\u0430\u043d/\u0445\u043e\u0446\u043e\u0440\u0441\u043e\u043d", L"Foresta/malfrua", L"\u7f3a/\u9072", L"\u0e02\u0e32\u0e14/\u0e2a\u0e32\u0e22", L"Absent/Late", L"Yok/Ge\u00e7", L"N\u0117ra/v\u0117luoja"},
        {L"Update Selected", L"Aggiorna selezionato", L"\u0421\u043e\u043d\u0433\u043e\u0441\u043d\u044b\u0433 \u0448\u0438\u043d\u044d\u0447\u043b\u044d\u0445", L"\u011cisdatigi elektitan", L"\u66f4\u65b0\u6240\u9078", L"\u0e2d\u0e31\u0e1b\u0e40\u0e14\u0e15\u0e17\u0e35\u0e48\u0e40\u0e25\u0e37\u0e2d\u0e01", L"I-update ang napili", L"Se\u00e7ileni g\u00fcncelle", L"Atnaujinti pasirinkt\u0105"},
        {L"Edit Selected", L"Modifica selezionato", L"\u0421\u043e\u043d\u0433\u043e\u0441\u043d\u044b\u0433 \u0437\u0430\u0441\u0430\u0445", L"Redakti elektitan", L"\u7de8\u6240\u9078", L"\u0e41\u0e01\u0e49\u0e44\u0e02\u0e17\u0e35\u0e48\u0e40\u0e25\u0e37\u0e2d\u0e01", L"I-edit ang napili", L"Se\u00e7ileni d\u00fczenle", L"Redaguoti pasirinkt\u0105"},
        {L"Mark All Present", L"Segna tutti presenti", L"\u0411\u04af\u0433\u0434\u0438\u0439\u0433 \u0438\u0440\u0441\u044d\u043d \u0431\u043e\u043b\u0433\u043e\u0445", L"Marki \u0109iujn \u0109eestaj", L"\u5168\u70ba\u51fa\u5e2d", L"\u0e17\u0e33\u0e40\u0e04\u0e23\u0e37\u0e48\u0e2d\u0e07\u0e2b\u0e21\u0e32\u0e22\u0e27\u0e48\u0e32\u0e21\u0e32\u0e17\u0e31\u0e49\u0e07\u0e2b\u0e21\u0e14", L"Markahan lahat na present", L"T\u00fcm\u00fcn\u00fc var i\u015faretle", L"Pa\u017eym\u0117ti visus dalyvaujan\u010diais"},
        {L"Create New", L"Nuovo", L"\u0428\u0438\u043d\u044d \u04af\u04af\u0441\u0433\u044d\u0445", L"Krei novan", L"\u65b0\u5efa", L"\u0e2a\u0e23\u0e49\u0e32\u0e07\u0e43\u0e2b\u0e21\u0e48", L"Gumawa ng bago", L"Yeni olu\u015ftur", L"Kurti nauj\u0105"},
        {L"Delete Options", L"Opzioni elimina", L"\u0423\u0441\u0442\u0433\u0430\u0445 \u0441\u043e\u043d\u0433\u043e\u043b\u0442", L"Forigaj opcioj", L"\u522a\u9664\u9078\u9805", L"\u0e15\u0e31\u0e27\u0e40\u0e25\u0e37\u0e2d\u0e01\u0e25\u0e1a", L"Mga opsyon sa delete", L"Silme se\u00e7enekleri", L"Trinimo parinktys"},
        {L"Save .attd", L"Salva .attd", L".attd \u0445\u0430\u0434\u0433\u0430\u043b\u0430\u0445", L"Konservi .attd", L"\u5132\u5b58 .attd", L"\u0e1a\u0e31\u0e19\u0e17\u0e36\u0e01 .attd", L"I-save .attd", L".attd kaydet", L"I\u0161saugoti .attd"},
        {L"Import .attd", L"Importa .attd", L".attd \u0438\u043c\u043f\u043e\u0440\u0442", L"Importi .attd", L"\u532f\u5165 .attd", L"\u0e19\u0e33\u0e40\u0e02\u0e49\u0e32 .attd", L"I-import .attd", L".attd i\u00e7e aktar", L"Importuoti .attd"},
        {L"Export CSV", L"Esporta CSV", L"CSV \u044d\u043a\u0441\u043f\u043e\u0440\u0442", L"Eksporti CSV", L"\u532f\u51fa CSV", L"\u0e2a\u0e48\u0e07\u0e2d\u0e2d\u0e01 CSV", L"I-export CSV", L"CSV d\u0131\u015fa aktar", L"Eksportuoti CSV"},
        {L"Export PowerPoint (.pptx)", L"Esporta PowerPoint (.pptx)", L"PowerPoint \u044d\u043a\u0441\u043f\u043e\u0440\u0442 (.pptx)", L"Eksporti PowerPoint (.pptx)", L"\u532f\u51fa\u6f14\u793a\u7a3f (.pptx)", L"\u0e2a\u0e48\u0e07\u0e2d\u0e2d\u0e01 PowerPoint (.pptx)", L"I-export PowerPoint (.pptx)", L"PowerPoint d\u0131\u015fa aktar (.pptx)", L"Eksportuoti PowerPoint (.pptx)"},
        {L"Courses", L"Corsi", L"\u041a\u0443\u0440\u0441\u0443\u0443\u0434", L"Kursoj", L"\u8ab2\u7a0b", L"\u0e04\u0e2d\u0e23\u0e4c\u0e2a", L"Mga kurso", L"Kurslar", L"Kursai"},
        {L"Tools", L"Strumenti", L"\u0425\u044d\u0440\u044d\u0433\u0441\u044d\u043b", L"Iloj", L"\u5de5\u5177", L"\u0e40\u0e04\u0e23\u0e37\u0e48\u0e2d\u0e07\u0e21\u0e37\u0e2d", L"Mga tool", L"Ara\u00e7lar", L"\u012erankiai"},
        {L"Settings", L"Impostazioni", L"\u0422\u043e\u0445\u0438\u0440\u0433\u043e\u043e", L"Agordoj", L"\u8a2d\u5b9a", L"\u0e15\u0e31\u0e49\u0e07\u0e04\u0e48\u0e32", L"Settings", L"Ayarlar", L"Nustatymai"},
        {L"Interface Settings", L"Impostazioni interfaccia", L"\u0418\u043d\u0442\u0435\u0440\u0444\u0435\u0439\u0441\u0438\u0439\u043d \u0442\u043e\u0445\u0438\u0440\u0433\u043e\u043e", L"Interfacaj agordoj", L"\u4ecb\u9762\u8a2d\u5b9a", L"\u0e15\u0e31\u0e49\u0e07\u0e04\u0e48\u0e32\u0e2d\u0e34\u0e19\u0e40\u0e17\u0e2d\u0e23\u0e4c\u0e40\u0e1f\u0e0b", L"Interface settings", L"Aray\u00fcz ayarlar\u0131", L"S\u0105sajos nustatymai"},
        {L"Language", L"Lingua", L"\u0425\u044d\u043b", L"Lingvo", L"\u8a9e\u8a00", L"\u0e20\u0e32\u0e29\u0e32", L"Wika", L"Dil", L"Kalba"},
        {L"Style", L"Stile", L"\u0425\u044d\u0432 \u043c\u0430\u044f\u0433", L"Stilo", L"\u98a8\u683c", L"\u0e2a\u0e44\u0e15\u0e25\u0e4c", L"Estilo", L"Stil", L"Stilius"},
        {L"Interface Font", L"Carattere interfaccia", L"\u0418\u043d\u0442\u0435\u0440\u0444\u0435\u0439\u0441\u0438\u0439\u043d \u0444\u043e\u043d\u0442", L"Interfaca tiparo", L"\u4ecb\u9762\u5b57\u578b", L"\u0e1f\u0e2d\u0e19\u0e15\u0e4c\u0e2d\u0e34\u0e19\u0e40\u0e17\u0e2d\u0e23\u0e4c\u0e40\u0e1f\u0e0b", L"Font ng interface", L"Aray\u00fcz yaz\u0131 tipi", L"S\u0105sajos \u0161riftas"},
        {L"Apply", L"Applica", L"\u0425\u044d\u0440\u044d\u0433\u0436\u04af\u04af\u043b\u044d\u0445", L"Apliki", L"\u5957\u7528", L"\u0e19\u0e33\u0e44\u0e1b\u0e43\u0e0a\u0e49", L"Ilapat", L"Uygula", L"Taikyti"},
        {L"Close", L"Chiudi", L"\u0425\u0430\u0430\u0445", L"Fermi", L"\u95dc\u9589", L"\u0e1b\u0e34\u0e14", L"Isara", L"Kapat", L"U\u017edaryti"},
        {L"Reset All Settings", L"Ripristina tutto", L"\u0411\u04af\u0445 \u0442\u043e\u0445\u0438\u0440\u0433\u043e\u043e\u0433 \u0441\u044d\u0440\u0433\u044d\u044d\u0445", L"Restarigi \u0109iujn agordojn", L"\u91cd\u8a2d\u8af8\u8a2d", L"\u0e23\u0e35\u0e40\u0e0b\u0e47\u0e15\u0e17\u0e31\u0e49\u0e07\u0e2b\u0e21\u0e14", L"I-reset lahat ng settings", L"T\u00fcm ayarlar\u0131 s\u0131f\u0131rla", L"Atkurti visus nustatymus"},
        {L"OK", L"OK", L"OK", L"Bone", L"\u53ef", L"OK", L"OK", L"Tamam", L"Gerai"},
        {L"Cancel", L"Annulla", L"\u0426\u0443\u0446\u043b\u0430\u0445", L"Nuligi", L"\u53d6\u6d88", L"\u0e22\u0e01\u0e40\u0e25\u0e34\u0e01", L"Kanselahin", L"\u0130ptal", L"At\u0161aukti"},
        {L"Imported successfully.", L"Importazione riuscita.", L"\u0410\u043c\u0436\u0438\u043b\u0442\u0442\u0430\u0439 \u0438\u043c\u043f\u043e\u0440\u0442\u043b\u043e\u043e.", L"Sukcese importita.", L"\u532f\u5165\u5df2\u6210\u3002", L"\u0e19\u0e33\u0e40\u0e02\u0e49\u0e32\u0e2a\u0e33\u0e40\u0e23\u0e47\u0e08", L"Matagumpay na na-import.", L"Ba\u015far\u0131yla i\u00e7e aktar\u0131ld\u0131.", L"S\u0117kmingai importuota."},
        {L"Saved successfully.", L"Salvato.", L"\u0410\u043c\u0436\u0438\u043b\u0442\u0442\u0430\u0439 \u0445\u0430\u0434\u0433\u0430\u043b\u043b\u0430\u0430.", L"Sukcese konservita.", L"\u5132\u5b58\u5df2\u6210\u3002", L"\u0e1a\u0e31\u0e19\u0e17\u0e36\u0e01\u0e2a\u0e33\u0e40\u0e23\u0e47\u0e08", L"Matagumpay na na-save.", L"Ba\u015far\u0131yla kaydedildi.", L"S\u0117kmingai i\u0161saugota."},
        {L"PowerPoint exported successfully.", L"PowerPoint esportato.", L"PowerPoint \u0430\u043c\u0436\u0438\u043b\u0442\u0442\u0430\u0439 \u044d\u043a\u0441\u043f\u043e\u0440\u0442\u043b\u043e\u043e.", L"PowerPoint sukcese eksportita.", L"\u6f14\u793a\u7a3f\u5df2\u6210\u3002", L"\u0e2a\u0e48\u0e07\u0e2d\u0e2d\u0e01 PowerPoint \u0e2a\u0e33\u0e40\u0e23\u0e47\u0e08", L"Matagumpay na na-export ang PowerPoint.", L"PowerPoint ba\u015far\u0131yla d\u0131\u015fa aktar\u0131ld\u0131.", L"PowerPoint s\u0117kmingai eksportuotas."},
        {L"Could not export the PowerPoint file.", L"Impossibile esportare il file PowerPoint.", L"PowerPoint \u0444\u0430\u0439\u043b \u044d\u043a\u0441\u043f\u043e\u0440\u0442\u043b\u043e\u0436 \u0447\u0430\u0434\u0441\u0430\u043d\u0433\u04af\u0439.", L"Ne eblis eksporti la PowerPoint-dosieron.", L"\u4e0d\u80fd\u532f\u51fa\u6f14\u793a\u7a3f\u3002", L"\u0e2a\u0e48\u0e07\u0e2d\u0e2d\u0e01\u0e44\u0e1f\u0e25\u0e4c PowerPoint \u0e44\u0e21\u0e48\u0e44\u0e14\u0e49", L"Hindi ma-export ang PowerPoint file.", L"PowerPoint dosyas\u0131 d\u0131\u015fa aktar\u0131lamad\u0131.", L"Nepavyko eksportuoti PowerPoint failo."},
        {L"Could not save the file.", L"Impossibile salvare il file.", L"\u0424\u0430\u0439\u043b \u0445\u0430\u0434\u0433\u0430\u043b\u0436 \u0447\u0430\u0434\u0441\u0430\u043d\u0433\u04af\u0439.", L"Ne eblis konservi la dosieron.", L"\u4e0d\u80fd\u5132\u6a94\u3002", L"\u0e1a\u0e31\u0e19\u0e17\u0e36\u0e01\u0e44\u0e1f\u0e25\u0e4c\u0e44\u0e21\u0e48\u0e44\u0e14\u0e49", L"Hindi ma-save ang file.", L"Dosya kaydedilemedi.", L"Nepavyko i\u0161saugoti failo."},
        {L"Could not open the file.", L"Impossibile aprire il file.", L"\u0424\u0430\u0439\u043b \u043d\u044d\u044d\u0436 \u0447\u0430\u0434\u0441\u0430\u043d\u0433\u04af\u0439.", L"Ne eblis malfermi la dosieron.", L"\u4e0d\u80fd\u958b\u6a94\u3002", L"\u0e40\u0e1b\u0e34\u0e14\u0e44\u0e1f\u0e25\u0e4c\u0e44\u0e21\u0e48\u0e44\u0e14\u0e49", L"Hindi mabuksan ang file.", L"Dosya a\u00e7\u0131lamad\u0131.", L"Nepavyko atidaryti failo."},
        {L"Please enter a date and time.", L"Inserisci data e ora.", L"\u041e\u0433\u043d\u043e\u043e, \u0446\u0430\u0433 \u043e\u0440\u0443\u0443\u043b\u043d\u0430 \u0443\u0443.", L"Enigu daton kaj tempon.", L"\u8acb\u8f38\u65e5\u6642\u3002", L"\u0e01\u0e23\u0e38\u0e13\u0e32\u0e43\u0e2a\u0e48\u0e27\u0e31\u0e19\u0e41\u0e25\u0e30\u0e40\u0e27\u0e25\u0e32", L"Ilagay ang petsa at oras.", L"L\u00fctfen tarih ve saat girin.", L"\u012eveskite dat\u0105 ir laik\u0105."},
        {L"Please enter a name.", L"Inserisci un nome.", L"\u041d\u044d\u0440 \u043e\u0440\u0443\u0443\u043b\u043d\u0430 \u0443\u0443.", L"Enigu nomon.", L"\u8acb\u8f38\u59d3\u540d\u3002", L"\u0e01\u0e23\u0e38\u0e13\u0e32\u0e43\u0e2a\u0e48\u0e0a\u0e37\u0e48\u0e2d", L"Ilagay ang pangalan.", L"L\u00fctfen ad girin.", L"\u012eveskite vard\u0105."},
        {L"Please fill the Other field.", L"Compila il campo Altro.", L"\u0411\u0443\u0441\u0430\u0434 \u0442\u0430\u043b\u0431\u0430\u0440\u044b\u0433 \u0431\u04e9\u0433\u043b\u04e9\u043d\u04e9 \u04af\u04af.", L"Plenigu la kampon Alia.", L"\u8acb\u586b\u5176\u4ed6\u6b04\u3002", L"\u0e01\u0e23\u0e38\u0e13\u0e32\u0e01\u0e23\u0e2d\u0e01\u0e0a\u0e48\u0e2d\u0e07\u0e2d\u0e37\u0e48\u0e19\u0e46", L"Punan ang field na Iba pa.", L"L\u00fctfen Di\u011fer alan\u0131n\u0131 doldurun.", L"U\u017epildykite lauk\u0105 Kita."},
        {L"Manage students", L"Gestisci studenti", L"\u0421\u0443\u0440\u0430\u0433\u0447\u0434\u044b\u0433 \u0443\u0434\u0438\u0440\u0434\u0430\u0445", L"Administri studentojn", L"\u7ba1\u5f1f\u5b50", L"\u0e08\u0e31\u0e14\u0e01\u0e32\u0e23\u0e19\u0e31\u0e01\u0e40\u0e23\u0e35\u0e22\u0e19", L"Pamahalaan ang mga estudyante", L"\u00d6\u011frencileri y\u00f6net", L"Tvarkyti mokinius"},
        {L"Add student to roster", L"Aggiungi studente alla lista", L"\u0421\u0443\u0440\u0430\u0433\u0447\u0438\u0439\u0433 \u0436\u0430\u0433\u0441\u0430\u0430\u043b\u0442\u0430\u0434 \u043d\u044d\u043c\u044d\u0445", L"Aldoni studenton al listo", L"\u7c4d\u4e2d\u589e\u5f1f\u5b50", L"\u0e40\u0e1e\u0e34\u0e48\u0e21\u0e19\u0e31\u0e01\u0e40\u0e23\u0e35\u0e22\u0e19\u0e25\u0e07\u0e23\u0e32\u0e22\u0e0a\u0e37\u0e48\u0e2d", L"Magdagdag ng estudyante sa roster", L"\u00d6\u011frenciyi listeye ekle", L"Prid\u0117ti mokin\u012f \u012f s\u0105ra\u0161\u0105"},
        {L"Remove student from roster", L"Rimuovi studente dalla lista", L"\u0421\u0443\u0440\u0430\u0433\u0447\u0438\u0439\u0433 \u0436\u0430\u0433\u0441\u0430\u0430\u043b\u0442\u0430\u0430\u0441 \u0445\u0430\u0441\u0430\u0445", L"Forigi studenton el listo", L"\u7c4d\u4e2d\u53bb\u5f1f\u5b50", L"\u0e25\u0e1a\u0e19\u0e31\u0e01\u0e40\u0e23\u0e35\u0e22\u0e19\u0e2d\u0e2d\u0e01\u0e08\u0e32\u0e01\u0e23\u0e32\u0e22\u0e0a\u0e37\u0e48\u0e2d", L"Alisin ang estudyante sa roster", L"\u00d6\u011frenciyi listeden kald\u0131r", L"Pa\u0161alinti mokin\u012f i\u0161 s\u0105ra\u0161o"},
        {L"Create records from roster", L"Crea registri dalla lista", L"\u0416\u0430\u0433\u0441\u0430\u0430\u043b\u0442\u0430\u0430\u0441 \u0431\u0438\u0447\u043b\u044d\u0433 \u04af\u04af\u0441\u0433\u044d\u0445", L"Krei registrojn el listo", L"\u64da\u540d\u7c4d\u7acb\u9304", L"\u0e2a\u0e23\u0e49\u0e32\u0e07\u0e23\u0e32\u0e22\u0e01\u0e32\u0e23\u0e08\u0e32\u0e01\u0e23\u0e32\u0e22\u0e0a\u0e37\u0e48\u0e2d", L"Gumawa ng records mula sa roster", L"Listeden kay\u0131t olu\u015ftur", L"Kurti \u012fra\u0161us i\u0161 s\u0105ra\u0161o"},
        {L"Course details", L"Dettagli corso", L"\u041a\u0443\u0440\u0441\u044b\u043d \u0434\u044d\u043b\u0433\u044d\u0440\u044d\u043d\u0433\u04af\u0439", L"Kursaj detaloj", L"\u8ab2\u4e4b\u8a73", L"\u0e23\u0e32\u0e22\u0e25\u0e30\u0e40\u0e2d\u0e35\u0e22\u0e14\u0e04\u0e2d\u0e23\u0e4c\u0e2a", L"Detalye ng kurso", L"Kurs ayr\u0131nt\u0131lar\u0131", L"Kurso informacija"},
        {L"Teacher/owner:", L"Insegnante/responsabile:", L"\u0411\u0430\u0433\u0448/\u0445\u0430\u0440\u0438\u0443\u0446\u0430\u0433\u0447:", L"Instruisto/posedanto:", L"\u5e2b/\u4e3b\u8005\uff1a", L"\u0e04\u0e23\u0e39/\u0e1c\u0e39\u0e49\u0e23\u0e31\u0e1a\u0e1c\u0e34\u0e14\u0e0a\u0e2d\u0e1a:", L"Guro/may-ari:", L"\u00d6\u011fretmen/sorumlu:", L"Mokytojas/savininkas:"},
        {L"Location:", L"Luogo:", L"\u0411\u0430\u0439\u0440\u0448\u0438\u043b:", L"Loko:", L"\u6240\u5728\uff1a", L"\u0e2a\u0e16\u0e32\u0e19\u0e17\u0e35\u0e48:", L"Lokasyon:", L"Konum:", L"Vieta:"},
        {L"Course notes:", L"Note corso:", L"\u041a\u0443\u0440\u0441\u044b\u043d \u0442\u044d\u043c\u0434\u044d\u0433\u043b\u044d\u043b:", L"Kursaj notoj:", L"\u8ab2\u8a18\uff1a", L"\u0e2b\u0e21\u0e32\u0e22\u0e40\u0e2b\u0e15\u0e38\u0e04\u0e2d\u0e23\u0e4c\u0e2a:", L"Tala ng kurso:", L"Kurs notlar\u0131:", L"Kurso pastabos:"},
        {L"Student name:", L"Nome studente:", L"\u0421\u0443\u0440\u0430\u0433\u0447\u0438\u0439\u043d \u043d\u044d\u0440:", L"Studenta nomo:", L"\u5f1f\u5b50\u59d3\u540d\uff1a", L"\u0e0a\u0e37\u0e48\u0e2d\u0e19\u0e31\u0e01\u0e40\u0e23\u0e35\u0e22\u0e19:", L"Pangalan ng estudyante:", L"\u00d6\u011frenci ad\u0131:", L"Mokinio vardas:"},
        {L"Student added.", L"Studente aggiunto.", L"\u0421\u0443\u0440\u0430\u0433\u0447 \u043d\u044d\u043c\u044d\u0433\u0434\u043b\u044d\u044d.", L"Studento aldonita.", L"\u5df2\u589e\u5f1f\u5b50\u3002", L"\u0e40\u0e1e\u0e34\u0e48\u0e21\u0e19\u0e31\u0e01\u0e40\u0e23\u0e35\u0e22\u0e19\u0e41\u0e25\u0e49\u0e27", L"Naidagdag ang estudyante.", L"\u00d6\u011frenci eklendi.", L"Mokinys prid\u0117tas."},
        {L"Student removed.", L"Studente rimosso.", L"\u0421\u0443\u0440\u0430\u0433\u0447 \u0445\u0430\u0441\u0430\u0433\u0434\u043b\u0430\u0430.", L"Studento forigita.", L"\u5df2\u53bb\u5f1f\u5b50\u3002", L"\u0e25\u0e1a\u0e19\u0e31\u0e01\u0e40\u0e23\u0e35\u0e22\u0e19\u0e41\u0e25\u0e49\u0e27", L"Naalis ang estudyante.", L"\u00d6\u011frenci kald\u0131r\u0131ld\u0131.", L"Mokinys pa\u0161alintas."},
        {L"No students in the roster.", L"Nessuno studente nella lista.", L"\u0416\u0430\u0433\u0441\u0430\u0430\u043b\u0442\u0430\u0434 \u0441\u0443\u0440\u0430\u0433\u0447 \u0430\u043b\u0433\u0430.", L"Neniu studento en la listo.", L"\u540d\u7c4d\u7121\u5f1f\u5b50\u3002", L"\u0e44\u0e21\u0e48\u0e21\u0e35\u0e19\u0e31\u0e01\u0e40\u0e23\u0e35\u0e22\u0e19\u0e43\u0e19\u0e23\u0e32\u0e22\u0e0a\u0e37\u0e48\u0e2d", L"Walang estudyante sa roster.", L"Listede \u00f6\u011frenci yok.", L"S\u0105ra\u0161e n\u0117ra mokini\u0173."},
        {L"Roster records created.", L"Registri creati dalla lista.", L"\u0416\u0430\u0433\u0441\u0430\u0430\u043b\u0442\u044b\u043d \u0431\u0438\u0447\u043b\u044d\u0433 \u04af\u04af\u0441\u043b\u044d\u044d.", L"Listaj registroj kreitaj.", L"\u540d\u7c4d\u4e4b\u9304\u5df2\u7acb\u3002", L"\u0e2a\u0e23\u0e49\u0e32\u0e07\u0e23\u0e32\u0e22\u0e01\u0e32\u0e23\u0e08\u0e32\u0e01\u0e23\u0e32\u0e22\u0e0a\u0e37\u0e48\u0e2d\u0e41\u0e25\u0e49\u0e27", L"Nagawa ang records mula sa roster.", L"Liste kay\u0131tlar\u0131 olu\u015fturuldu.", L"\u012era\u0161ai i\u0161 s\u0105ra\u0161o sukurti."},
        {L"Statistics summary", L"Riepilogo statistiche", L"\u0421\u0442\u0430\u0442\u0438\u0441\u0442\u0438\u043a\u0438\u0439\u043d \u0445\u0443\u0440\u0430\u0430\u043d\u0433\u0443\u0439", L"Statistika resumo", L"\u6578\u64da\u6458\u8981", L"\u0e2a\u0e23\u0e38\u0e1b\u0e2a\u0e16\u0e34\u0e15\u0e34", L"Buod ng estadistika", L"\u0130statistik \u00f6zeti", L"Statistikos santrauka"},
        {L"Top absent/late students", L"Studenti pi\u00f9 assenti/in ritardo", L"\u0425\u0430\u043c\u0433\u0438\u0439\u043d \u0438\u0445 \u0442\u0430\u0441\u0430\u043b\u0441\u0430\u043d/\u0445\u043e\u0446\u043e\u0440\u0441\u043e\u043d \u0441\u0443\u0440\u0430\u0433\u0447\u0438\u0434", L"Plej forestantaj/malfruaj studentoj", L"\u95d5\u9072\u6700\u591a\u8005", L"\u0e19\u0e31\u0e01\u0e40\u0e23\u0e35\u0e22\u0e19\u0e02\u0e32\u0e14/\u0e2a\u0e32\u0e22\u0e21\u0e32\u0e01\u0e2a\u0e38\u0e14", L"Pinakamaraming absent/late", L"En \u00e7ok yok/ge\u00e7 kalan \u00f6\u011frenciler", L"Daugiausia nedalyvav\u0119/v\u0117lav\u0119 mokiniai"},
        {L"Backup now", L"Esegui backup ora", L"\u041e\u0434\u043e\u043e \u043d\u04e9\u04e9\u0446\u043b\u04e9\u0445", L"Sekurkopii nun", L"\u5373\u5099\u4efd", L"\u0e2a\u0e33\u0e23\u0e2d\u0e07\u0e15\u0e2d\u0e19\u0e19\u0e35\u0e49", L"Mag-backup ngayon", L"\u015eimdi yedekle", L"Kurti atsargin\u0119 kopij\u0105"},
        {L"Restore latest backup", L"Ripristina ultimo backup", L"\u0421\u04af\u04af\u043b\u0438\u0439\u043d \u043d\u04e9\u04e9\u0446\u0438\u0439\u0433 \u0441\u044d\u0440\u0433\u044d\u044d\u0445", L"Restarigi lastan sekurkopion", L"\u9084\u6700\u65b0\u5099\u4efd", L"\u0e01\u0e39\u0e49\u0e04\u0e37\u0e19\u0e44\u0e1f\u0e25\u0e4c\u0e2a\u0e33\u0e23\u0e2d\u0e07\u0e25\u0e48\u0e32\u0e2a\u0e38\u0e14", L"I-restore ang pinakabagong backup", L"Son yede\u011fi geri y\u00fckle", L"Atkurti naujausi\u0105 kopij\u0105"},
        {L"Open recent file", L"Apri file recente", L"\u0421\u04af\u04af\u043b\u0438\u0439\u043d \u0444\u0430\u0439\u043b\u044b\u0433 \u043d\u044d\u044d\u0445", L"Malfermi lastatempan dosieron", L"\u958b\u8fd1\u6a94", L"\u0e40\u0e1b\u0e34\u0e14\u0e44\u0e1f\u0e25\u0e4c\u0e25\u0e48\u0e32\u0e2a\u0e38\u0e14", L"Buksan ang recent file", L"Son dosyay\u0131 a\u00e7", L"Atidaryti naujausi\u0105 fail\u0105"},
        {L"Backup created:", L"Backup creato:", L"\u041d\u04e9\u04e9\u0446 \u04af\u04af\u0441\u043b\u044d\u044d:", L"Sekurkopio kreita:", L"\u5099\u4efd\u5df2\u6210\uff1a", L"\u0e2a\u0e23\u0e49\u0e32\u0e07\u0e44\u0e1f\u0e25\u0e4c\u0e2a\u0e33\u0e23\u0e2d\u0e07\u0e41\u0e25\u0e49\u0e27:", L"Nagawa ang backup:", L"Yedek olu\u015fturuldu:", L"Atsargin\u0117 kopija sukurta:"},
        {L"No backup file was found.", L"Nessun file di backup trovato.", L"\u041d\u04e9\u04e9\u0446 \u0444\u0430\u0439\u043b \u043e\u043b\u0434\u0441\u043e\u043d\u0433\u04af\u0439.", L"Neniu sekurkopia dosiero trovita.", L"\u672a\u5f97\u5099\u4efd\u6a94\u3002", L"\u0e44\u0e21\u0e48\u0e1e\u0e1a\u0e44\u0e1f\u0e25\u0e4c\u0e2a\u0e33\u0e23\u0e2d\u0e07", L"Walang nakitang backup file.", L"Yedek dosyas\u0131 bulunamad\u0131.", L"Atsargin\u0117s kopijos failas nerastas."},
        {L"No recent file was found.", L"Nessun file recente trovato.", L"\u0421\u04af\u04af\u043b\u0438\u0439\u043d \u0444\u0430\u0439\u043b \u043e\u043b\u0434\u0441\u043e\u043d\u0433\u04af\u0439.", L"Neniu lastatempa dosiero trovita.", L"\u672a\u5f97\u8fd1\u6a94\u3002", L"\u0e44\u0e21\u0e48\u0e1e\u0e1a\u0e44\u0e1f\u0e25\u0e4c\u0e25\u0e48\u0e32\u0e2a\u0e38\u0e14", L"Walang nakitang recent file.", L"Son dosya bulunamad\u0131.", L"Naujausias failas nerastas."},
        {L"Set default save folder", L"Imposta cartella di salvataggio", L"\u04e8\u0433\u04e9\u0433\u0434\u043c\u04e9\u043b \u0445\u0430\u0434\u0433\u0430\u043b\u0430\u0445 \u0445\u0430\u0432\u0442\u0430\u0441", L"Agordi defa\u016dltan konservan dosierujon", L"\u5b9a\u9810\u8a2d\u85cf\u6a94\u8655", L"\u0e15\u0e31\u0e49\u0e07\u0e42\u0e1f\u0e25\u0e40\u0e14\u0e2d\u0e23\u0e4c\u0e1a\u0e31\u0e19\u0e17\u0e36\u0e01\u0e40\u0e23\u0e34\u0e48\u0e21\u0e15\u0e49\u0e19", L"Itakda ang default save folder", L"Varsay\u0131lan kay\u0131t klas\u00f6r\u00fcn\u00fc ayarla", L"Nustatyti numatyt\u0105j\u012f aplank\u0105"},
        {L"Default save folder updated:", L"Cartella di salvataggio aggiornata:", L"\u04e8\u0433\u04e9\u0433\u0434\u043c\u04e9\u043b \u0445\u0430\u0434\u0433\u0430\u043b\u0430\u0445 \u0445\u0430\u0432\u0442\u0430\u0441 \u0448\u0438\u043d\u044d\u0447\u043b\u044d\u0433\u0434\u043b\u044d\u044d:", L"Defa\u016dlta konserva dosierujo \u011disdatigita:", L"\u9810\u8a2d\u85cf\u6a94\u8655\u5df2\u66f4\uff1a", L"\u0e2d\u0e31\u0e1b\u0e40\u0e14\u0e15\u0e42\u0e1f\u0e25\u0e40\u0e14\u0e2d\u0e23\u0e4c\u0e1a\u0e31\u0e19\u0e17\u0e36\u0e01\u0e40\u0e23\u0e34\u0e48\u0e21\u0e15\u0e49\u0e19\u0e41\u0e25\u0e49\u0e27:", L"Na-update ang default save folder:", L"Varsay\u0131lan kay\u0131t klas\u00f6r\u00fc g\u00fcncellendi:", L"Numatytasis aplankas atnaujintas:"}
    };

    for (const auto& entry : entries) {
        if (key == entry.key) {
            switch (g_language) {
            case UiLanguage::Italian: out = entry.it; return true;
            case UiLanguage::Mongolian: out = entry.mn; return true;
            case UiLanguage::Esperanto: out = entry.eo; return true;
            case UiLanguage::ClassicalChinese: out = entry.lzh; return true;
            case UiLanguage::Thai: out = entry.th; return true;
            case UiLanguage::Filipino: out = entry.fil; return true;
            case UiLanguage::Turkish: out = entry.tr; return true;
            case UiLanguage::Lithuanian: out = entry.lt; return true;
            default: return false;
            }
        }
    }
    return false;
}

std::wstring Tr(const wchar_t* english, const wchar_t*) {
    std::wstring key = english;
    if (g_language == UiLanguage::English) return key;
    std::wstring additional;
    if (TranslateAdditionalLanguage(key, additional)) return additional;

    auto zh = [&](const wchar_t* s) -> std::wstring { return s; };
    auto mt = [&](const wchar_t* s) -> std::wstring { return s; };
    auto ja = [&](const wchar_t* s) -> std::wstring { return s; };
    auto fr = [&](const wchar_t* s) -> std::wstring { return s; };
    auto de = [&](const wchar_t* s) -> std::wstring { return s; };
    auto ru = [&](const wchar_t* s) -> std::wstring { return s; };
    auto zht = [&](const wchar_t* s) -> std::wstring { return s; };
    auto es = [&](const wchar_t* s) -> std::wstring { return s; };

    struct Entry {
        const wchar_t* key;
        const wchar_t* zh;
        const wchar_t* mt;
        const wchar_t* ja;
        const wchar_t* fr;
        const wchar_t* de;
        const wchar_t* ru;
        const wchar_t* zht;
        const wchar_t* es;
    };

    static const Entry entries[] = {
        {L"AttendanceApp - .attd Roll Call Manager", L"AttendanceApp - .attd \u70b9\u540d\u7ba1\u7406\u5668", L"AttendanceApp - Mani\u0121er tal-Attendenza .attd", L"AttendanceApp - .attd \u51fa\u5e2d\u7ba1\u7406", L"AttendanceApp - Gestionnaire d'appel .attd", L"AttendanceApp - .attd Anwesenheitsmanager", L"AttendanceApp - \u0436\u0443\u0440\u043d\u0430\u043b .attd", L"AttendanceApp - .attd \u9ede\u540d\u7ba1\u7406\u5668", L"AttendanceApp - gestor de asistencia .attd"},
        {L"Attendance Manager", L"\u70b9\u540d\u7ba1\u7406\u5668", L"Mani\u0121er tal-Attendenza", L"\u51fa\u5e2d\u7ba1\u7406", L"Gestionnaire d'appel", L"Anwesenheitsmanager", L"\u0416\u0443\u0440\u043d\u0430\u043b \u043f\u043e\u0441\u0435\u0449\u0430\u0435\u043c\u043e\u0441\u0442\u0438", L"\u9ede\u540d\u7ba1\u7406\u5668", L"Gestor de asistencia"},
        {L"Create, edit, export, save, import, and batch clean .attd roll calls.", L"\u521b\u5efa\u3001\u7f16\u8f91\u3001\u5bfc\u51fa\u3001\u4fdd\u5b58\u3001\u5bfc\u5165\u5e76\u6279\u91cf\u6574\u7406 .attd \u70b9\u540d\u8bb0\u5f55\u3002", L"O\u0127loq, editja, esporta, issejvja, importa u naddaf re\u0121istri .attd.", L".attd \u306e\u51fa\u5e2d\u8a18\u9332\u3092\u4f5c\u6210\u3001\u7de8\u96c6\u3001\u30a8\u30af\u30b9\u30dd\u30fc\u30c8\u3001\u4fdd\u5b58\u3001\u30a4\u30f3\u30dd\u30fc\u30c8\u3001\u4e00\u62ec\u6574\u7406\u3002", L"Cr\u00e9er, modifier, exporter, enregistrer, importer et nettoyer les appels .attd.", L".attd-Anwesenheiten erstellen, bearbeiten, exportieren, speichern, importieren und bereinigen.", L"\u0421\u043e\u0437\u0434\u0430\u0432\u0430\u0439\u0442\u0435, \u0440\u0435\u0434\u0430\u043a\u0442\u0438\u0440\u0443\u0439\u0442\u0435, \u044d\u043a\u0441\u043f\u043e\u0440\u0442\u0438\u0440\u0443\u0439\u0442\u0435 \u0438 \u0438\u043c\u043f\u043e\u0440\u0442\u0438\u0440\u0443\u0439\u0442\u0435 .attd.", L"\u5efa\u7acb\u3001\u7de8\u8f2f\u3001\u532f\u51fa\u3001\u5132\u5b58\u3001\u532f\u5165\u4e26\u6279\u6b21\u6574\u7406 .attd \u9ede\u540d\u8a18\u9304\u3002", L"Crea, edita, exporta, guarda, importa y limpia registros .attd."},
        {L"Tip: double-click a row to edit. Ctrl/Shift supports multi-select.", L"\u63d0\u793a\uff1a\u53cc\u51fb\u8bb0\u5f55\u53ef\u7f16\u8f91\uff0cCtrl/Shift \u53ef\u591a\u9009\u3002", L"\u0126jiel: ikklikkja darbtejn biex teditja. Ctrl/Shift jag\u0127\u017cel aktar minn wa\u0127da.", L"\u30d2\u30f3\u30c8\uff1a\u884c\u3092\u30c0\u30d6\u30eb\u30af\u30ea\u30c3\u30af\u3067\u7de8\u96c6\u3002Ctrl/Shift \u3067\u8907\u6570\u9078\u629e\u3002", L"Astuce : double-cliquez une ligne pour modifier. Ctrl/Shift pour s\u00e9lection multiple.", L"Tipp: Doppelklick zum Bearbeiten. Strg/Umschalt f\u00fcr Mehrfachauswahl.", L"\u0421\u043e\u0432\u0435\u0442: \u0434\u0432\u043e\u0439\u043d\u043e\u0439 \u0449\u0435\u043b\u0447\u043e\u043a \u0434\u043b\u044f \u043f\u0440\u0430\u0432\u043a\u0438. Ctrl/Shift \u0434\u043b\u044f \u0432\u044b\u0431\u043e\u0440\u0430.", L"\u63d0\u793a\uff1a\u96d9\u64ca\u8a18\u9304\u53ef\u7de8\u8f2f\uff0cCtrl/Shift \u53ef\u591a\u9078\u3002", L"Consejo: doble clic para editar. Ctrl/Shift permite selecci\u00f3n m\u00faltiple."},
        {L"Date/Time", L"\u65e5\u671f\u65f6\u95f4", L"Data/\u0126in", L"\u65e5\u6642", L"Date/heure", L"Datum/Uhrzeit", L"\u0414\u0430\u0442\u0430/\u0432\u0440\u0435\u043c\u044f", L"\u65e5\u671f\u6642\u9593", L"Fecha/hora"},
        {L"Name", L"\u59d3\u540d", L"Isem", L"\u540d\u524d", L"Nom", L"Name", L"\u0418\u043c\u044f", L"\u59d3\u540d", L"Nombre"},
        {L"Other", L"\u5176\u4ed6", L"Ie\u0127or", L"\u305d\u306e\u4ed6", L"Autre", L"Andere", L"\u0414\u0440\u0443\u0433\u043e\u0435", L"\u5176\u4ed6", L"Otro"},
        {L"Search", L"\u641c\u7d22", L"Fittex", L"\u691c\u7d22", L"Recherche", L"Suchen", L"\u041f\u043e\u0438\u0441\u043a", L"\u641c\u5c0b", L"Buscar"},
        {L"Clear Filter", L"\u6e05\u9664\u7b5b\u9009", L"Naddaf filtru", L"\u30d5\u30a3\u30eb\u30bf\u30fc\u89e3\u9664", L"Effacer filtre", L"Filter leeren", L"\u0421\u0431\u0440\u043e\u0441\u0438\u0442\u044c \u0444\u0438\u043b\u044c\u0442\u0440", L"\u6e05\u9664\u7be9\u9078", L"Limpiar filtro"},
        {L"Present", L"\u51fa\u5e2d", L"Pre\u017centi", L"\u51fa\u5e2d", L"Pr\u00e9sent", L"Anwesend", L"\u041f\u0440\u0438\u0441\u0443\u0442\u0441\u0442\u0432\u0443\u0435\u0442", L"\u51fa\u5e2d", L"Presente"},
        {L"Absent", L"\u7f3a\u5e2d", L"Assenti", L"\u6b20\u5e2d", L"Absent", L"Abwesend", L"\u041e\u0442\u0441\u0443\u0442\u0441\u0442\u0432\u0443\u0435\u0442", L"\u7f3a\u5e2d", L"Ausente"},
        {L"Late", L"\u8fdf\u5230", L"Tard", L"\u9045\u523b", L"En retard", L"Versp\u00e4tet", L"\u041e\u043f\u043e\u0437\u0434\u0430\u043b", L"\u9072\u5230", L"Tarde"},
        {L"Total", L"\u603b\u6570", L"Total", L"\u5408\u8a08", L"Total", L"Gesamt", L"\u0412\u0441\u0435\u0433\u043e", L"\u7e3d\u6578", L"Total"},
        {L"Showing", L"\u663e\u793a", L"Qed jintwerew", L"\u8868\u793a", L"Affichage", L"Angezeigt", L"\u041f\u043e\u043a\u0430\u0437\u0430\u043d\u043e", L"\u986f\u793a", L"Mostrando"},
        {L"Attendance", L"\u51fa\u52e4\u7387", L"Attendenza", L"\u51fa\u5e2d\u7387", L"Pr\u00e9sence", L"Anwesenheit", L"\u041f\u043e\u0441\u0435\u0449\u0430\u0435\u043c\u043e\u0441\u0442\u044c", L"\u51fa\u52e4\u7387", L"Asistencia"},
        {L"Attendance Sheet", L"\u70b9\u540d\u8868", L"Folja tal-attendenza", L"\u51fa\u5e2d\u30b7\u30fc\u30c8", L"Feuille d'appel", L"Anwesenheitsblatt", L"\u041b\u0438\u0441\u0442 \u043f\u043e\u0441\u0435\u0449\u0430\u0435\u043c\u043e\u0441\u0442\u0438", L"\u9ede\u540d\u8868", L"Hoja de asistencia"},
        {L"Absent/Late", L"\u7f3a\u5e2d/\u8fdf\u5230", L"Assenti/Tard", L"\u6b20\u5e2d/\u9045\u523b", L"Absent/retard", L"Abwesend/versp\u00e4tet", L"\u041e\u0442\u0441\u0443\u0442./\u043e\u043f\u043e\u0437\u0434.", L"\u7f3a\u5e2d/\u9072\u5230", L"Ausente/tarde"},
        {L"Update Selected", L"\u66f4\u65b0\u9009\u4e2d", L"A\u0121\u0121orna mag\u0127\u017cul", L"\u9078\u629e\u3092\u66f4\u65b0", L"Mettre \u00e0 jour", L"Auswahl aktualisieren", L"\u041e\u0431\u043d\u043e\u0432\u0438\u0442\u044c", L"\u66f4\u65b0\u9078\u53d6", L"Actualizar selecci\u00f3n"},
        {L"Edit Selected", L"\u7f16\u8f91\u9009\u4e2d", L"Editja mag\u0127\u017cul", L"\u9078\u629e\u3092\u7de8\u96c6", L"Modifier", L"Auswahl bearbeiten", L"\u0418\u0437\u043c\u0435\u043d\u0438\u0442\u044c", L"\u7de8\u8f2f\u9078\u53d6", L"Editar selecci\u00f3n"},
        {L"Mark All Present", L"\u5168\u5458\u51fa\u5e2d", L"Kollha pre\u017centi", L"\u5168\u54e1\u51fa\u5e2d", L"Tout pr\u00e9sent", L"Alle anwesend", L"\u0412\u0441\u0435 \u043f\u0440\u0438\u0441\u0443\u0442\u0441\u0442\u0432\u0443\u044e\u0442", L"\u5168\u54e1\u51fa\u5e2d", L"Todos presentes"},
        {L"Create New", L"\u65b0\u5efa\u70b9\u540d", L"\u0120did", L"\u65b0\u898f\u4f5c\u6210", L"Nouveau", L"Neu erstellen", L"\u0421\u043e\u0437\u0434\u0430\u0442\u044c", L"\u65b0\u589e\u9ede\u540d", L"Crear nuevo"},
        {L"Delete Options", L"\u5220\u9664\u9009\u9879", L"G\u0127a\u017cliet ta' t\u0127assir", L"\u524a\u9664\u30aa\u30d7\u30b7\u30e7\u30f3", L"Options de suppression", L"L\u00f6schoptionen", L"\u041f\u0430\u0440\u0430\u043c\u0435\u0442\u0440\u044b \u0443\u0434\u0430\u043b\u0435\u043d\u0438\u044f", L"\u522a\u9664\u9078\u9805", L"Opciones de borrar"},
        {L"Delete", L"\u5220\u9664", L"\u0126assar", L"\u524a\u9664", L"Supprimer", L"L\u00f6schen", L"\u0423\u0434\u0430\u043b\u0438\u0442\u044c", L"\u522a\u9664", L"Eliminar"},
        {L"selected record(s)?", L"\u6761\u9009\u4e2d\u8bb0\u5f55\uff1f", L"rekord(s) mag\u0127\u017cula?", L"\u4ef6\u306e\u9078\u629e\u8a18\u9332\uff1f", L"enregistrement(s) s\u00e9lectionn\u00e9(s) ?", L"ausgew\u00e4hlte Eintr\u00e4ge l\u00f6schen?", L"\u0432\u044b\u0431\u0440\u0430\u043d\u043d\u044b\u0445 \u0437\u0430\u043f\u0438\u0441\u0435\u0439?", L"\u7b46\u9078\u53d6\u8a18\u9304\uff1f", L"registro(s) seleccionado(s)?"},
        {L"Delete Selected", L"\u5220\u9664\u9009\u4e2d", L"\u0126assar mag\u0127\u017cul", L"\u9078\u629e\u3092\u524a\u9664", L"Supprimer la s\u00e9lection", L"Auswahl l\u00f6schen", L"\u0423\u0434\u0430\u043b\u0438\u0442\u044c \u0432\u044b\u0431\u0440\u0430\u043d\u043d\u043e\u0435", L"\u522a\u9664\u9078\u53d6", L"Eliminar selecci\u00f3n"},
        {L"Delete selected records", L"\u5220\u9664\u9009\u4e2d\u8bb0\u5f55", L"\u0126assar rekords mag\u0127\u017cula", L"\u9078\u629e\u3057\u305f\u8a18\u9332\u3092\u524a\u9664", L"Supprimer les enregistrements s\u00e9lectionn\u00e9s", L"Ausgew\u00e4hlte Eintr\u00e4ge l\u00f6schen", L"\u0423\u0434\u0430\u043b\u0438\u0442\u044c \u0432\u044b\u0431\u0440\u0430\u043d\u043d\u044b\u0435 \u0437\u0430\u043f\u0438\u0441\u0438", L"\u522a\u9664\u9078\u53d6\u8a18\u9304", L"Eliminar registros seleccionados"},
        {L"Delete all Absent records", L"\u5220\u9664\u6240\u6709\u7f3a\u5e2d\u8bb0\u5f55", L"\u0126assar ir-rekords Assenti kollha", L"\u3059\u3079\u3066\u306e\u6b20\u5e2d\u8a18\u9332\u3092\u524a\u9664", L"Supprimer tous les absents", L"Alle abwesenden Eintr\u00e4ge l\u00f6schen", L"\u0423\u0434\u0430\u043b\u0438\u0442\u044c \u0432\u0441\u0435 \u043e\u0442\u0441\u0443\u0442\u0441\u0442\u0432\u0438\u044f", L"\u522a\u9664\u6240\u6709\u7f3a\u5e2d\u8a18\u9304", L"Eliminar todos los ausentes"},
        {L"Delete all Late records", L"\u5220\u9664\u6240\u6709\u8fdf\u5230\u8bb0\u5f55", L"\u0126assar ir-rekords Tard kollha", L"\u3059\u3079\u3066\u306e\u9045\u523b\u8a18\u9332\u3092\u524a\u9664", L"Supprimer tous les retards", L"Alle versp\u00e4teten Eintr\u00e4ge l\u00f6schen", L"\u0423\u0434\u0430\u043b\u0438\u0442\u044c \u0432\u0441\u0435 \u043e\u043f\u043e\u0437\u0434\u0430\u043d\u0438\u044f", L"\u522a\u9664\u6240\u6709\u9072\u5230\u8a18\u9304", L"Eliminar todos los tarde"},
        {L"Clear all records", L"\u6e05\u7a7a\u6240\u6709\u8bb0\u5f55", L"Naddaf ir-rekords kollha", L"\u3059\u3079\u3066\u306e\u8a18\u9332\u3092\u30af\u30ea\u30a2", L"Effacer tous les enregistrements", L"Alle Eintr\u00e4ge l\u00f6schen", L"\u041e\u0447\u0438\u0441\u0442\u0438\u0442\u044c \u0432\u0441\u0435 \u0437\u0430\u043f\u0438\u0441\u0438", L"\u6e05\u7a7a\u6240\u6709\u8a18\u9304", L"Borrar todos los registros"},
        {L"Delete all", L"\u5220\u9664\u6240\u6709", L"\u0126assar kollha", L"\u3059\u3079\u3066\u524a\u9664", L"Supprimer tous les", L"Alle l\u00f6schen", L"\u0423\u0434\u0430\u043b\u0438\u0442\u044c \u0432\u0441\u0435", L"\u522a\u9664\u6240\u6709", L"Eliminar todos"},
        {L"records?", L"\u8bb0\u5f55\uff1f", L"rekords?", L"\u8a18\u9332\uff1f", L"enregistrements ?", L"Eintr\u00e4ge?", L"\u0437\u0430\u043f\u0438\u0441\u0438?", L"\u8a18\u9304\uff1f", L"registros?"},
        {L"Batch Delete", L"\u6279\u91cf\u5220\u9664", L"T\u0127assir bil-lott", L"\u4e00\u62ec\u524a\u9664", L"Suppression group\u00e9e", L"Mehrfach l\u00f6schen", L"\u041f\u0430\u043a\u0435\u0442\u043d\u043e\u0435 \u0443\u0434\u0430\u043b\u0435\u043d\u0438\u0435", L"\u6279\u6b21\u522a\u9664", L"Eliminaci\u00f3n por lote"},
        {L"Clear every attendance record in this sheet?", L"\u6e05\u7a7a\u5f53\u524d\u8868\u5185\u7684\u6240\u6709\u70b9\u540d\u8bb0\u5f55\uff1f", L"Tnaddaf kull rekord f'din il-folja?", L"\u3053\u306e\u30b7\u30fc\u30c8\u306e\u51fa\u5e2d\u8a18\u9332\u3092\u3059\u3079\u3066\u30af\u30ea\u30a2\u3057\u307e\u3059\u304b\uff1f", L"Effacer tous les enregistrements de cette feuille ?", L"Alle Eintr\u00e4ge in diesem Blatt l\u00f6schen?", L"\u041e\u0447\u0438\u0441\u0442\u0438\u0442\u044c \u0432\u0441\u0435 \u0437\u0430\u043f\u0438\u0441\u0438 \u044d\u0442\u043e\u0433\u043e \u043b\u0438\u0441\u0442\u0430?", L"\u6e05\u7a7a\u6b64\u8868\u5167\u6240\u6709\u9ede\u540d\u8a18\u9304\uff1f", L"\u00bfBorrar todos los registros de esta hoja?"},
        {L"Clear All", L"\u5168\u90e8\u6e05\u7a7a", L"Naddaf kollox", L"\u3059\u3079\u3066\u30af\u30ea\u30a2", L"Tout effacer", L"Alles l\u00f6schen", L"\u041e\u0447\u0438\u0441\u0442\u0438\u0442\u044c \u0432\u0441\u0435", L"\u5168\u90e8\u6e05\u7a7a", L"Borrar todo"},
        {L"Save .attd", L"\u4fdd\u5b58 .attd", L"Issejvja .attd", L".attd \u4fdd\u5b58", L"Enregistrer .attd", L".attd speichern", L"\u0421\u043e\u0445\u0440\u0430\u043d\u0438\u0442\u044c .attd", L"\u5132\u5b58 .attd", L"Guardar .attd"},
        {L"Import .attd", L"\u5bfc\u5165 .attd", L"Importa .attd", L".attd \u8aad\u307f\u8fbc\u307f", L"Importer .attd", L".attd importieren", L"\u0418\u043c\u043f\u043e\u0440\u0442 .attd", L"\u532f\u5165 .attd", L"Importar .attd"},
        {L"Export CSV", L"\u5bfc\u51fa CSV", L"Esporta CSV", L"CSV \u30a8\u30af\u30b9\u30dd\u30fc\u30c8", L"Exporter CSV", L"CSV exportieren", L"\u042d\u043a\u0441\u043f\u043e\u0440\u0442 CSV", L"\u532f\u51fa CSV", L"Exportar CSV"},
        {L"Settings", L"\u8bbe\u7f6e", L"Settings", L"\u8a2d\u5b9a", L"Param\u00e8tres", L"Einstellungen", L"\u041d\u0430\u0441\u0442\u0440\u043e\u0439\u043a\u0438", L"\u8a2d\u5b9a", L"Configuraci\u00f3n"},
        {L"Courses", L"\u8bfe\u7a0b", L"Korsijiet", L"\u30b3\u30fc\u30b9", L"Cours", L"Kurse", L"\u041a\u0443\u0440\u0441\u044b", L"\u8ab2\u7a0b", L"Cursos"},
        {L"Tools", L"\u5de5\u5177", L"G\u0127odod", L"\u30c4\u30fc\u30eb", L"Outils", L"Werkzeuge", L"\u0418\u043d\u0441\u0442\u0440\u0443\u043c\u0435\u043d\u0442\u044b", L"\u5de5\u5177", L"Herramientas"},
        {L"OK", L"\u786e\u5b9a", L"OK", L"OK", L"OK", L"OK", L"OK", L"\u78ba\u5b9a", L"OK"},
        {L"Cancel", L"\u53d6\u6d88", L"Ikkan\u010bella", L"\u30ad\u30e3\u30f3\u30bb\u30eb", L"Annuler", L"Abbrechen", L"\u041e\u0442\u043c\u0435\u043d\u0430", L"\u53d6\u6d88", L"Cancelar"},
        {L"Add course/class", L"\u65b0\u589e\u8bfe\u7a0b/\u73ed\u7ea7", L"\u017bid kors/klassi", L"\u30b3\u30fc\u30b9/\u30af\u30e9\u30b9\u3092\u8ffd\u52a0", L"Ajouter cours/classe", L"Kurs/Klasse hinzuf\u00fcgen", L"\u0414\u043e\u0431\u0430\u0432\u0438\u0442\u044c \u043a\u0443\u0440\u0441/\u043a\u043b\u0430\u0441\u0441", L"\u65b0\u589e\u8ab2\u7a0b/\u73ed\u7d1a", L"A\u00f1adir curso/clase"},
        {L"Rename current course/class", L"\u91cd\u547d\u540d\u5f53\u524d\u8bfe\u7a0b/\u73ed\u7ea7", L"Semmi mill-\u0121did il-kors/klassi", L"\u73fe\u5728\u306e\u30b3\u30fc\u30b9/\u30af\u30e9\u30b9\u540d\u3092\u5909\u66f4", L"Renommer cours/classe", L"Kurs/Klasse umbenennen", L"\u041f\u0435\u0440\u0435\u0438\u043c\u0435\u043d\u043e\u0432\u0430\u0442\u044c \u043a\u0443\u0440\u0441/\u043a\u043b\u0430\u0441\u0441", L"\u91cd\u65b0\u547d\u540d\u76ee\u524d\u8ab2\u7a0b/\u73ed\u7d1a", L"Renombrar curso/clase"},
        {L"Delete current course/class", L"\u5220\u9664\u5f53\u524d\u8bfe\u7a0b/\u73ed\u7ea7", L"\u0126assar il-kors/klassi", L"\u73fe\u5728\u306e\u30b3\u30fc\u30b9/\u30af\u30e9\u30b9\u3092\u524a\u9664", L"Supprimer cours/classe", L"Kurs/Klasse l\u00f6schen", L"\u0423\u0434\u0430\u043b\u0438\u0442\u044c \u043a\u0443\u0440\u0441/\u043a\u043b\u0430\u0441\u0441", L"\u522a\u9664\u76ee\u524d\u8ab2\u7a0b/\u73ed\u7d1a", L"Eliminar curso/clase"},
        {L"Add Course/Class", L"\u65b0\u589e\u8bfe\u7a0b/\u73ed\u7ea7", L"\u017bid kors/klassi", L"\u30b3\u30fc\u30b9/\u30af\u30e9\u30b9\u8ffd\u52a0", L"Ajouter cours/classe", L"Kurs/Klasse hinzuf\u00fcgen", L"\u0414\u043e\u0431\u0430\u0432\u0438\u0442\u044c \u043a\u0443\u0440\u0441/\u043a\u043b\u0430\u0441\u0441", L"\u65b0\u589e\u8ab2\u7a0b/\u73ed\u7d1a", L"A\u00f1adir curso/clase"},
        {L"Course or class name:", L"\u8bfe\u7a0b\u6216\u73ed\u7ea7\u540d\u79f0\uff1a", L"Isem tal-kors jew klassi:", L"\u30b3\u30fc\u30b9\u307e\u305f\u306f\u30af\u30e9\u30b9\u540d:", L"Nom du cours ou de la classe :", L"Kurs- oder Klassenname:", L"\u0418\u043c\u044f \u043a\u0443\u0440\u0441\u0430/\u043a\u043b\u0430\u0441\u0441\u0430:", L"\u8ab2\u7a0b\u6216\u73ed\u7d1a\u540d\u7a31\uff1a", L"Nombre del curso o clase:"},
        {L"Rename Course/Class", L"\u91cd\u547d\u540d\u8bfe\u7a0b/\u73ed\u7ea7", L"Semmi mill-\u0121did", L"\u30b3\u30fc\u30b9/\u30af\u30e9\u30b9\u540d\u5909\u66f4", L"Renommer cours/classe", L"Kurs/Klasse umbenennen", L"\u041f\u0435\u0440\u0435\u0438\u043c\u0435\u043d\u043e\u0432\u0430\u0442\u044c", L"\u91cd\u65b0\u547d\u540d\u8ab2\u7a0b/\u73ed\u7d1a", L"Renombrar curso/clase"},
        {L"New course or class name:", L"\u65b0\u8bfe\u7a0b\u6216\u73ed\u7ea7\u540d\u79f0\uff1a", L"Isem \u0121did tal-kors jew klassi:", L"\u65b0\u3057\u3044\u30b3\u30fc\u30b9/\u30af\u30e9\u30b9\u540d:", L"Nouveau nom :", L"Neuer Kurs-/Klassenname:", L"\u041d\u043e\u0432\u043e\u0435 \u0438\u043c\u044f:", L"\u65b0\u8ab2\u7a0b\u6216\u73ed\u7d1a\u540d\u7a31\uff1a", L"Nuevo nombre:"},
        {L"Import student roster (CSV)", L"\u5bfc\u5165\u5b66\u751f\u540d\u5355 (CSV)", L"Importa lista tal-istudenti (CSV)", L"\u5b66\u751f\u540d\u7c3f\u3092\u8aad\u307f\u8fbc\u3080 (CSV)", L"Importer liste d'\u00e9tudiants (CSV)", L"Sch\u00fclerliste importieren (CSV)", L"\u0418\u043c\u043f\u043e\u0440\u0442 \u0441\u043f\u0438\u0441\u043a\u0430 (CSV)", L"\u532f\u5165\u5b78\u751f\u540d\u55ae (CSV)", L"Importar lista de estudiantes (CSV)"},
        {L"Manage students", L"\u7ba1\u7406\u5b66\u751f", L"Immani\u0121\u0121ja studenti", L"\u5b66\u751f\u7ba1\u7406", L"G\u00e9rer les \u00e9tudiants", L"Sch\u00fcler verwalten", L"\u0423\u043f\u0440\u0430\u0432\u043b\u044f\u0442\u044c \u0443\u0447\u0435\u043d\u0438\u043a\u0430\u043c\u0438", L"\u7ba1\u7406\u5b78\u751f", L"Gestionar estudiantes"},
        {L"Add student to roster", L"\u6dfb\u52a0\u5b66\u751f\u5230\u540d\u5355", L"\u017bid student mal-lista", L"\u540d\u7c3f\u306b\u5b66\u751f\u3092\u8ffd\u52a0", L"Ajouter un \u00e9tudiant \u00e0 la liste", L"Sch\u00fcler zur Liste hinzuf\u00fcgen", L"\u0414\u043e\u0431\u0430\u0432\u0438\u0442\u044c \u0443\u0447\u0435\u043d\u0438\u043a\u0430 \u0432 \u0441\u043f\u0438\u0441\u043e\u043a", L"\u65b0\u589e\u5b78\u751f\u5230\u540d\u55ae", L"Agregar estudiante a la lista"},
        {L"Remove student from roster", L"\u4ece\u540d\u5355\u79fb\u9664\u5b66\u751f", L"Ne\u0127\u0127i student mil-lista", L"\u540d\u7c3f\u304b\u3089\u5b66\u751f\u3092\u524a\u9664", L"Retirer un \u00e9tudiant de la liste", L"Sch\u00fcler aus Liste entfernen", L"\u0423\u0434\u0430\u043b\u0438\u0442\u044c \u0443\u0447\u0435\u043d\u0438\u043a\u0430 \u0438\u0437 \u0441\u043f\u0438\u0441\u043a\u0430", L"\u5f9e\u540d\u55ae\u79fb\u9664\u5b78\u751f", L"Quitar estudiante de la lista"},
        {L"Create records from roster", L"\u4ece\u540d\u5355\u521b\u5efa\u70b9\u540d\u8bb0\u5f55", L"O\u0127loq rekords mil-lista", L"\u540d\u7c3f\u304b\u3089\u8a18\u9332\u3092\u4f5c\u6210", L"Cr\u00e9er des enregistrements depuis la liste", L"Eintr\u00e4ge aus Liste erstellen", L"\u0421\u043e\u0437\u0434\u0430\u0442\u044c \u0437\u0430\u043f\u0438\u0441\u0438 \u0438\u0437 \u0441\u043f\u0438\u0441\u043a\u0430", L"\u5f9e\u540d\u55ae\u5efa\u7acb\u9ede\u540d\u8a18\u9304", L"Crear registros desde la lista"},
        {L"Course details", L"\u8bfe\u7a0b\u8d44\u6599", L"Dettalji tal-kors", L"\u30b3\u30fc\u30b9\u8a73\u7d30", L"D\u00e9tails du cours", L"Kursdetails", L"\u0421\u0432\u0435\u0434\u0435\u043d\u0438\u044f \u043e \u043a\u0443\u0440\u0441\u0435", L"\u8ab2\u7a0b\u8cc7\u6599", L"Detalles del curso"},
        {L"Teacher/owner:", L"\u6559\u5e08/\u8d1f\u8d23\u4eba\uff1a", L"G\u0127alliem/sid:", L"\u6559\u5e2b/\u62c5\u5f53:", L"Enseignant/responsable :", L"Lehrer/verantwortlich:", L"\u0423\u0447\u0438\u0442\u0435\u043b\u044c/\u043e\u0442\u0432\u0435\u0442\u0441\u0442\u0432\u0435\u043d\u043d\u044b\u0439:", L"\u6559\u5e2b/\u8ca0\u8cac\u4eba\uff1a", L"Profesor/responsable:"},
        {L"Location:", L"\u5730\u70b9\uff1a", L"Post:", L"\u5834\u6240:", L"Lieu :", L"Ort:", L"\u041c\u0435\u0441\u0442\u043e:", L"\u5730\u9ede\uff1a", L"Ubicaci\u00f3n:"},
        {L"Course notes:", L"\u8bfe\u7a0b\u5907\u6ce8\uff1a", L"Noti tal-kors:", L"\u30b3\u30fc\u30b9\u30e1\u30e2:", L"Notes du cours :", L"Kursnotizen:", L"\u0417\u0430\u043c\u0435\u0442\u043a\u0438 \u043a\u0443\u0440\u0441\u0430:", L"\u8ab2\u7a0b\u5099\u8a3b\uff1a", L"Notas del curso:"},
        {L"Student name:", L"\u5b66\u751f\u59d3\u540d\uff1a", L"Isem tal-istudent:", L"\u5b66\u751f\u540d:", L"Nom de l'\u00e9tudiant :", L"Sch\u00fclername:", L"\u0418\u043c\u044f \u0443\u0447\u0435\u043d\u0438\u043a\u0430:", L"\u5b78\u751f\u59d3\u540d\uff1a", L"Nombre del estudiante:"},
        {L"Student added.", L"\u5df2\u6dfb\u52a0\u5b66\u751f\u3002", L"Student mi\u017cjud.", L"\u5b66\u751f\u3092\u8ffd\u52a0\u3057\u307e\u3057\u305f\u3002", L"\u00c9tudiant ajout\u00e9.", L"Sch\u00fcler hinzugef\u00fcgt.", L"\u0423\u0447\u0435\u043d\u0438\u043a \u0434\u043e\u0431\u0430\u0432\u043b\u0435\u043d.", L"\u5df2\u65b0\u589e\u5b78\u751f\u3002", L"Estudiante agregado."},
        {L"Student removed.", L"\u5df2\u79fb\u9664\u5b66\u751f\u3002", L"Student imne\u0127\u0127i.", L"\u5b66\u751f\u3092\u524a\u9664\u3057\u307e\u3057\u305f\u3002", L"\u00c9tudiant retir\u00e9.", L"Sch\u00fcler entfernt.", L"\u0423\u0447\u0435\u043d\u0438\u043a \u0443\u0434\u0430\u043b\u0451\u043d.", L"\u5df2\u79fb\u9664\u5b78\u751f\u3002", L"Estudiante quitado."},
        {L"No students in the roster.", L"\u540d\u5355\u4e2d\u6ca1\u6709\u5b66\u751f\u3002", L"M'hemmx studenti fil-lista.", L"\u540d\u7c3f\u306b\u5b66\u751f\u304c\u3044\u307e\u305b\u3093\u3002", L"Aucun \u00e9tudiant dans la liste.", L"Keine Sch\u00fcler in der Liste.", L"\u0412 \u0441\u043f\u0438\u0441\u043a\u0435 \u043d\u0435\u0442 \u0443\u0447\u0435\u043d\u0438\u043a\u043e\u0432.", L"\u540d\u55ae\u4e2d\u6c92\u6709\u5b78\u751f\u3002", L"No hay estudiantes en la lista."},
        {L"Roster records created.", L"\u5df2\u4ece\u540d\u5355\u521b\u5efa\u8bb0\u5f55\u3002", L"In\u0127olqu rekords mil-lista.", L"\u540d\u7c3f\u304b\u3089\u8a18\u9332\u3092\u4f5c\u6210\u3057\u307e\u3057\u305f\u3002", L"Enregistrements cr\u00e9\u00e9s depuis la liste.", L"Eintr\u00e4ge aus Liste erstellt.", L"\u0417\u0430\u043f\u0438\u0441\u0438 \u0438\u0437 \u0441\u043f\u0438\u0441\u043a\u0430 \u0441\u043e\u0437\u0434\u0430\u043d\u044b.", L"\u5df2\u5f9e\u540d\u55ae\u5efa\u7acb\u8a18\u9304\u3002", L"Registros creados desde la lista."},
        {L"Statistics summary", L"\u7edf\u8ba1\u6458\u8981", L"Sommarju statistiku", L"\u7d71\u8a08\u30b5\u30de\u30ea\u30fc", L"R\u00e9sum\u00e9 statistique", L"Statistik\u00fcbersicht", L"\u0421\u0432\u043e\u0434\u043a\u0430 \u0441\u0442\u0430\u0442\u0438\u0441\u0442\u0438\u043a\u0438", L"\u7d71\u8a08\u6458\u8981", L"Resumen estad\u00edstico"},
        {L"Top absent/late students", L"\u7f3a\u5e2d/\u8fdf\u5230\u6700\u591a\u5b66\u751f", L"L-aktar studenti assenti/tard", L"\u6b20\u5e2d/\u9045\u523b\u304c\u591a\u3044\u5b66\u751f", L"\u00c9tudiants les plus absents/en retard", L"Sch\u00fcler mit meisten Fehlzeiten/Versp\u00e4tungen", L"\u0427\u0430\u0449\u0435 \u0432\u0441\u0435\u0433\u043e \u043e\u0442\u0441\u0443\u0442\u0441\u0442\u0432\u0443\u044e\u0442/\u043e\u043f\u0430\u0437\u0434\u044b\u0432\u0430\u044e\u0442", L"\u7f3a\u5e2d/\u9072\u5230\u6700\u591a\u5b78\u751f", L"Estudiantes con m\u00e1s ausencias/tardanzas"},
        {L"Backup now", L"\u7acb\u5373\u5907\u4efd", L"Ag\u0127mel backup issa", L"\u4eca\u3059\u3050\u30d0\u30c3\u30af\u30a2\u30c3\u30d7", L"Sauvegarder maintenant", L"Jetzt sichern", L"\u0421\u043e\u0437\u0434\u0430\u0442\u044c \u0440\u0435\u0437\u0435\u0440\u0432\u043d\u0443\u044e \u043a\u043e\u043f\u0438\u044e", L"\u7acb\u5373\u5099\u4efd", L"Hacer copia ahora"},
        {L"Restore latest backup", L"\u6062\u590d\u6700\u65b0\u5907\u4efd", L"Irrestawra l-a\u0127\u0127ar backup", L"\u6700\u65b0\u30d0\u30c3\u30af\u30a2\u30c3\u30d7\u3092\u5fa9\u5143", L"Restaurer la derni\u00e8re sauvegarde", L"Letzte Sicherung wiederherstellen", L"\u0412\u043e\u0441\u0441\u0442\u0430\u043d\u043e\u0432\u0438\u0442\u044c \u043f\u043e\u0441\u043b\u0435\u0434\u043d\u044e\u044e \u043a\u043e\u043f\u0438\u044e", L"\u9084\u539f\u6700\u65b0\u5099\u4efd", L"Restaurar copia reciente"},
        {L"Open recent file", L"\u6253\u5f00\u6700\u8fd1\u6587\u4ef6", L"Ifta\u0127 fajl re\u010benti", L"\u6700\u8fd1\u306e\u30d5\u30a1\u30a4\u30eb\u3092\u958b\u304f", L"Ouvrir un fichier r\u00e9cent", L"Zuletzt verwendete Datei \u00f6ffnen", L"\u041e\u0442\u043a\u0440\u044b\u0442\u044c \u043d\u0435\u0434\u0430\u0432\u043d\u0438\u0439 \u0444\u0430\u0439\u043b", L"\u958b\u555f\u6700\u8fd1\u6a94\u6848", L"Abrir archivo reciente"},
        {L"Backup created:", L"\u5907\u4efd\u5df2\u521b\u5efa\uff1a", L"Backup ma\u0127luq:", L"\u30d0\u30c3\u30af\u30a2\u30c3\u30d7\u4f5c\u6210\u5148:", L"Sauvegarde cr\u00e9\u00e9e :", L"Sicherung erstellt:", L"\u0420\u0435\u0437\u0435\u0440\u0432\u043d\u0430\u044f \u043a\u043e\u043f\u0438\u044f \u0441\u043e\u0437\u0434\u0430\u043d\u0430:", L"\u5099\u4efd\u5df2\u5efa\u7acb\uff1a", L"Copia creada:"},
        {L"No backup file was found.", L"\u672a\u627e\u5230\u5907\u4efd\u6587\u4ef6\u3002", L"Ma nstabx fajl backup.", L"\u30d0\u30c3\u30af\u30a2\u30c3\u30d7\u30d5\u30a1\u30a4\u30eb\u304c\u3042\u308a\u307e\u305b\u3093\u3002", L"Aucune sauvegarde trouv\u00e9e.", L"Keine Sicherungsdatei gefunden.", L"\u0424\u0430\u0439\u043b \u043a\u043e\u043f\u0438\u0438 \u043d\u0435 \u043d\u0430\u0439\u0434\u0435\u043d.", L"\u672a\u627e\u5230\u5099\u4efd\u6a94\u6848\u3002", L"No se encontr\u00f3 copia."},
        {L"No recent file was found.", L"\u672a\u627e\u5230\u6700\u8fd1\u6587\u4ef6\u3002", L"Ma nstabx fajl re\u010benti.", L"\u6700\u8fd1\u306e\u30d5\u30a1\u30a4\u30eb\u304c\u3042\u308a\u307e\u305b\u3093\u3002", L"Aucun fichier r\u00e9cent trouv\u00e9.", L"Keine zuletzt verwendete Datei gefunden.", L"\u041d\u0435\u0434\u0430\u0432\u043d\u0438\u0439 \u0444\u0430\u0439\u043b \u043d\u0435 \u043d\u0430\u0439\u0434\u0435\u043d.", L"\u672a\u627e\u5230\u6700\u8fd1\u6a94\u6848\u3002", L"No se encontr\u00f3 archivo reciente."},
        {L"Set default save folder", L"\u8bbe\u7f6e\u9ed8\u8ba4\u4fdd\u5b58\u6587\u4ef6\u5939", L"Issettja folder ta' save default", L"\u65e2\u5b9a\u306e\u4fdd\u5b58\u30d5\u30a9\u30eb\u30c0\u30fc\u3092\u8a2d\u5b9a", L"D\u00e9finir le dossier d'enregistrement", L"Standard-Speicherordner festlegen", L"\u0417\u0430\u0434\u0430\u0442\u044c \u043f\u0430\u043f\u043a\u0443 \u0441\u043e\u0445\u0440\u0430\u043d\u0435\u043d\u0438\u044f", L"\u8a2d\u5b9a\u9810\u8a2d\u5132\u5b58\u8cc7\u6599\u593e", L"Definir carpeta predeterminada"},
        {L"Default save folder updated:", L"\u9ed8\u8ba4\u4fdd\u5b58\u6587\u4ef6\u5939\u5df2\u66f4\u65b0\uff1a", L"Folder ta' save default a\u0121\u0121ornat:", L"\u65e2\u5b9a\u306e\u4fdd\u5b58\u30d5\u30a9\u30eb\u30c0\u30fc\u3092\u66f4\u65b0:", L"Dossier d'enregistrement mis \u00e0 jour :", L"Standard-Speicherordner aktualisiert:", L"\u041f\u0430\u043f\u043a\u0430 \u0441\u043e\u0445\u0440\u0430\u043d\u0435\u043d\u0438\u044f \u043e\u0431\u043d\u043e\u0432\u043b\u0435\u043d\u0430:", L"\u9810\u8a2d\u5132\u5b58\u8cc7\u6599\u593e\u5df2\u66f4\u65b0\uff1a", L"Carpeta predeterminada actualizada:"},
        {L"Print / export PDF", L"\u6253\u5370 / \u5bfc\u51fa PDF", L"Stampa / esporta PDF", L"\u5370\u5237 / PDF \u51fa\u529b", L"Imprimer / exporter PDF", L"Drucken / PDF exportieren", L"\u041f\u0435\u0447\u0430\u0442\u044c / PDF", L"\u5217\u5370 / \u532f\u51fa PDF", L"Imprimir / exportar PDF"},
        {L"Export PowerPoint (.pptx)", L"\u5bfc\u51fa PowerPoint (.pptx)", L"Esporta PowerPoint (.pptx)", L"PowerPoint \u51fa\u529b (.pptx)", L"Exporter PowerPoint (.pptx)", L"PowerPoint exportieren (.pptx)", L"\u042d\u043a\u0441\u043f\u043e\u0440\u0442 PowerPoint (.pptx)", L"\u532f\u51fa PowerPoint (.pptx)", L"Exportar PowerPoint (.pptx)"},
        {L"Statistics chart", L"\u7edf\u8ba1\u56fe\u8868", L"\u010aart tal-istatistika", L"\u7d71\u8a08\u30b0\u30e9\u30d5", L"Graphique statistique", L"Statistikdiagramm", L"\u0413\u0440\u0430\u0444\u0438\u043a", L"\u7d71\u8a08\u5716\u8868", L"Gr\u00e1fico estad\u00edstico"},
        {L"Undo", L"\u64a4\u9500", L"\u0126oll", L"\u5143\u306b\u623b\u3059", L"Annuler", L"R\u00fcckg\u00e4ngig", L"\u041e\u0442\u043c\u0435\u043d\u0438\u0442\u044c", L"\u5fa9\u539f", L"Deshacer"},
        {L"Redo", L"\u91cd\u505a", L"Er\u0121a'", L"\u3084\u308a\u76f4\u3057", L"R\u00e9tablir", L"Wiederholen", L"\u041f\u043e\u0432\u0442\u043e\u0440\u0438\u0442\u044c", L"\u91cd\u505a", L"Rehacer"},
        {L"Keyboard shortcuts", L"\u5feb\u6377\u952e", L"Shortcuts", L"\u30b7\u30e7\u30fc\u30c8\u30ab\u30c3\u30c8", L"Raccourcis clavier", L"Tastenk\u00fcrzel", L"\u0413\u043e\u0440\u044f\u0447\u0438\u0435 \u043a\u043b\u0430\u0432\u0438\u0448\u0438", L"\u5feb\u6377\u9375", L"Atajos de teclado"},
        {L"Export database mirror", L"\u5bfc\u51fa\u6570\u636e\u5e93\u955c\u50cf", L"Esporta kopja tad-database", L"\u30c7\u30fc\u30bf\u30d9\u30fc\u30b9\u30df\u30e9\u30fc\u51fa\u529b", L"Exporter miroir de base", L"Datenbankspiegel exportieren", L"\u042d\u043a\u0441\u043f\u043e\u0440\u0442 \u0437\u0435\u0440\u043a\u0430\u043b\u0430 \u0411\u0414", L"\u532f\u51fa\u8cc7\u6599\u5eab\u93e1\u50cf", L"Exportar espejo de base"},
        {L"Open autosave", L"\u6253\u5f00\u81ea\u52a8\u4fdd\u5b58", L"Ifta\u0127 autosave", L"\u81ea\u52d5\u4fdd\u5b58\u3092\u958b\u304f", L"Ouvrir autosauvegarde", L"Autosave \u00f6ffnen", L"\u041e\u0442\u043a\u0440\u044b\u0442\u044c \u0430\u0432\u0442\u043e\u0441\u043e\u0445\u0440.", L"\u958b\u555f\u81ea\u52d5\u5132\u5b58", L"Abrir autoguardado"},
        {L"At least one course must remain.", L"\u81f3\u5c11\u9700\u8981\u4fdd\u7559\u4e00\u4e2a\u8bfe\u7a0b\u3002", L"Irid jibqa' mill-inqas kors wie\u0127ed.", L"\u5c11\u306a\u304f\u3068\u30821\u3064\u306e\u30b3\u30fc\u30b9\u304c\u5fc5\u8981\u3067\u3059\u3002", L"Au moins un cours doit rester.", L"Mindestens ein Kurs muss bleiben.", L"\u0414\u043e\u043b\u0436\u0435\u043d \u043e\u0441\u0442\u0430\u0442\u044c\u0441\u044f \u0445\u043e\u0442\u044f \u0431\u044b \u043e\u0434\u0438\u043d \u043a\u0443\u0440\u0441.", L"\u81f3\u5c11\u9700\u4fdd\u7559\u4e00\u500b\u8ab2\u7a0b\u3002", L"Debe quedar al menos un curso."},
        {L"Delete the current course and its records?", L"\u5220\u9664\u5f53\u524d\u8bfe\u7a0b\u53ca\u5176\u8bb0\u5f55\uff1f", L"\u0126assar il-kors u r-rekords?", L"\u73fe\u5728\u306e\u30b3\u30fc\u30b9\u3068\u8a18\u9332\u3092\u524a\u9664\u3057\u307e\u3059\u304b\uff1f", L"Supprimer ce cours et ses enregistrements ?", L"Aktuellen Kurs und Eintr\u00e4ge l\u00f6schen?", L"\u0423\u0434\u0430\u043b\u0438\u0442\u044c \u0442\u0435\u043a\u0443\u0449\u0438\u0439 \u043a\u0443\u0440\u0441 \u0438 \u0437\u0430\u043f\u0438\u0441\u0438?", L"\u522a\u9664\u76ee\u524d\u8ab2\u7a0b\u53ca\u5176\u8a18\u9304\uff1f", L"\u00bfEliminar el curso actual y sus registros?"},
        {L"Delete Course", L"\u5220\u9664\u8bfe\u7a0b", L"\u0126assar kors", L"\u30b3\u30fc\u30b9\u524a\u9664", L"Supprimer le cours", L"Kurs l\u00f6schen", L"\u0423\u0434\u0430\u043b\u0438\u0442\u044c \u043a\u0443\u0440\u0441", L"\u522a\u9664\u8ab2\u7a0b", L"Eliminar curso"},
        {L"Nothing to undo.", L"\u6ca1\u6709\u53ef\u64a4\u9500\u7684\u64cd\u4f5c\u3002", L"Xejn x'tre\u0121\u0121a' lura.", L"\u5143\u306b\u623b\u3059\u64cd\u4f5c\u306f\u3042\u308a\u307e\u305b\u3093\u3002", L"Rien \u00e0 annuler.", L"Nichts r\u00fcckg\u00e4ngig zu machen.", L"\u041d\u0435\u0447\u0435\u0433\u043e \u043e\u0442\u043c\u0435\u043d\u044f\u0442\u044c.", L"\u6c92\u6709\u53ef\u5fa9\u539f\u7684\u64cd\u4f5c\u3002", L"Nada que deshacer."},
        {L"Nothing to redo.", L"\u6ca1\u6709\u53ef\u91cd\u505a\u7684\u64cd\u4f5c\u3002", L"Xejn x'ter\u0121a' tag\u0127mel.", L"\u3084\u308a\u76f4\u3059\u64cd\u4f5c\u306f\u3042\u308a\u307e\u305b\u3093\u3002", L"Rien \u00e0 r\u00e9tablir.", L"Nichts zu wiederholen.", L"\u041d\u0435\u0447\u0435\u0433\u043e \u043f\u043e\u0432\u0442\u043e\u0440\u044f\u0442\u044c.", L"\u6c92\u6709\u53ef\u91cd\u505a\u7684\u64cd\u4f5c\u3002", L"Nada que rehacer."},
        {L"Could not export the print file.", L"\u65e0\u6cd5\u5bfc\u51fa\u6253\u5370\u6587\u4ef6\u3002", L"Ma setax ji\u0121i esportat il-fajl.", L"\u5370\u5237\u30d5\u30a1\u30a4\u30eb\u3092\u51fa\u529b\u3067\u304d\u307e\u305b\u3093\u3002", L"Impossible d'exporter le fichier.", L"Druckdatei konnte nicht exportiert werden.", L"\u041d\u0435 \u0443\u0434\u0430\u043b\u043e\u0441\u044c \u044d\u043a\u0441\u043f\u043e\u0440\u0442.", L"\u7121\u6cd5\u532f\u51fa\u5217\u5370\u6a94\u6848\u3002", L"No se pudo exportar el archivo."},
        {L"Could not open the roster file.", L"\u65e0\u6cd5\u6253\u5f00\u540d\u5355\u6587\u4ef6\u3002", L"Ma setax jinfeta\u0127 il-fajl.", L"\u540d\u7c3f\u30d5\u30a1\u30a4\u30eb\u3092\u958b\u3051\u307e\u305b\u3093\u3002", L"Impossible d'ouvrir la liste.", L"Listendatei konnte nicht ge\u00f6ffnet werden.", L"\u041d\u0435 \u0443\u0434\u0430\u043b\u043e\u0441\u044c \u043e\u0442\u043a\u0440\u044b\u0442\u044c \u0441\u043f\u0438\u0441\u043e\u043a.", L"\u7121\u6cd5\u958b\u555f\u540d\u55ae\u6a94\u6848\u3002", L"No se pudo abrir la lista."},
        {L"Imported", L"\u5df2\u5bfc\u5165", L"Importati", L"\u8aad\u307f\u8fbc\u307f\u6e08\u307f", L"Import\u00e9", L"Importiert", L"\u0418\u043c\u043f\u043e\u0440\u0442\u0438\u0440\u043e\u0432\u0430\u043d\u043e", L"\u5df2\u532f\u5165", L"Importado"},
        {L"students into the current course.", L"\u540d\u5b66\u751f\u5230\u5f53\u524d\u8bfe\u7a0b\u3002", L"studenti fil-kors attwali.", L"\u4eba\u306e\u5b66\u751f\u3092\u73fe\u5728\u306e\u30b3\u30fc\u30b9\u306b\u8ffd\u52a0\u3002", L"\u00e9tudiants dans le cours actuel.", L"Sch\u00fcler in den aktuellen Kurs.", L"\u0441\u0442\u0443\u0434\u0435\u043d\u0442\u043e\u0432 \u0432 \u0442\u0435\u043a\u0443\u0449\u0438\u0439 \u043a\u0443\u0440\u0441.", L"\u540d\u5b78\u751f\u5230\u76ee\u524d\u8ab2\u7a0b\u3002", L"estudiantes al curso actual."},
        {L"Imported roster entries start as Absent until marked.", L"\u5bfc\u5165\u7684\u540d\u5355\u8bb0\u5f55\u5728\u70b9\u540d\u524d\u9ed8\u8ba4\u4e3a\u7f3a\u5e2d\u3002", L"Rekords importati jibdew b\u0127ala Assenti.", L"\u8aad\u307f\u8fbc\u3093\u3060\u540d\u7c3f\u306f\u51fa\u5e2d\u78ba\u8a8d\u307e\u3067\u6b20\u5e2d\u3068\u3057\u3066\u6271\u308f\u308c\u307e\u3059\u3002", L"Les entr\u00e9es import\u00e9es commencent comme absentes.", L"Importierte Eintr\u00e4ge starten als abwesend.", L"\u0418\u043c\u043f\u043e\u0440\u0442\u0438\u0440\u043e\u0432\u0430\u043d\u043d\u044b\u0435 \u0437\u0430\u043f\u0438\u0441\u0438 \u0441\u043d\u0430\u0447\u0430\u043b\u0430 \u043e\u0442\u043c\u0435\u0447\u0435\u043d\u044b \u043a\u0430\u043a \u043e\u0442\u0441\u0443\u0442\u0441\u0442\u0432\u0443\u044e\u0449\u0438\u0435.", L"\u532f\u5165\u7684\u540d\u55ae\u8a18\u9304\u5728\u9ede\u540d\u524d\u9810\u8a2d\u70ba\u7f3a\u5e2d\u3002", L"Las entradas importadas empiezan como ausentes."},
        {L"Could not write the database file.", L"\u65e0\u6cd5\u5199\u5165\u6570\u636e\u5e93\u6587\u4ef6\u3002", L"Ma setax jinkiteb id-database.", L"\u30c7\u30fc\u30bf\u30d9\u30fc\u30b9\u30d5\u30a1\u30a4\u30eb\u3092\u66f8\u304d\u8fbc\u3081\u307e\u305b\u3093\u3002", L"Impossible d'\u00e9crire la base.", L"Datenbankdatei konnte nicht geschrieben werden.", L"\u041d\u0435 \u0443\u0434\u0430\u043b\u043e\u0441\u044c \u0437\u0430\u043f\u0438\u0441\u0430\u0442\u044c \u0411\u0414.", L"\u7121\u6cd5\u5beb\u5165\u8cc7\u6599\u5eab\u6a94\u6848\u3002", L"No se pudo escribir la base."},
        {L"Database mirror exported to:", L"\u6570\u636e\u5e93\u955c\u50cf\u5df2\u5bfc\u51fa\u5230\uff1a", L"Kopja tad-database esportata lejn:", L"\u30c7\u30fc\u30bf\u30d9\u30fc\u30b9\u30df\u30e9\u30fc\u306e\u51fa\u529b\u5148:", L"Miroir de base export\u00e9 vers :", L"Datenbankspiegel exportiert nach:", L"\u0417\u0435\u0440\u043a\u0430\u043b\u043e \u0411\u0414 \u044d\u043a\u0441\u043f\u043e\u0440\u0442\u0438\u0440\u043e\u0432\u0430\u043d\u043e:", L"\u8cc7\u6599\u5eab\u93e1\u50cf\u5df2\u532f\u51fa\u5230\uff1a", L"Espejo exportado a:"},
        {L"No autosave file was found.", L"\u672a\u627e\u5230\u81ea\u52a8\u4fdd\u5b58\u6587\u4ef6\u3002", L"Ma nstabx autosave.", L"\u81ea\u52d5\u4fdd\u5b58\u30d5\u30a1\u30a4\u30eb\u306f\u3042\u308a\u307e\u305b\u3093\u3002", L"Aucune autosauvegarde trouv\u00e9e.", L"Keine Autosave-Datei gefunden.", L"\u0410\u0432\u0442\u043e\u0441\u043e\u0445\u0440. \u043d\u0435 \u043d\u0430\u0439\u0434\u0435\u043d\u043e.", L"\u672a\u627e\u5230\u81ea\u52d5\u5132\u5b58\u6a94\u6848\u3002", L"No se encontr\u00f3 autoguardado."},
        {L"An autosaved attendance file was found. Restore it now?", L"\u627e\u5230\u81ea\u52a8\u4fdd\u5b58\u7684\u70b9\u540d\u6587\u4ef6\u3002\u662f\u5426\u73b0\u5728\u6062\u590d\uff1f", L"Instab fajl autosave. Tirrestawrah issa?", L"\u81ea\u52d5\u4fdd\u5b58\u304c\u898b\u3064\u304b\u308a\u307e\u3057\u305f\u3002\u5fa9\u5143\u3057\u307e\u3059\u304b\uff1f", L"Autosauvegarde trouv\u00e9e. Restaurer ?", L"Autosave gefunden. Jetzt wiederherstellen?", L"\u041d\u0430\u0439\u0434\u0435\u043d\u043e \u0430\u0432\u0442\u043e\u0441\u043e\u0445\u0440. \u0412\u043e\u0441\u0441\u0442\u0430\u043d\u043e\u0432\u0438\u0442\u044c?", L"\u627e\u5230\u81ea\u52d5\u5132\u5b58\u6a94\u3002\u662f\u5426\u73fe\u5728\u9084\u539f\uff1f", L"Se encontr\u00f3 autoguardado. \u00bfRestaurar?"},
        {L"Restore Autosave", L"\u6062\u590d\u81ea\u52a8\u4fdd\u5b58", L"Irrestawra autosave", L"\u81ea\u52d5\u4fdd\u5b58\u5fa9\u5143", L"Restaurer autosauvegarde", L"Autosave wiederherstellen", L"\u0412\u043e\u0441\u0441\u0442. \u0430\u0432\u0442\u043e\u0441\u043e\u0445\u0440.", L"\u9084\u539f\u81ea\u52d5\u5132\u5b58", L"Restaurar autoguardado"},
        {L"Keyboard Shortcuts", L"\u5feb\u6377\u952e", L"Shortcuts", L"\u30b7\u30e7\u30fc\u30c8\u30ab\u30c3\u30c8", L"Raccourcis clavier", L"Tastenk\u00fcrzel", L"\u0413\u043e\u0440\u044f\u0447\u0438\u0435 \u043a\u043b\u0430\u0432\u0438\u0448\u0438", L"\u5feb\u6377\u9375", L"Atajos de teclado"},
        {L"Statistics Chart", L"\u7edf\u8ba1\u56fe\u8868", L"\u010aart tal-istatistika", L"\u7d71\u8a08\u30b0\u30e9\u30d5", L"Graphique statistique", L"Statistikdiagramm", L"\u0413\u0440\u0430\u0444\u0438\u043a", L"\u7d71\u8a08\u5716\u8868", L"Gr\u00e1fico estad\u00edstico"},
        {L"Print / Save as PDF", L"\u6253\u5370 / \u53e6\u5b58\u4e3a PDF", L"Stampa / PDF", L"\u5370\u5237 / PDF\u4fdd\u5b58", L"Imprimer / PDF", L"Drucken / PDF speichern", L"\u041f\u0435\u0447\u0430\u0442\u044c / PDF", L"\u5217\u5370 / \u53e6\u5b58 PDF", L"Imprimir / guardar PDF"},
        {L"New attendance sheet name:", L"\u65b0\u70b9\u540d\u8868\u540d\u79f0\uff1a", L"Isem tal-folja \u0121dida:", L"\u65b0\u3057\u3044\u51fa\u5e2d\u30b7\u30fc\u30c8\u540d:", L"Nom de la nouvelle feuille :", L"Name des neuen Blatts:", L"\u0418\u043c\u044f \u043d\u043e\u0432\u043e\u0433\u043e \u043b\u0438\u0441\u0442\u0430:", L"\u65b0\u9ede\u540d\u8868\u540d\u7a31\uff1a", L"Nombre de la nueva hoja:"},
        {L"New Attendance", L"\u65b0\u5efa\u70b9\u540d", L"Attendenza \u0121dida", L"\u65b0\u898f\u51fa\u5e2d", L"Nouvel appel", L"Neue Anwesenheit", L"\u041d\u043e\u0432\u0430\u044f \u043f\u043e\u0441\u0435\u0449\u0430\u0435\u043c\u043e\u0441\u0442\u044c", L"\u65b0\u589e\u9ede\u540d", L"Nueva asistencia"},
        {L"Please enter a date and time.", L"\u8bf7\u8f93\u5165\u65e5\u671f\u548c\u65f6\u95f4\u3002", L"Da\u0127\u0127al data u \u0127in.", L"\u65e5\u6642\u3092\u5165\u529b\u3057\u3066\u304f\u3060\u3055\u3044\u3002", L"Veuillez saisir la date et l'heure.", L"Bitte Datum und Uhrzeit eingeben.", L"\u0412\u0432\u0435\u0434\u0438\u0442\u0435 \u0434\u0430\u0442\u0443 \u0438 \u0432\u0440\u0435\u043c\u044f.", L"\u8acb\u8f38\u5165\u65e5\u671f\u548c\u6642\u9593\u3002", L"Introduce fecha y hora."},
        {L"Please enter a name.", L"\u8bf7\u8f93\u5165\u59d3\u540d\u3002", L"Da\u0127\u0127al isem.", L"\u540d\u524d\u3092\u5165\u529b\u3057\u3066\u304f\u3060\u3055\u3044\u3002", L"Veuillez saisir un nom.", L"Bitte Namen eingeben.", L"\u0412\u0432\u0435\u0434\u0438\u0442\u0435 \u0438\u043c\u044f.", L"\u8acb\u8f38\u5165\u59d3\u540d\u3002", L"Introduce un nombre."},
        {L"Please fill the Other field.", L"\u8bf7\u586b\u5199\u5176\u4ed6\u5b57\u6bb5\u3002", L"Imla l-qasam Other.", L"Other \u6b04\u3092\u5165\u529b\u3057\u3066\u304f\u3060\u3055\u3044\u3002", L"Veuillez remplir le champ Autre.", L"Bitte das Feld Andere ausf\u00fcllen.", L"\u0417\u0430\u043f\u043e\u043b\u043d\u0438\u0442\u0435 \u043f\u043e\u043b\u0435 Other.", L"\u8acb\u586b\u5beb\u5176\u4ed6\u6b04\u4f4d\u3002", L"Rellena el campo Otro."},
        {L"Please select a record to edit.", L"\u8bf7\u9009\u62e9\u8981\u7f16\u8f91\u7684\u8bb0\u5f55\u3002", L"Ag\u0127\u017cel rekord biex teditja.", L"\u7de8\u96c6\u3059\u308b\u8a18\u9332\u3092\u9078\u629e\u3057\u3066\u304f\u3060\u3055\u3044\u3002", L"S\u00e9lectionnez un enregistrement.", L"Bitte einen Eintrag ausw\u00e4hlen.", L"\u0412\u044b\u0431\u0435\u0440\u0438\u0442\u0435 \u0437\u0430\u043f\u0438\u0441\u044c.", L"\u8acb\u9078\u64c7\u8981\u7de8\u8f2f\u7684\u8a18\u9304\u3002", L"Selecciona un registro."},
        {L"There are no records to mark.", L"\u6ca1\u6709\u53ef\u6807\u8bb0\u7684\u8bb0\u5f55\u3002", L"M'hemmx rekords.", L"\u30de\u30fc\u30af\u3059\u308b\u8a18\u9332\u304c\u3042\u308a\u307e\u305b\u3093\u3002", L"Aucun enregistrement.", L"Keine Eintr\u00e4ge vorhanden.", L"\u041d\u0435\u0442 \u0437\u0430\u043f\u0438\u0441\u0435\u0439.", L"\u6c92\u6709\u53ef\u6a19\u8a18\u7684\u8a18\u9304\u3002", L"No hay registros."},
        {L"Mark every record as Present?", L"\u5c06\u6240\u6709\u8bb0\u5f55\u6807\u8bb0\u4e3a\u51fa\u5e2d\uff1f", L"Immarka kollha Present?", L"\u3059\u3079\u3066 Present \u306b\u3057\u307e\u3059\u304b\uff1f", L"Tout marquer pr\u00e9sent ?", L"Alle als anwesend markieren?", L"\u0412\u0441\u0435\u0445 \u043e\u0442\u043c\u0435\u0442\u0438\u0442\u044c Present?", L"\u5c07\u6240\u6709\u8a18\u9304\u6a19\u8a18\u70ba\u51fa\u5e2d\uff1f", L"\u00bfMarcar todo como Present?"},
        {L"All Present", L"\u5168\u5458\u51fa\u5e2d", L"Kollha pre\u017centi", L"\u5168\u54e1\u51fa\u5e2d", L"Tout pr\u00e9sent", L"Alle anwesend", L"\u0412\u0441\u0435 Present", L"\u5168\u54e1\u51fa\u5e2d", L"Todos presentes"},
        {L"Please select one or more records to delete.", L"\u8bf7\u9009\u62e9\u8981\u5220\u9664\u7684\u4e00\u6761\u6216\u591a\u6761\u8bb0\u5f55\u3002", L"Ag\u0127\u017cel rekord/i biex t\u0127assar.", L"\u524a\u9664\u3059\u308b\u8a18\u9332\u3092\u9078\u629e\u3057\u3066\u304f\u3060\u3055\u3044\u3002", L"S\u00e9lectionnez des enregistrements.", L"Bitte Eintr\u00e4ge zum L\u00f6schen ausw\u00e4hlen.", L"\u0412\u044b\u0431\u0435\u0440\u0438\u0442\u0435 \u0437\u0430\u043f\u0438\u0441\u0438.", L"\u8acb\u9078\u64c7\u8981\u522a\u9664\u7684\u8a18\u9304\u3002", L"Selecciona registros."},
        {L"No matching records found.", L"\u672a\u627e\u5230\u5339\u914d\u8bb0\u5f55\u3002", L"Ma nstabux rekords.", L"\u8a72\u5f53\u8a18\u9332\u306f\u3042\u308a\u307e\u305b\u3093\u3002", L"Aucun enregistrement.", L"Keine passenden Eintr\u00e4ge.", L"\u041d\u0435\u0442 \u0441\u043e\u0432\u043f\u0430\u0434\u0435\u043d\u0438\u0439.", L"\u672a\u627e\u5230\u7b26\u5408\u8a18\u9304\u3002", L"No hay coincidencias."},
        {L"There are no records to clear.", L"\u6ca1\u6709\u53ef\u6e05\u7a7a\u7684\u8bb0\u5f55\u3002", L"M'hemmx rekords x't\u0127assar.", L"\u30af\u30ea\u30a2\u3059\u308b\u8a18\u9332\u304c\u3042\u308a\u307e\u305b\u3093\u3002", L"Aucun enregistrement \u00e0 effacer.", L"Keine Eintr\u00e4ge zum Leeren.", L"\u041d\u0435\u0442 \u0437\u0430\u043f\u0438\u0441\u0435\u0439.", L"\u6c92\u6709\u53ef\u6e05\u7a7a\u7684\u8a18\u9304\u3002", L"No hay registros."},
        {L"There are no records to export.", L"\u6ca1\u6709\u53ef\u5bfc\u51fa\u7684\u8bb0\u5f55\u3002", L"M'hemmx rekords x'tesporta.", L"\u51fa\u529b\u3059\u308b\u8a18\u9332\u304c\u3042\u308a\u307e\u305b\u3093\u3002", L"Aucun enregistrement \u00e0 exporter.", L"Keine Eintr\u00e4ge zum Exportieren.", L"\u041d\u0435\u0442 \u0437\u0430\u043f\u0438\u0441\u0435\u0439 \u0434\u043b\u044f \u044d\u043a\u0441\u043f\u043e\u0440\u0442\u0430.", L"\u6c92\u6709\u53ef\u532f\u51fa\u7684\u8a18\u9304\u3002", L"No hay registros para exportar."},
        {L"Could not export the CSV file.", L"\u65e0\u6cd5\u5bfc\u51fa CSV \u6587\u4ef6\u3002", L"Ma setax ji\u0121i esportat CSV.", L"CSV \u3092\u51fa\u529b\u3067\u304d\u307e\u305b\u3093\u3002", L"Impossible d'exporter le CSV.", L"CSV konnte nicht exportiert werden.", L"\u041d\u0435 \u0443\u0434\u0430\u043b\u043e\u0441\u044c CSV.", L"\u7121\u6cd5\u532f\u51fa CSV\u3002", L"No se pudo exportar CSV."},
        {L"CSV exported successfully.", L"CSV \u5bfc\u51fa\u6210\u529f\u3002", L"CSV esportat.", L"CSV \u3092\u51fa\u529b\u3057\u307e\u3057\u305f\u3002", L"CSV export\u00e9.", L"CSV exportiert.", L"CSV \u044d\u043a\u0441\u043f\u043e\u0440\u0442\u0438\u0440\u043e\u0432\u0430\u043d.", L"CSV \u532f\u51fa\u6210\u529f\u3002", L"CSV exportado."},
        {L"Could not save the file.", L"\u65e0\u6cd5\u4fdd\u5b58\u6587\u4ef6\u3002", L"Ma setax ji\u0121i ssejvjat.", L"\u30d5\u30a1\u30a4\u30eb\u3092\u4fdd\u5b58\u3067\u304d\u307e\u305b\u3093\u3002", L"Impossible d'enregistrer.", L"Datei konnte nicht gespeichert werden.", L"\u041d\u0435 \u0443\u0434\u0430\u043b\u043e\u0441\u044c \u0441\u043e\u0445\u0440\u0430\u043d\u0438\u0442\u044c.", L"\u7121\u6cd5\u5132\u5b58\u6a94\u6848\u3002", L"No se pudo guardar."},
        {L"Saved successfully.", L"\u4fdd\u5b58\u6210\u529f\u3002", L"Issejvjat.", L"\u4fdd\u5b58\u3057\u307e\u3057\u305f\u3002", L"Enregistr\u00e9.", L"Gespeichert.", L"\u0421\u043e\u0445\u0440\u0430\u043d\u0435\u043d\u043e.", L"\u5132\u5b58\u6210\u529f\u3002", L"Guardado."},
        {L"PowerPoint exported successfully.", L"PowerPoint \u5bfc\u51fa\u6210\u529f\u3002", L"PowerPoint esportat.", L"PowerPoint \u3092\u51fa\u529b\u3057\u307e\u3057\u305f\u3002", L"PowerPoint export\u00e9.", L"PowerPoint exportiert.", L"PowerPoint \u044d\u043a\u0441\u043f\u043e\u0440\u0442\u0438\u0440\u043e\u0432\u0430\u043d.", L"PowerPoint \u532f\u51fa\u6210\u529f\u3002", L"PowerPoint exportado."},
        {L"Could not export the PowerPoint file.", L"\u65e0\u6cd5\u5bfc\u51fa PowerPoint \u6587\u4ef6\u3002", L"Ma setax ji\u0121i esportat PowerPoint.", L"PowerPoint \u30d5\u30a1\u30a4\u30eb\u3092\u51fa\u529b\u3067\u304d\u307e\u305b\u3093\u3002", L"Impossible d'exporter PowerPoint.", L"PowerPoint konnte nicht exportiert werden.", L"\u041d\u0435 \u0443\u0434\u0430\u043b\u043e\u0441\u044c PowerPoint.", L"\u7121\u6cd5\u532f\u51fa PowerPoint \u6a94\u6848\u3002", L"No se pudo exportar PowerPoint."},
        {L"Could not open the file.", L"\u65e0\u6cd5\u6253\u5f00\u6587\u4ef6\u3002", L"Ma setax jinfeta\u0127.", L"\u30d5\u30a1\u30a4\u30eb\u3092\u958b\u3051\u307e\u305b\u3093\u3002", L"Impossible d'ouvrir.", L"Datei konnte nicht ge\u00f6ffnet werden.", L"\u041d\u0435 \u0443\u0434\u0430\u043b\u043e\u0441\u044c \u043e\u0442\u043a\u0440\u044b\u0442\u044c.", L"\u7121\u6cd5\u958b\u555f\u6a94\u6848\u3002", L"No se pudo abrir."},
        {L"This .attd file could not be decoded.", L"\u65e0\u6cd5\u89e3\u7801\u8be5 .attd \u6587\u4ef6\u3002", L"Dan il-fajl .attd ma setax jinqara.", L".attd \u3092\u30c7\u30b3\u30fc\u30c9\u3067\u304d\u307e\u305b\u3093\u3002", L"Impossible de d\u00e9coder ce .attd.", L".attd konnte nicht dekodiert werden.", L"\u041d\u0435 \u0443\u0434\u0430\u043b\u043e\u0441\u044c \u0434\u0435\u043a\u043e\u0434\u0438\u0440\u043e\u0432\u0430\u0442\u044c .attd.", L"\u7121\u6cd5\u89e3\u78bc\u6b64 .attd \u6a94\u3002", L"No se pudo decodificar .attd."},
        {L"Imported successfully.", L"\u5bfc\u5165\u6210\u529f\u3002", L"Importat.", L"\u8aad\u307f\u8fbc\u307f\u6210\u529f\u3002", L"Import\u00e9.", L"Importiert.", L"\u0418\u043c\u043f\u043e\u0440\u0442\u0438\u0440\u043e\u0432\u0430\u043d\u043e.", L"\u532f\u5165\u6210\u529f\u3002", L"Importado."},
        {L"Fullscreen", L"\u5168\u5c4f", L"Skrin s\u0127i\u0127", L"\u5168\u753b\u9762", L"Plein \u00e9cran", L"Vollbild", L"\u041f\u043e\u043b\u043d\u044b\u0439 \u044d\u043a\u0440\u0430\u043d", L"\u5168\u87a2\u5e55", L"Pantalla completa"},
        {L"Double-click row: Edit selected record", L"\u53cc\u51fb\u884c\uff1a\u7f16\u8f91\u9009\u4e2d\u8bb0\u5f55", L"Ikklikkja darbtejn: editja", L"\u884c\u3092\u30c0\u30d6\u30eb\u30af\u30ea\u30c3\u30af\uff1a\u7de8\u96c6", L"Double-clic : modifier", L"Doppelklick: bearbeiten", L"\u0414\u0432\u043e\u0439\u043d\u043e\u0439 \u0449\u0435\u043b\u0447\u043e\u043a: \u043f\u0440\u0430\u0432\u043a\u0430", L"\u96d9\u64ca\u884c\uff1a\u7de8\u8f2f\u9078\u53d6\u8a18\u9304", L"Doble clic: editar"},
        {L"Ctrl/Shift click: Multi-select rows", L"Ctrl/Shift \u70b9\u51fb\uff1a\u591a\u9009\u884c", L"Ctrl/Shift: ag\u0127\u017cel aktar", L"Ctrl/Shift\uff1a\u8907\u6570\u9078\u629e", L"Ctrl/Shift : multi-s\u00e9lection", L"Strg/Umschalt: Mehrfachauswahl", L"Ctrl/Shift: \u043c\u0443\u043b\u044c\u0442\u0438\u0432\u044b\u0431\u043e\u0440", L"Ctrl/Shift \u9ede\u64ca\uff1a\u591a\u9078\u884c", L"Ctrl/Shift: selecci\u00f3n m\u00faltiple"},
        {L"Date / Time", L"\u65e5\u671f / \u65f6\u95f4", L"Data / \u0126in", L"\u65e5\u6642", L"Date / heure", L"Datum / Uhrzeit", L"\u0414\u0430\u0442\u0430 / \u0432\u0440\u0435\u043c\u044f", L"\u65e5\u671f / \u6642\u9593", L"Fecha / hora"},
        {L"Status", L"\u72b6\u6001", L"Status", L"\u72b6\u614b", L"Statut", L"Status", L"\u0421\u0442\u0430\u0442\u0443\u0441", L"\u72c0\u614b", L"Estado"},
        {L"Interface Settings", L"\u754c\u9762\u8bbe\u7f6e", L"Settings tal-interface", L"\u30a4\u30f3\u30bf\u30fc\u30d5\u30a7\u30fc\u30b9\u8a2d\u5b9a", L"Param\u00e8tres de l'interface", L"Oberfl\u00e4cheneinstellungen", L"\u041d\u0430\u0441\u0442\u0440\u043e\u0439\u043a\u0438 \u0438\u043d\u0442\u0435\u0440\u0444\u0435\u0439\u0441\u0430", L"\u4ecb\u9762\u8a2d\u5b9a", L"Configuraci\u00f3n de interfaz"},
        {L"Language", L"\u8bed\u8a00", L"Lingwa", L"\u8a00\u8a9e", L"Langue", L"Sprache", L"\u042f\u0437\u044b\u043a", L"\u8a9e\u8a00", L"Idioma"},
        {L"Style", L"\u98ce\u683c", L"Stil", L"\u30b9\u30bf\u30a4\u30eb", L"Style", L"Stil", L"\u0421\u0442\u0438\u043b\u044c", L"\u98a8\u683c", L"Estilo"},
        {L"Interface Font", L"\u754c\u9762\u5b57\u4f53", L"Font tal-interface", L"\u30a4\u30f3\u30bf\u30fc\u30d5\u30a7\u30fc\u30b9\u30d5\u30a9\u30f3\u30c8", L"Police de l'interface", L"Schriftart", L"\u0428\u0440\u0438\u0444\u0442", L"\u4ecb\u9762\u5b57\u578b", L"Fuente de interfaz"},
        {L"Apply", L"\u5e94\u7528", L"Applika", L"\u9069\u7528", L"Appliquer", L"Anwenden", L"\u041f\u0440\u0438\u043c\u0435\u043d\u0438\u0442\u044c", L"\u5957\u7528", L"Aplicar"},
        {L"Close", L"\u5173\u95ed", L"Ag\u0127laq", L"\u9589\u3058\u308b", L"Fermer", L"Schlie\u00dfen", L"\u0417\u0430\u043a\u0440\u044b\u0442\u044c", L"\u95dc\u9589", L"Cerrar"},
        {L"Reset All Settings", L"\u91cd\u7f6e\u6240\u6709\u8bbe\u7f6e", L"Irrisettja kollox", L"\u3059\u3079\u3066\u30ea\u30bb\u30c3\u30c8", L"Tout r\u00e9initialiser", L"Alles zur\u00fccksetzen", L"\u0421\u0431\u0440\u043e\u0441\u0438\u0442\u044c \u0432\u0441\u0451", L"\u91cd\u8a2d\u6240\u6709\u8a2d\u5b9a", L"Restablecer todo"},
    };
    for (const auto& entry : entries) {
        if (key == entry.key) {
            switch (g_language) {
            case UiLanguage::ChineseSimplified: return zh(entry.zh);
            case UiLanguage::Maltese: return mt(entry.mt);
            case UiLanguage::Japanese: return ja(entry.ja);
            case UiLanguage::French: return fr(entry.fr);
            case UiLanguage::German: return de(entry.de);
            case UiLanguage::Russian: return ru(entry.ru);
            case UiLanguage::ChineseTraditional: return zht(entry.zht);
            case UiLanguage::Spanish: return es(entry.es);
            default: return key;
            }
        }
    }
    return key;
}

HFONT CreateUiFont(int height, int weight) {
    return CreateFontW(height, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, g_fontFamily.c_str());
}

int CALLBACK EnumFontFamilyProc(const LOGFONTW* logFont, const TEXTMETRICW*, DWORD, LPARAM lParam) {
    auto* fonts = reinterpret_cast<std::set<std::wstring>*>(lParam);
    if (logFont && logFont->lfFaceName[0]) {
        fonts->insert(logFont->lfFaceName);
    }
    return 1;
}

void FillInstalledFontFamilies() {
    std::set<std::wstring> fonts;
    HDC hdc = GetDC(nullptr);
    if (hdc) {
        LOGFONTW lf{};
        lf.lfCharSet = DEFAULT_CHARSET;
        EnumFontFamiliesExW(hdc, &lf, EnumFontFamilyProc, reinterpret_cast<LPARAM>(&fonts), 0);
        ReleaseDC(nullptr, hdc);
    }
    if (fonts.empty()) {
        fonts.insert(L"Segoe UI");
        fonts.insert(L"Microsoft YaHei UI");
    }
    fonts.insert(g_fontFamily);
    g_availableFonts.assign(fonts.begin(), fonts.end());
}

std::wstring GetComboSelectedText(HWND combo) {
    int index = (int)SendMessageW(combo, CB_GETCURSEL, 0, 0);
    if (index < 0) return {};
    int len = (int)SendMessageW(combo, CB_GETLBTEXTLEN, index, 0);
    if (len < 0) return {};
    std::wstring text(len + 1, L'\0');
    SendMessageW(combo, CB_GETLBTEXT, index, (LPARAM)text.data());
    text.resize(len);
    return text;
}

void CommitComboSelectionNow(HWND combo) {
    if (!combo) return;
    RedrawWindow(combo, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    COMBOBOXINFO info{};
    info.cbSize = sizeof(info);
    if (GetComboBoxInfo(combo, &info) && info.hwndList) {
        RedrawWindow(info.hwndList, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    }
}

std::filesystem::path SettingsFilePath() {
    PWSTR roamingPath = nullptr;
    std::filesystem::path result;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &roamingPath))) {
        result = std::filesystem::path(roamingPath) / L"AttendanceApp" / L"settings.ini";
        CoTaskMemFree(roamingPath);
    }
    return result;
}

std::string LanguageToString(UiLanguage language) {
    switch (language) {
    case UiLanguage::ChineseSimplified: return "zh-CN";
    case UiLanguage::Maltese: return "mt";
    case UiLanguage::Japanese: return "ja";
    case UiLanguage::French: return "fr";
    case UiLanguage::German: return "de";
    case UiLanguage::Russian: return "ru";
    case UiLanguage::ChineseTraditional: return "zh-TW";
    case UiLanguage::Spanish: return "es";
    case UiLanguage::Italian: return "it";
    case UiLanguage::Mongolian: return "mn";
    case UiLanguage::Esperanto: return "eo";
    case UiLanguage::ClassicalChinese: return "lzh";
    case UiLanguage::Thai: return "th";
    case UiLanguage::Filipino: return "fil";
    case UiLanguage::Turkish: return "tr";
    case UiLanguage::Lithuanian: return "lt";
    case UiLanguage::Norwegian: return "no";
    case UiLanguage::Vietnamese: return "vi";
    case UiLanguage::ChineseTraditionalHongKong: return "zh-HK";
    default: return "en";
    }
}

UiLanguage LanguageFromString(const std::string& value) {
    if (value == "zh-CN") return UiLanguage::ChineseSimplified;
    if (value == "mt") return UiLanguage::Maltese;
    if (value == "ja") return UiLanguage::Japanese;
    if (value == "fr") return UiLanguage::French;
    if (value == "de") return UiLanguage::German;
    if (value == "ru") return UiLanguage::Russian;
    if (value == "zh-TW") return UiLanguage::ChineseTraditional;
    if (value == "es") return UiLanguage::Spanish;
    if (value == "it") return UiLanguage::Italian;
    if (value == "mn") return UiLanguage::Mongolian;
    if (value == "eo") return UiLanguage::Esperanto;
    if (value == "lzh") return UiLanguage::ClassicalChinese;
    if (value == "th") return UiLanguage::Thai;
    if (value == "fil") return UiLanguage::Filipino;
    if (value == "tr") return UiLanguage::Turkish;
    if (value == "lt") return UiLanguage::Lithuanian;
    if (value == "no") return UiLanguage::Norwegian;
    if (value == "vi") return UiLanguage::Vietnamese;
    if (value == "zh-HK") return UiLanguage::ChineseTraditionalHongKong;
    return UiLanguage::English;
}

void SaveSettings() {
    auto path = SettingsFilePath();
    if (path.empty()) return;
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    if (!file) return;
    file << "language=" << LanguageToString(g_language) << "\n";
    file << "font=" << WideToUtf8(g_fontFamily) << "\n";
    file << "default_save_dir=" << WideToUtf8(g_defaultSaveDir) << "\n";
    file << "particles=" << (g_particlesEnabled ? "1" : "0") << "\n";
}

void LoadSettings() {
    auto path = SettingsFilePath();
    if (path.empty() || !std::filesystem::exists(path)) return;

    std::ifstream file(path, std::ios::binary);
    if (!file) return;

    std::string line;
    while (std::getline(file, line)) {
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        if (key == "language") g_language = LanguageFromString(value);
        else if (key == "theme") g_theme = UiTheme::Dark;
        else if (key == "font" && !value.empty()) g_fontFamily = Utf8ToWide(value);
        else if (key == "default_save_dir") g_defaultSaveDir = Utf8ToWide(value);
        else if (key == "particles") g_particlesEnabled = value != "0";
    }
    g_theme = UiTheme::Dark;
}

void ResetSettings() {
    auto path = SettingsFilePath();
    if (!path.empty()) {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
    g_language = UiLanguage::English;
    g_theme = UiTheme::Dark;
    g_fontFamily = L"Segoe UI";
    g_defaultSaveDir.clear();
    g_particlesEnabled = true;
}

std::string WideToUtf8(const std::wstring& input) {
    if (input.empty()) return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, input.c_str(), (int)input.size(), nullptr, 0, nullptr, nullptr);
    std::string output(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, input.c_str(), (int)input.size(), output.data(), size, nullptr, nullptr);
    return output;
}

std::wstring Utf8ToWide(const std::string& input) {
    if (input.empty()) return {};
    int size = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), (int)input.size(), nullptr, 0);
    std::wstring output(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, input.c_str(), (int)input.size(), output.data(), size);
    return output;
}

std::string Escape(const std::string& input) {
    std::string out;
    for (char ch : input) {
        if (ch == '\\') out += "\\\\";
        else if (ch == '\t') out += "\\t";
        else if (ch == '\n') out += "\\n";
        else if (ch == '\r') out += "\\r";
        else out += ch;
    }
    return out;
}

std::string Unescape(const std::string& input) {
    std::string out;
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\\' && i + 1 < input.size()) {
            char next = input[++i];
            if (next == 't') out += '\t';
            else if (next == 'n') out += '\n';
            else if (next == 'r') out += '\r';
            else out += next;
        } else {
            out += input[i];
        }
    }
    return out;
}

std::vector<std::string> SplitTabs(const std::string& line) {
    std::vector<std::string> parts;
    std::string current;
    bool escaped = false;
    for (char ch : line) {
        if (escaped) {
            current += '\\';
            current += ch;
            escaped = false;
        } else if (ch == '\\') {
            escaped = true;
        } else if (ch == '\t') {
            parts.push_back(current);
            current.clear();
        } else {
            current += ch;
        }
    }
    if (escaped) current += '\\';
    parts.push_back(current);
    return parts;
}

AttendanceSheet MakeSheet(const std::wstring& name, std::vector<AttendanceRecord> records = {}) {
    AttendanceSheet sheet;
    sheet.name = name.empty() ? L"Course" : name;
    sheet.records = std::move(records);
    return sheet;
}

std::string SerializeRecords() {
    std::ostringstream ss;
    ss << "ATTENDANCE_V1\n";
    ss << g_records.size() << "\n";
    for (const auto& record : g_records) {
        ss << Escape(WideToUtf8(record.dateTime)) << '\t'
           << Escape(WideToUtf8(record.name)) << '\t'
           << Escape(WideToUtf8(record.status)) << '\t'
           << Escape(WideToUtf8(record.other)) << '\n';
    }
    return ss.str();
}

bool DeserializeRecords(const std::string& plainText, std::vector<AttendanceRecord>& output) {
    std::istringstream ss(plainText);
    std::string header;
    std::getline(ss, header);
    if (header != "ATTENDANCE_V1") return false;

    std::string countLine;
    std::getline(ss, countLine);
    size_t count = 0;
    try {
        count = std::stoull(countLine);
    } catch (...) {
        return false;
    }

    std::vector<AttendanceRecord> records;
    for (size_t i = 0; i < count; ++i) {
        std::string line;
        if (!std::getline(ss, line)) return false;
        auto parts = SplitTabs(line);
        if (parts.size() != 4) return false;
        records.push_back({
            Utf8ToWide(Unescape(parts[0])),
            Utf8ToWide(Unescape(parts[1])),
            Utf8ToWide(Unescape(parts[2])),
            Utf8ToWide(Unescape(parts[3]))
        });
    }

    output = std::move(records);
    return true;
}

void EnsureSheets() {
    if (g_sheets.empty()) {
        g_sheets.push_back(MakeSheet(L"Default Course", g_records));
        g_activeSheet = 0;
    }
}

void SyncActiveSheet() {
    EnsureSheets();
    if (g_activeSheet < 0) g_activeSheet = 0;
    if (g_activeSheet >= (int)g_sheets.size()) g_activeSheet = (int)g_sheets.size() - 1;
    if (g_activeSheet >= 0 && g_activeSheet < (int)g_sheets.size()) {
        g_sheets[g_activeSheet].records = g_records;
    }
}

std::string SerializeWorkbook() {
    SyncActiveSheet();
    std::ostringstream ss;
    ss << "ATTENDANCE_V4\n";
    ss << g_activeSheet << "\n";
    ss << g_sheets.size() << "\n";
    for (const auto& sheet : g_sheets) {
        ss << Escape(WideToUtf8(sheet.name)) << "\n";
        ss << Escape(WideToUtf8(sheet.teacher)) << "\n";
        ss << Escape(WideToUtf8(sheet.location)) << "\n";
        ss << Escape(WideToUtf8(sheet.notes)) << "\n";
        ss << sheet.students.size() << "\n";
        for (const auto& student : sheet.students) {
            ss << Escape(WideToUtf8(student)) << "\n";
        }
        ss << sheet.records.size() << "\n";
        for (const auto& record : sheet.records) {
            ss << Escape(WideToUtf8(record.dateTime)) << '\t'
               << Escape(WideToUtf8(record.name)) << '\t'
               << Escape(WideToUtf8(record.status)) << '\t'
               << Escape(WideToUtf8(record.other)) << '\n';
        }
    }
    return ss.str();
}

bool DeserializeWorkbook(const std::string& plainText, std::vector<AttendanceSheet>& output) {
    std::istringstream ss(plainText);
    std::string header;
    std::getline(ss, header);
    g_loadedActiveSheet = 0;
    if (header == "ATTENDANCE_V1") {
        std::vector<AttendanceRecord> records;
        if (!DeserializeRecords(plainText, records)) return false;
        output = {MakeSheet(L"Default Course", std::move(records))};
        return true;
    }
    if (header != "ATTENDANCE_V2" && header != "ATTENDANCE_V3" && header != "ATTENDANCE_V4") return false;

    std::string countLine;
    if (header == "ATTENDANCE_V3" || header == "ATTENDANCE_V4") {
        std::string activeLine;
        std::getline(ss, activeLine);
        try {
            g_loadedActiveSheet = std::stoi(activeLine);
        } catch (...) {
            g_loadedActiveSheet = 0;
        }
    }
    std::getline(ss, countLine);
    size_t sheetCount = 0;
    try {
        sheetCount = std::stoull(countLine);
    } catch (...) {
        return false;
    }

    std::vector<AttendanceSheet> sheets;
    for (size_t s = 0; s < sheetCount; ++s) {
        std::string nameLine;
        std::string recordCountLine;
        if (!std::getline(ss, nameLine)) return false;

        AttendanceSheet sheet;
        sheet.name = Utf8ToWide(Unescape(nameLine));
        if (sheet.name.empty()) sheet.name = L"Course";
        if (header == "ATTENDANCE_V4") {
            std::string teacherLine;
            std::string locationLine;
            std::string notesLine;
            std::string studentCountLine;
            if (!std::getline(ss, teacherLine)) return false;
            if (!std::getline(ss, locationLine)) return false;
            if (!std::getline(ss, notesLine)) return false;
            if (!std::getline(ss, studentCountLine)) return false;
            sheet.teacher = Utf8ToWide(Unescape(teacherLine));
            sheet.location = Utf8ToWide(Unescape(locationLine));
            sheet.notes = Utf8ToWide(Unescape(notesLine));
            size_t studentCount = 0;
            try {
                studentCount = std::stoull(studentCountLine);
            } catch (...) {
                return false;
            }
            for (size_t i = 0; i < studentCount; ++i) {
                std::string studentLine;
                if (!std::getline(ss, studentLine)) return false;
                std::wstring student = Utf8ToWide(Unescape(studentLine));
                if (!student.empty()) sheet.students.push_back(student);
            }
        }
        if (!std::getline(ss, recordCountLine)) return false;
        size_t recordCount = 0;
        try {
            recordCount = std::stoull(recordCountLine);
        } catch (...) {
            return false;
        }
        for (size_t i = 0; i < recordCount; ++i) {
            std::string line;
            if (!std::getline(ss, line)) return false;
            auto parts = SplitTabs(line);
            if (parts.size() != 4) return false;
            sheet.records.push_back({
                Utf8ToWide(Unescape(parts[0])),
                Utf8ToWide(Unescape(parts[1])),
                Utf8ToWide(Unescape(parts[2])),
                Utf8ToWide(Unescape(parts[3]))
            });
        }
        sheets.push_back(std::move(sheet));
    }

    if (sheets.empty()) sheets.push_back(MakeSheet(L"Default Course"));
    output = std::move(sheets);
    return true;
}

uint32_t Rotl(uint32_t value, int bits) {
    return (value << bits) | (value >> (32 - bits));
}

std::string XorCrypt(const std::string& data) {
    uint32_t state = 0xA77D2026u;
    const std::string key = "AttendanceProject.attd.CustomKey";
    std::string out = data;
    for (size_t i = 0; i < out.size(); ++i) {
        state ^= (uint8_t)key[i % key.size()] + (uint32_t)i * 2654435761u;
        state = Rotl(state, 7) * 1664525u + 1013904223u;
        out[i] = (char)((uint8_t)out[i] ^ (uint8_t)(state >> 16));
    }
    return out;
}

std::string Base64Encode(const std::string& input) {
    static const char* table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string output;
    int val = 0;
    int valb = -6;
    for (uint8_t ch : input) {
        val = (val << 8) + ch;
        valb += 8;
        while (valb >= 0) {
            output.push_back(table[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) output.push_back(table[((val << 8) >> (valb + 8)) & 0x3F]);
    while (output.size() % 4) output.push_back('=');
    return output;
}

bool Base64Decode(const std::string& input, std::string& output) {
    static const std::string table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<int> map(256, -1);
    for (int i = 0; i < 64; ++i) map[(uint8_t)table[i]] = i;

    output.clear();
    int val = 0;
    int valb = -8;
    for (uint8_t ch : input) {
        if (ch == '=') break;
        if (map[ch] == -1) {
            if (ch == '\r' || ch == '\n' || ch == ' ') continue;
            return false;
        }
        val = (val << 6) + map[ch];
        valb += 6;
        if (valb >= 0) {
            output.push_back((char)((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return true;
}

std::string EncodeAttd(const std::string& plainText) {
    return "ATTD1:" + Base64Encode(XorCrypt(plainText));
}

bool DecodeAttd(const std::string& fileText, std::string& plainText) {
    const std::string prefix = "ATTD1:";
    if (fileText.rfind(prefix, 0) != 0) return false;
    std::string encrypted;
    if (!Base64Decode(fileText.substr(prefix.size()), encrypted)) return false;
    plainText = XorCrypt(encrypted);
    return true;
}

std::wstring CurrentDateTimeText() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t buffer[64]{};
    swprintf_s(buffer, L"%04d-%02d-%02d %02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
    return buffer;
}

void ShowMessage(const std::wstring& text, const std::wstring& title = L"AttendanceApp") {
    ThemedMessageBox(g_hwnd, text, title, false);
}

void UpdateStats() {
    int present = 0;
    int absent = 0;
    int late = 0;
    int other = 0;
    for (const auto& record : g_records) {
        if (record.status == L"Present") ++present;
        else if (record.status == L"Absent") ++absent;
        else if (record.status == L"Late") ++late;
        else ++other;
    }

    double attendanceRate = g_records.empty() ? 0.0 : (present * 100.0 / g_records.size());
    double issueRate = g_records.empty() ? 0.0 : ((absent + late) * 100.0 / g_records.size());
    std::wstringstream ss;
    ss << Tr(L"Total", L"\u603b\u6570") << L" " << g_records.size()
       << L"  |  " << Tr(L"Attendance", L"\u51fa\u52e4\u7387") << L" " << std::fixed << std::setprecision(1) << attendanceRate << L"%"
       << L"  |  " << Tr(L"Absent/Late", L"\u7f3a\u5e2d/\u8fdf\u5230") << L" " << std::fixed << std::setprecision(1) << issueRate << L"%"
       << L"  |  " << Tr(L"Present", L"\u51fa\u5e2d") << L" " << present
       << L"  " << Tr(L"Absent", L"\u7f3a\u5e2d") << L" " << absent
       << L"  " << Tr(L"Late", L"\u8fdf\u5230") << L" " << late
       << L"  " << Tr(L"Other", L"\u5176\u4ed6") << L" " << other;
    if (!g_filterText.empty()) {
        ss << L"  |  " << Tr(L"Showing", L"\u663e\u793a") << L" " << g_visibleRows.size() << L"/" << g_records.size();
    }
    SetStaticTextClean(GetDlgItem(g_hwnd, IDC_STATS), ss.str());

    std::wstringstream totalCard;
    totalCard << Tr(L"Total", L"\u603b\u6570") << L"\n" << g_records.size();
    SetStaticTextClean(GetDlgItem(g_hwnd, IDC_STAT_TOTAL), totalCard.str());

    std::wstringstream attendanceCard;
    attendanceCard << Tr(L"Attendance", L"\u51fa\u52e4\u7387") << L"\n" << std::fixed << std::setprecision(1) << attendanceRate << L"%";
    SetStaticTextClean(GetDlgItem(g_hwnd, IDC_STAT_ATTENDANCE), attendanceCard.str());

    std::wstringstream issuesCard;
    issuesCard << Tr(L"Absent/Late", L"\u7f3a\u5e2d/\u8fdf\u5230") << L"\n" << std::fixed << std::setprecision(1) << issueRate << L"%";
    SetStaticTextClean(GetDlgItem(g_hwnd, IDC_STAT_ISSUES), issuesCard.str());

    std::wstringstream visibleCard;
    visibleCard << Tr(L"Showing", L"\u663e\u793a") << L"\n" << g_visibleRows.size() << L" / " << g_records.size();
    SetStaticTextClean(GetDlgItem(g_hwnd, IDC_STAT_VISIBLE), visibleCard.str());
}

void RefreshList() {
    SyncActiveSheet();
    ListView_DeleteAllItems(g_list);
    g_visibleRows.clear();
    for (int i = 0; i < (int)g_records.size(); ++i) {
        if (!RecordMatchesFilter(g_records[i])) continue;
        int visibleIndex = (int)g_visibleRows.size();
        g_visibleRows.push_back(i);
        LVITEMW item{};
        item.mask = LVIF_TEXT;
        item.iItem = visibleIndex;
        item.pszText = const_cast<wchar_t*>(g_records[i].dateTime.c_str());
        ListView_InsertItem(g_list, &item);
        ListView_SetItemText(g_list, visibleIndex, 1, const_cast<wchar_t*>(g_records[i].name.c_str()));
        ListView_SetItemText(g_list, visibleIndex, 2, const_cast<wchar_t*>(g_records[i].status.c_str()));
        ListView_SetItemText(g_list, visibleIndex, 3, const_cast<wchar_t*>(g_records[i].other.c_str()));
    }
    UpdateStats();
    StartListTransition();
}

void RefreshCourseCombo() {
    if (!g_courseCombo) return;
    EnsureSheets();
    SendMessageW(g_courseCombo, CB_RESETCONTENT, 0, 0);
    for (const auto& sheet : g_sheets) {
        SendMessageW(g_courseCombo, CB_ADDSTRING, 0, (LPARAM)sheet.name.c_str());
    }
    SendMessageW(g_courseCombo, CB_SETCURSEL, g_activeSheet, 0);
    RedrawWindow(g_courseCombo, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
}

void SwitchCourse(int index) {
    EnsureSheets();
    if (index < 0 || index >= (int)g_sheets.size() || index == g_activeSheet) return;
    SyncActiveSheet();
    g_activeSheet = index;
    g_records = g_sheets[g_activeSheet].records;
    RefreshCourseCombo();
    RefreshList();
}

void AddCourse() {
    SyncActiveSheet();
    std::wstring name = L"Course " + std::to_wstring(g_sheets.size() + 1);
    if (!PromptText(Tr(L"Add Course/Class", L"\u65b0\u589e\u8bfe\u7a0b/\u73ed\u7ea7"), Tr(L"Course or class name:", L"\u8bfe\u7a0b\u6216\u73ed\u7ea7\u540d\u79f0\uff1a"), name)) return;
    if (name.empty()) return;
    PushUndo();
    MarkDirty();
    g_sheets.push_back(MakeSheet(name));
    g_activeSheet = (int)g_sheets.size() - 1;
    g_records.clear();
    RefreshCourseCombo();
    RefreshList();
}

void RenameCurrentCourse() {
    EnsureSheets();
    std::wstring name = g_sheets[g_activeSheet].name;
    if (!PromptText(Tr(L"Rename Course/Class", L"\u91cd\u547d\u540d\u8bfe\u7a0b/\u73ed\u7ea7"), Tr(L"New course or class name:", L"\u65b0\u8bfe\u7a0b\u6216\u73ed\u7ea7\u540d\u79f0\uff1a"), name)) return;
    if (name.empty()) return;
    PushUndo();
    MarkDirty();
    g_sheets[g_activeSheet].name = name;
    RefreshCourseCombo();
}

void DeleteCurrentCourse() {
    EnsureSheets();
    if (g_sheets.size() <= 1) {
        ShowMessage(Tr(L"At least one course must remain.", L"\u81f3\u5c11\u9700\u8981\u4fdd\u7559\u4e00\u4e2a\u8bfe\u7a0b\u3002"));
        return;
    }
    std::wstring deleteCourseMsg = Tr(L"Delete the current course and its records?", L"\u5220\u9664\u5f53\u524d\u8bfe\u7a0b\u53ca\u5176\u8bb0\u5f55\uff1f");
    std::wstring deleteCourseTitle = Tr(L"Delete Course", L"\u5220\u9664\u8bfe\u7a0b");
    if (ThemedMessageBox(g_hwnd, deleteCourseMsg, deleteCourseTitle, true) != IDYES) {
        return;
    }
    PushUndo();
    MarkDirty();
    g_sheets.erase(g_sheets.begin() + g_activeSheet);
    g_activeSheet = std::max(0, g_activeSheet - 1);
    g_records = g_sheets[g_activeSheet].records;
    RefreshCourseCombo();
    RefreshList();
}

int ThemedMenuItemTop(const ThemedMenuState* state, int index) {
    int y = THEMED_MENU_PAD_Y;
    for (int i = 0; i < index; ++i) {
        y += state->items[i].separator ? THEMED_MENU_SEPARATOR_H : THEMED_MENU_ITEM_H;
    }
    return y;
}

int ThemedMenuTotalHeight(const ThemedMenuState* state) {
    int height = THEMED_MENU_PAD_Y * 2;
    for (const auto& item : state->items) {
        height += item.separator ? THEMED_MENU_SEPARATOR_H : THEMED_MENU_ITEM_H;
    }
    return height;
}

int ThemedMenuHitTest(ThemedMenuState* state, int y) {
    for (int i = 0; i < (int)state->items.size(); ++i) {
        int top = ThemedMenuItemTop(state, i);
        int height = state->items[i].separator ? THEMED_MENU_SEPARATOR_H : THEMED_MENU_ITEM_H;
        if (y >= top && y < top + height) {
            return state->items[i].separator ? -1 : i;
        }
    }
    return -1;
}

void DrawSoftDivider(HDC hdc, int left, int right, int y) {
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

RECT ThemedMenuItemRect(HWND hwnd, const ThemedMenuState* state, int index) {
    RECT rc{};
    GetClientRect(hwnd, &rc);
    if (!state || index < 0 || index >= (int)state->items.size() || state->items[index].separator) {
        return rc;
    }
    int top = ThemedMenuItemTop(state, index);
    return RECT{6, top, rc.right - 6, top + THEMED_MENU_ITEM_H};
}

void ApplyThemedMenuReveal(HWND hwnd, ThemedMenuState* state) {
    if (!hwnd || !state) return;
    double reveal = GetAnimationValue(hwnd, AnimChannel::Reveal, 1.0);
    int y = state->finalY - (int)std::lround((1.0 - reveal) * THEMED_MENU_SLIDE_PX);
    BYTE alpha = (BYTE)std::clamp((int)std::lround(255.0 * reveal), 1, 255);
    SetLayeredWindowAttributes(hwnd, 0, alpha, LWA_ALPHA);
    SetWindowPos(hwnd, HWND_TOPMOST, state->finalX, y, state->width, state->height,
        SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}

void PaintThemedPopupMenuContent(HWND hwnd, HDC hdc, ThemedMenuState* state) {
    RECT rc{};
    GetClientRect(hwnd, &rc);
    double reveal = GetAnimationValue(hwnd, AnimChannel::Reveal, 1.0);
    COLORREF fill = BlendColor(RGB(0, 0, 0), RGB(14, 14, 14), reveal);
    COLORREF border = BlendColor(RGB(28, 28, 28), RGB(68, 68, 68), reveal);
    COLORREF hover = RGB(36, 36, 36);
    int blurRadius = (int)std::lround(reveal * 10.0);

    HBRUSH bg = CreateSolidBrush(fill);
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);

    for (int i = 0; i < blurRadius; i += 3) {
        RECT glow{rc.left + i, rc.top + i, rc.right - i, rc.bottom - i};
        HPEN glowPen = CreatePen(PS_SOLID, 1, BlendColor(RGB(10, 10, 10), RGB(42, 42, 42), reveal * (1.0 - i / 12.0)));
        HGDIOBJ oldGlowPen = SelectObject(hdc, glowPen);
        HGDIOBJ oldGlowBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
        Rectangle(hdc, glow.left, glow.top, glow.right, glow.bottom);
        SelectObject(hdc, oldGlowBrush);
        SelectObject(hdc, oldGlowPen);
        DeleteObject(glowPen);
    }

    HGDIOBJ oldFont = SelectObject(hdc, g_font);
    SetBkMode(hdc, TRANSPARENT);

    for (int i = 0; i < (int)state->items.size(); ++i) {
        double itemProgress = EaseOut(reveal);
        int top = ThemedMenuItemTop(state, i);
        if (state->items[i].separator) {
            DrawSoftDivider(hdc, 12, rc.right - 12, top + 6);
            continue;
        }

        RECT itemRc{0, top, rc.right, top + THEMED_MENU_ITEM_H};
        if (i == state->hover) {
            double hot = GetAnimationValue(hwnd, AnimChannel::Hover, 1.0);
            HBRUSH hoverBrush = CreateSolidBrush(BlendColor(fill, hover, hot));
            FillRect(hdc, &itemRc, hoverBrush);
            DeleteObject(hoverBrush);
        }

        RECT textRc = itemRc;
        textRc.left += 16;
        textRc.right -= 16;
        SetTextColor(hdc, BlendColor(COLOR_MUTED, COLOR_TEXT, itemProgress));
        DrawTextW(hdc, state->items[i].text.c_str(), -1, &textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }

    SelectObject(hdc, oldFont);

    HPEN borderPen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ oldPen = SelectObject(hdc, borderPen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(borderPen);
}

void PaintThemedPopupMenu(HWND hwnd, HDC hdc, ThemedMenuState* state) {
    RECT rc{};
    GetClientRect(hwnd, &rc);
    int width = std::max(1, (int)(rc.right - rc.left));
    int height = std::max(1, (int)(rc.bottom - rc.top));

    HDC memDc = CreateCompatibleDC(hdc);
    HBITMAP bitmap = CreateCompatibleBitmap(hdc, width, height);
    HGDIOBJ oldBitmap = SelectObject(memDc, bitmap);
    PaintThemedPopupMenuContent(hwnd, memDc, state);
    BitBlt(hdc, 0, 0, width, height, memDc, 0, 0, SRCCOPY);
    SelectObject(memDc, oldBitmap);
    DeleteObject(bitmap);
    DeleteDC(memDc);
}

void CloseThemedPopupMenu(HWND hwnd, ThemedMenuState* state, int selected) {
    if (!state || state->closing) return;
    state->selected = selected;
    state->closing = true;
    ReleaseCapture();
    BeginAnimatedClose(hwnd, 160);
}

LRESULT CALLBACK ThemedMenuProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* state = reinterpret_cast<ThemedMenuState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_NCCREATE: {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        return TRUE;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        if (state) PaintThemedPopupMenu(hwnd, hdc, state);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_MOUSEMOVE:
        if (state && !state->closing) {
            int next = ThemedMenuHitTest(state, GET_Y_LPARAM(lParam));
            if (next != state->hover) {
                RECT oldRc = ThemedMenuItemRect(hwnd, state, state->hover);
                state->hover = next;
                StartAnimation(hwnd, AnimChannel::Hover, next >= 0 ? 1.0 : 0.0, 110);
                RECT newRc = ThemedMenuItemRect(hwnd, state, state->hover);
                InvalidateRect(hwnd, &oldRc, FALSE);
                InvalidateRect(hwnd, &newRc, FALSE);
            }
        }
        return 0;
    case WM_LBUTTONDOWN:
        if (state && !state->closing) {
            POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            RECT rc{};
            GetClientRect(hwnd, &rc);
            if (!PtInRect(&rc, pt)) {
                CloseThemedPopupMenu(hwnd, state, 0);
                return 0;
            }
        }
        return 0;
    case WM_LBUTTONUP:
        if (state && !state->closing) {
            int hit = ThemedMenuHitTest(state, GET_Y_LPARAM(lParam));
            CloseThemedPopupMenu(hwnd, state, hit >= 0 ? state->items[hit].command : 0);
        }
        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE && state && !state->closing) {
            CloseThemedPopupMenu(hwnd, state, 0);
            return 0;
        }
        break;
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE && state && !state->done && !state->closing) {
            CloseThemedPopupMenu(hwnd, state, 0);
            return 0;
        }
        break;
    case WM_CAPTURECHANGED:
        if (state && !state->done && !state->closing && (HWND)lParam != hwnd) {
            CloseThemedPopupMenu(hwnd, state, 0);
            return 0;
        }
        break;
    case WM_APP_ANIMATION_TICK:
        if (state && !state->closing) ApplyThemedMenuReveal(hwnd, state);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_APP_ANIMATION_DONE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        ReleaseCapture();
        if (state) state->done = true;
        RemoveAnimationState(hwnd);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void RegisterThemedMenuClass() {
    static bool registered = false;
    if (registered) return;

    WNDCLASSW wc{};
    wc.style = CS_DROPSHADOW;
    wc.lpfnWndProc = ThemedMenuProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"AttendanceThemedPopupMenu";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    RegisterClassW(&wc);
    registered = true;
}

int MeasureThemedMenuWidth(const std::vector<ThemedMenuItem>& items) {
    HDC hdc = GetDC(g_hwnd);
    HGDIOBJ oldFont = SelectObject(hdc, g_font);
    int width = 180;
    for (const auto& item : items) {
        if (item.separator) continue;
        SIZE size{};
        GetTextExtentPoint32W(hdc, item.text.c_str(), (int)item.text.size(), &size);
        width = std::max(width, (int)size.cx + 42);
    }
    SelectObject(hdc, oldFont);
    ReleaseDC(g_hwnd, hdc);
    return std::min(width, 360);
}

int ShowThemedPopupMenu(HWND button, const std::vector<ThemedMenuItem>& items) {
    if (!button || items.empty()) return 0;
    RegisterThemedMenuClass();

    ThemedMenuState state;
    state.items = items;

    RECT buttonRc{};
    GetWindowRect(button, &buttonRc);
    int width = MeasureThemedMenuWidth(items);
    int height = ThemedMenuTotalHeight(&state);
    int x = buttonRc.right - width;
    int y = buttonRc.bottom + 4;

    HMONITOR monitor = MonitorFromRect(&buttonRc, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfoW(monitor, &mi)) {
        int minX = (int)mi.rcWork.left + 4;
        int maxX = std::max(minX, (int)mi.rcWork.right - width - 4);
        int minY = (int)mi.rcWork.top + 4;
        int maxY = std::max(minY, (int)mi.rcWork.bottom - height - 4);
        x = std::clamp(x, minX, maxX);
        if (y + height > (int)mi.rcWork.bottom) y = (int)buttonRc.top - height - 4;
        y = std::clamp(y, minY, maxY);
    }
    state.finalX = x;
    state.finalY = y;
    state.width = width;
    state.height = height;

    HWND popup = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_LAYERED,
        L"AttendanceThemedPopupMenu", L"",
        WS_POPUP,
        x, y - THEMED_MENU_SLIDE_PX, width, height,
        g_hwnd, nullptr, GetModuleHandleW(nullptr), &state
    );
    if (!popup) return 0;

    BOOL dark = TRUE;
    DwmSetWindowAttribute(popup, 20, &dark, sizeof(dark));
    SetLayeredWindowAttributes(popup, 0, 1, LWA_ALPHA);
    StartWindowReveal(popup, 180);
    ShowWindow(popup, SW_SHOW);
    ApplyThemedMenuReveal(popup, &state);
    SetForegroundWindow(popup);
    SetFocus(popup);
    SetCapture(popup);

    MSG msg{};
    while (!state.done && IsWindow(popup) && GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    if (IsWindow(popup)) DestroyWindow(popup);
    return state.selected;
}

void ShowCourseMenu(HWND button) {
    std::wstring addCourseText = Tr(L"Add course/class", L"\u65b0\u589e\u8bfe\u7a0b/\u73ed\u7ea7");
    std::wstring renameCourseText = Tr(L"Rename current course/class", L"\u91cd\u547d\u540d\u5f53\u524d\u8bfe\u7a0b/\u73ed\u7ea7");
    std::wstring deleteCourseText = Tr(L"Delete current course/class", L"\u5220\u9664\u5f53\u524d\u8bfe\u7a0b/\u73ed\u7ea7");
    int command = ShowThemedPopupMenu(button, {
        {IDM_COURSE_ADD, addCourseText, false},
        {IDM_COURSE_RENAME, renameCourseText, false},
        {IDM_COURSE_DELETE, deleteCourseText, false}
    });
    if (command == IDM_COURSE_ADD) AddCourse();
    else if (command == IDM_COURSE_RENAME) RenameCurrentCourse();
    else if (command == IDM_COURSE_DELETE) DeleteCurrentCourse();
}

void ResetBrushes() {
    if (g_bgBrush) DeleteObject(g_bgBrush);
    if (g_inputBrush) DeleteObject(g_inputBrush);
    if (g_panelBrush) DeleteObject(g_panelBrush);
    g_bgBrush = CreateSolidBrush(COLOR_BG);
    g_inputBrush = CreateSolidBrush(COLOR_INPUT);
    g_panelBrush = CreateSolidBrush(COLOR_PANEL);
}

void ApplyThemePalette() {
    g_theme = UiTheme::Dark;
    COLOR_BG = RGB(0, 0, 0);
    COLOR_PANEL = RGB(18, 18, 18);
    COLOR_INPUT = RGB(26, 26, 26);
    COLOR_TEXT = RGB(255, 255, 255);
    COLOR_MUTED = RGB(224, 224, 224);
    COLOR_ACCENT = RGB(188, 188, 188);
    COLOR_DANGER = RGB(176, 112, 112);
    ResetBrushes();
}

void PaintAppBackground(HWND hwnd, HDC hdc) {
    RECT rc{};
    GetClientRect(hwnd, &rc);
    if (rc.right <= rc.left || rc.bottom <= rc.top) return;
    FillRect(hdc, &rc, g_bgBrush);
}

void PaintAppBackgroundSlice(HWND child, HDC hdc) {
    RECT rc{};
    GetClientRect(child, &rc);
    FillRect(hdc, &rc, g_bgBrush);
}

BOOL CALLBACK ApplyFontToChild(HWND child, LPARAM) {
    SendMessageW(child, WM_SETFONT, (WPARAM)g_font, TRUE);
    return TRUE;
}

void RecreateFonts() {
    if (g_font) DeleteObject(g_font);
    if (g_titleFont) DeleteObject(g_titleFont);
    if (g_smallFont) DeleteObject(g_smallFont);

    g_font = CreateUiFont(20, FW_NORMAL);
    g_titleFont = CreateUiFont(30, FW_SEMIBOLD);
    g_smallFont = CreateUiFont(17, FW_NORMAL);

    if (g_hwnd) {
        EnumChildWindows(g_hwnd, ApplyFontToChild, 0);
        SendMessageW(GetDlgItem(g_hwnd, IDC_TITLE), WM_SETFONT, (WPARAM)g_titleFont, TRUE);
        SendMessageW(GetDlgItem(g_hwnd, IDC_SUBTITLE), WM_SETFONT, (WPARAM)g_smallFont, TRUE);
        SendMessageW(GetDlgItem(g_hwnd, IDC_STATS), WM_SETFONT, (WPARAM)g_smallFont, TRUE);
        SendMessageW(GetDlgItem(g_hwnd, IDC_HINT), WM_SETFONT, (WPARAM)g_smallFont, TRUE);
        SendMessageW(GetDlgItem(g_hwnd, IDC_STAT_TOTAL), WM_SETFONT, (WPARAM)g_smallFont, TRUE);
        SendMessageW(GetDlgItem(g_hwnd, IDC_STAT_ATTENDANCE), WM_SETFONT, (WPARAM)g_smallFont, TRUE);
        SendMessageW(GetDlgItem(g_hwnd, IDC_STAT_ISSUES), WM_SETFONT, (WPARAM)g_smallFont, TRUE);
        SendMessageW(GetDlgItem(g_hwnd, IDC_STAT_VISIBLE), WM_SETFONT, (WPARAM)g_smallFont, TRUE);
    }
}

BOOL CALLBACK ApplyThemeToChild(HWND child, LPARAM) {
    SetWindowTheme(child, L"DarkMode_Explorer", nullptr);
    wchar_t className[32]{};
    GetClassNameW(child, className, 32);
    if (lstrcmpiW(className, L"ComboBox") == 0) ApplyComboDropDownTheme(child);
    InvalidateRect(child, nullptr, TRUE);
    return TRUE;
}

void ApplyThemedControls(HWND root) {
    if (!root) return;
    SetWindowTheme(root, L"DarkMode_Explorer", nullptr);
    EnumChildWindows(root, ApplyThemeToChild, 0);
}

void SetListColumnText(int index, const std::wstring& text) {
    LVCOLUMNW col{};
    col.mask = LVCF_TEXT;
    col.pszText = const_cast<wchar_t*>(text.c_str());
    ListView_SetColumn(g_list, index, &col);
}

void ApplyMainLanguage() {
    if (!g_hwnd) return;
    std::wstring mainTitle = Tr(L"AttendanceApp - .attd Roll Call Manager", L"AttendanceApp - .attd \u70b9\u540d\u7ba1\u7406\u5668");
    SetWindowTextW(g_hwnd, mainTitle.c_str());
    SetText(GetDlgItem(g_hwnd, IDC_TITLE), Tr(L"Attendance Manager", L"\u70b9\u540d\u7ba1\u7406\u5668"));
    SetText(GetDlgItem(g_hwnd, IDC_SUBTITLE),
        Tr(L"Create, edit, export, save, import, and batch clean .attd roll calls.",
           L"\u521b\u5efa\u3001\u7f16\u8f91\u3001\u5bfc\u51fa\u3001\u4fdd\u5b58\u3001\u5bfc\u5165\u5e76\u6279\u91cf\u6574\u7406 .attd \u70b9\u540d\u8bb0\u5f55\u3002"));
    SetText(GetDlgItem(g_hwnd, IDC_HINT),
        Tr(L"Tip: double-click a row to edit. Ctrl/Shift supports multi-select.",
           L"\u63d0\u793a\uff1a\u53cc\u51fb\u8bb0\u5f55\u53ef\u7f16\u8f91\uff0cCtrl/Shift \u53ef\u591a\u9009\u3002"));
    SetText(GetDlgItem(g_hwnd, 2001), Tr(L"Date/Time", L"\u65e5\u671f\u65f6\u95f4"));
    SetText(GetDlgItem(g_hwnd, 2002), Tr(L"Name", L"\u59d3\u540d"));
    SetText(GetDlgItem(g_hwnd, 2003), Tr(L"Other", L"\u5176\u4ed6"));
    SetText(GetDlgItem(g_hwnd, IDC_FILTER_LABEL), Tr(L"Search", L"\u641c\u7d22"));
    SetText(GetDlgItem(g_hwnd, IDC_CLEAR_FILTER), Tr(L"Clear Filter", L"\u6e05\u9664\u7b5b\u9009"));
    SetText(GetDlgItem(g_hwnd, IDC_PRESENT), Tr(L"Present", L"\u51fa\u5e2d"));
    SetText(GetDlgItem(g_hwnd, IDC_ABSENT), Tr(L"Absent", L"\u7f3a\u5e2d"));
    SetText(GetDlgItem(g_hwnd, IDC_LATE), Tr(L"Late", L"\u8fdf\u5230"));
    SetText(GetDlgItem(g_hwnd, IDC_OTHER_STATUS), Tr(L"Other", L"\u5176\u4ed6"));
    SetText(GetDlgItem(g_hwnd, IDC_ADD_UPDATE), Tr(L"Update Selected", L"\u66f4\u65b0\u9009\u4e2d"));
    SetText(GetDlgItem(g_hwnd, IDC_EDIT_SELECTED), Tr(L"Edit Selected", L"\u7f16\u8f91\u9009\u4e2d"));
    SetText(GetDlgItem(g_hwnd, IDC_ALL_PRESENT), Tr(L"Mark All Present", L"\u5168\u5458\u51fa\u5e2d"));
    SetText(GetDlgItem(g_hwnd, IDC_NEW), Tr(L"Create New", L"\u65b0\u5efa\u70b9\u540d"));
    SetText(GetDlgItem(g_hwnd, IDC_DELETE), Tr(L"Delete Options", L"\u5220\u9664\u9009\u9879"));
    SetText(GetDlgItem(g_hwnd, IDC_SAVE), Tr(L"Save .attd", L"\u4fdd\u5b58 .attd"));
    SetText(GetDlgItem(g_hwnd, IDC_IMPORT), Tr(L"Import .attd", L"\u5bfc\u5165 .attd"));
    SetText(GetDlgItem(g_hwnd, IDC_EXPORT_CSV), Tr(L"Export CSV", L"\u5bfc\u51fa CSV"));
    SetText(GetDlgItem(g_hwnd, IDC_SETTINGS), Tr(L"Settings", L"\u8bbe\u7f6e"));
    SetText(GetDlgItem(g_hwnd, IDC_COURSE_OPTIONS), Tr(L"Courses", L"\u8bfe\u7a0b"));
    SetText(GetDlgItem(g_hwnd, IDC_TOOLS), Tr(L"Tools", L"\u5de5\u5177"));

    SetListColumnText(0, Tr(L"Date / Time", L"\u65e5\u671f / \u65f6\u95f4"));
    SetListColumnText(1, Tr(L"Name", L"\u59d3\u540d"));
    SetListColumnText(2, Tr(L"Status", L"\u72b6\u6001"));
    SetListColumnText(3, Tr(L"Other", L"\u5176\u4ed6"));
    UpdateStats();
}

void ApplyVisualSettings() {
    ApplyThemePalette();
    RecreateFonts();
    ApplyMainLanguage();
    if (g_list) {
        ApplyDarkMode(g_hwnd);
    }
    BOOL dark = TRUE;
    if (g_hwnd) DwmSetWindowAttribute(g_hwnd, 20, &dark, sizeof(dark));
    if (g_settingsWindow) DwmSetWindowAttribute(g_settingsWindow, 20, &dark, sizeof(dark));
    if (g_chartWindow) DwmSetWindowAttribute(g_chartWindow, 20, &dark, sizeof(dark));
    ApplyGlassTitleBar(g_hwnd);
    ApplyThemedControls(g_hwnd);
    ApplyThemedControls(g_settingsWindow);
    if (g_hwnd) {
        InvalidateRect(g_hwnd, nullptr, TRUE);
        ResizeLayout(g_hwnd);
    }
}

void AddOrUpdateRecord(const std::wstring& status) {
    std::wstring dateTime = GetText(g_dateEdit);
    std::wstring name = GetText(g_nameEdit);
    std::wstring other = status == L"Other" ? GetText(g_otherEdit) : L"";

    if (dateTime.empty()) {
        ShowMessage(Tr(L"Please enter a date and time.", L"\u8bf7\u8f93\u5165\u65e5\u671f\u548c\u65f6\u95f4\u3002"));
        return;
    }
    if (name.empty()) {
        ShowMessage(Tr(L"Please enter a name.", L"\u8bf7\u8f93\u5165\u59d3\u540d\u3002"));
        return;
    }
    if (status == L"Other" && other.empty()) {
        ShowMessage(Tr(L"Please fill the Other field.", L"\u8bf7\u586b\u5199\u5176\u4ed6\u5b57\u6bb5\u3002"));
        return;
    }

    int selected = VisibleToRecordIndex(ListView_GetNextItem(g_list, -1, LVNI_SELECTED));
    AttendanceRecord record{dateTime, name, status, other};
    PushUndo();
    MarkDirty();
    if (selected >= 0 && selected < (int)g_records.size()) {
        g_records[selected] = record;
    } else {
        g_records.push_back(record);
    }
    RefreshList();
}

void LoadRecordIntoEditor(int index) {
    index = VisibleToRecordIndex(index);
    if (index < 0 || index >= (int)g_records.size()) {
        ShowMessage(Tr(L"Please select a record to edit.", L"\u8bf7\u9009\u62e9\u8981\u7f16\u8f91\u7684\u8bb0\u5f55\u3002"));
        return;
    }

    const auto& record = g_records[index];
    SetText(g_dateEdit, record.dateTime);
    SetText(g_nameEdit, record.name);
    SetText(g_otherEdit, record.other);
    int visibleIndex = RecordToVisibleIndex(index);
    if (visibleIndex >= 0) {
        ListView_SetItemState(g_list, visibleIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(g_list, visibleIndex, FALSE);
    }
}

void UpdateSelectedRecord() {
    int selected = VisibleToRecordIndex(ListView_GetNextItem(g_list, -1, LVNI_SELECTED));
    if (selected < 0 || selected >= (int)g_records.size()) {
        AddOrUpdateRecord(L"Present");
        return;
    }

    std::wstring dateTime = GetText(g_dateEdit);
    std::wstring name = GetText(g_nameEdit);
    std::wstring other = GetText(g_otherEdit);
    if (dateTime.empty()) {
        ShowMessage(Tr(L"Please enter a date and time.", L"\u8bf7\u8f93\u5165\u65e5\u671f\u548c\u65f6\u95f4\u3002"));
        return;
    }
    if (name.empty()) {
        ShowMessage(Tr(L"Please enter a name.", L"\u8bf7\u8f93\u5165\u59d3\u540d\u3002"));
        return;
    }

    PushUndo();
    MarkDirty();
    g_records[selected].dateTime = dateTime;
    g_records[selected].name = name;
    g_records[selected].other = g_records[selected].status == L"Other" ? other : L"";
    RefreshList();
    int visibleIndex = RecordToVisibleIndex(selected);
    if (visibleIndex >= 0) {
        ListView_SetItemState(g_list, visibleIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }
}

void MarkAllPresent() {
    if (g_records.empty()) {
        ShowMessage(Tr(L"There are no records to mark.", L"\u6ca1\u6709\u53ef\u6807\u8bb0\u7684\u8bb0\u5f55\u3002"));
        return;
    }
    std::wstring allPresentMsg = Tr(L"Mark every record as Present?", L"\u5c06\u6240\u6709\u8bb0\u5f55\u6807\u8bb0\u4e3a\u51fa\u5e2d\uff1f");
    std::wstring allPresentTitle = Tr(L"All Present", L"\u5168\u5458\u51fa\u5e2d");
    if (ThemedMessageBox(g_hwnd, allPresentMsg, allPresentTitle, true) == IDYES) {
        PushUndo();
        MarkDirty();
        for (auto& record : g_records) {
            record.status = L"Present";
            record.other.clear();
        }
        RefreshList();
    }
}

std::vector<int> SelectedRows() {
    std::vector<int> rows;
    int selected = -1;
    while ((selected = ListView_GetNextItem(g_list, selected, LVNI_SELECTED)) != -1) {
        int recordIndex = VisibleToRecordIndex(selected);
        if (recordIndex >= 0) rows.push_back(recordIndex);
    }
    return rows;
}

void EraseRows(std::vector<int> rows) {
    PushUndo();
    MarkDirty();
    std::sort(rows.begin(), rows.end());
    rows.erase(std::unique(rows.begin(), rows.end()), rows.end());
    for (auto it = rows.rbegin(); it != rows.rend(); ++it) {
        g_records.erase(g_records.begin() + *it);
    }
    RefreshList();
}

void DeleteSelectedRecords() {
    auto rows = SelectedRows();
    if (rows.empty()) {
        ShowMessage(Tr(L"Please select one or more records to delete.", L"\u8bf7\u9009\u62e9\u8981\u5220\u9664\u7684\u4e00\u6761\u6216\u591a\u6761\u8bb0\u5f55\u3002"));
        return;
    }

    std::wstringstream ss;
    ss << Tr(L"Delete", L"\u5220\u9664") << L" " << rows.size() << L" "
       << Tr(L"selected record(s)?", L"\u6761\u9009\u4e2d\u8bb0\u5f55\uff1f");
    std::wstring deleteTitle = Tr(L"Delete Selected", L"\u5220\u9664\u9009\u4e2d");
    if (ThemedMessageBox(g_hwnd, ss.str(), deleteTitle, true) == IDYES) {
        EraseRows(rows);
    }
}

void DeleteRecordsByStatus(const std::wstring& status) {
    std::vector<int> rows;
    for (int i = 0; i < (int)g_records.size(); ++i) {
        if (g_records[i].status == status) rows.push_back(i);
    }

    if (rows.empty()) {
        ShowMessage(Tr(L"No matching records found.", L"\u672a\u627e\u5230\u5339\u914d\u8bb0\u5f55\u3002"));
        return;
    }

    std::wstringstream ss;
    ss << Tr(L"Delete all", L"\u5220\u9664\u6240\u6709") << L" " << status << L" "
       << Tr(L"records?", L"\u8bb0\u5f55\uff1f") << L" (" << rows.size() << L")";
    std::wstring batchTitle = Tr(L"Batch Delete", L"\u6279\u91cf\u5220\u9664");
    if (ThemedMessageBox(g_hwnd, ss.str(), batchTitle, true) == IDYES) {
        EraseRows(rows);
    }
}

void ClearAllRecords() {
    if (g_records.empty()) {
        ShowMessage(Tr(L"There are no records to clear.", L"\u6ca1\u6709\u53ef\u6e05\u7a7a\u7684\u8bb0\u5f55\u3002"));
        return;
    }
    std::wstring clearMsg = Tr(L"Clear every attendance record in this sheet?", L"\u6e05\u7a7a\u5f53\u524d\u8868\u5185\u7684\u6240\u6709\u70b9\u540d\u8bb0\u5f55\uff1f");
    std::wstring clearTitle = Tr(L"Clear All", L"\u5168\u90e8\u6e05\u7a7a");
    if (ThemedMessageBox(g_hwnd, clearMsg, clearTitle, true) == IDYES) {
        PushUndo();
        MarkDirty();
        g_records.clear();
        RefreshList();
    }
}

void ShowDeleteMenu(HWND button) {
    std::wstring selected = Tr(L"Delete selected records", L"\u5220\u9664\u9009\u4e2d\u8bb0\u5f55");
    std::wstring absent = Tr(L"Delete all Absent records", L"\u5220\u9664\u6240\u6709\u7f3a\u5e2d\u8bb0\u5f55");
    std::wstring late = Tr(L"Delete all Late records", L"\u5220\u9664\u6240\u6709\u8fdf\u5230\u8bb0\u5f55");
    std::wstring clear = Tr(L"Clear all records", L"\u6e05\u7a7a\u6240\u6709\u8bb0\u5f55");
    int command = ShowThemedPopupMenu(button, {
        {IDM_DELETE_SELECTED, selected, false},
        {IDM_DELETE_ABSENT, absent, false},
        {IDM_DELETE_LATE, late, false},
        {0, L"", true},
        {IDM_CLEAR_ALL, clear, false}
    });

    switch (command) {
    case IDM_DELETE_SELECTED: DeleteSelectedRecords(); break;
    case IDM_DELETE_ABSENT: DeleteRecordsByStatus(L"Absent"); break;
    case IDM_DELETE_LATE: DeleteRecordsByStatus(L"Late"); break;
    case IDM_CLEAR_ALL: ClearAllRecords(); break;
    }
}

std::wstring SaveFileDialog() {
    wchar_t fileName[MAX_PATH] = L"attendance.attd";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwnd;
    ofn.lpstrFilter = L"Attendance Files (*.attd)\0*.attd\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"attd";
    if (!g_defaultSaveDir.empty()) ofn.lpstrInitialDir = g_defaultSaveDir.c_str();
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    return GetSaveFileNameW(&ofn) ? fileName : L"";
}

std::wstring SaveCsvFileDialog() {
    wchar_t fileName[MAX_PATH] = L"attendance.csv";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwnd;
    ofn.lpstrFilter = L"CSV Files (*.csv)\0*.csv\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"csv";
    if (!g_defaultSaveDir.empty()) ofn.lpstrInitialDir = g_defaultSaveDir.c_str();
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    return GetSaveFileNameW(&ofn) ? fileName : L"";
}

std::wstring OpenFileDialog() {
    wchar_t fileName[MAX_PATH] = L"";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwnd;
    ofn.lpstrFilter = L"Attendance Files (*.attd)\0*.attd\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    return GetOpenFileNameW(&ofn) ? fileName : L"";
}

std::wstring OpenCsvFileDialog() {
    wchar_t fileName[MAX_PATH] = L"";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwnd;
    ofn.lpstrFilter = L"CSV Files (*.csv)\0*.csv\0Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    return GetOpenFileNameW(&ofn) ? fileName : L"";
}

std::wstring SaveHtmlFileDialog() {
    wchar_t fileName[MAX_PATH] = L"attendance-print.html";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwnd;
    ofn.lpstrFilter = L"HTML Files (*.html)\0*.html\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"html";
    if (!g_defaultSaveDir.empty()) ofn.lpstrInitialDir = g_defaultSaveDir.c_str();
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    return GetSaveFileNameW(&ofn) ? fileName : L"";
}

std::wstring SavePptxFileDialog() {
    wchar_t fileName[MAX_PATH] = L"attendance-report.pptx";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwnd;
    ofn.lpstrFilter = L"PowerPoint Files (*.pptx)\0*.pptx\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"pptx";
    if (!g_defaultSaveDir.empty()) ofn.lpstrInitialDir = g_defaultSaveDir.c_str();
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    return GetSaveFileNameW(&ofn) ? fileName : L"";
}

std::wstring ChooseFolderDialog() {
    BROWSEINFOW bi{};
    bi.hwndOwner = g_hwnd;
    std::wstring title = Tr(L"Set default save folder", L"\u8bbe\u7f6e\u9ed8\u8ba4\u4fdd\u5b58\u6587\u4ef6\u5939");
    bi.lpszTitle = title.c_str();
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
    if (!pidl) return {};
    wchar_t path[MAX_PATH]{};
    bool ok = SHGetPathFromIDListW(pidl, path) != FALSE;
    CoTaskMemFree(pidl);
    return ok ? std::wstring(path) : L"";
}

std::filesystem::path AppDataFilePath(const wchar_t* fileName) {
    PWSTR roamingPath = nullptr;
    std::filesystem::path result;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &roamingPath))) {
        result = std::filesystem::path(roamingPath) / L"AttendanceApp" / fileName;
        CoTaskMemFree(roamingPath);
        std::filesystem::create_directories(result.parent_path());
    }
    return result;
}

std::string CsvCell(const std::wstring& value) {
    std::string text = WideToUtf8(value);
    bool quote = text.find_first_of(",\"\r\n") != std::string::npos;
    std::string out;
    if (quote) out.push_back('"');
    for (char ch : text) {
        if (ch == '"') out += "\"\"";
        else out.push_back(ch);
    }
    if (quote) out.push_back('"');
    return out;
}

std::string TsvCell(const std::wstring& value) {
    return Escape(WideToUtf8(value));
}

std::vector<std::string> ParseCsvLine(const std::string& line) {
    std::vector<std::string> cells;
    std::string cell;
    bool inQuotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char ch = line[i];
        if (ch == '"') {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                cell.push_back('"');
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (ch == ',' && !inQuotes) {
            cells.push_back(cell);
            cell.clear();
        } else {
            cell.push_back(ch);
        }
    }
    cells.push_back(cell);
    return cells;
}

std::string TrimUtf8(std::string value) {
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t' || value.front() == '\r')) value.erase(value.begin());
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r')) value.pop_back();
    return value;
}

bool LooksLikeNameHeader(const std::wstring& value) {
    std::wstring lower = value;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](wchar_t ch) { return (wchar_t)towlower(ch); });
    return lower == L"name" || lower == L"student" || lower == L"student name" || lower == L"\u59d3\u540d" || lower == L"\u5b66\u751f" || lower == L"\u5b66\u751f\u59d3\u540d";
}

void ExportCsv() {
    if (g_records.empty()) {
        ShowMessage(Tr(L"There are no records to export.", L"\u6ca1\u6709\u53ef\u5bfc\u51fa\u7684\u8bb0\u5f55\u3002"));
        return;
    }

    std::wstring path = SaveCsvFileDialog();
    if (path.empty()) return;

    std::ofstream file(std::filesystem::path(path), std::ios::binary);
    if (!file) {
        ShowMessage(Tr(L"Could not export the CSV file.", L"\u65e0\u6cd5\u5bfc\u51fa CSV \u6587\u4ef6\u3002"));
        return;
    }

    file << "\xEF\xBB\xBF";
    file << CsvCell(Tr(L"Date/Time", L"\u65e5\u671f\u65f6\u95f4")) << ','
         << CsvCell(Tr(L"Name", L"\u59d3\u540d")) << ','
         << CsvCell(Tr(L"Status", L"\u72b6\u6001")) << ','
         << CsvCell(Tr(L"Other", L"\u5176\u4ed6")) << '\n';
    for (const auto& record : g_records) {
        file << CsvCell(record.dateTime) << ','
             << CsvCell(record.name) << ','
             << CsvCell(record.status) << ','
             << CsvCell(record.other) << '\n';
    }
    ShowMessage(Tr(L"CSV exported successfully.", L"CSV \u5bfc\u51fa\u6210\u529f\u3002"));
}

void PushUndo() {
    SyncActiveSheet();
    g_undoStack.push_back(SerializeWorkbook());
    if (g_undoStack.size() > 50) g_undoStack.erase(g_undoStack.begin());
    g_redoStack.clear();
}

void RestoreSnapshot(const std::string& snapshot) {
    std::vector<AttendanceSheet> sheets;
    if (!DeserializeWorkbook(snapshot, sheets)) return;
    g_sheets = std::move(sheets);
    g_activeSheet = std::clamp(g_loadedActiveSheet, 0, (int)g_sheets.size() - 1);
    g_records = g_sheets[g_activeSheet].records;
    MarkDirty();
    RefreshCourseCombo();
    RefreshList();
}

void UndoLast() {
    if (g_undoStack.empty()) {
        ShowMessage(Tr(L"Nothing to undo.", L"\u6ca1\u6709\u53ef\u64a4\u9500\u7684\u64cd\u4f5c\u3002"));
        return;
    }
    SyncActiveSheet();
    g_redoStack.push_back(SerializeWorkbook());
    std::string snapshot = g_undoStack.back();
    g_undoStack.pop_back();
    RestoreSnapshot(snapshot);
}

void RedoLast() {
    if (g_redoStack.empty()) {
        ShowMessage(Tr(L"Nothing to redo.", L"\u6ca1\u6709\u53ef\u91cd\u505a\u7684\u64cd\u4f5c\u3002"));
        return;
    }
    SyncActiveSheet();
    g_undoStack.push_back(SerializeWorkbook());
    std::string snapshot = g_redoStack.back();
    g_redoStack.pop_back();
    RestoreSnapshot(snapshot);
}

std::string HtmlCell(const std::wstring& value) {
    std::string text = WideToUtf8(value);
    std::string out;
    for (char ch : text) {
        if (ch == '&') out += "&amp;";
        else if (ch == '<') out += "&lt;";
        else if (ch == '>') out += "&gt;";
        else if (ch == '"') out += "&quot;";
        else out.push_back(ch);
    }
    return out;
}

std::string XmlCell(const std::wstring& value) {
    return HtmlCell(value);
}

std::string XmlCellUtf8(const std::string& value) {
    std::string out;
    for (char ch : value) {
        if (ch == '&') out += "&amp;";
        else if (ch == '<') out += "&lt;";
        else if (ch == '>') out += "&gt;";
        else if (ch == '"') out += "&quot;";
        else out.push_back(ch);
    }
    return out;
}

struct ZipEntry {
    std::string name;
    std::string data;
    uint32_t crc = 0;
    uint32_t offset = 0;
};

uint32_t Crc32(const std::string& data) {
    uint32_t crc = 0xFFFFFFFFu;
    for (unsigned char ch : data) {
        crc ^= ch;
        for (int i = 0; i < 8; ++i) {
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
        }
    }
    return ~crc;
}

void Write16(std::ofstream& file, uint16_t value) {
    file.put((char)(value & 0xFF));
    file.put((char)((value >> 8) & 0xFF));
}

void Write32(std::ofstream& file, uint32_t value) {
    Write16(file, (uint16_t)(value & 0xFFFF));
    Write16(file, (uint16_t)((value >> 16) & 0xFFFF));
}

bool WriteZipStore(const std::filesystem::path& path, std::vector<ZipEntry> entries) {
    std::ofstream file(path, std::ios::binary);
    if (!file) return false;

    for (auto& entry : entries) {
        entry.crc = Crc32(entry.data);
        entry.offset = (uint32_t)file.tellp();
        Write32(file, 0x04034b50);
        Write16(file, 20);
        Write16(file, 0);
        Write16(file, 0);
        Write16(file, 0);
        Write16(file, 0);
        Write32(file, entry.crc);
        Write32(file, (uint32_t)entry.data.size());
        Write32(file, (uint32_t)entry.data.size());
        Write16(file, (uint16_t)entry.name.size());
        Write16(file, 0);
        file.write(entry.name.data(), entry.name.size());
        file.write(entry.data.data(), entry.data.size());
    }

    uint32_t centralOffset = (uint32_t)file.tellp();
    for (const auto& entry : entries) {
        Write32(file, 0x02014b50);
        Write16(file, 20);
        Write16(file, 20);
        Write16(file, 0);
        Write16(file, 0);
        Write16(file, 0);
        Write16(file, 0);
        Write32(file, entry.crc);
        Write32(file, (uint32_t)entry.data.size());
        Write32(file, (uint32_t)entry.data.size());
        Write16(file, (uint16_t)entry.name.size());
        Write16(file, 0);
        Write16(file, 0);
        Write16(file, 0);
        Write16(file, 0);
        Write32(file, 0);
        Write32(file, entry.offset);
        file.write(entry.name.data(), entry.name.size());
    }
    uint32_t centralSize = (uint32_t)file.tellp() - centralOffset;
    Write32(file, 0x06054b50);
    Write16(file, 0);
    Write16(file, 0);
    Write16(file, (uint16_t)entries.size());
    Write16(file, (uint16_t)entries.size());
    Write32(file, centralSize);
    Write32(file, centralOffset);
    Write16(file, 0);
    return true;
}

struct PptStats {
    int present = 0;
    int absent = 0;
    int late = 0;
    int other = 0;
    int total = 0;
};

PptStats CountRecords(const std::vector<AttendanceRecord>& records) {
    PptStats stats{};
    stats.total = (int)records.size();
    for (const auto& r : records) {
        if (r.status == L"Present") ++stats.present;
        else if (r.status == L"Absent") ++stats.absent;
        else if (r.status == L"Late") ++stats.late;
        else ++stats.other;
    }
    return stats;
}

std::string ShapeText(int id, int x, int y, int cx, int cy, const std::wstring& text, int size, const char* color, bool bold = false) {
    std::ostringstream ss;
    ss << "<p:sp><p:nvSpPr><p:cNvPr id=\"" << id << "\" name=\"Text" << id
       << "\"/><p:cNvSpPr txBox=\"1\"/><p:nvPr/></p:nvSpPr><p:spPr><a:xfrm><a:off x=\"" << x << "\" y=\"" << y
       << "\"/><a:ext cx=\"" << cx << "\" cy=\"" << cy << "\"/></a:xfrm><a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom>"
       << "<a:noFill/><a:ln><a:noFill/></a:ln></p:spPr><p:txBody><a:bodyPr wrap=\"square\"/><a:lstStyle/><a:p>"
       << "<a:r><a:rPr lang=\"en-US\" sz=\"" << size << "\"" << (bold ? " b=\"1\"" : "") << "><a:solidFill><a:srgbClr val=\""
       << color << "\"/></a:solidFill></a:rPr><a:t>" << XmlCell(text) << "</a:t></a:r></a:p></p:txBody></p:sp>";
    return ss.str();
}

std::string RectShape(int id, int x, int y, int cx, int cy, const char* fill, const char* line = "FFFFFF", int alpha = 100000) {
    std::ostringstream ss;
    ss << "<p:sp><p:nvSpPr><p:cNvPr id=\"" << id << "\" name=\"Rect" << id
       << "\"/><p:cNvSpPr/><p:nvPr/></p:nvSpPr><p:spPr><a:xfrm><a:off x=\"" << x << "\" y=\"" << y
       << "\"/><a:ext cx=\"" << cx << "\" cy=\"" << cy << "\"/></a:xfrm><a:prstGeom prst=\"roundRect\"><a:avLst/></a:prstGeom>"
       << "<a:solidFill><a:srgbClr val=\"" << fill << "\"><a:alpha val=\"" << alpha << "\"/></a:srgbClr></a:solidFill>"
       << "<a:ln w=\"12000\"><a:solidFill><a:srgbClr val=\"" << line << "\"/></a:solidFill></a:ln></p:spPr></p:sp>";
    return ss.str();
}

std::string LineShape(int id, int x1, int y1, int x2, int y2, const char* color, int width = 36000) {
    std::ostringstream ss;
    bool flipH = x2 < x1;
    bool flipV = y2 < y1;
    ss << "<p:cxnSp><p:nvCxnSpPr><p:cNvPr id=\"" << id << "\" name=\"Line" << id
       << "\"/><p:cNvCxnSpPr/><p:nvPr/></p:nvCxnSpPr><p:spPr><a:xfrm";
    if (flipH) ss << " flipH=\"1\"";
    if (flipV) ss << " flipV=\"1\"";
    ss << "><a:off x=\"" << std::min(x1, x2) << "\" y=\"" << std::min(y1, y2)
       << "\"/><a:ext cx=\"" << std::max(1, std::abs(x2 - x1)) << "\" cy=\"" << std::max(1, std::abs(y2 - y1)) << "\"/></a:xfrm>"
       << "<a:prstGeom prst=\"line\"><a:avLst/></a:prstGeom><a:ln w=\"" << width << "\"><a:solidFill><a:srgbClr val=\""
       << color << "\"/></a:solidFill></a:ln></p:spPr></p:cxnSp>";
    return ss.str();
}

std::string TriangleShape(int id, int x1, int y1, int x2, int y2, int x3, int y3, const char* fill) {
    std::ostringstream ss;
    ss << "<p:sp><p:nvSpPr><p:cNvPr id=\"" << id << "\" name=\"PieSlice" << id
       << "\"/><p:cNvSpPr/><p:nvPr/></p:nvSpPr><p:spPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"12192000\" cy=\"6858000\"/></a:xfrm>"
       << "<a:custGeom><a:avLst/><a:gdLst/><a:ahLst/><a:cxnLst/><a:rect l=\"0\" t=\"0\" r=\"12192000\" b=\"6858000\"/>"
       << "<a:pathLst><a:path w=\"12192000\" h=\"6858000\"><a:moveTo><a:pt x=\"" << x1 << "\" y=\"" << y1
       << "\"/></a:moveTo><a:lnTo><a:pt x=\"" << x2 << "\" y=\"" << y2 << "\"/></a:lnTo><a:lnTo><a:pt x=\""
       << x3 << "\" y=\"" << y3 << "\"/></a:lnTo><a:close/></a:path></a:pathLst></a:custGeom>"
       << "<a:solidFill><a:srgbClr val=\"" << fill << "\"/></a:solidFill><a:ln><a:noFill/></a:ln></p:spPr></p:sp>";
    return ss.str();
}

std::string EllipseShape(int id, int x, int y, int cx, int cy, const char* fill, const char* line = "FFFFFF", int alpha = 100000) {
    std::ostringstream ss;
    ss << "<p:sp><p:nvSpPr><p:cNvPr id=\"" << id << "\" name=\"Ellipse" << id
       << "\"/><p:cNvSpPr/><p:nvPr/></p:nvSpPr><p:spPr><a:xfrm><a:off x=\"" << x << "\" y=\"" << y
       << "\"/><a:ext cx=\"" << cx << "\" cy=\"" << cy << "\"/></a:xfrm><a:prstGeom prst=\"ellipse\"><a:avLst/></a:prstGeom>"
       << "<a:solidFill><a:srgbClr val=\"" << fill << "\"><a:alpha val=\"" << alpha << "\"/></a:srgbClr></a:solidFill>"
       << "<a:ln w=\"18000\"><a:solidFill><a:srgbClr val=\"" << line << "\"/></a:solidFill></a:ln></p:spPr></p:sp>";
    return ss.str();
}

void AddPieShape(std::ostringstream& ss, int& id, const PptStats& stats, int cx, int cy, int radius) {
    int values[] = {stats.present, stats.absent, stats.late, stats.other};
    const char* colors[] = {"8CA39A", "A67575", "A89876", "9A9A9A"};
    if (stats.total <= 0) {
        ss << EllipseShape(id++, cx - radius, cy - radius, radius * 2, radius * 2, "1A1A1A", "E0E0E0", 100000);
        return;
    }

    double angle = -90.0;
    for (int i = 0; i < 4; ++i) {
        if (values[i] <= 0) continue;
        double sweep = values[i] * 360.0 / stats.total;
        int steps = std::max(1, (int)std::ceil(sweep / 8.0));
        for (int step = 0; step < steps; ++step) {
            double a1 = angle + sweep * step / steps - (step > 0 ? 0.55 : 0.0);
            double a2 = angle + sweep * (step + 1) / steps + (step + 1 < steps ? 0.55 : 0.0);
            double r1 = a1 * 3.14159265358979323846 / 180.0;
            double r2 = a2 * 3.14159265358979323846 / 180.0;
            int x2 = cx + (int)std::round(radius * std::cos(r1));
            int y2 = cy + (int)std::round(radius * std::sin(r1));
            int x3 = cx + (int)std::round(radius * std::cos(r2));
            int y3 = cy + (int)std::round(radius * std::sin(r2));
            ss << TriangleShape(id++, cx, cy, x2, y2, x3, y3, colors[i]);
        }
        angle += sweep;
    }
    ss << EllipseShape(id++, cx - radius, cy - radius, radius * 2, radius * 2, "FFFFFF", "E0E0E0", 0);
}

std::string SlideBackground() {
    return "<p:sp><p:nvSpPr><p:cNvPr id=\"2\" name=\"Background\"/><p:cNvSpPr/><p:nvPr/></p:nvSpPr>"
           "<p:spPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"12192000\" cy=\"6858000\"/></a:xfrm>"
           "<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom><a:solidFill><a:srgbClr val=\"000000\"/></a:solidFill><a:ln><a:noFill/></a:ln></p:spPr></p:sp>";
}

std::string SlideXml(const std::string& body) {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
           "<p:sld xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\">"
           "<p:cSld><p:spTree><p:nvGrpSpPr><p:cNvPr id=\"1\" name=\"\"/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr><p:grpSpPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"0\" cy=\"0\"/><a:chOff x=\"0\" y=\"0\"/><a:chExt cx=\"0\" cy=\"0\"/></a:xfrm></p:grpSpPr>"
           + body +
           "</p:spTree></p:cSld><p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr></p:sld>";
}

std::string BuildPptSlide1(const std::wstring& course, const PptStats& stats) {
    int id = 3;
    double rate = stats.total ? stats.present * 100.0 / stats.total : 0.0;
    std::wstringstream rateText;
    rateText.setf(std::ios::fixed);
    rateText.precision(1);
    rateText << rate << L"%";
    std::ostringstream ss;
    ss << SlideBackground();
    ss << ShapeText(id++, 760000, 520000, 8000000, 340000, Tr(L"Attendance", L"\u51fa\u52e4\u7387") + L" Report", 1300, "BDBDBD", true);
    ss << ShapeText(id++, 740000, 900000, 8400000, 700000, Tr(L"Attendance Manager", L"\u70b9\u540d\u7ba1\u7406\u5668"), 3400, "FFFFFF", true);
    ss << ShapeText(id++, 760000, 1600000, 7600000, 420000, course, 1600, "E0E0E0");
    ss << RectShape(id++, 760000, 2180000, 10670000, 22000, "BDBDBD", "BDBDBD", 100000);

    ss << RectShape(id++, 800000, 2680000, 4650000, 2450000, "121212", "444444", 93000);
    ss << ShapeText(id++, 1160000, 3020000, 3600000, 360000, Tr(L"Attendance", L"\u51fa\u52e4\u7387"), 1550, "8CA39A", true);
    ss << ShapeText(id++, 1120000, 3420000, 3400000, 920000, rateText.str(), 4400, "FFFFFF", true);
    ss << ShapeText(id++, 1160000, 4520000, 3600000, 340000, Tr(L"Present", L"\u51fa\u5e2d") + L" " + std::to_wstring(stats.present) + L" / " + Tr(L"Total", L"\u603b\u6570") + L" " + std::to_wstring(stats.total), 1250, "B8B8B8", true);

    const wchar_t* labels[] = {L"Total", L"Present", L"Absent", L"Late"};
    int values[] = {stats.total, stats.present, stats.absent, stats.late};
    const char* fills[] = {"1A1A1A", "18221E", "2A1F1F", "292515"};
    for (int i = 0; i < 4; ++i) {
        int x = 5850000 + (i % 2) * 2550000;
        int y = 2680000 + (i / 2) * 1250000;
        ss << RectShape(id++, x, y, 2300000, 960000, fills[i], "444444", 94000);
        ss << ShapeText(id++, x + 240000, y + 170000, 1700000, 280000, Tr(labels[i], labels[i]), 1150, "E0E0E0", true);
        ss << ShapeText(id++, x + 240000, y + 460000, 1700000, 380000, std::to_wstring(values[i]), 2500, "FFFFFF", true);
    }

    ss << ShapeText(id++, 760000, 5950000, 10400000, 340000, Tr(L"Create, edit, export, save, import, and batch clean .attd roll calls.", L""), 1050, "B8B8B8");
    return SlideXml(ss.str());
}

std::string BuildPptSlide2(const PptStats& stats) {
    int id = 3;
    std::ostringstream ss;
    ss << SlideBackground();
    ss << ShapeText(id++, 760000, 520000, 9000000, 520000, Tr(L"Statistics Chart", L"\u7edf\u8ba1\u56fe\u8868"), 2900, "FFFFFF", true);
    ss << ShapeText(id++, 780000, 1050000, 9000000, 300000, Tr(L"Status", L"\u72b6\u6001") + L" " + Tr(L"Statistics chart", L"\u7edf\u8ba1\u56fe\u8868"), 1150, "B8B8B8");
    ss << RectShape(id++, 760000, 1420000, 10670000, 22000, "BDBDBD", "BDBDBD", 100000);
    const wchar_t* labels[] = {L"Present", L"Absent", L"Late", L"Other"};
    int values[] = {stats.present, stats.absent, stats.late, stats.other};
    const char* colors[] = {"8CA39A", "A67575", "A89876", "9A9A9A"};

    ss << RectShape(id++, 760000, 1750000, 4550000, 3900000, "121212", "444444", 93000);
    ss << ShapeText(id++, 1080000, 2020000, 3400000, 330000, Tr(L"Attendance Sheet", L"\u70b9\u540d\u8868"), 1350, "FFFFFF", true);
    ss << EllipseShape(id++, 2000000, 2600000, 1800000, 1800000, "000000", "444444", 100000);
    AddPieShape(ss, id, stats, 2900000, 3500000, 850000);
    ss << RectShape(id++, 4050000, 3050000, 850000, 900000, "1A1A1A", "444444", 86000);
    ss << ShapeText(id++, 4210000, 3220000, 560000, 240000, Tr(L"Total", L"\u603b\u6570"), 850, "B8B8B8", true);
    ss << ShapeText(id++, 4210000, 3500000, 560000, 350000, std::to_wstring(stats.total), 1800, "FFFFFF", true);

    ss << RectShape(id++, 5600000, 1750000, 5830000, 3900000, "121212", "444444", 93000);
    ss << ShapeText(id++, 5920000, 2020000, 2400000, 330000, Tr(L"Total", L"\u603b\u6570") + L" " + std::to_wstring(stats.total), 1350, "FFFFFF", true);
    ss << ShapeText(id++, 9100000, 2020000, 1900000, 330000, Tr(L"Attendance", L"\u51fa\u52e4\u7387"), 1200, "8CA39A", true);
    int attendancePct = stats.total ? (int)std::round(stats.present * 100.0 / stats.total) : 0;
    ss << ShapeText(id++, 10180000, 1940000, 850000, 460000, std::to_wstring(attendancePct) + L"%", 2050, "FFFFFF", true);

    int pctY = 2550000;
    for (int i = 0; i < 4; ++i) {
        int x = 5920000 + (i % 2) * 2650000;
        int y = pctY + (i / 2) * 600000;
        int pct = stats.total ? (int)std::round(values[i] * 100.0 / stats.total) : 0;
        ss << RectShape(id++, x, y, 2350000, 460000, "1A1A1A", "444444", 88000);
        ss << RectShape(id++, x + 180000, y + 115000, 220000, 220000, colors[i], colors[i], 100000);
        ss << ShapeText(id++, x + 500000, y + 70000, 1000000, 240000, Tr(labels[i], labels[i]), 900, "E0E0E0", true);
        ss << ShapeText(id++, x + 500000, y + 270000, 1200000, 240000, std::to_wstring(values[i]) + L" | " + std::to_wstring(pct) + L"%", 1050, "FFFFFF", true);
    }

    int maxVal = std::max(1, std::max(std::max(values[0], values[1]), std::max(values[2], values[3])));
    ss << ShapeText(id++, 5920000, 3930000, 2500000, 310000, Tr(L"Statistics chart", L"\u7edf\u8ba1\u56fe\u8868"), 1200, "FFFFFF", true);
    for (int i = 0; i < 4; ++i) {
        int y = 4300000 + i * 280000;
        ss << ShapeText(id++, 5920000, y - 50000, 950000, 240000, Tr(labels[i], labels[i]), 780, "B8B8B8", true);
        ss << RectShape(id++, 6920000, y, 3000000, 120000, "242424", "242424", 100000);
        ss << RectShape(id++, 6920000, y, values[i] * 3000000 / maxVal + 1000, 120000, colors[i], colors[i], 100000);
        ss << ShapeText(id++, 10120000, y - 60000, 600000, 230000, std::to_wstring(values[i]), 780, "FFFFFF", true);
    }
    return SlideXml(ss.str());
}

std::string BuildPptSlide3(const std::vector<AttendanceRecord>& records) {
    int id = 3;
    std::vector<std::pair<std::wstring, PptStats>> days;
    for (const auto& r : records) {
        std::wstring day = r.dateTime.size() >= 10 ? r.dateTime.substr(0, 10) : r.dateTime;
        auto it = std::find_if(days.begin(), days.end(), [&](const auto& p) { return p.first == day; });
        if (it == days.end()) {
            PptStats dayStats{};
            dayStats.total = 1;
            if (r.status == L"Present") ++dayStats.present;
            else if (r.status == L"Absent") ++dayStats.absent;
            else if (r.status == L"Late") ++dayStats.late;
            else ++dayStats.other;
            days.push_back({day, dayStats});
        } else {
            if (r.status == L"Present") ++it->second.present;
            else if (r.status == L"Absent") ++it->second.absent;
            else if (r.status == L"Late") ++it->second.late;
            else ++it->second.other;
            ++it->second.total;
        }
    }
    std::sort(days.begin(), days.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    if (days.size() > 8) days.erase(days.begin(), days.end() - 8);

    std::ostringstream ss;
    ss << SlideBackground();
    ss << ShapeText(id++, 760000, 520000, 8800000, 520000, Tr(L"Attendance", L"\u51fa\u52e4\u7387") + L" " + Tr(L"Statistics chart", L"\u7edf\u8ba1\u56fe\u8868"), 2800, "FFFFFF", true);
    ss << ShapeText(id++, 780000, 1050000, 7200000, 300000, Tr(L"Attendance", L"\u51fa\u52e4\u7387") + L" trend by date", 1150, "B8B8B8");
    ss << RectShape(id++, 760000, 1420000, 10670000, 22000, "BDBDBD", "BDBDBD", 100000);
    const wchar_t* labels[] = {L"Present", L"Absent", L"Late", L"Other"};
    const char* colors[] = {"8CA39A", "A67575", "A89876", "9A9A9A"};

    double latestRate = 0.0;
    if (!days.empty()) {
        const auto& latest = days.back().second;
        latestRate = latest.total ? latest.present * 100.0 / latest.total : 0.0;
    }
    std::wstringstream pct;
    pct.setf(std::ios::fixed);
    pct.precision(0);
    pct << latestRate << L"%";

    ss << RectShape(id++, 9000000, 520000, 2140000, 700000, "121212", "444444", 93000);
    ss << ShapeText(id++, 9250000, 650000, 1100000, 260000, Tr(L"Attendance", L"\u51fa\u52e4\u7387"), 900, "8CA39A", true);
    ss << ShapeText(id++, 10200000, 600000, 800000, 400000, pct.str(), 1800, "FFFFFF", true);

    ss << RectShape(id++, 760000, 1700000, 10670000, 3950000, "121212", "444444", 93000);
    for (int i = 0; i < 4; ++i) {
        int x = 1180000 + i * 1750000;
        ss << RectShape(id++, x, 1950000, 210000, 210000, colors[i], colors[i], 100000);
        ss << ShapeText(id++, x + 280000, 1895000, 1180000, 300000, Tr(labels[i], labels[i]), 900, "E0E0E0", true);
    }

    int x0 = 1180000, y0 = 5300000, w = 9650000, h = 2800000;
    ss << RectShape(id++, x0, y0, w, 18000, "BDBDBD", "BDBDBD", 100000);
    ss << RectShape(id++, x0, y0 - h, 18000, h, "BDBDBD", "BDBDBD", 100000);
    ss << LineShape(id++, x0, y0 - h / 2, x0 + w, y0 - h / 2, "444444", 14000);
    if (days.empty()) {
        ss << ShapeText(id++, x0 + 600000, y0 - 1900000, 6000000, 500000, Tr(L"There are no records to export.", L"\u6ca1\u6709\u53ef\u5bfc\u51fa\u7684\u8bb0\u5f55\u3002"), 1800, "FFFFFF", true);
    } else {
        int slot = w / std::max(1, (int)days.size());
        int barW = std::clamp(slot / 8, 90000, 210000);
        int gap = std::max(25000, barW / 4);
        int maxCount = 1;
        for (const auto& day : days) {
            maxCount = std::max(maxCount, std::max(std::max(day.second.present, day.second.absent), std::max(day.second.late, day.second.other)));
        }
        std::vector<POINT> trend;
        for (int i = 0; i < (int)days.size(); ++i) {
            int groupW = barW * 4 + gap * 3;
            int groupX = x0 + i * slot + (slot - groupW) / 2;
            int counts[] = {days[i].second.present, days[i].second.absent, days[i].second.late, days[i].second.other};
            for (int j = 0; j < 4; ++j) {
                int barH = counts[j] == 0 ? 45000 : std::max(80000, counts[j] * h / maxCount);
                int x = groupX + j * (barW + gap);
                int y = y0 - barH;
                ss << RectShape(id++, x, y, barW, barH, colors[j], colors[j], 100000);
            }
            double rate = days[i].second.total ? days[i].second.present * 1.0 / days[i].second.total : 0.0;
            int tx = x0 + i * slot + slot / 2;
            int ty = y0 - (int)(rate * h);
            trend.push_back({tx, ty});
            ss << ShapeText(id++, x0 + i * slot, y0 + 110000, slot, 260000, days[i].first, 650, "E0E0E0");
        }
        for (int i = 1; i < (int)trend.size(); ++i) {
            ss << LineShape(id++, trend[i - 1].x, trend[i - 1].y, trend[i].x, trend[i].y, "FFFFFF", 30000);
        }
    }
    return SlideXml(ss.str());
}

std::string PptContentTypes() {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
           "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
           "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
           "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
           "<Override PartName=\"/ppt/presentation.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.presentation.main+xml\"/>"
           "<Override PartName=\"/ppt/slides/slide1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slide+xml\"/>"
           "<Override PartName=\"/ppt/slides/slide2.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slide+xml\"/>"
           "<Override PartName=\"/ppt/slides/slide3.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slide+xml\"/>"
           "<Override PartName=\"/ppt/presProps.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.presProps+xml\"/>"
           "<Override PartName=\"/ppt/viewProps.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.viewProps+xml\"/>"
           "<Override PartName=\"/ppt/tableStyles.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.tableStyles+xml\"/>"
           "<Override PartName=\"/ppt/slideMasters/slideMaster1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slideMaster+xml\"/>"
           "<Override PartName=\"/ppt/slideLayouts/slideLayout1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slideLayout+xml\"/>"
           "<Override PartName=\"/ppt/theme/theme1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.theme+xml\"/>"
           "<Override PartName=\"/docProps/core.xml\" ContentType=\"application/vnd.openxmlformats-package.core-properties+xml\"/>"
           "<Override PartName=\"/docProps/app.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.extended-properties+xml\"/>"
           "</Types>";
}

std::string PptDefaultTextStyleXml() {
    return "<p:defaultTextStyle><a:defPPr><a:defRPr lang=\"en-US\"/></a:defPPr>"
           "<a:lvl1pPr marL=\"0\" algn=\"l\" defTabSz=\"914400\" rtl=\"0\" eaLnBrk=\"1\" latinLnBrk=\"0\" hangingPunct=\"1\"><a:defRPr sz=\"1800\" kern=\"1200\"><a:solidFill><a:schemeClr val=\"tx1\"/></a:solidFill><a:latin typeface=\"+mn-lt\"/><a:ea typeface=\"+mn-ea\"/><a:cs typeface=\"+mn-cs\"/></a:defRPr></a:lvl1pPr>"
           "<a:lvl2pPr marL=\"457200\" algn=\"l\" defTabSz=\"914400\" rtl=\"0\" eaLnBrk=\"1\" latinLnBrk=\"0\" hangingPunct=\"1\"><a:defRPr sz=\"1800\" kern=\"1200\"><a:solidFill><a:schemeClr val=\"tx1\"/></a:solidFill><a:latin typeface=\"+mn-lt\"/><a:ea typeface=\"+mn-ea\"/><a:cs typeface=\"+mn-cs\"/></a:defRPr></a:lvl2pPr>"
           "<a:lvl3pPr marL=\"914400\" algn=\"l\" defTabSz=\"914400\" rtl=\"0\" eaLnBrk=\"1\" latinLnBrk=\"0\" hangingPunct=\"1\"><a:defRPr sz=\"1800\" kern=\"1200\"><a:solidFill><a:schemeClr val=\"tx1\"/></a:solidFill><a:latin typeface=\"+mn-lt\"/><a:ea typeface=\"+mn-ea\"/><a:cs typeface=\"+mn-cs\"/></a:defRPr></a:lvl3pPr>"
           "<a:lvl4pPr marL=\"1371600\" algn=\"l\" defTabSz=\"914400\" rtl=\"0\" eaLnBrk=\"1\" latinLnBrk=\"0\" hangingPunct=\"1\"><a:defRPr sz=\"1800\" kern=\"1200\"><a:solidFill><a:schemeClr val=\"tx1\"/></a:solidFill><a:latin typeface=\"+mn-lt\"/><a:ea typeface=\"+mn-ea\"/><a:cs typeface=\"+mn-cs\"/></a:defRPr></a:lvl4pPr>"
           "<a:lvl5pPr marL=\"1828800\" algn=\"l\" defTabSz=\"914400\" rtl=\"0\" eaLnBrk=\"1\" latinLnBrk=\"0\" hangingPunct=\"1\"><a:defRPr sz=\"1800\" kern=\"1200\"><a:solidFill><a:schemeClr val=\"tx1\"/></a:solidFill><a:latin typeface=\"+mn-lt\"/><a:ea typeface=\"+mn-ea\"/><a:cs typeface=\"+mn-cs\"/></a:defRPr></a:lvl5pPr>"
           "</p:defaultTextStyle>";
}

std::string PptPresPropsXml() {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
           "<p:presentationPr xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\"/>";
}

std::string PptViewPropsXml() {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
           "<p:viewPr xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\"><p:normalViewPr><p:restoredLeft sz=\"15620\"/><p:restoredTop sz=\"94660\"/></p:normalViewPr><p:slideViewPr><p:cSldViewPr snapToGrid=\"0\"><p:cViewPr varScale=\"1\"><p:scale><a:sx n=\"70\" d=\"100\"/><a:sy n=\"70\" d=\"100\"/></p:scale><p:origin x=\"0\" y=\"0\"/></p:cViewPr><p:guideLst/></p:cSldViewPr></p:slideViewPr><p:notesTextViewPr><p:cViewPr><p:scale><a:sx n=\"1\" d=\"1\"/><a:sy n=\"1\" d=\"1\"/></p:scale><p:origin x=\"0\" y=\"0\"/></p:cViewPr></p:notesTextViewPr><p:gridSpacing cx=\"72008\" cy=\"72008\"/></p:viewPr>";
}

std::string PptTableStylesXml() {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
           "<a:tblStyleLst xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" def=\"{5C22544A-7EE6-4342-B048-85BDC9FD1C3A}\"/>";
}

std::string PptThemeXml() {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
           "<a:theme xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" name=\"AttendanceApp\">"
           "<a:themeElements><a:clrScheme name=\"AttendanceApp\"><a:dk1><a:srgbClr val=\"000000\"/></a:dk1><a:lt1><a:srgbClr val=\"FFFFFF\"/></a:lt1><a:dk2><a:srgbClr val=\"121212\"/></a:dk2><a:lt2><a:srgbClr val=\"E0E0E0\"/></a:lt2><a:accent1><a:srgbClr val=\"8CA39A\"/></a:accent1><a:accent2><a:srgbClr val=\"A67575\"/></a:accent2><a:accent3><a:srgbClr val=\"A89876\"/></a:accent3><a:accent4><a:srgbClr val=\"9A9A9A\"/></a:accent4><a:accent5><a:srgbClr val=\"B8B8B8\"/></a:accent5><a:accent6><a:srgbClr val=\"FFFFFF\"/></a:accent6><a:hlink><a:srgbClr val=\"E0E0E0\"/></a:hlink><a:folHlink><a:srgbClr val=\"B8B8B8\"/></a:folHlink></a:clrScheme>"
           "<a:fontScheme name=\"AttendanceApp\"><a:majorFont><a:latin typeface=\"Segoe UI\"/><a:ea typeface=\"\"/><a:cs typeface=\"\"/></a:majorFont><a:minorFont><a:latin typeface=\"Segoe UI\"/><a:ea typeface=\"\"/><a:cs typeface=\"\"/></a:minorFont></a:fontScheme>"
           "<a:fmtScheme name=\"AttendanceApp\"><a:fillStyleLst>"
           "<a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill>"
           "<a:gradFill rotWithShape=\"1\"><a:gsLst><a:gs pos=\"0\"><a:schemeClr val=\"phClr\"><a:lumMod val=\"110000\"/><a:satMod val=\"105000\"/></a:schemeClr></a:gs><a:gs pos=\"100000\"><a:schemeClr val=\"phClr\"><a:lumMod val=\"90000\"/><a:satMod val=\"105000\"/></a:schemeClr></a:gs></a:gsLst><a:lin ang=\"5400000\" scaled=\"0\"/></a:gradFill>"
           "<a:gradFill rotWithShape=\"1\"><a:gsLst><a:gs pos=\"0\"><a:schemeClr val=\"phClr\"><a:lumMod val=\"105000\"/></a:schemeClr></a:gs><a:gs pos=\"100000\"><a:schemeClr val=\"phClr\"><a:lumMod val=\"75000\"/></a:schemeClr></a:gs></a:gsLst><a:lin ang=\"5400000\" scaled=\"0\"/></a:gradFill>"
           "</a:fillStyleLst><a:lnStyleLst>"
           "<a:ln w=\"9525\"><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill><a:prstDash val=\"solid\"/></a:ln>"
           "<a:ln w=\"25400\"><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill><a:prstDash val=\"solid\"/></a:ln>"
           "<a:ln w=\"38100\"><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill><a:prstDash val=\"solid\"/></a:ln>"
           "</a:lnStyleLst><a:effectStyleLst><a:effectStyle><a:effectLst/></a:effectStyle><a:effectStyle><a:effectLst/></a:effectStyle><a:effectStyle><a:effectLst/></a:effectStyle></a:effectStyleLst>"
           "<a:bgFillStyleLst><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill><a:solidFill><a:schemeClr val=\"phClr\"><a:tint val=\"95000\"/></a:schemeClr></a:solidFill><a:solidFill><a:schemeClr val=\"phClr\"><a:shade val=\"85000\"/></a:schemeClr></a:solidFill></a:bgFillStyleLst>"
           "</a:fmtScheme></a:themeElements></a:theme>";
}

std::string PptSlideLayoutXml() {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
           "<p:sldLayout xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\" type=\"blank\" preserve=\"1\"><p:cSld name=\"Blank\"><p:spTree><p:nvGrpSpPr><p:cNvPr id=\"1\" name=\"\"/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr><p:grpSpPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"0\" cy=\"0\"/><a:chOff x=\"0\" y=\"0\"/><a:chExt cx=\"0\" cy=\"0\"/></a:xfrm></p:grpSpPr></p:spTree></p:cSld><p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr></p:sldLayout>";
}

std::string PptSlideMasterXml() {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
           "<p:sldMaster xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\"><p:cSld><p:spTree><p:nvGrpSpPr><p:cNvPr id=\"1\" name=\"\"/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr><p:grpSpPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"0\" cy=\"0\"/><a:chOff x=\"0\" y=\"0\"/><a:chExt cx=\"0\" cy=\"0\"/></a:xfrm></p:grpSpPr></p:spTree></p:cSld><p:clrMap bg1=\"lt1\" tx1=\"dk1\" bg2=\"lt2\" tx2=\"dk2\" accent1=\"accent1\" accent2=\"accent2\" accent3=\"accent3\" accent4=\"accent4\" accent5=\"accent5\" accent6=\"accent6\" hlink=\"hlink\" folHlink=\"folHlink\"/><p:sldLayoutIdLst><p:sldLayoutId id=\"2147483649\" r:id=\"rId1\"/></p:sldLayoutIdLst><p:txStyles><p:titleStyle/><p:bodyStyle/><p:otherStyle/></p:txStyles></p:sldMaster>";
}

bool ExportPptxFile(const std::wstring& path) {
    SyncActiveSheet();
    EnsureSheets();
    const auto& sheet = g_sheets[g_activeSheet];
    PptStats stats = CountRecords(sheet.records);
    std::vector<ZipEntry> entries;
    entries.push_back({"[Content_Types].xml", PptContentTypes()});
    entries.push_back({"_rels/.rels", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"ppt/presentation.xml\"/><Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties\" Target=\"docProps/core.xml\"/><Relationship Id=\"rId3\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties\" Target=\"docProps/app.xml\"/></Relationships>"});
    entries.push_back({"docProps/app.xml", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Properties xmlns=\"http://schemas.openxmlformats.org/officeDocument/2006/extended-properties\" xmlns:vt=\"http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes\"><Application>AttendanceApp</Application><PresentationFormat>Widescreen</PresentationFormat><Slides>3</Slides></Properties>"});
    entries.push_back({"docProps/core.xml", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><cp:coreProperties xmlns:cp=\"http://schemas.openxmlformats.org/package/2006/metadata/core-properties\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:dcterms=\"http://purl.org/dc/terms/\" xmlns:dcmitype=\"http://purl.org/dc/dcmitype/\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"><dc:title>Attendance Report</dc:title><dc:creator>AttendanceApp</dc:creator></cp:coreProperties>"});
    entries.push_back({"ppt/presentation.xml", std::string("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><p:presentation xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\" saveSubsetFonts=\"1\"><p:sldMasterIdLst><p:sldMasterId id=\"2147483648\" r:id=\"rId4\"/></p:sldMasterIdLst><p:sldIdLst><p:sldId id=\"256\" r:id=\"rId1\"/><p:sldId id=\"257\" r:id=\"rId2\"/><p:sldId id=\"258\" r:id=\"rId3\"/></p:sldIdLst><p:sldSz cx=\"12192000\" cy=\"6858000\"/><p:notesSz cx=\"6858000\" cy=\"9144000\"/>") + PptDefaultTextStyleXml() + "</p:presentation>"});
    entries.push_back({"ppt/_rels/presentation.xml.rels", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slide\" Target=\"slides/slide1.xml\"/><Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slide\" Target=\"slides/slide2.xml\"/><Relationship Id=\"rId3\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slide\" Target=\"slides/slide3.xml\"/><Relationship Id=\"rId4\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster\" Target=\"slideMasters/slideMaster1.xml\"/><Relationship Id=\"rId5\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/presProps\" Target=\"presProps.xml\"/><Relationship Id=\"rId6\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/viewProps\" Target=\"viewProps.xml\"/><Relationship Id=\"rId7\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/tableStyles\" Target=\"tableStyles.xml\"/><Relationship Id=\"rId8\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme\" Target=\"theme/theme1.xml\"/></Relationships>"});
    entries.push_back({"ppt/presProps.xml", PptPresPropsXml()});
    entries.push_back({"ppt/viewProps.xml", PptViewPropsXml()});
    entries.push_back({"ppt/tableStyles.xml", PptTableStylesXml()});
    entries.push_back({"ppt/theme/theme1.xml", PptThemeXml()});
    entries.push_back({"ppt/slideMasters/slideMaster1.xml", PptSlideMasterXml()});
    entries.push_back({"ppt/slideMasters/_rels/slideMaster1.xml.rels", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout\" Target=\"../slideLayouts/slideLayout1.xml\"/><Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme\" Target=\"../theme/theme1.xml\"/></Relationships>"});
    entries.push_back({"ppt/slideLayouts/slideLayout1.xml", PptSlideLayoutXml()});
    entries.push_back({"ppt/slideLayouts/_rels/slideLayout1.xml.rels", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster\" Target=\"../slideMasters/slideMaster1.xml\"/></Relationships>"});
    entries.push_back({"ppt/slides/slide1.xml", BuildPptSlide1(sheet.name, stats)});
    entries.push_back({"ppt/slides/slide2.xml", BuildPptSlide2(stats)});
    entries.push_back({"ppt/slides/slide3.xml", BuildPptSlide3(sheet.records)});
    entries.push_back({"ppt/slides/_rels/slide1.xml.rels", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout\" Target=\"../slideLayouts/slideLayout1.xml\"/></Relationships>"});
    entries.push_back({"ppt/slides/_rels/slide2.xml.rels", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout\" Target=\"../slideLayouts/slideLayout1.xml\"/></Relationships>"});
    entries.push_back({"ppt/slides/_rels/slide3.xml.rels", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout\" Target=\"../slideLayouts/slideLayout1.xml\"/></Relationships>"});
    return WriteZipStore(std::filesystem::path(path), entries);
}

void ExportPptx() {
    if (g_records.empty()) {
        ShowMessage(Tr(L"There are no records to export.", L"\u6ca1\u6709\u53ef\u5bfc\u51fa\u7684\u8bb0\u5f55\u3002"));
        return;
    }
    std::wstring path = SavePptxFileDialog();
    if (path.empty()) return;
    if (!ExportPptxFile(path)) {
        ShowMessage(Tr(L"Could not export the PowerPoint file.", L"\u65e0\u6cd5\u5bfc\u51fa PowerPoint \u6587\u4ef6\u3002"));
        return;
    }
    ShowMessage(Tr(L"PowerPoint exported successfully.", L"PowerPoint \u5bfc\u51fa\u6210\u529f\u3002"));
}

void ExportPrintHtml() {
    SyncActiveSheet();
    std::wstring path = SaveHtmlFileDialog();
    if (path.empty()) return;
    std::ofstream file(std::filesystem::path(path), std::ios::binary);
    if (!file) {
        ShowMessage(Tr(L"Could not export the print file.", L"\u65e0\u6cd5\u5bfc\u51fa\u6253\u5370\u6587\u4ef6\u3002"));
        return;
    }
    file << "\xEF\xBB\xBF";
    file << "<!doctype html><meta charset='utf-8'><title>" << HtmlCell(Tr(L"Attendance", L"\u51fa\u52e4\u7387")) << "</title>";
    file << "<style>body{font-family:Segoe UI,Arial;margin:32px}table{border-collapse:collapse;width:100%}"
            "th,td{border:1px solid #999;padding:8px;text-align:left}th{background:#eee}"
            "@media print{button{display:none}}</style>";
    file << "<button onclick='window.print()'>" << HtmlCell(Tr(L"Print / Save as PDF", L"\u6253\u5370 / \u53e6\u5b58\u4e3a PDF")) << "</button>";
    file << "<h1>" << HtmlCell(g_sheets[g_activeSheet].name) << "</h1>";
    if (!g_sheets[g_activeSheet].teacher.empty() || !g_sheets[g_activeSheet].location.empty() || !g_sheets[g_activeSheet].notes.empty()) {
        file << "<p>";
        if (!g_sheets[g_activeSheet].teacher.empty()) file << "<strong>" << HtmlCell(Tr(L"Teacher/owner:", L"\u6559\u5e08/\u8d1f\u8d23\u4eba\uff1a")) << "</strong> " << HtmlCell(g_sheets[g_activeSheet].teacher) << "<br>";
        if (!g_sheets[g_activeSheet].location.empty()) file << "<strong>" << HtmlCell(Tr(L"Location:", L"\u5730\u70b9\uff1a")) << "</strong> " << HtmlCell(g_sheets[g_activeSheet].location) << "<br>";
        if (!g_sheets[g_activeSheet].notes.empty()) file << "<strong>" << HtmlCell(Tr(L"Course notes:", L"\u8bfe\u7a0b\u5907\u6ce8\uff1a")) << "</strong> " << HtmlCell(g_sheets[g_activeSheet].notes);
        file << "</p>";
    }
    if (!g_sheets[g_activeSheet].students.empty()) {
        file << "<p><strong>" << HtmlCell(Tr(L"Manage students", L"\u7ba1\u7406\u5b66\u751f")) << ":</strong> " << g_sheets[g_activeSheet].students.size() << "</p>";
    }
    file << "<table><tr><th>" << HtmlCell(Tr(L"Date/Time", L"\u65e5\u671f\u65f6\u95f4")) << "</th><th>"
         << HtmlCell(Tr(L"Name", L"\u59d3\u540d")) << "</th><th>"
         << HtmlCell(Tr(L"Status", L"\u72b6\u6001")) << "</th><th>"
         << HtmlCell(Tr(L"Other", L"\u5176\u4ed6")) << "</th></tr>";
    for (const auto& r : g_records) {
        file << "<tr><td>" << HtmlCell(r.dateTime) << "</td><td>" << HtmlCell(r.name)
             << "</td><td>" << HtmlCell(r.status) << "</td><td>" << HtmlCell(r.other) << "</td></tr>";
    }
    file << "</table><script>setTimeout(()=>window.print(),300)</script>";
    ShellExecuteW(g_hwnd, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void ImportRosterCsv() {
    std::wstring path = OpenCsvFileDialog();
    if (path.empty()) return;
    std::ifstream file(std::filesystem::path(path), std::ios::binary);
    if (!file) {
        ShowMessage(Tr(L"Could not open the roster file.", L"\u65e0\u6cd5\u6253\u5f00\u540d\u5355\u6587\u4ef6\u3002"));
        return;
    }
    std::string line;
    std::vector<AttendanceRecord> imported;
    while (std::getline(file, line)) {
        if (line.rfind("\xEF\xBB\xBF", 0) == 0) line = line.substr(3);
        if (line.empty()) continue;
        auto cells = ParseCsvLine(line);
        std::wstring name;
        for (const auto& cell : cells) {
            std::string trimmed = TrimUtf8(cell);
            if (!trimmed.empty()) {
                name = Utf8ToWide(trimmed);
                break;
            }
        }
        if (name.empty() || LooksLikeNameHeader(name)) continue;
        imported.push_back({CurrentDateTimeText(), name, L"Absent", L""});
    }
    if (!imported.empty()) {
        PushUndo();
        MarkDirty();
        SyncActiveSheet();
        for (const auto& record : imported) {
            if (std::find(g_sheets[g_activeSheet].students.begin(), g_sheets[g_activeSheet].students.end(), record.name) == g_sheets[g_activeSheet].students.end()) {
                g_sheets[g_activeSheet].students.push_back(record.name);
            }
        }
        g_records.insert(g_records.end(), imported.begin(), imported.end());
        RefreshList();
    }
    std::wstringstream ss;
    ss << Tr(L"Imported", L"\u5df2\u5bfc\u5165") << L" " << imported.size() << L" " << Tr(L"students into the current course.", L"\u540d\u5b66\u751f\u5230\u5f53\u524d\u8bfe\u7a0b\u3002");
    if (!imported.empty()) {
        ss << L"\n" << Tr(L"Imported roster entries start as Absent until marked.", L"\u5bfc\u5165\u7684\u540d\u5355\u8bb0\u5f55\u5728\u70b9\u540d\u524d\u9ed8\u8ba4\u4e3a\u7f3a\u5e2d\u3002");
    }
    ShowMessage(ss.str());
}

void AddStudentToRoster() {
    SyncActiveSheet();
    std::wstring name;
    if (!PromptText(Tr(L"Add student to roster", L"\u6dfb\u52a0\u5b66\u751f\u5230\u540d\u5355"), Tr(L"Student name:", L"\u5b66\u751f\u59d3\u540d\uff1a"), name)) return;
    if (name.empty()) return;
    auto& students = g_sheets[g_activeSheet].students;
    if (std::find(students.begin(), students.end(), name) == students.end()) {
        PushUndo();
        MarkDirty();
        students.push_back(name);
        ShowMessage(Tr(L"Student added.", L"\u5b66\u751f\u5df2\u6dfb\u52a0\u3002"));
    }
}

void RemoveStudentFromRoster() {
    SyncActiveSheet();
    if (g_sheets[g_activeSheet].students.empty()) {
        ShowMessage(Tr(L"No students in the roster.", L"\u540d\u5355\u4e2d\u6ca1\u6709\u5b66\u751f\u3002"));
        return;
    }
    std::wstring name;
    if (!PromptText(Tr(L"Remove student from roster", L"\u4ece\u540d\u5355\u79fb\u9664\u5b66\u751f"), Tr(L"Student name:", L"\u5b66\u751f\u59d3\u540d\uff1a"), name)) return;
    auto& students = g_sheets[g_activeSheet].students;
    auto it = std::find(students.begin(), students.end(), name);
    if (it != students.end()) {
        PushUndo();
        MarkDirty();
        students.erase(it);
        ShowMessage(Tr(L"Student removed.", L"\u5b66\u751f\u5df2\u79fb\u9664\u3002"));
    } else {
        ShowMessage(Tr(L"No matching records found.", L"\u672a\u627e\u5230\u5339\u914d\u8bb0\u5f55\u3002"));
    }
}

void CreateRecordsFromRoster() {
    SyncActiveSheet();
    const auto& students = g_sheets[g_activeSheet].students;
    if (students.empty()) {
        ShowMessage(Tr(L"No students in the roster.", L"\u540d\u5355\u4e2d\u6ca1\u6709\u5b66\u751f\u3002"));
        return;
    }
    PushUndo();
    MarkDirty();
    std::wstring dateTime = GetText(g_dateEdit);
    if (dateTime.empty()) dateTime = CurrentDateTimeText();
    int added = 0;
    for (const auto& student : students) {
        auto duplicate = std::find_if(g_records.begin(), g_records.end(), [&](const AttendanceRecord& record) {
            return record.name == student && record.dateTime == dateTime;
        });
        if (duplicate == g_records.end()) {
            g_records.push_back({dateTime, student, L"Absent", L""});
            ++added;
        }
    }
    RefreshList();
    std::wstringstream ss;
    ss << Tr(L"Roster records created.", L"\u5df2\u4ece\u540d\u5355\u521b\u5efa\u70b9\u540d\u8bb0\u5f55\u3002") << L" (" << added << L")";
    ShowMessage(ss.str());
}

void EditCourseDetails() {
    SyncActiveSheet();
    auto& sheet = g_sheets[g_activeSheet];
    std::wstring teacher = sheet.teacher;
    std::wstring location = sheet.location;
    std::wstring notes = sheet.notes;
    if (!PromptText(Tr(L"Course details", L"\u8bfe\u7a0b\u8d44\u6599"), Tr(L"Teacher/owner:", L"\u6559\u5e08/\u8d1f\u8d23\u4eba\uff1a"), teacher)) return;
    if (!PromptText(Tr(L"Course details", L"\u8bfe\u7a0b\u8d44\u6599"), Tr(L"Location:", L"\u5730\u70b9\uff1a"), location)) return;
    if (!PromptText(Tr(L"Course details", L"\u8bfe\u7a0b\u8d44\u6599"), Tr(L"Course notes:", L"\u8bfe\u7a0b\u5907\u6ce8\uff1a"), notes)) return;
    PushUndo();
    MarkDirty();
    sheet.teacher = teacher;
    sheet.location = location;
    sheet.notes = notes;
}

void ShowStatisticsSummary() {
    SyncActiveSheet();
    int present = 0;
    int absent = 0;
    int late = 0;
    int other = 0;
    CountStatuses(present, absent, late, other);
    std::vector<std::pair<std::wstring, int>> studentIssues;
    for (const auto& record : g_records) {
        if (record.status != L"Absent" && record.status != L"Late") continue;
        auto it = std::find_if(studentIssues.begin(), studentIssues.end(), [&](const auto& item) { return item.first == record.name; });
        if (it == studentIssues.end()) studentIssues.push_back({record.name, 1});
        else ++it->second;
    }
    std::sort(studentIssues.begin(), studentIssues.end(), [](const auto& a, const auto& b) { return a.second > b.second; });

    std::wstringstream ss;
    ss << Tr(L"Statistics summary", L"\u7edf\u8ba1\u6458\u8981") << L"\n\n";
    ss << Tr(L"Courses", L"\u8bfe\u7a0b") << L": " << g_sheets[g_activeSheet].name << L"\n";
    ss << Tr(L"Total", L"\u603b\u6570") << L": " << g_records.size()
       << L"    " << Tr(L"Present", L"\u51fa\u5e2d") << L": " << present << L"\n";
    ss << Tr(L"Absent", L"\u7f3a\u5e2d") << L": " << absent << L"\n";
    ss << Tr(L"Late", L"\u8fdf\u5230") << L": " << late << L"\n";
    ss << Tr(L"Other", L"\u5176\u4ed6") << L": " << other << L"\n";
    if (!studentIssues.empty()) {
        ss << L"\n" << Tr(L"Top absent/late students", L"\u7f3a\u5e2d/\u8fdf\u5230\u6700\u591a\u5b66\u751f") << L":\n";
        for (int i = 0; i < (int)std::min<size_t>(5, studentIssues.size()); ++i) {
            ss << L"- " << studentIssues[i].first << L": " << studentIssues[i].second << L"\n";
        }
    }
    ShowMessage(ss.str(), Tr(L"Statistics summary", L"\u7edf\u8ba1\u6458\u8981"));
}

std::filesystem::path RecentFileRecordPath() {
    return AppDataFilePath(L"recent.txt");
}

void SaveRecentFilePath(const std::wstring& path) {
    auto recent = RecentFileRecordPath();
    if (recent.empty()) return;
    std::ofstream file(recent, std::ios::binary);
    if (file) file << WideToUtf8(path);
}

std::wstring LoadRecentFilePath() {
    auto recent = RecentFileRecordPath();
    if (recent.empty() || !std::filesystem::exists(recent)) return {};
    std::ifstream file(recent, std::ios::binary);
    std::string value((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return Utf8ToWide(value);
}

std::filesystem::path LatestBackupPath() {
    return AppDataFilePath(L"backup-latest.attd");
}

void BackupNow() {
    SyncActiveSheet();
    auto path = LatestBackupPath();
    if (path.empty()) return;
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        ShowMessage(Tr(L"Could not save the file.", L"\u65e0\u6cd5\u4fdd\u5b58\u6587\u4ef6\u3002"));
        return;
    }
    file << EncodeAttd(SerializeWorkbook());
    ShowMessage(Tr(L"Backup created:", L"\u5907\u4efd\u5df2\u521b\u5efa\uff1a") + L"\n" + path.wstring());
}

void RestoreLatestBackup() {
    auto path = LatestBackupPath();
    if (path.empty() || !std::filesystem::exists(path)) {
        ShowMessage(Tr(L"No backup file was found.", L"\u672a\u627e\u5230\u5907\u4efd\u6587\u4ef6\u3002"));
        return;
    }
    if (!ConfirmDiscardUnsaved(Tr(L"Restore latest backup", L"\u6062\u590d\u6700\u65b0\u5907\u4efd"))) return;
    LoadAttendanceFile(path.wstring(), true);
}

void OpenRecentFile() {
    std::wstring path = LoadRecentFilePath();
    if (path.empty() || !std::filesystem::exists(path)) {
        ShowMessage(Tr(L"No recent file was found.", L"\u672a\u627e\u5230\u6700\u8fd1\u6587\u4ef6\u3002"));
        return;
    }
    if (!ConfirmDiscardUnsaved(Tr(L"Open recent file", L"\u6253\u5f00\u6700\u8fd1\u6587\u4ef6"))) return;
    LoadAttendanceFile(path, true);
}

void SetDefaultSaveFolder() {
    std::wstring folder = ChooseFolderDialog();
    if (folder.empty()) return;
    g_defaultSaveDir = folder;
    SaveSettings();
    ShowMessage(Tr(L"Default save folder updated:", L"\u9ed8\u8ba4\u4fdd\u5b58\u6587\u4ef6\u5939\u5df2\u66f4\u65b0\uff1a") + L"\n" + folder);
}

void ExportDatabaseMirror() {
    SyncActiveSheet();
    auto path = AppDataFilePath(L"attendance.attddb");
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        ShowMessage(Tr(L"Could not write the database file.", L"\u65e0\u6cd5\u5199\u5165\u6570\u636e\u5e93\u6587\u4ef6\u3002"));
        return;
    }
    file << "\xEF\xBB\xBF";
    file << "course\tteacher\tlocation\tstudent_count\tdate_time\tname\tstatus\tother\n";
    for (const auto& sheet : g_sheets) {
        for (const auto& r : sheet.records) {
            file << TsvCell(sheet.name) << '\t' << TsvCell(sheet.teacher) << '\t' << TsvCell(sheet.location) << '\t' << sheet.students.size()
                 << '\t' << TsvCell(r.dateTime) << '\t' << TsvCell(r.name)
                 << '\t' << TsvCell(r.status) << '\t' << TsvCell(r.other) << '\n';
        }
    }
    std::wstring msg = Tr(L"Database mirror exported to:", L"\u6570\u636e\u5e93\u955c\u50cf\u5df2\u5bfc\u51fa\u5230\uff1a") + L"\n" + path.wstring();
    ShowMessage(msg);
}

void AutoSaveNow() {
    if (!g_allowAutosaveOverwrite) return;
    if (!g_dirty) return;
    SyncActiveSheet();
    auto path = AppDataFilePath(L"autosave.attd");
    if (path.empty()) return;
    std::ofstream file(path, std::ios::binary);
    if (!file) return;
    file << EncodeAttd(SerializeWorkbook());
}

void OpenAutosave() {
    auto path = AppDataFilePath(L"autosave.attd");
    if (path.empty() || !std::filesystem::exists(path)) {
        ShowMessage(Tr(L"No autosave file was found.", L"\u672a\u627e\u5230\u81ea\u52a8\u4fdd\u5b58\u6587\u4ef6\u3002"));
        return;
    }
    if (!ConfirmDiscardUnsaved(Tr(L"Open autosave", L"\u6253\u5f00\u81ea\u52a8\u4fdd\u5b58"))) return;
    LoadAttendanceFile(path.wstring(), true);
}

void PromptRestoreAutosave() {
    auto path = AppDataFilePath(L"autosave.attd");
    if (path.empty() || !std::filesystem::exists(path)) return;
    std::wstring restoreMsg = Tr(L"An autosaved attendance file was found. Restore it now?", L"\u627e\u5230\u81ea\u52a8\u4fdd\u5b58\u7684\u70b9\u540d\u6587\u4ef6\u3002\u662f\u5426\u73b0\u5728\u6062\u590d\uff1f");
    std::wstring restoreTitle = Tr(L"Restore Autosave", L"\u6062\u590d\u81ea\u52a8\u4fdd\u5b58");
    if (ThemedMessageBox(g_hwnd, restoreMsg, restoreTitle, true) == IDYES) {
        LoadAttendanceFile(path.wstring(), false);
    } else {
        g_allowAutosaveOverwrite = false;
    }
}

void ShowShortcuts() {
    std::wstring text =
        L"F11: " + Tr(L"Fullscreen", L"\u5168\u5c4f") + L"\n" +
        L"Ctrl+S: " + Tr(L"Save .attd", L"\u4fdd\u5b58 .attd") + L"\n" +
        L"Ctrl+O: " + Tr(L"Import .attd", L"\u5bfc\u5165 .attd") + L"\n" +
        L"Ctrl+Z  " + Tr(L"Undo", L"\u64a4\u9500") + L"\n" +
        L"Ctrl+Y  " + Tr(L"Redo", L"\u91cd\u505a") + L"\n" +
        Tr(L"Double-click row: Edit selected record", L"\u53cc\u51fb\u884c\uff1a\u7f16\u8f91\u9009\u4e2d\u8bb0\u5f55") + L"\n" +
        Tr(L"Ctrl/Shift click: Multi-select rows", L"Ctrl/Shift \u70b9\u51fb\uff1a\u591a\u9009\u884c");
    ShowMessage(text, Tr(L"Keyboard Shortcuts", L"\u5feb\u6377\u952e"));
}

void CountStatuses(int& present, int& absent, int& late, int& other) {
    present = absent = late = other = 0;
    for (const auto& r : g_records) {
        if (r.status == L"Present") ++present;
        else if (r.status == L"Absent") ++absent;
        else if (r.status == L"Late") ++late;
        else ++other;
    }
}

void PaintStatsChart(HWND hwnd, HDC hdc) {
    RECT rc{};
    GetClientRect(hwnd, &rc);
    FillRect(hdc, &rc, g_bgBrush);

    int present, absent, late, other;
    CountStatuses(present, absent, late, other);
    int values[] = {present, absent, late, other};
    const wchar_t* labels[] = {L"Present", L"Absent", L"Late", L"Other"};
    COLORREF colors[] = {RGB(126, 154, 142), RGB(170, 112, 112), RGB(164, 146, 104), RGB(126, 126, 126)};
    int maxValue = std::max(1, std::max(std::max(present, absent), std::max(late, other)));
    double reveal = GetAnimationValue(hwnd, AnimChannel::Reveal, 1.0);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, COLOR_TEXT);
    SelectObject(hdc, g_titleFont);
    std::wstring title = Tr(L"Statistics Chart", L"\u7edf\u8ba1\u56fe\u8868") + L" - " + g_sheets[g_activeSheet].name;
    TextOutW(hdc, 24, 20, title.c_str(), (int)title.size());

    SelectObject(hdc, g_font);
    int labelX = 34;
    int barX = 150;
    int y = 86;
    int barMaxW = std::max(120, (int)(rc.right - barX - 90));
    for (int i = 0; i < 4; ++i) {
        std::wstring label = Tr(labels[i], labels[i]);
        TextOutW(hdc, labelX, y + 6, label.c_str(), (int)label.size());
        RECT barBg{barX, y, barX + barMaxW, y + 30};
        HBRUSH bg = CreateSolidBrush(RGB(24, 24, 24));
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

LRESULT CALLBACK ChartProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        PaintStatsChart(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_APP_ANIMATION_TICK:
        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;
    case WM_APP_ANIMATION_DONE:
        DestroyWindow(hwnd);
        return 0;
    case WM_CLOSE:
        BeginAnimatedClose(hwnd);
        return 0;
    case WM_DESTROY:
        RemoveAnimationState(hwnd);
        g_chartWindow = nullptr;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void ShowStatsChart() {
    if (g_chartWindow && IsWindow(g_chartWindow)) {
        InvalidateRect(g_chartWindow, nullptr, TRUE);
        SetForegroundWindow(g_chartWindow);
        return;
    }

    HINSTANCE instance = GetModuleHandleW(nullptr);
    const wchar_t* className = L"AttendanceStatsChart";
    static bool registered = false;
    if (!registered) {
        WNDCLASSW wc{};
        wc.style = CS_DROPSHADOW;
        wc.lpfnWndProc = ChartProc;
        wc.hInstance = instance;
        wc.lpszClassName = className;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON));
        RegisterClassW(&wc);
        registered = true;
    }
    std::wstring chartTitle = Tr(L"Statistics Chart", L"\u7edf\u8ba1\u56fe\u8868");
    g_chartWindow = CreateWindowExW(
        0, className, chartTitle.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 760, 420,
        g_hwnd, nullptr, instance, nullptr
    );
    BOOL dark = TRUE;
    DwmSetWindowAttribute(g_chartWindow, 20, &dark, sizeof(dark));
    StartWindowReveal(g_chartWindow, 190);
    ShowWindow(g_chartWindow, SW_SHOW);
    UpdateWindow(g_chartWindow);
}

void ShowToolsMenu(HWND button) {
    std::wstring roster = Tr(L"Import student roster (CSV)", L"\u5bfc\u5165\u5b66\u751f\u540d\u5355 (CSV)");
    std::wstring print = Tr(L"Print / export PDF", L"\u6253\u5370 / \u5bfc\u51fa PDF");
    std::wstring pptx = Tr(L"Export PowerPoint (.pptx)", L"\u5bfc\u51fa PowerPoint (.pptx)");
    std::wstring chart = Tr(L"Statistics chart", L"\u7edf\u8ba1\u56fe\u8868");
    std::wstring summary = Tr(L"Statistics summary", L"\u7edf\u8ba1\u6458\u8981");
    std::wstring courseDetails = Tr(L"Course details", L"\u8bfe\u7a0b\u8d44\u6599");
    std::wstring addStudent = Tr(L"Add student to roster", L"\u6dfb\u52a0\u5b66\u751f\u5230\u540d\u5355");
    std::wstring removeStudent = Tr(L"Remove student from roster", L"\u4ece\u540d\u5355\u79fb\u9664\u5b66\u751f");
    std::wstring createFromRoster = Tr(L"Create records from roster", L"\u4ece\u540d\u5355\u521b\u5efa\u70b9\u540d\u8bb0\u5f55");
    std::wstring undo = Tr(L"Undo", L"\u64a4\u9500");
    std::wstring redo = Tr(L"Redo", L"\u91cd\u505a");
    std::wstring shortcuts = Tr(L"Keyboard shortcuts", L"\u5feb\u6377\u952e");
    std::wstring db = Tr(L"Export database mirror", L"\u5bfc\u51fa\u6570\u636e\u5e93\u955c\u50cf");
    std::wstring autosave = Tr(L"Open autosave", L"\u6253\u5f00\u81ea\u52a8\u4fdd\u5b58");
    std::wstring backup = Tr(L"Backup now", L"\u7acb\u5373\u5907\u4efd");
    std::wstring restore = Tr(L"Restore latest backup", L"\u6062\u590d\u6700\u65b0\u5907\u4efd");
    std::wstring recent = Tr(L"Open recent file", L"\u6253\u5f00\u6700\u8fd1\u6587\u4ef6");
    std::wstring saveDir = Tr(L"Set default save folder", L"\u8bbe\u7f6e\u9ed8\u8ba4\u4fdd\u5b58\u6587\u4ef6\u5939");
    int command = ShowThemedPopupMenu(button, {
        {IDM_IMPORT_ROSTER, roster, false},
        {IDM_STUDENTS_ADD, addStudent, false},
        {IDM_STUDENTS_REMOVE, removeStudent, false},
        {IDM_STUDENTS_GENERATE, createFromRoster, false},
        {IDM_COURSE_DETAILS, courseDetails, false},
        {0, L"", true},
        {IDM_PRINT_HTML, print, false},
        {IDM_EXPORT_PPTX, pptx, false},
        {IDM_STATS_CHART, chart, false},
        {IDM_STATS_SUMMARY, summary, false},
        {0, L"", true},
        {IDM_UNDO, undo, false},
        {IDM_REDO, redo, false},
        {IDM_SHORTCUTS, shortcuts, false},
        {0, L"", true},
        {IDM_BACKUP_NOW, backup, false},
        {IDM_RESTORE_BACKUP, restore, false},
        {IDM_OPEN_RECENT, recent, false},
        {IDM_SET_SAVE_DIR, saveDir, false},
        {0, L"", true},
        {IDM_EXPORT_DB, db, false},
        {IDM_OPEN_AUTOSAVE, autosave, false}
    });
    switch (command) {
    case IDM_IMPORT_ROSTER: ImportRosterCsv(); break;
    case IDM_STUDENTS_ADD: AddStudentToRoster(); break;
    case IDM_STUDENTS_REMOVE: RemoveStudentFromRoster(); break;
    case IDM_STUDENTS_GENERATE: CreateRecordsFromRoster(); break;
    case IDM_COURSE_DETAILS: EditCourseDetails(); break;
    case IDM_PRINT_HTML: ExportPrintHtml(); break;
    case IDM_EXPORT_PPTX: ExportPptx(); break;
    case IDM_STATS_CHART: ShowStatsChart(); break;
    case IDM_STATS_SUMMARY: ShowStatisticsSummary(); break;
    case IDM_UNDO: UndoLast(); break;
    case IDM_REDO: RedoLast(); break;
    case IDM_SHORTCUTS: ShowShortcuts(); break;
    case IDM_BACKUP_NOW: BackupNow(); break;
    case IDM_RESTORE_BACKUP: RestoreLatestBackup(); break;
    case IDM_OPEN_RECENT: OpenRecentFile(); break;
    case IDM_SET_SAVE_DIR: SetDefaultSaveFolder(); break;
    case IDM_EXPORT_DB: ExportDatabaseMirror(); break;
    case IDM_OPEN_AUTOSAVE: OpenAutosave(); break;
    }
}

void SaveAttendance() {
    std::wstring path = SaveFileDialog();
    if (path.empty()) return;

    std::ofstream file(std::filesystem::path(path), std::ios::binary);
    if (!file) {
        ShowMessage(Tr(L"Could not save the file.", L"\u65e0\u6cd5\u4fdd\u5b58\u6587\u4ef6\u3002"));
        return;
    }
    file << EncodeAttd(SerializeWorkbook());
    g_dirty = false;
    g_allowAutosaveOverwrite = true;
    SaveRecentFilePath(path);
    ShowMessage(Tr(L"Saved successfully.", L"\u4fdd\u5b58\u6210\u529f\u3002"));
}

bool LoadAttendanceFile(const std::wstring& path, bool showSuccess) {
    std::ifstream file(std::filesystem::path(path), std::ios::binary);
    if (!file) {
        ShowMessage(Tr(L"Could not open the file.", L"\u65e0\u6cd5\u6253\u5f00\u6587\u4ef6\u3002"));
        return false;
    }

    std::string fileText((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::string plainText;
    std::vector<AttendanceSheet> imported;
    if (!DecodeAttd(fileText, plainText) || !DeserializeWorkbook(plainText, imported)) {
        ShowMessage(Tr(L"This .attd file could not be decoded.", L"\u65e0\u6cd5\u89e3\u7801\u8be5 .attd \u6587\u4ef6\u3002"));
        return false;
    }

    g_sheets = std::move(imported);
    g_activeSheet = std::clamp(g_loadedActiveSheet, 0, (int)g_sheets.size() - 1);
    g_records = g_sheets[g_activeSheet].records;
    g_undoStack.clear();
    g_redoStack.clear();
    g_dirty = false;
    g_allowAutosaveOverwrite = true;
    SaveRecentFilePath(path);
    RefreshCourseCombo();
    RefreshList();
    if (showSuccess) ShowMessage(Tr(L"Imported successfully.", L"\u5bfc\u5165\u6210\u529f\u3002"));
    return true;
}

void ImportAttendance() {
    std::wstring path = OpenFileDialog();
    if (path.empty()) return;
    if (!ConfirmDiscardUnsaved(Tr(L"Import .attd", L"\u5bfc\u5165 .attd"))) return;
    LoadAttendanceFile(path, true);
}

HWND MakeControl(const wchar_t* cls, const wchar_t* text, DWORD style, int id) {
    DWORD finalStyle = WS_CHILD | WS_VISIBLE | style;
    if (lstrcmpiW(cls, L"EDIT") == 0) finalStyle |= WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOHSCROLL | ES_NOHIDESEL;
    HWND hwnd = CreateWindowExW(
        0, cls, text,
        finalStyle,
        0, 0, 100, 30,
        g_hwnd, (HMENU)(intptr_t)id, GetModuleHandleW(nullptr), nullptr
    );
    SendMessageW(hwnd, WM_SETFONT, (WPARAM)g_font, TRUE);
    if (lstrcmpiW(cls, L"STATIC") != 0) EnableInteractiveAnimation(hwnd);
    if (lstrcmpiW(cls, L"EDIT") == 0) EnableEditShortcuts(hwnd);
    if (lstrcmpiW(cls, L"COMBOBOX") == 0 && (style & CBS_OWNERDRAWFIXED)) {
        SendMessageW(hwnd, CB_SETITEMHEIGHT, (WPARAM)-1, 30);
        SendMessageW(hwnd, CB_SETITEMHEIGHT, 0, 30);
        SendMessageW(hwnd, CB_SETMINVISIBLE, 12, 0);
        EnableComboPaint(hwnd);
    }
    return hwnd;
}

HWND MakeButton(const wchar_t* text, int id) {
    return MakeControl(L"BUTTON", text, BS_PUSHBUTTON | BS_OWNERDRAW, id);
}

HWND MakeSettingsControl(HWND parent, const wchar_t* cls, const wchar_t* text, DWORD style, int id) {
    DWORD finalStyle = WS_CHILD | WS_VISIBLE | style;
    if (lstrcmpiW(cls, L"EDIT") == 0) finalStyle |= WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOHSCROLL | ES_NOHIDESEL;
    HWND hwnd = CreateWindowExW(
        0, cls, text,
        finalStyle,
        0, 0, 100, 30,
        parent, (HMENU)(intptr_t)id, GetModuleHandleW(nullptr), nullptr
    );
    SendMessageW(hwnd, WM_SETFONT, (WPARAM)g_font, TRUE);
    if (lstrcmpiW(cls, L"STATIC") != 0) EnableInteractiveAnimation(hwnd);
    if (lstrcmpiW(cls, L"EDIT") == 0) EnableEditShortcuts(hwnd);
    if (lstrcmpiW(cls, L"COMBOBOX") == 0 && (style & CBS_OWNERDRAWFIXED)) {
        SendMessageW(hwnd, CB_SETITEMHEIGHT, (WPARAM)-1, 30);
        SendMessageW(hwnd, CB_SETITEMHEIGHT, 0, 30);
        SendMessageW(hwnd, CB_SETMINVISIBLE, 12, 0);
        EnableComboPaint(hwnd);
    }
    return hwnd;
}

void FillSettingsCombos(HWND hwnd) {
    HWND language = GetDlgItem(hwnd, IDC_SETTINGS_LANGUAGE);
    HWND font = GetDlgItem(hwnd, IDC_SETTINGS_FONT);

    SendMessageW(language, CB_RESETCONTENT, 0, 0);
    for (int i = 0; i <= (int)UiLanguage::ChineseTraditionalHongKong; ++i) {
        SendMessageW(language, CB_ADDSTRING, 0, (LPARAM)LanguageName((UiLanguage)i));
    }
    SendMessageW(language, CB_SETCURSEL, (WPARAM)g_language, 0);

    SendMessageW(font, CB_RESETCONTENT, 0, 0);
    FillInstalledFontFamilies();
    int selected = 0;
    for (int i = 0; i < (int)g_availableFonts.size(); ++i) {
        SendMessageW(font, CB_ADDSTRING, 0, (LPARAM)g_availableFonts[i].c_str());
        if (g_fontFamily == g_availableFonts[i]) selected = i;
    }
    SendMessageW(font, CB_SETCURSEL, selected, 0);
    RedrawWindow(language, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
    RedrawWindow(font, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
}

void ApplySettingsLanguage(HWND hwnd) {
    std::wstring settingsTitle = Tr(L"Settings", L"\u8bbe\u7f6e");
    SetWindowTextW(hwnd, settingsTitle.c_str());
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_TITLE), Tr(L"Interface Settings", L"\u754c\u9762\u8bbe\u7f6e"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_LANG_LABEL), Tr(L"Language", L"\u8bed\u8a00"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_FONT_LABEL), Tr(L"Interface Font", L"\u754c\u9762\u5b57\u4f53"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_PARTICLES), Tr(L"Enable particle effects", L"\u542f\u7528\u7c92\u5b50\u7279\u6548"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_APPLY), Tr(L"Apply", L"\u5e94\u7528"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_CLOSE), Tr(L"Close", L"\u5173\u95ed"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_RESET), Tr(L"Reset All Settings", L"\u91cd\u7f6e\u6240\u6709\u8bbe\u7f6e"));
    SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_PARTICLES), BM_SETCHECK, g_particlesEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
    FillSettingsCombos(hwnd);
}

void ApplySettingsFromWindow(HWND hwnd) {
    int language = (int)SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_LANGUAGE), CB_GETCURSEL, 0, 0);
    int fontIndex = (int)SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_FONT), CB_GETCURSEL, 0, 0);

    if (language < 0 || language > (int)UiLanguage::ChineseTraditionalHongKong) language = 0;
    g_language = (UiLanguage)language;
    g_theme = UiTheme::Dark;

    if (fontIndex >= 0) {
        std::wstring fontName = GetComboSelectedText(GetDlgItem(hwnd, IDC_SETTINGS_FONT));
        if (!fontName.empty()) g_fontFamily = fontName;
    }
    ApplyVisualSettings();
    EnumChildWindows(hwnd, ApplyFontToChild, 0);
    SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_TITLE), WM_SETFONT, (WPARAM)g_titleFont, TRUE);
    ApplySettingsLanguage(hwnd);
    SaveSettings();
    InvalidateRect(hwnd, nullptr, TRUE);
}

void ResizeSettingsLayout(HWND hwnd) {
    int pad = 24;
    int labelW = 130;
    int rowH = 34;
    int x = pad;
    int y = 22;
    int comboX = x + labelW;
    int comboW = 230;

    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_TITLE), x, y, 360, 34, TRUE);
    y += 56;
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_LANG_LABEL), x, y + 6, labelW, rowH, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_LANGUAGE), comboX, y, comboW, 430, TRUE);
    y += 48;
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_FONT_LABEL), x, y + 6, labelW, rowH, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_FONT), comboX, y, comboW, 300, TRUE);
    y += 48;
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_PARTICLES), comboX, y, labelW + comboW, 30, TRUE);
    y += 46;
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_RESET), x, y, labelW + comboW, 36, TRUE);
    y += 48;
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_APPLY), comboX - 4, y, 110, 38, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_CLOSE), comboX + 120, y, 110, 38, TRUE);
}

LRESULT CALLBACK SettingsProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        MakeSettingsControl(hwnd, L"STATIC", L"", 0, IDC_SETTINGS_TITLE);
        MakeSettingsControl(hwnd, L"STATIC", L"", 0, IDC_SETTINGS_LANG_LABEL);
        MakeSettingsControl(hwnd, L"STATIC", L"", 0, IDC_SETTINGS_FONT_LABEL);
        MakeSettingsControl(hwnd, L"COMBOBOX", L"", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP, IDC_SETTINGS_LANGUAGE);
        MakeSettingsControl(hwnd, L"COMBOBOX", L"", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP, IDC_SETTINGS_FONT);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_OWNERDRAW, IDC_SETTINGS_PARTICLES);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_PUSHBUTTON | BS_OWNERDRAW | WS_TABSTOP, IDC_SETTINGS_RESET);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_PUSHBUTTON | BS_OWNERDRAW | WS_TABSTOP, IDC_SETTINGS_APPLY);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_PUSHBUTTON | BS_OWNERDRAW | WS_TABSTOP, IDC_SETTINGS_CLOSE);
        SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_TITLE), WM_SETFONT, (WPARAM)g_titleFont, TRUE);
        ApplySettingsLanguage(hwnd);
        ResizeSettingsLayout(hwnd);
        return 0;
    }
    case WM_SIZE:
        ResizeSettingsLayout(hwnd);
        return 0;
    case WM_COMMAND:
        if (HIWORD(wParam) == CBN_DROPDOWN) {
            StartAnimation((HWND)lParam, AnimChannel::ComboOpen, 1.0, 340);
            ApplyComboDropDownTheme((HWND)lParam);
            return 0;
        }
        if (HIWORD(wParam) == CBN_CLOSEUP) {
            StartAnimation((HWND)lParam, AnimChannel::ComboOpen, 0.0, 220);
            return 0;
        }
        if (LOWORD(wParam) == IDC_SETTINGS_FONT && HIWORD(wParam) == CBN_SELCHANGE) {
            CommitComboSelectionNow((HWND)lParam);
            ApplySettingsFromWindow(hwnd);
            return 0;
        }
        if (LOWORD(wParam) == IDC_SETTINGS_PARTICLES && HIWORD(wParam) == BN_CLICKED) {
            HWND toggle = GetDlgItem(hwnd, IDC_SETTINGS_PARTICLES);
            g_particlesEnabled = !g_particlesEnabled;
            SendMessageW(toggle, BM_SETCHECK, g_particlesEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
            SaveSettings();
            InvalidateRect(toggle, nullptr, FALSE);
            return 0;
        }
        if (LOWORD(wParam) == IDC_SETTINGS_APPLY) {
            ApplySettingsFromWindow(hwnd);
            return 0;
        }
        if (LOWORD(wParam) == IDC_SETTINGS_CLOSE) {
            BeginAnimatedClose(hwnd);
            return 0;
        }
        if (LOWORD(wParam) == IDC_SETTINGS_RESET) {
            std::wstring resetMsg = Tr(L"Reset all settings and delete the AppData configuration file?", L"\u91cd\u7f6e\u6240\u6709\u8bbe\u7f6e\u5e76\u5220\u9664 AppData \u914d\u7f6e\u6587\u4ef6\uff1f");
            std::wstring resetTitle = Tr(L"Reset Settings", L"\u91cd\u7f6e\u8bbe\u7f6e");
            if (ThemedMessageBox(hwnd, resetMsg, resetTitle, true) == IDYES) {
                ResetSettings();
                ApplyVisualSettings();
                EnumChildWindows(hwnd, ApplyFontToChild, 0);
                SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_TITLE), WM_SETFONT, (WPARAM)g_titleFont, TRUE);
                ApplySettingsLanguage(hwnd);
                InvalidateRect(hwnd, nullptr, TRUE);
            }
            return 0;
        }
        break;
    case WM_MEASUREITEM: {
        auto* measure = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
        if (measure->CtlType == ODT_COMBOBOX) {
            measure->itemHeight = 30;
            return TRUE;
        }
        break;
    }
    case WM_DRAWITEM: {
        auto* draw = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (DrawComboItem(draw)) return TRUE;
        if (DrawParticleToggleItem(draw)) return TRUE;
        if (DrawButtonItem(draw)) return TRUE;
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        PaintAppBackground(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        HWND control = (HWND)lParam;
        SetTextColor(hdc, GetDlgCtrlID(control) == IDC_SETTINGS_TITLE ? COLOR_TEXT : COLOR_MUTED);
        SetBkMode(hdc, TRANSPARENT);
        return (LRESULT)GetStockObject(HOLLOW_BRUSH);
    }
    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, COLOR_TEXT);
        SetBkMode(hdc, TRANSPARENT);
        return (LRESULT)g_bgBrush;
    }
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, COLOR_TEXT);
        SetBkColor(hdc, COLOR_INPUT);
        return (LRESULT)g_inputBrush;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_APP_ANIMATION_TICK:
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
        g_settingsWindow = nullptr;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void ShowSettingsWindow() {
    if (g_settingsWindow && IsWindow(g_settingsWindow)) {
        ShowWindow(g_settingsWindow, SW_SHOW);
        SetForegroundWindow(g_settingsWindow);
        return;
    }

    HINSTANCE instance = GetModuleHandleW(nullptr);
    const wchar_t* className = L"AttendanceSettingsWindow";
    static bool registered = false;
    if (!registered) {
        WNDCLASSW wc{};
        wc.style = CS_DROPSHADOW;
        wc.lpfnWndProc = SettingsProc;
        wc.hInstance = instance;
        wc.lpszClassName = className;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON));
        RegisterClassW(&wc);
        registered = true;
    }

    std::wstring settingsTitle = Tr(L"Settings", L"\u8bbe\u7f6e");
    g_settingsWindow = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        className,
        settingsTitle.c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 470, 370,
        g_hwnd, nullptr, instance, nullptr
    );
    BOOL dark = TRUE;
    DwmSetWindowAttribute(g_settingsWindow, 20, &dark, sizeof(dark));
    ApplyThemedControls(g_settingsWindow);
    ShowWindow(g_settingsWindow, SW_SHOW);
    UpdateWindow(g_settingsWindow);
}

void ToggleFullscreen(HWND hwnd) {
    g_fullscreen = !g_fullscreen;
    if (g_fullscreen) {
        g_previousStyle = GetWindowLongW(hwnd, GWL_STYLE);
        GetWindowPlacement(hwnd, &g_previousPlacement);

        MONITORINFO mi{sizeof(mi)};
        GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi);
        SetWindowLongW(hwnd, GWL_STYLE, g_previousStyle & ~WS_OVERLAPPEDWINDOW);
        SetWindowPos(hwnd, HWND_TOP,
            mi.rcMonitor.left, mi.rcMonitor.top,
            mi.rcMonitor.right - mi.rcMonitor.left,
            mi.rcMonitor.bottom - mi.rcMonitor.top,
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    } else {
        SetWindowLongW(hwnd, GWL_STYLE, g_previousStyle);
        SetWindowPlacement(hwnd, &g_previousPlacement);
        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

void ResizeLayout(HWND hwnd) {
    static bool inLayout = false;
    if (inLayout) return;
    inLayout = true;

    RECT rc{};
    GetClientRect(hwnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    if (w <= 0 || h <= 0) {
        inLayout = false;
        return;
    }

    SendMessageW(hwnd, WM_SETREDRAW, FALSE, 0);

    int layoutW = std::max(w, MIN_LAYOUT_W);
    int layoutH = std::max(h, MIN_LAYOUT_H);
    g_contentW = layoutW;
    g_contentH = layoutH;

    g_scrollX = std::clamp(g_scrollX, 0, std::max(0, g_contentW - w));
    g_scrollY = std::clamp(g_scrollY, 0, std::max(0, g_contentH - h));

    int pad = 28;
    auto move = [&](int id, int x, int y, int width, int height) {
        HWND child = GetDlgItem(hwnd, id);
        if (child) MoveWindow(child, x - g_scrollX, y - g_scrollY, width, height, FALSE);
    };
    auto moveHwnd = [&](HWND child, int x, int y, int width, int height) {
        if (child) MoveWindow(child, x - g_scrollX, y - g_scrollY, width, height, FALSE);
    };

    int titleY = 20;
    move(IDC_TITLE, pad, titleY, 430, 38);
    move(IDC_SUBTITLE, pad, titleY + 42, std::min(720, layoutW - pad * 2), 26);

    int courseW = 300;
    int courseBtnW = 124;
    int courseX = std::max(pad + 520, layoutW - pad - courseW - courseBtnW - 12);
    moveHwnd(g_courseCombo, courseX, titleY + 6, courseW, 430);
    move(IDC_COURSE_OPTIONS, courseX + courseW + 12, titleY + 6, courseBtnW, 38);

    int statsY = 104;
    int statsGap = 16;
    int statsH = 92;
    int statW = std::max(180, (layoutW - pad * 2 - statsGap * 3) / 4);
    int statX = pad;
    move(IDC_STAT_TOTAL, statX, statsY, statW, statsH);
    move(IDC_STAT_ATTENDANCE, statX + (statW + statsGap), statsY, statW, statsH);
    move(IDC_STAT_ISSUES, statX + (statW + statsGap) * 2, statsY, statW, statsH);
    move(IDC_STAT_VISIBLE, statX + (statW + statsGap) * 3, statsY, statW, statsH);

    int filterY = statsY + statsH + 18;
    int listX = pad;
    int listW = std::max(720, layoutW - pad * 2);
    int clearW = 136;
    int filterLabelW = 74;
    int filterW = std::min(460, std::max(240, listW / 3));
    move(IDC_FILTER_LABEL, listX, filterY + 8, filterLabelW, 24);
    moveHwnd(g_filterEdit, listX + filterLabelW, filterY, filterW, 36);
    move(IDC_CLEAR_FILTER, listX + filterLabelW + filterW + 12, filterY - 1, clearW, 38);

    int dockPrimaryH = 56;
    int drawerH = 40;
    int drawerY = std::max(filterY + 430, layoutH - 68);
    int dockY = drawerY - 72;
    int composerY = dockY - 66;
    int listY = filterY + 52;
    int listH = std::max(260, composerY - listY - 20);
    moveHwnd(g_list, listX, listY, listW, listH);

    int composerW = std::min(1060, layoutW - pad * 2);
    int composerX = pad + (layoutW - pad * 2 - composerW) / 2;
    int labelY = composerY - 18;
    int dateW = std::max(210, composerW / 4);
    int nameW = std::max(240, composerW / 4);
    int otherW = composerW - dateW - nameW - 32;
    move(2001, composerX, labelY, dateW, 18);
    moveHwnd(g_dateEdit, composerX, composerY, dateW, 38);
    move(2002, composerX + dateW + 16, labelY, nameW, 18);
    moveHwnd(g_nameEdit, composerX + dateW + 16, composerY, nameW, 38);
    move(2003, composerX + dateW + nameW + 32, labelY, otherW, 18);
    moveHwnd(g_otherEdit, composerX + dateW + nameW + 32, composerY, otherW, 38);

    int primaryGap = 12;
    int primaryW = std::clamp((layoutW - pad * 2 - primaryGap * 5) / 6, 128, 168);
    int primaryTotalW = primaryW * 6 + primaryGap * 5;
    int primaryX = pad + (layoutW - pad * 2 - primaryTotalW) / 2;
    move(IDC_PRESENT, primaryX, dockY, primaryW, dockPrimaryH);
    move(IDC_ABSENT, primaryX + (primaryW + primaryGap), dockY, primaryW, dockPrimaryH);
    move(IDC_LATE, primaryX + (primaryW + primaryGap) * 2, dockY, primaryW, dockPrimaryH);
    move(IDC_OTHER_STATUS, primaryX + (primaryW + primaryGap) * 3, dockY, primaryW, dockPrimaryH);
    move(IDC_SAVE, primaryX + (primaryW + primaryGap) * 4, dockY, primaryW, dockPrimaryH);
    move(IDC_EXPORT_CSV, primaryX + (primaryW + primaryGap) * 5, dockY, primaryW, dockPrimaryH);

    int drawerGap = 10;
    int drawerW = std::clamp((layoutW - pad * 2 - drawerGap * 7) / 8, 112, 148);
    int drawerTotalW = drawerW * 8 + drawerGap * 7;
    int drawerX = pad + (layoutW - pad * 2 - drawerTotalW) / 2;
    move(IDC_ADD_UPDATE, drawerX, drawerY, drawerW, drawerH);
    move(IDC_EDIT_SELECTED, drawerX + (drawerW + drawerGap), drawerY, drawerW, drawerH);
    move(IDC_ALL_PRESENT, drawerX + (drawerW + drawerGap) * 2, drawerY, drawerW, drawerH);
    move(IDC_NEW, drawerX + (drawerW + drawerGap) * 3, drawerY, drawerW, drawerH);
    move(IDC_IMPORT, drawerX + (drawerW + drawerGap) * 4, drawerY, drawerW, drawerH);
    move(IDC_DELETE, drawerX + (drawerW + drawerGap) * 5, drawerY, drawerW, drawerH);
    move(IDC_TOOLS, drawerX + (drawerW + drawerGap) * 6, drawerY, drawerW, drawerH);
    move(IDC_SETTINGS, drawerX + (drawerW + drawerGap) * 7, drawerY, drawerW, drawerH);

    move(IDC_HINT, pad, layoutH - 24, std::max(260, layoutW / 2), 22);
    int footerStatsW = std::min(720, std::max(420, layoutW - pad * 2));
    move(IDC_STATS, std::max(pad, layoutW - footerStatsW - 12), layoutH - 22, footerStatsW, 20);

    int widths[4] = {
        std::max(210, listW / 5),
        std::max(220, listW / 4),
        std::max(150, listW / 7),
        std::max(260, listW - (listW / 5) - (listW / 4) - (listW / 7) - 24)
    };
    for (int i = 0; i < 4; ++i) ListView_SetColumnWidth(g_list, i, widths[i]);

    SCROLLINFO si{};
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = std::max(0, g_contentW - 1);
    si.nPage = std::max(1, w);
    si.nPos = g_scrollX;
    SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);

    si.nMax = std::max(0, g_contentH - 1);
    si.nPage = std::max(1, h);
    si.nPos = g_scrollY;
    SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

    SendMessageW(hwnd, WM_SETREDRAW, TRUE, 0);
    UINT redrawFlags = RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN;
    if (!g_liveResizing) redrawFlags |= RDW_UPDATENOW;
    RedrawWindow(hwnd, nullptr, nullptr, redrawFlags);
    inLayout = false;
}

void ScrollMainWindow(HWND hwnd, int bar, int code, int wheelDelta) {
    RECT rc{};
    GetClientRect(hwnd, &rc);
    int viewport = bar == SB_VERT ? rc.bottom - rc.top : rc.right - rc.left;
    int content = bar == SB_VERT ? g_contentH : g_contentW;
    int current = bar == SB_VERT ? g_scrollY : g_scrollX;
    int maxPos = std::max(0, content - viewport);
    int next = current;

    switch (code) {
    case SB_LINEUP:
        next -= 36;
        break;
    case SB_LINEDOWN:
        next += 36;
        break;
    case SB_PAGEUP:
        next -= std::max(120, viewport - 80);
        break;
    case SB_PAGEDOWN:
        next += std::max(120, viewport - 80);
        break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION: {
        SCROLLINFO si{};
        si.cbSize = sizeof(si);
        si.fMask = SIF_TRACKPOS;
        GetScrollInfo(hwnd, bar, &si);
        next = si.nTrackPos;
        break;
    }
    case SB_TOP:
        next = 0;
        break;
    case SB_BOTTOM:
        next = maxPos;
        break;
    case SB_ENDSCROLL:
        return;
    default:
        if (wheelDelta != 0) {
            next += -wheelDelta / WHEEL_DELTA * 90;
            if (std::abs(wheelDelta) < WHEEL_DELTA) next += wheelDelta < 0 ? 90 : -90;
        }
        break;
    }

    next = std::clamp(next, 0, maxPos);
    if (next == current) return;

    if (bar == SB_VERT) g_scrollY = next;
    else g_scrollX = next;
    ResizeLayout(hwnd);
    InvalidateRect(hwnd, nullptr, TRUE);
}

void ApplyDarkMode(HWND hwnd) {
    BOOL value = TRUE;
    if (hwnd) DwmSetWindowAttribute(hwnd, 20, &value, sizeof(value));
    if (hwnd == g_hwnd) ApplyGlassTitleBar(hwnd);
    ApplyThemedControls(hwnd);
    if (g_list) {
        SetWindowTheme(g_list, L"DarkMode_Explorer", nullptr);
        HWND header = ListView_GetHeader(g_list);
        if (header) {
            SetWindowTheme(header, L"DarkMode_Explorer", nullptr);
            InvalidateRect(header, nullptr, TRUE);
        }
        ListView_SetBkColor(g_list, COLOR_PANEL);
        ListView_SetTextBkColor(g_list, COLOR_PANEL);
        ListView_SetTextColor(g_list, COLOR_TEXT);
        ListView_SetExtendedListViewStyle(g_list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);
    }
}

void InitListColumns() {
    const wchar_t* titles[] = {L"Date / Time", L"Name", L"Status", L"Other"};
    for (int i = 0; i < 4; ++i) {
        LVCOLUMNW col{};
        col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        col.pszText = const_cast<wchar_t*>(titles[i]);
        col.cx = 180;
        col.iSubItem = i;
        ListView_InsertColumn(g_list, i, &col);
    }
}

bool IsCoreParticleButton(int id) {
    switch (id) {
    case IDC_PRESENT:
    case IDC_ABSENT:
    case IDC_LATE:
    case IDC_OTHER_STATUS:
    case IDC_ADD_UPDATE:
    case IDC_ALL_PRESENT:
    case IDC_SAVE:
    case IDC_EXPORT_CSV:
    case IDC_IMPORT:
    case IDC_NEW:
    case IDC_CLEAR_FILTER:
    case IDC_COURSE_OPTIONS:
    case IDC_TOOLS:
    case IDC_SETTINGS:
        return true;
    default:
        return false;
    }
}

double RandomRange(double minValue, double maxValue) {
    std::uniform_real_distribution<double> dist(minValue, maxValue);
    return dist(g_effectRng);
}

int RandomInt(int minValue, int maxValue) {
    std::uniform_int_distribution<int> dist(minValue, maxValue);
    return dist(g_effectRng);
}

void TriggerButtonFeedback(HWND hwnd, POINT origin) {
    if (!hwnd) return;
    wchar_t className[32]{};
    GetClassNameW(hwnd, className, 32);
    if (lstrcmpiW(className, L"Button") != 0) return;
    if ((GetWindowLongPtrW(hwnd, GWL_STYLE) & BS_OWNERDRAW) == 0) return;

    int id = GetDlgCtrlID(hwnd);
    if (id == IDC_SETTINGS_PARTICLES) return;
    ButtonEffectState state;
    state.origin = origin;
    state.startMs = AnimationNowMs();

    if (g_particlesEnabled && IsCoreParticleButton(id)) {
        int count = RandomInt(30, 50);
        state.particles.reserve(count);
        for (int i = 0; i < count; ++i) {
            double angle = RandomRange(0.0, 6.28318530717958647692);
            double speed = RandomRange(95.0, 250.0);
            ButtonParticle particle;
            particle.x = (double)origin.x;
            particle.y = (double)origin.y;
            particle.vx = std::cos(angle) * speed;
            particle.vy = std::sin(angle) * speed - RandomRange(40.0, 110.0);
            particle.radius = RandomRange(1.0, 3.0);
            particle.lifetimeMs = (uint32_t)RandomInt(800, 1200);
            particle.star = RandomInt(0, 5) == 0;
            particle.color = RandomInt(0, 12) == 0 ? RGB(102, 204, 255) : RGB(255, 255, 255);
            state.particles.push_back(particle);
        }
    }

    g_buttonEffects[hwnd] = std::move(state);
    RestartAnimation(hwnd, AnimChannel::Effect, 1.0, 1250);
    RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
}

void DrawButtonEffects(const DRAWITEMSTRUCT* draw, const RECT& buttonRc, COLORREF baseFill) {
    auto it = g_buttonEffects.find(draw->hwndItem);
    if (it == g_buttonEffects.end()) return;

    uint64_t now = AnimationNowMs();
    ButtonEffectState& state = it->second;
    uint64_t elapsedMs = now > state.startMs ? now - state.startMs : 0;
    if (elapsedMs > 1300) {
        g_buttonEffects.erase(it);
        return;
    }

    HRGN clip = CreateRoundRectRgn(buttonRc.left, buttonRc.top, buttonRc.right + 1, buttonRc.bottom + 1, 6, 6);
    SelectClipRgn(draw->hDC, clip);

    double rippleProgress = std::clamp(elapsedMs / 400.0, 0.0, 1.0);
    if (rippleProgress < 1.0) {
        double eased = EaseOut(rippleProgress);
        int radius = (int)std::lround(80.0 * eased);
        double strength = 0.30 * (1.0 - rippleProgress);
        COLORREF rippleColor = BlendColor(baseFill, RGB(255, 255, 255), strength);
        HPEN ripplePen = CreatePen(PS_SOLID, 2, rippleColor);
        HGDIOBJ oldPen = SelectObject(draw->hDC, ripplePen);
        HGDIOBJ oldBrush = SelectObject(draw->hDC, GetStockObject(HOLLOW_BRUSH));
        Ellipse(draw->hDC, state.origin.x - radius, state.origin.y - radius,
            state.origin.x + radius, state.origin.y + radius);
        SelectObject(draw->hDC, oldBrush);
        SelectObject(draw->hDC, oldPen);
        DeleteObject(ripplePen);
    }

    for (const auto& particle : state.particles) {
        double progress = std::clamp(elapsedMs / (double)particle.lifetimeMs, 0.0, 1.0);
        if (progress >= 1.0) continue;
        double seconds = elapsedMs / 1000.0;
        double gravity = 260.0 * 0.3;
        int x = (int)std::lround(particle.x + particle.vx * seconds);
        int y = (int)std::lround(particle.y + particle.vy * seconds + 0.5 * gravity * seconds * seconds);
        double size = particle.radius * (1.0 + 0.5 * progress);
        int r = std::max(1, (int)std::lround(size));
        double opacity = std::pow(1.0 - progress, 1.35);
        COLORREF color = BlendColor(particle.color, RGB(136, 136, 136), progress);
        color = BlendColor(baseFill, color, opacity);

        HBRUSH brush = CreateSolidBrush(color);
        HPEN pen = CreatePen(PS_SOLID, 1, color);
        HGDIOBJ oldBrush = SelectObject(draw->hDC, brush);
        HGDIOBJ oldPen = SelectObject(draw->hDC, pen);
        if (particle.star) {
            POINT pts[8] = {
                {x, y - r * 2}, {x + r / 2, y - r / 2}, {x + r * 2, y},
                {x + r / 2, y + r / 2}, {x, y + r * 2}, {x - r / 2, y + r / 2},
                {x - r * 2, y}, {x - r / 2, y - r / 2}
            };
            Polygon(draw->hDC, pts, 8);
        } else {
            Ellipse(draw->hDC, x - r, y - r, x + r, y + r);
        }
        SelectObject(draw->hDC, oldPen);
        SelectObject(draw->hDC, oldBrush);
        DeleteObject(pen);
        DeleteObject(brush);
    }

    SelectClipRgn(draw->hDC, nullptr);
    DeleteObject(clip);
}

bool DrawParticleToggleItem(const DRAWITEMSTRUCT* draw) {
    if (!draw || draw->CtlType != ODT_BUTTON || draw->CtlID != IDC_SETTINGS_PARTICLES) return false;

    wchar_t text[160]{};
    GetWindowTextW(draw->hwndItem, text, 160);

    RECT rc = draw->rcItem;
    HBRUSH bg = CreateSolidBrush(COLOR_BG);
    FillRect(draw->hDC, &rc, bg);
    DeleteObject(bg);

    bool checked = g_particlesEnabled;
    double hover = GetAnimationValue(draw->hwndItem, AnimChannel::Hover, 0.0);
    COLORREF border = BlendColor(RGB(51, 51, 51), RGB(85, 85, 85), hover);

    int box = 18;
    int boxTop = (int)rc.top + std::max(0, ((int)(rc.bottom - rc.top) - box) / 2);
    RECT boxRc{
        rc.left + 2,
        boxTop,
        rc.left + 2 + box,
        boxTop + box
    };

    HBRUSH fill = CreateSolidBrush(RGB(0, 0, 0));
    HPEN borderPen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ oldBrush = SelectObject(draw->hDC, fill);
    HGDIOBJ oldPen = SelectObject(draw->hDC, borderPen);
    Rectangle(draw->hDC, boxRc.left, boxRc.top, boxRc.right, boxRc.bottom);
    SelectObject(draw->hDC, oldPen);
    SelectObject(draw->hDC, oldBrush);
    DeleteObject(borderPen);
    DeleteObject(fill);

    if (checked) {
        HPEN checkPen = CreatePen(PS_SOLID, 2, COLOR_TEXT);
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
    SetTextColor(draw->hDC, COLOR_TEXT);
    HGDIOBJ oldFont = SelectObject(draw->hDC, g_font);
    DrawTextW(draw->hDC, text, -1, &textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    SelectObject(draw->hDC, oldFont);
    return true;
}

bool DrawButtonItem(const DRAWITEMSTRUCT* draw) {
    if (!draw || draw->CtlType != ODT_BUTTON) return false;

    wchar_t text[128]{};
    GetWindowTextW(draw->hwndItem, text, 128);
    bool pressedState = (draw->itemState & ODS_SELECTED) != 0;
    bool focused = (draw->itemState & ODS_FOCUS) != 0;
    double hover = GetAnimationValue(draw->hwndItem, AnimChannel::Hover, focused ? 1.0 : 0.0);
    double press = std::max(pressedState ? 1.0 : 0.0, GetAnimationValue(draw->hwndItem, AnimChannel::Press, 0.0));

    COLORREF fill = BlendColor(COLOR_INPUT, RGB(38, 38, 38), hover);
    COLORREF border = BlendColor(RGB(64, 64, 64), COLOR_ACCENT, focused ? 1.0 : hover);
    COLORREF buttonText = BlendColor(COLOR_MUTED, COLOR_TEXT, std::clamp(0.45 + hover * 0.55, 0.0, 1.0));

    bool isDanger = draw->CtlID == IDC_DELETE || draw->CtlID == IDC_SETTINGS_RESET;
    bool isGreen = draw->CtlID == IDC_PRESENT || draw->CtlID == IDC_SAVE || draw->CtlID == IDC_ALL_PRESENT || draw->CtlID == IDC_SETTINGS_APPLY;
    bool isBlue = draw->CtlID == IDC_IMPORT || draw->CtlID == IDC_NEW || draw->CtlID == IDC_EXPORT_CSV;

    if (isDanger) {
        fill = BlendColor(RGB(36, 24, 24), RGB(58, 38, 38), hover);
        border = BlendColor(RGB(92, 64, 64), COLOR_DANGER, focused ? 1.0 : hover);
        buttonText = RGB(255, 238, 238);
    } else if (isGreen) {
        fill = BlendColor(RGB(24, 30, 28), RGB(38, 48, 44), hover);
        border = BlendColor(RGB(68, 78, 74), RGB(146, 166, 156), focused ? 1.0 : hover);
    } else if (isBlue) {
        fill = BlendColor(RGB(25, 25, 25), RGB(44, 44, 44), hover);
        border = BlendColor(RGB(72, 72, 72), COLOR_ACCENT, focused ? 1.0 : hover);
    }

    fill = BlendColor(fill, RGB(50, 50, 50), press * 0.45);

    HBRUSH outerBrush = CreateSolidBrush(COLOR_BG);
    FillRect(draw->hDC, &draw->rcItem, outerBrush);
    DeleteObject(outerBrush);

    RECT buttonRc = draw->rcItem;
    int inset = (int)std::lround(press * 2.0);
    InflateRect(&buttonRc, -inset, -inset);
    OffsetRect(&buttonRc, 0, (int)std::lround(press * 1.0));
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ oldPen = SelectObject(draw->hDC, pen);
    HGDIOBJ oldBrush = SelectObject(draw->hDC, brush);
    RoundRect(draw->hDC, buttonRc.left, buttonRc.top, buttonRc.right, buttonRc.bottom, 6, 6);
    SelectObject(draw->hDC, oldBrush);
    SelectObject(draw->hDC, oldPen);
    DeleteObject(pen);
    DeleteObject(brush);

    int r = std::max(buttonRc.left, buttonRc.right - 1);
    int b = std::max(buttonRc.top, buttonRc.bottom - 1);
    SetPixel(draw->hDC, buttonRc.left, buttonRc.top, border);
    SetPixel(draw->hDC, r, buttonRc.top, border);
    SetPixel(draw->hDC, buttonRc.left, b, border);
    SetPixel(draw->hDC, r, b, border);

    DrawButtonEffects(draw, buttonRc, fill);

    SetBkMode(draw->hDC, TRANSPARENT);
    SetTextColor(draw->hDC, buttonText);
    DrawTextW(draw->hDC, text, -1, &buttonRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    return true;
}

bool DrawComboItem(const DRAWITEMSTRUCT* draw) {
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
    COLORREF rowFill = selected ? RGB(42, 42, 42) : COLOR_INPUT;
    COLORREF fill = BlendColor(COLOR_BG, rowFill, itemProgress);
    COLORREF textColor = BlendColor(COLOR_MUTED, COLOR_TEXT, itemProgress);

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
    DrawTextW(draw->hDC, text.c_str(), -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    return true;
}

void PaintComboClosed(HWND hwnd, HDC hdc) {
    RECT rc{};
    GetClientRect(hwnd, &rc);
    bool enabled = IsWindowEnabled(hwnd) != FALSE;
    bool focused = GetFocus() == hwnd;
    double hover = GetAnimationValue(hwnd, AnimChannel::Hover, focused ? 1.0 : 0.0);
    double open = GetAnimationValue(hwnd, AnimChannel::ComboOpen, 0.0);

    double active = std::clamp(std::max(hover, open), 0.0, 1.0);
    COLORREF fill = enabled ? BlendColor(COLOR_INPUT, RGB(36, 36, 36), active) : RGB(18, 18, 18);
    COLORREF border = BlendColor(RGB(64, 64, 64), COLOR_ACCENT, focused ? 1.0 : active);
    COLORREF arrowFill = BlendColor(RGB(20, 20, 20), RGB(42, 42, 42), active);
    COLORREF textColor = enabled ? COLOR_TEXT : COLOR_MUTED;

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
    HGDIOBJ oldFont = SelectObject(hdc, g_font);
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

LRESULT CALLBACK ComboPaintProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR) {
    switch (msg) {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, ComboPaintProc, 5);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        PaintComboClosed(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case CB_SETCURSEL: {
        LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
        RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
        return result;
    }
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
    case WM_ENABLE:
    case WM_THEMECHANGED: {
        LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
        InvalidateRect(hwnd, nullptr, TRUE);
        return result;
    }
    case WM_APP_ANIMATION_TICK: {
        RedrawAnimatedControl(hwnd);
        COMBOBOXINFO info{};
        info.cbSize = sizeof(info);
        if (GetComboBoxInfo(hwnd, &info) && info.hwndList) {
            RedrawWindow(info.hwndList, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
        }
        return 0;
    }
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void ApplyComboDropDownTheme(HWND combo) {
    if (!combo) return;
    COMBOBOXINFO info{};
    info.cbSize = sizeof(info);
    if (!GetComboBoxInfo(combo, &info)) return;

    const wchar_t* themeName = L"DarkMode_Explorer";
    SetWindowTheme(combo, themeName, nullptr);
    if (info.hwndItem) SetWindowTheme(info.hwndItem, themeName, nullptr);
    if (info.hwndList) {
        SetWindowTheme(info.hwndList, themeName, nullptr);
        InvalidateRect(info.hwndList, nullptr, TRUE);
        RedrawWindow(info.hwndList, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME);
    }
}

void EnableComboPaint(HWND combo) {
    if (!combo) return;
    SetWindowSubclass(combo, ComboPaintProc, 5, 0);
    ApplyComboDropDownTheme(combo);
}

void PaintHeaderControl(HWND hwnd, HDC hdc) {
    RECT full{};
    GetClientRect(hwnd, &full);
    COLORREF fill = RGB(18, 18, 18);
    COLORREF border = RGB(68, 68, 68);
    HBRUSH bg = CreateSolidBrush(fill);
    FillRect(hdc, &full, bg);
    DeleteObject(bg);

    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, COLOR_TEXT);
    SelectObject(hdc, g_font);

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

    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

LRESULT CALLBACK HeaderPaintProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR) {
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

void EnableHeaderPaint(HWND header) {
    if (header) SetWindowSubclass(header, HeaderPaintProc, 3, 0);
}

LRESULT CALLBACK StatsPaintProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR) {
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
            HBRUSH outer = CreateSolidBrush(COLOR_BG);
            FillRect(hdc, &rc, outer);
            DeleteObject(outer);

            RECT cardRc = rc;
            InflateRect(&cardRc, -1, -1);
            HBRUSH fill = CreateSolidBrush(RGB(16, 16, 16));
            HPEN pen = CreatePen(PS_SOLID, 1, RGB(16, 16, 16));
            HGDIOBJ oldBrush = SelectObject(hdc, fill);
            HGDIOBJ oldPen = SelectObject(hdc, pen);
            RoundRect(hdc, cardRc.left, cardRc.top, cardRc.right, cardRc.bottom, 8, 8);
            SelectObject(hdc, oldPen);
            SelectObject(hdc, oldBrush);
            DeleteObject(pen);
            DeleteObject(fill);

            for (int glow = 0; glow < 8; ++glow) {
                double amount = 0.050 * (1.0 - glow / 8.0);
                COLORREF glowColor = BlendColor(RGB(16, 16, 16), RGB(255, 255, 255), amount);
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

            HGDIOBJ oldFont = SelectObject(hdc, g_smallFont ? g_smallFont : g_font);
            SetBkMode(hdc, TRANSPARENT);
            RECT labelRc = cardRc;
            labelRc.left += 18;
            labelRc.top += 14;
            labelRc.right -= 18;
            labelRc.bottom = labelRc.top + 24;
            SetTextColor(hdc, COLOR_MUTED);
            DrawTextW(hdc, label.c_str(), -1, &labelRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

            RECT valueRc = cardRc;
            valueRc.left += 18;
            valueRc.top += 40;
            valueRc.right -= 18;
            valueRc.bottom -= 10;
            SetTextColor(hdc, COLOR_TEXT);
            DrawTextW(hdc, value.c_str(), -1, &valueRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
            SelectObject(hdc, oldFont);
            EndPaint(hwnd, &ps);
            return 0;
        }

        PaintAppBackgroundSlice(hwnd, hdc);
        rc.left += 2;
        rc.right -= 4;

        HGDIOBJ oldFont = SelectObject(hdc, g_smallFont ? g_smallFont : g_font);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, COLOR_ACCENT);
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

void EnableStatsPaint(HWND stats) {
    if (stats) SetWindowSubclass(stats, StatsPaintProc, 4, 0);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_hwnd = hwnd;
        ApplyThemePalette();
        ApplyGlassTitleBar(hwnd);
        RecreateFonts();

        HWND title = MakeControl(L"STATIC", L"Attendance Manager", 0, IDC_TITLE);
        HWND subtitle = MakeControl(L"STATIC", L"Create, edit, export, save, import, and batch clean .attd roll calls.", 0, IDC_SUBTITLE);
        HWND stats = MakeControl(L"STATIC", L"Total 0    Present 0    Absent 0    Late 0    Other 0", SS_LEFTNOWORDWRAP | SS_ENDELLIPSIS, IDC_STATS);
        HWND hint = MakeControl(L"STATIC", L"Tip: double-click a row to edit. Ctrl/Shift supports multi-select.", 0, IDC_HINT);
        HWND statTotal = MakeControl(L"STATIC", L"Total\n0", SS_LEFT | SS_NOPREFIX, IDC_STAT_TOTAL);
        HWND statAttendance = MakeControl(L"STATIC", L"Attendance\n0%", SS_LEFT | SS_NOPREFIX, IDC_STAT_ATTENDANCE);
        HWND statIssues = MakeControl(L"STATIC", L"Absent/Late\n0%", SS_LEFT | SS_NOPREFIX, IDC_STAT_ISSUES);
        HWND statVisible = MakeControl(L"STATIC", L"Showing\n0 / 0", SS_LEFT | SS_NOPREFIX, IDC_STAT_VISIBLE);
        SendMessageW(title, WM_SETFONT, (WPARAM)g_titleFont, TRUE);
        SendMessageW(subtitle, WM_SETFONT, (WPARAM)g_smallFont, TRUE);
        SendMessageW(stats, WM_SETFONT, (WPARAM)g_smallFont, TRUE);
        SendMessageW(hint, WM_SETFONT, (WPARAM)g_smallFont, TRUE);
        SendMessageW(statTotal, WM_SETFONT, (WPARAM)g_smallFont, TRUE);
        SendMessageW(statAttendance, WM_SETFONT, (WPARAM)g_smallFont, TRUE);
        SendMessageW(statIssues, WM_SETFONT, (WPARAM)g_smallFont, TRUE);
        SendMessageW(statVisible, WM_SETFONT, (WPARAM)g_smallFont, TRUE);
        EnableStatsPaint(stats);
        EnableStatsPaint(statTotal);
        EnableStatsPaint(statAttendance);
        EnableStatsPaint(statIssues);
        EnableStatsPaint(statVisible);
        g_courseCombo = MakeControl(L"COMBOBOX", L"", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP, IDC_COURSE_COMBO);
        MakeButton(L"Courses", IDC_COURSE_OPTIONS);
        MakeControl(L"STATIC", L"Date/Time", 0, 2001);
        MakeControl(L"STATIC", L"Name", 0, 2002);
        MakeControl(L"STATIC", L"Other", 0, 2003);
        MakeControl(L"STATIC", L"Search", 0, IDC_FILTER_LABEL);
        g_dateEdit = MakeControl(L"EDIT", CurrentDateTimeText().c_str(), WS_TABSTOP | ES_AUTOHSCROLL, IDC_DATE);
        g_nameEdit = MakeControl(L"EDIT", L"", WS_TABSTOP | ES_AUTOHSCROLL, IDC_NAME);
        g_otherEdit = MakeControl(L"EDIT", L"", WS_TABSTOP | ES_AUTOHSCROLL, IDC_OTHER);
        g_filterEdit = MakeControl(L"EDIT", L"", WS_TABSTOP | ES_AUTOHSCROLL, IDC_FILTER);

        MakeButton(L"Present", IDC_PRESENT);
        MakeButton(L"Absent", IDC_ABSENT);
        MakeButton(L"Late", IDC_LATE);
        MakeButton(L"Other", IDC_OTHER_STATUS);
        MakeButton(L"Update Selected", IDC_ADD_UPDATE);
        MakeButton(L"Edit Selected", IDC_EDIT_SELECTED);
        MakeButton(L"Mark All Present", IDC_ALL_PRESENT);
        MakeButton(L"Create New Attendance", IDC_NEW);
        MakeButton(L"Delete Options", IDC_DELETE);
        MakeButton(L"Save .attd", IDC_SAVE);
        MakeButton(L"Import .attd", IDC_IMPORT);
        MakeButton(L"Export CSV", IDC_EXPORT_CSV);
        MakeButton(L"Tools", IDC_TOOLS);
        MakeButton(L"Settings", IDC_SETTINGS);
        MakeButton(L"Clear Filter", IDC_CLEAR_FILTER);

        g_list = MakeControl(WC_LISTVIEWW, L"", LVS_REPORT | WS_TABSTOP | WS_BORDER, IDC_LIST);
        EnableMouseWheelForward(g_list);
        InitListColumns();
        EnableHeaderPaint(ListView_GetHeader(g_list));
        EnsureSheets();
        RefreshCourseCombo();
        ApplyMainLanguage();
        ResizeLayout(hwnd);
        ApplyDarkMode(hwnd);
        InvalidateRect(hwnd, nullptr, TRUE);
        SetTimer(hwnd, 1, 30000, nullptr);
        return 0;
    }
    case WM_SIZE:
        ResizeLayout(hwnd);
        return 0;
    case WM_ENTERSIZEMOVE:
        g_liveResizing = true;
        return 0;
    case WM_EXITSIZEMOVE:
        g_liveResizing = false;
        ResizeLayout(hwnd);
        InvalidateRect(hwnd, nullptr, FALSE);
        UpdateWindow(hwnd);
        return 0;
    case WM_LBUTTONDOWN:
        CancelMainWindowClose();
        break;
    case WM_VSCROLL:
        ScrollMainWindow(hwnd, SB_VERT, LOWORD(wParam), 0);
        return 0;
    case WM_HSCROLL:
        ScrollMainWindow(hwnd, SB_HORZ, LOWORD(wParam), 0);
        return 0;
    case WM_MOUSEWHEEL:
        ScrollMainWindow(hwnd, SB_VERT, 0, GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;
    case WM_MOUSEHWHEEL:
        ScrollMainWindow(hwnd, SB_HORZ, 0, GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;
    case WM_COMMAND: {
        if (HIWORD(wParam) == CBN_DROPDOWN) {
            StartAnimation((HWND)lParam, AnimChannel::ComboOpen, 1.0, 340);
            ApplyComboDropDownTheme((HWND)lParam);
            return 0;
        }
        if (HIWORD(wParam) == CBN_CLOSEUP) {
            StartAnimation((HWND)lParam, AnimChannel::ComboOpen, 0.0, 220);
            return 0;
        }
        if (LOWORD(wParam) == IDC_COURSE_COMBO && HIWORD(wParam) == CBN_SELCHANGE) {
            CommitComboSelectionNow((HWND)lParam);
            int index = (int)SendMessageW(g_courseCombo, CB_GETCURSEL, 0, 0);
            SwitchCourse(index);
            return 0;
        }
        if (LOWORD(wParam) == IDC_FILTER && HIWORD(wParam) == EN_CHANGE) {
            g_filterText = LowerText(GetText(g_filterEdit));
            RefreshList();
            return 0;
        }
        switch (LOWORD(wParam)) {
        case IDC_PRESENT: AddOrUpdateRecord(L"Present"); return 0;
        case IDC_ABSENT: AddOrUpdateRecord(L"Absent"); return 0;
        case IDC_LATE: AddOrUpdateRecord(L"Late"); return 0;
        case IDC_OTHER_STATUS: AddOrUpdateRecord(L"Other"); return 0;
        case IDC_ADD_UPDATE: UpdateSelectedRecord(); return 0;
        case IDC_EDIT_SELECTED: LoadRecordIntoEditor(ListView_GetNextItem(g_list, -1, LVNI_SELECTED)); return 0;
        case IDC_ALL_PRESENT: MarkAllPresent(); return 0;
        case IDC_EXPORT_CSV: ExportCsv(); return 0;
        case IDC_SETTINGS: ShowSettingsWindow(); return 0;
        case IDC_TOOLS: ShowToolsMenu(GetDlgItem(hwnd, IDC_TOOLS)); return 0;
        case IDC_COURSE_OPTIONS: ShowCourseMenu(GetDlgItem(hwnd, IDC_COURSE_OPTIONS)); return 0;
        case IDC_SAVE: SaveAttendance(); return 0;
        case IDC_IMPORT: ImportAttendance(); return 0;
        case IDC_CLEAR_FILTER:
            SetText(g_filterEdit, L"");
            g_filterText.clear();
            RefreshList();
            return 0;
        case IDC_NEW:
            {
            SyncActiveSheet();
            std::wstring sheetName = Tr(L"Attendance Sheet", L"\u70b9\u540d\u8868") + L" " + std::to_wstring(g_sheets.size() + 1);
            std::wstring newTitle = Tr(L"New Attendance", L"\u65b0\u5efa\u70b9\u540d");
            if (!PromptText(newTitle, Tr(L"New attendance sheet name:", L"\u65b0\u70b9\u540d\u8868\u540d\u79f0\uff1a"), sheetName)) return 0;
            if (sheetName.empty()) return 0;
            PushUndo();
            MarkDirty();
            g_sheets.push_back(MakeSheet(sheetName));
            g_activeSheet = (int)g_sheets.size() - 1;
            g_records.clear();
            RefreshCourseCombo();
            RefreshList();
            SetText(g_dateEdit, CurrentDateTimeText());
            SetText(g_nameEdit, L"");
            SetText(g_otherEdit, L"");
            }
            return 0;
        case IDC_DELETE: {
            ShowDeleteMenu(GetDlgItem(hwnd, IDC_DELETE));
            return 0;
        }
        }
        break;
    }
    case WM_KEYDOWN:
        CancelMainWindowClose();
        if ((GetKeyState(VK_CONTROL) & 0x8000) && wParam == 'S') {
            SaveAttendance();
            return 0;
        }
        if ((GetKeyState(VK_CONTROL) & 0x8000) && wParam == 'O') {
            ImportAttendance();
            return 0;
        }
        if ((GetKeyState(VK_CONTROL) & 0x8000) && wParam == 'Z') {
            UndoLast();
            return 0;
        }
        if ((GetKeyState(VK_CONTROL) & 0x8000) && wParam == 'Y') {
            RedoLast();
            return 0;
        }
        if (wParam == VK_F11) {
            ToggleFullscreen(hwnd);
            return 0;
        }
        break;
    case WM_TIMER:
        AutoSaveNow();
        return 0;
    case WM_MEASUREITEM: {
        auto* measure = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
        if (measure->CtlType == ODT_COMBOBOX) {
            measure->itemHeight = 30;
            return TRUE;
        }
        break;
    }
    case WM_DRAWITEM: {
        auto* draw = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (DrawComboItem(draw)) return TRUE;
        if (DrawButtonItem(draw)) return TRUE;
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        PaintAppBackground(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_NOTIFY: {
        auto* hdr = reinterpret_cast<NMHDR*>(lParam);
        if (hdr->idFrom == IDC_LIST && hdr->code == NM_CUSTOMDRAW) {
            return HandleListCustomDraw(lParam);
        }
        if (hdr->idFrom == IDC_LIST && hdr->code == LVN_ITEMCHANGED) {
            auto* item = reinterpret_cast<NMLISTVIEW*>(lParam);
            int recordIndex = VisibleToRecordIndex(item->iItem);
            if ((item->uNewState & LVIS_SELECTED) && recordIndex >= 0) {
                const auto& record = g_records[recordIndex];
                SetText(g_dateEdit, record.dateTime);
                SetText(g_nameEdit, record.name);
                SetText(g_otherEdit, record.other);
            }
        } else if (hdr->idFrom == IDC_LIST && hdr->code == NM_DBLCLK) {
            auto* item = reinterpret_cast<NMITEMACTIVATE*>(lParam);
            if (item->iItem >= 0) LoadRecordIntoEditor(item->iItem);
        }
        break;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        HWND control = (HWND)lParam;
        if (GetDlgCtrlID(control) == IDC_TITLE) SetTextColor(hdc, COLOR_TEXT);
        else if (GetDlgCtrlID(control) == IDC_STATS) {
            SetTextColor(hdc, COLOR_ACCENT);
        }
        else SetTextColor(hdc, COLOR_MUTED);
        SetBkMode(hdc, TRANSPARENT);
        return (LRESULT)GetStockObject(HOLLOW_BRUSH);
    }
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, COLOR_TEXT);
        SetBkColor(hdc, COLOR_INPUT);
        return (LRESULT)g_inputBrush;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_APP_ANIMATION_TICK:
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_APP_ANIMATION_DONE:
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        AutoSaveNow();
        RemoveAnimationState(hwnd);
        StopAnimationThread();
        if (g_font) DeleteObject(g_font);
        if (g_titleFont) DeleteObject(g_titleFont);
        if (g_smallFont) DeleteObject(g_smallFont);
        if (g_bgBrush) DeleteObject(g_bgBrush);
        if (g_inputBrush) DeleteObject(g_inputBrush);
        if (g_panelBrush) DeleteObject(g_panelBrush);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR commandLine, int showCmd) {
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

    HWND hwnd = CreateWindowExW(
        0,
        className,
        L"AttendanceApp - .attd Roll Call Manager",
        WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_HSCROLL,
        CW_USEDEFAULT, CW_USEDEFAULT, 1600, 900,
        nullptr, nullptr, instance, nullptr
    );

    if (!hwnd) return 1;
    SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON)));
    SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON)));
    ApplyGlassTitleBar(hwnd);
    ShowWindow(hwnd, showCmd);
    UpdateWindow(hwnd);

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(commandLine, &argc);
    bool loadedFromArg = false;
    if (argv && argc > 0 && std::filesystem::path(argv[0]).extension() == L".attd") {
        LoadAttendanceFile(argv[0], false);
        loadedFromArg = true;
    }
    if (argv) LocalFree(argv);
    if (!loadedFromArg) PromptRestoreAutosave();

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}
