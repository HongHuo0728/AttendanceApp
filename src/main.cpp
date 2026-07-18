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
static constexpr int IDM_LESSON_CREATE_TODAY = 3038;
static constexpr int IDM_LESSON_SWITCH = 3039;
static constexpr int IDM_STUDENT_PROFILE = 3040;
static constexpr int IDM_ADVANCED_FILTER = 3041;
static constexpr int IDM_STATS_RANGE = 3042;
static constexpr int IDM_RISK_STUDENTS = 3043;
static constexpr int IDM_BACKUP_MANAGER = 3044;
static constexpr int IDM_REPORT_TEMPLATE = 3045;
static constexpr int IDM_COMMAND_PALETTE = 3046;
static constexpr int IDM_SHORTCUT_CENTER = 3047;
static constexpr int IDM_LESSON_COMPLETION = 3048;
static constexpr int IDM_QUICK_ROLL_CALL = 3049;

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
static constexpr int IDC_SETTINGS_ANIMATION_LABEL = 4012;
static constexpr int IDC_SETTINGS_ANIMATION_LEVEL = 4013;
static constexpr int IDC_SETTINGS_PARTICLE_LABEL = 4014;
static constexpr int IDC_SETTINGS_PARTICLE_LEVEL = 4015;
static constexpr int IDC_SETTINGS_RISK_ALERTS = 4016;
static constexpr int IDC_SETTINGS_AUTOSAVE_PROMPT = 4017;
static constexpr int IDC_SETTINGS_COMMAND_PALETTE = 4018;
static constexpr int IDC_SETTINGS_REPORT_LABEL = 4019;
static constexpr int IDC_SETTINGS_REPORT_TEMPLATE = 4020;
static constexpr int IDC_SETTINGS_ADVANCED_FILTER = 4021;
static constexpr int IDC_SETTINGS_EXPERIMENTAL = 4022;
static constexpr int IDC_SETTINGS_ADVANCED_TITLE = 4023;
static constexpr int IDC_SETTINGS_ACCENT_LABEL = 4024;
static constexpr int IDC_SETTINGS_ACCENT = 4025;
static constexpr int IDC_SETTINGS_TEXT_SCALE_LABEL = 4026;
static constexpr int IDC_SETTINGS_TEXT_SCALE = 4027;
static constexpr int IDC_SETTINGS_DENSITY_LABEL = 4028;
static constexpr int IDC_SETTINGS_DENSITY = 4029;
static constexpr int IDC_SETTINGS_SHOW_STATS = 4030;
static constexpr int IDC_SETTINGS_SHOW_HINT = 4031;
static constexpr int IDC_SETTINGS_TABLE_GRID = 4032;
static constexpr int IDC_SETTINGS_AUTOSAVE_LABEL = 4033;
static constexpr int IDC_SETTINGS_AUTOSAVE_INTERVAL = 4034;
static constexpr int IDC_INPUT_EDIT = 5001;
static constexpr int IDC_INPUT_OK = 5002;
static constexpr int IDC_INPUT_CANCEL = 5003;
static constexpr int IDC_MESSAGE_TEXT = 6001;
static constexpr int IDC_MESSAGE_YES = 6002;
static constexpr int IDC_MESSAGE_NO = 6003;
static constexpr int IDC_MESSAGE_OK = 6004;
static constexpr int IDC_QUICK_TITLE = 7001;
static constexpr int IDC_QUICK_COURSE = 7002;
static constexpr int IDC_QUICK_LESSON = 7003;
static constexpr int IDC_QUICK_PROGRESS = 7004;
static constexpr int IDC_QUICK_STUDENT = 7005;
static constexpr int IDC_QUICK_STATUS = 7006;
static constexpr int IDC_QUICK_NOTES_LABEL = 7007;
static constexpr int IDC_QUICK_NOTES = 7008;
static constexpr int IDC_QUICK_PRESENT = 7009;
static constexpr int IDC_QUICK_ABSENT = 7010;
static constexpr int IDC_QUICK_LATE = 7011;
static constexpr int IDC_QUICK_OTHER = 7012;
static constexpr int IDC_QUICK_PREVIOUS = 7013;
static constexpr int IDC_QUICK_SKIP = 7014;
static constexpr int IDC_QUICK_FINISH = 7015;
static constexpr int IDC_QUICK_CANCEL = 7016;
static constexpr int IDC_QUICK_HINT = 7017;

static HWND g_hwnd = nullptr;
static HWND g_dateEdit = nullptr;
static HWND g_nameEdit = nullptr;
static HWND g_otherEdit = nullptr;
static HWND g_list = nullptr;
static HWND g_courseCombo = nullptr;
static HWND g_filterEdit = nullptr;
static HWND g_settingsWindow = nullptr;
static HWND g_chartWindow = nullptr;
static HWND g_quickRollCallWindow = nullptr;
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
static int g_openThemedPopupMenus = 0;
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

struct QuickRollCallState {
    std::wstring lesson;
    std::wstring course;
    std::vector<std::wstring> students;
    std::vector<AttendanceRecord> workingRecords;
    std::vector<bool> reviewed;
    size_t index = 0;
    bool changed = false;
    bool allowClose = false;
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
enum class UiTheme { Dark, Light };
enum class AccentTone { Neutral, Sage, Cyan };
enum class TextScale { Small, Standard, Large };
enum class LayoutDensity { Compact, Comfortable, Spacious };
enum class AnimationLevel { Off, Standard, Advanced };
enum class ParticleLevel { Low, Medium, High };
enum class ReportTemplate { Simple, Teacher, Parent, Complete };
enum class StatsRange { All, ThisWeek, ThisMonth };

static UiLanguage g_language = UiLanguage::English;
static UiTheme g_theme = UiTheme::Dark;
static AccentTone g_accentTone = AccentTone::Neutral;
static TextScale g_textScale = TextScale::Standard;
static LayoutDensity g_layoutDensity = LayoutDensity::Comfortable;
static std::wstring g_fontFamily = L"Segoe UI";
static std::wstring g_defaultSaveDir;
static std::vector<std::wstring> g_availableFonts;
static AnimationLevel g_animationLevel = AnimationLevel::Advanced;
static ParticleLevel g_particleLevel = ParticleLevel::Medium;
static ReportTemplate g_reportTemplate = ReportTemplate::Complete;
static StatsRange g_statsRange = StatsRange::All;
static bool g_riskAlertsEnabled = true;
static bool g_autosavePromptEnabled = true;
static bool g_commandPaletteEnabled = true;
static bool g_advancedFilterEnabled = true;
static bool g_advancedPersonalizationEnabled = false;
static bool g_showStatsCards = true;
static bool g_showFooterHint = true;
static bool g_showTableGrid = true;
static int g_autosaveIntervalSeconds = 30;
static std::wstring g_shortcutSave = L"Ctrl+S";
static std::wstring g_shortcutImport = L"Ctrl+O";
static std::wstring g_shortcutUndo = L"Ctrl+Z";
static std::wstring g_shortcutRedo = L"Ctrl+Y";
static std::wstring g_shortcutCommand = L"Ctrl+K";
static std::wstring g_shortcutFullscreen = L"F11";

static COLORREF COLOR_BG = RGB(0, 0, 0);
static COLORREF COLOR_PANEL = RGB(18, 18, 18);
static COLORREF COLOR_INPUT = RGB(26, 26, 26);
static COLORREF COLOR_TEXT = RGB(255, 255, 255);
static COLORREF COLOR_MUTED = RGB(224, 224, 224);
static COLORREF COLOR_ACCENT = RGB(188, 188, 188);
static COLORREF COLOR_DANGER = RGB(176, 112, 112);
static COLORREF COLOR_BORDER = RGB(64, 64, 64);
static COLORREF COLOR_HOVER = RGB(38, 38, 38);
static COLORREF COLOR_CARD = RGB(16, 16, 16);
static COLORREF COLOR_ROW_ALT = RGB(14, 14, 14);
static COLORREF COLOR_SELECTED = RGB(42, 42, 42);
static COLORREF COLOR_CHECK_FILL = RGB(0, 0, 0);

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
    uint32_t durationMs = 0;
    bool rippleEnabled = true;
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
void SaveAttendance();
void ImportAttendance();
void ExportCsv();
void BackupNow();
void ShowSettingsWindow();
void ReviewLessonCompletion();
void ShowQuickRollCall();
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
std::wstring ParticleEffectsLabel();
std::wstring AnimationLevelName(AnimationLevel level);
std::wstring ParticleLevelName(ParticleLevel level);
std::wstring ReportTemplateName(ReportTemplate templateType);
std::wstring StatsRangeName(StatsRange range);
void RunCommandPalette();
std::wstring NormalizeShortcut(const std::wstring& value, const std::wstring& fallback);
bool ShortcutMatches(const std::wstring& shortcut, WPARAM key);

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
    int contentHeight = 0;
    int scrollY = 0;
    int maxScrollY = 0;
    int mainScrollX = 0;
    int mainScrollY = 0;
    POINT lastMouse{};
    bool scrollable = false;
    bool keyboardMode = false;
    bool hasMousePoint = false;
    bool done = false;
    bool closing = false;
};

static constexpr int THEMED_MENU_PAD_Y = 8;
static constexpr int THEMED_MENU_ITEM_H = 34;
static constexpr int THEMED_MENU_SEPARATOR_H = 13;
static constexpr int THEMED_MENU_SLIDE_PX = 5;
// Keep the long Tools menu compact; the remaining actions stay reachable by wheel/keyboard scroll.
static constexpr int THEMED_MENU_MAX_H = 520;

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

uint32_t EffectiveAnimationDuration(AnimChannel channel, uint32_t durationMs) {
    // Particle frames must follow their physical lifetime, independently of
    // the interface animation preset. Shortening this channel freezes effects.
    if (channel == AnimChannel::Effect) return durationMs;
    if (g_animationLevel == AnimationLevel::Off) return 0;
    if (g_animationLevel == AnimationLevel::Standard) return std::min<uint32_t>(durationMs, 160);
    return durationMs;
}

void StartAnimation(HWND hwnd, AnimChannel channel, double target, uint32_t durationMs, bool closeOnDone) {
    if (!hwnd) return;
    durationMs = EffectiveAnimationDuration(channel, durationMs);
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
    durationMs = EffectiveAnimationDuration(channel, durationMs);
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
    BOOL dark = g_theme == UiTheme::Dark ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

    COLORREF caption = COLOR_BG;
    COLORREF text = COLOR_TEXT;
    COLORREF border = COLOR_BORDER;
    DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &caption, sizeof(caption));
    DwmSetWindowAttribute(hwnd, DWMWA_TEXT_COLOR, &text, sizeof(text));
    DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &border, sizeof(border));

    int backdrop = 2; // DWMSBT_MAINWINDOW / Mica on supported Windows builds.
    DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop, sizeof(backdrop));
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE |
        SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME | RDW_NOERASE);
}

void CenterOwnedWindow(HWND window, HWND owner) {
    if (!window) return;
    RECT windowRc{};
    if (!GetWindowRect(window, &windowRc)) return;

    HMONITOR monitor = MonitorFromWindow(owner && IsWindow(owner) ? owner : window, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(monitor, &mi)) return;

    RECT anchor = mi.rcWork;
    if (owner && IsWindow(owner)) GetWindowRect(owner, &anchor);
    int width = windowRc.right - windowRc.left;
    int height = windowRc.bottom - windowRc.top;
    int x = anchor.left + ((anchor.right - anchor.left) - width) / 2;
    int y = anchor.top + ((anchor.bottom - anchor.top) - height) / 2;
    x = std::clamp(x, (int)mi.rcWork.left, std::max((int)mi.rcWork.left, (int)mi.rcWork.right - width));
    y = std::clamp(y, (int)mi.rcWork.top, std::max((int)mi.rcWork.top, (int)mi.rcWork.bottom - height));
    SetWindowPos(window, nullptr, x, y, 0, 0,
        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}

void StartListTransition() {
    if (!g_list) return;
    RestartAnimation(g_list, AnimChannel::ListReveal, 1.0, 260);
    RedrawWindow(g_list, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME | RDW_NOERASE);
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
            ? COLOR_SELECTED
            : (item % 2 == 0 ? COLOR_PANEL : COLOR_ROW_ALT);
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
        RedrawWindow(parent, &rc, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
    }
    SetWindowTextW(hwnd, text.c_str());
    InvalidateRect(hwnd, nullptr, FALSE);
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

std::wstring TrimWide(std::wstring value) {
    auto isSpace = [](wchar_t ch) { return iswspace(ch) != 0; };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](wchar_t ch) { return !isSpace(ch); }));
    value.erase(std::find_if(value.rbegin(), value.rend(), [&](wchar_t ch) { return !isSpace(ch); }).base(), value.end());
    return value;
}

std::wstring UpperText(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t ch) {
        return (wchar_t)towupper(ch);
    });
    return value;
}

std::wstring NormalizeShortcut(const std::wstring& value, const std::wstring& fallback) {
    std::wstring shortcut = TrimWide(value);
    shortcut.erase(std::remove_if(shortcut.begin(), shortcut.end(), [](wchar_t ch) {
        return iswspace(ch) != 0;
    }), shortcut.end());
    std::replace(shortcut.begin(), shortcut.end(), L'-', L'+');
    shortcut = UpperText(shortcut);

    if (shortcut.rfind(L"CONTROL+", 0) == 0) {
        shortcut = L"CTRL+" + shortcut.substr(8);
    }

    auto normalizeKeyPart = [](const std::wstring& key) -> std::wstring {
        if (key.size() == 1 && ((key[0] >= L'A' && key[0] <= L'Z') || (key[0] >= L'0' && key[0] <= L'9'))) {
            return key;
        }
        if (key.size() >= 2 && key[0] == L'F') {
            try {
                int number = std::stoi(key.substr(1));
                if (number >= 1 && number <= 24) return L"F" + std::to_wstring(number);
            } catch (...) {
            }
        }
        return {};
    };

    if (shortcut.rfind(L"CTRL+", 0) == 0) {
        std::wstring key = normalizeKeyPart(shortcut.substr(5));
        if (!key.empty()) return L"Ctrl+" + key;
    } else {
        std::wstring key = normalizeKeyPart(shortcut);
        if (!key.empty()) return key;
    }
    return fallback;
}

bool ShortcutKeyPartMatches(const std::wstring& key, WPARAM wParam) {
    if (key.size() == 1) {
        wchar_t ch = key[0];
        if (ch >= L'a' && ch <= L'z') ch = (wchar_t)towupper(ch);
        return (wParam == (WPARAM)ch);
    }
    if (key.size() >= 2 && key[0] == L'F') {
        try {
            int number = std::stoi(key.substr(1));
            return number >= 1 && number <= 24 && wParam == (WPARAM)(VK_F1 + number - 1);
        } catch (...) {
        }
    }
    return false;
}

bool ShortcutMatches(const std::wstring& shortcut, WPARAM key) {
    std::wstring normalized = NormalizeShortcut(shortcut, L"");
    if (normalized.empty()) return false;
    bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    bool altDown = (GetKeyState(VK_MENU) & 0x8000) != 0;
    if (normalized.rfind(L"Ctrl+", 0) == 0) {
        return ctrlDown && !altDown && ShortcutKeyPartMatches(normalized.substr(5), key);
    }
    return !ctrlDown && !altDown && ShortcutKeyPartMatches(normalized, key);
}

std::wstring ShortcutConfigText() {
    return L"save=" + g_shortcutSave
        + L"; import=" + g_shortcutImport
        + L"; undo=" + g_shortcutUndo
        + L"; redo=" + g_shortcutRedo
        + L"; command=" + g_shortcutCommand
        + L"; fullscreen=" + g_shortcutFullscreen;
}

bool ApplyShortcutConfigText(const std::wstring& config) {
    bool changed = false;
    std::wstringstream stream(config);
    std::wstring entry;
    while (std::getline(stream, entry, L';')) {
        auto pos = entry.find(L'=');
        if (pos == std::wstring::npos) continue;
        std::wstring key = LowerText(TrimWide(entry.substr(0, pos)));
        std::wstring value = TrimWide(entry.substr(pos + 1));
        std::wstring* target = nullptr;
        std::wstring fallback;
        if (key == L"save" || key == L"s") {
            target = &g_shortcutSave;
            fallback = L"Ctrl+S";
        } else if (key == L"import" || key == L"open" || key == L"o") {
            target = &g_shortcutImport;
            fallback = L"Ctrl+O";
        } else if (key == L"undo" || key == L"z") {
            target = &g_shortcutUndo;
            fallback = L"Ctrl+Z";
        } else if (key == L"redo" || key == L"y") {
            target = &g_shortcutRedo;
            fallback = L"Ctrl+Y";
        } else if (key == L"command" || key == L"palette" || key == L"command_palette" || key == L"commandpalette") {
            target = &g_shortcutCommand;
            fallback = L"Ctrl+K";
        } else if (key == L"fullscreen" || key == L"full" || key == L"f11") {
            target = &g_shortcutFullscreen;
            fallback = L"F11";
        }
        if (!target) continue;
        std::wstring normalized = NormalizeShortcut(value, *target);
        if (!normalized.empty() && normalized != *target) {
            *target = normalized;
            changed = true;
        }
    }
    return changed;
}

std::wstring RecordDateKey(const AttendanceRecord& record) {
    return record.dateTime.size() >= 10 ? record.dateTime.substr(0, 10) : record.dateTime;
}

std::wstring TodayDateKey() {
    SYSTEMTIME st{};
    GetLocalTime(&st);
    wchar_t buffer[16]{};
    swprintf_s(buffer, L"%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
    return buffer;
}

bool DateKeyToFileTime(const std::wstring& date, FILETIME& out) {
    if (date.size() < 10) return false;
    SYSTEMTIME st{};
    try {
        st.wYear = (WORD)std::stoi(date.substr(0, 4));
        st.wMonth = (WORD)std::stoi(date.substr(5, 2));
        st.wDay = (WORD)std::stoi(date.substr(8, 2));
    } catch (...) {
        return false;
    }
    st.wHour = 12;
    return SystemTimeToFileTime(&st, &out) != FALSE;
}

int64_t FileTimeTicks(const FILETIME& ft) {
    ULARGE_INTEGER value{};
    value.LowPart = ft.dwLowDateTime;
    value.HighPart = ft.dwHighDateTime;
    return (int64_t)value.QuadPart;
}

bool DateWithinRecentDays(const std::wstring& date, int days) {
    FILETIME recordFt{};
    if (!DateKeyToFileTime(date, recordFt)) return true;
    SYSTEMTIME nowSt{};
    FILETIME nowFt{};
    GetLocalTime(&nowSt);
    nowSt.wHour = 12;
    nowSt.wMinute = 0;
    nowSt.wSecond = 0;
    nowSt.wMilliseconds = 0;
    SystemTimeToFileTime(&nowSt, &nowFt);
    constexpr int64_t dayTicks = 864000000000LL;
    int64_t delta = FileTimeTicks(nowFt) - FileTimeTicks(recordFt);
    return delta >= 0 && delta <= (int64_t)std::max(1, days) * dayTicks;
}

bool DateWithinCurrentMonth(const std::wstring& date) {
    std::wstring today = TodayDateKey();
    return date.size() >= 7 && today.size() >= 7 && date.substr(0, 7) == today.substr(0, 7);
}

bool RecordMatchesStatsRange(const AttendanceRecord& record) {
    std::wstring date = RecordDateKey(record);
    if (g_statsRange == StatsRange::ThisWeek) return DateWithinRecentDays(date, 7);
    if (g_statsRange == StatsRange::ThisMonth) return DateWithinCurrentMonth(date);
    return true;
}

std::vector<std::wstring> SplitWords(const std::wstring& text) {
    std::wistringstream ss(text);
    std::vector<std::wstring> words;
    std::wstring word;
    while (ss >> word) words.push_back(word);
    return words;
}

bool RecordMatchesFilter(const AttendanceRecord& record) {
    if (g_filterText.empty()) return true;
    if (!g_advancedFilterEnabled) {
        return ContainsText(record.dateTime, g_filterText)
            || ContainsText(record.name, g_filterText)
            || ContainsText(record.status, g_filterText)
            || ContainsText(record.other, g_filterText);
    }

    std::wstring nameFilter;
    std::wstring statusFilter;
    std::wstring courseFilter;
    std::wstring exactDate;
    std::wstring fromDate;
    std::wstring toDate;
    std::vector<std::wstring> freeTerms;

    for (const auto& token : SplitWords(g_filterText)) {
        auto colon = token.find(L':');
        if (colon == std::wstring::npos) {
            freeTerms.push_back(token);
            continue;
        }
        std::wstring key = token.substr(0, colon);
        std::wstring value = token.substr(colon + 1);
        if (key == L"name") nameFilter = value;
        else if (key == L"status") statusFilter = value;
        else if (key == L"course") courseFilter = value;
        else if (key == L"date") exactDate = value;
        else if (key == L"from") fromDate = value;
        else if (key == L"to") toDate = value;
        else freeTerms.push_back(token);
    }

    if (std::find(freeTerms.begin(), freeTerms.end(), L"thisweek") != freeTerms.end()
        || std::find(freeTerms.begin(), freeTerms.end(), L"week") != freeTerms.end()) {
        if (!DateWithinRecentDays(RecordDateKey(record), 7)) return false;
    }
    if (std::find(freeTerms.begin(), freeTerms.end(), L"thismonth") != freeTerms.end()
        || std::find(freeTerms.begin(), freeTerms.end(), L"month") != freeTerms.end()) {
        if (!DateWithinCurrentMonth(RecordDateKey(record))) return false;
    }

    std::wstring date = RecordDateKey(record);
    if (!exactDate.empty() && !ContainsText(LowerText(record.dateTime), exactDate)) return false;
    if (!fromDate.empty() && date < fromDate) return false;
    if (!toDate.empty() && date > toDate) return false;
    if (!nameFilter.empty() && !ContainsText(record.name, nameFilter)) return false;
    if (!statusFilter.empty() && !ContainsText(record.status, statusFilter)) return false;
    if (!courseFilter.empty()) {
        if (g_activeSheet < 0 || g_activeSheet >= (int)g_sheets.size() || !ContainsText(g_sheets[g_activeSheet].name, courseFilter)) return false;
    }

    std::wstring joinedFree;
    for (const auto& term : freeTerms) {
        if (term == L"thisweek" || term == L"week" || term == L"thismonth" || term == L"month") continue;
        if (!joinedFree.empty()) joinedFree += L" ";
        joinedFree += term;
    }
    if (joinedFree.empty()) return true;
    return ContainsText(record.dateTime, joinedFree)
        || ContainsText(record.name, joinedFree)
        || ContainsText(record.status, joinedFree)
        || ContainsText(record.other, joinedFree);
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
        if (g_openThemedPopupMenus > 0) return 0;
        ScrollMainWindow(g_hwnd, SB_VERT, 0, GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;
    }
    if (msg == WM_MOUSEHWHEEL && g_hwnd) {
        if (g_openThemedPopupMenus > 0) return 0;
        ScrollMainWindow(g_hwnd, SB_HORZ, 0, GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void EnableMouseWheelForward(HWND hwnd) {
    SetWindowSubclass(hwnd, WheelForwardProc, 2, 0);
}

void RedrawAnimatedControl(HWND hwnd) {
    RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME | RDW_UPDATENOW | RDW_NOERASE);
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
    COLORREF border = BlendColor(COLOR_BORDER, COLOR_ACCENT, hover);
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
    AdjustWindowRectEx(&windowRc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE, WS_EX_CONTROLPARENT);
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
        SetTextColor(hdc, COLOR_TEXT);
        SetBkMode(hdc, OPAQUE);
        SetBkColor(hdc, COLOR_BG);
        return (LRESULT)g_bgBrush;
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
    HWND parent = owner ? owner : g_hwnd;
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

    BOOL dark = g_theme == UiTheme::Dark ? TRUE : FALSE;
    DwmSetWindowAttribute(dialog, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
    ApplyGlassTitleBar(dialog);
    CenterOwnedWindow(dialog, parent);
    ShowWindow(dialog, SW_SHOW);
    UpdateWindow(dialog);
    SetFocus(GetDlgItem(dialog, state.yesNo ? IDC_MESSAGE_NO : IDC_MESSAGE_OK));

    MSG msg{};
    while (!state.done && IsWindow(dialog) && GetMessageW(&msg, nullptr, 0, 0) > 0) {
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
        SetBkMode(hdc, OPAQUE);
        SetBkColor(hdc, COLOR_BG);
        return (LRESULT)g_bgBrush;
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
    EnableWindow(g_hwnd, FALSE);
    HWND dialog = CreateWindowExW(
        WS_EX_CONTROLPARENT,
        className,
        title.c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 430, 190,
        g_hwnd, nullptr, instance, &state
    );
    if (!dialog) {
        EnableWindow(g_hwnd, TRUE);
        return false;
    }
    ApplyGlassTitleBar(dialog);
    ApplyThemedControls(dialog);
    CenterOwnedWindow(dialog, g_hwnd);
    ShowWindow(dialog, SW_SHOW);
    UpdateWindow(dialog);

    MSG msg{};
    while (IsWindow(dialog) && GetMessageW(&msg, nullptr, 0, 0)) {
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

bool TranslateFeatureText(const std::wstring& key, std::wstring& out) {
    if (g_language == UiLanguage::English) return false;

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
        const wchar_t* it;
        const wchar_t* mn;
        const wchar_t* eo;
        const wchar_t* lzh;
        const wchar_t* th;
        const wchar_t* fil;
        const wchar_t* tr;
        const wchar_t* lt;
        const wchar_t* no;
        const wchar_t* vi;
        const wchar_t* zhhk;
    };

    static const Entry entries[] = {
        {L"Animation level", L"动画强度", L"Livell ta' animazzjoni", L"アニメーションの強さ", L"Niveau d'animation", L"Animationsstufe", L"Уровень анимации", L"動畫強度", L"Nivel de animación", L"Livello animazione", L"Хөдөлгөөний түвшин", L"Nivelo de animacio", L"動畫之度", L"ระดับภาพเคลื่อนไหว", L"Antas ng animation", L"Animasyon düzeyi", L"Animacijos lygis", L"Animasjonsnivå", L"Mức hoạt ảnh", L"動畫強度"},
        {L"Particle density", L"粒子密度", L"Densità tal-partiċelli", L"パーティクル密度", L"Densité des particules", L"Partikeldichte", L"Плотность частиц", L"粒子密度", L"Densidad de partículas", L"Densità particelle", L"Бөөмсийн нягт", L"Denseco de eroj", L"粒子之密", L"ความหนาแน่นของอนุภาค", L"Densidad ng particle", L"Parçacık yoğunluğu", L"Dalelių tankis", L"Partikkeltetthet", L"Mật độ hạt", L"粒子密度"},
        {L"Report template", L"报告模板", L"Mudell tar-rapport", L"レポートテンプレート", L"Modèle de rapport", L"Berichtsvorlage", L"Шаблон отчёта", L"報告範本", L"Plantilla de informe", L"Modello di rapporto", L"Тайлангийн загвар", L"Raporta ŝablono", L"報告之式", L"แม่แบบรายงาน", L"Template ng ulat", L"Rapor şablonu", L"Ataskaitos šablonas", L"Rapportmal", L"Mẫu báo cáo", L"報告範本"},
        {L"Enable risk student reminders", L"启用风险学生提醒", L"Ippermetti tfakkiriet għal studenti f'riskju", L"要注意生徒の通知を有効にする", L"Activer les rappels pour élèves à risque", L"Erinnerungen für Risikoschüler aktivieren", L"Включить напоминания о группе риска", L"啟用風險學生提醒", L"Activar avisos de estudiantes en riesgo", L"Attiva avvisi studenti a rischio", L"Эрсдэлтэй сурагчийн сануулгыг идэвхжүүлэх", L"Ŝalti memorigojn pri riskaj lernantoj", L"啟危生之告", L"เปิดการเตือนนักเรียนกลุ่มเสี่ยง", L"I-enable ang paalala sa estudyanteng may panganib", L"Riskli öğrenci uyarılarını etkinleştir", L"Įjungti rizikos mokinių priminimus", L"Aktiver påminnelser om risikostudenter", L"Bật nhắc nhở học sinh có nguy cơ", L"啟用風險學生提示"},
        {L"Prompt for autosave recovery", L"启动时提示恢复自动保存", L"Staqsi dwar irkupru tas-salvataġġ awtomatiku", L"自動保存の復元を確認する", L"Proposer la récupération automatique", L"Wiederherstellung der Autospeicherung anbieten", L"Предлагать восстановление автосохранения", L"提示復原自動儲存", L"Preguntar por recuperación automática", L"Chiedi recupero salvataggio automatico", L"Автомат хадгалалт сэргээхийг асуух", L"Demandi pri aŭtomata reakiro", L"問自藏之復", L"ถามการกู้คืนบันทึกอัตโนมัติ", L"Magtanong tungkol sa awtomatikong pagbawi", L"Otomatik kaydetme kurtarmasını sor", L"Klausti dėl automatinio atkūrimo", L"Spør om gjenoppretting av autolagring", L"Nhắc khôi phục lưu tự động", L"啟動時提示還原自動儲存"},
        {L"Enable Ctrl+K command palette", L"启用 Ctrl+K 命令面板", L"Ippermetti l-paletta tal-kmand Ctrl+K", L"Ctrl+K コマンドパレットを有効にする", L"Activer la palette de commandes Ctrl+K", L"Ctrl+K-Befehlspalette aktivieren", L"Включить палитру команд Ctrl+K", L"啟用 Ctrl+K 命令面板", L"Activar paleta de comandos Ctrl+K", L"Attiva tavolozza comandi Ctrl+K", L"Ctrl+K командын самбарыг идэвхжүүлэх", L"Ŝalti komandan paletron Ctrl+K", L"啟 Ctrl+K 命令板", L"เปิดจานคำสั่ง Ctrl+K", L"I-enable ang command palette na Ctrl+K", L"Ctrl+K komut paletini etkinleştir", L"Įjungti Ctrl+K komandų paletę", L"Aktiver Ctrl+K-kommandopaletten", L"Bật bảng lệnh Ctrl+K", L"啟用 Ctrl+K 指令面板"},
        {L"Enable advanced filter syntax", L"启用高级筛选语法", L"Ippermetti sintassi ta' filtru avvanzat", L"高度なフィルター構文を有効にする", L"Activer la syntaxe de filtre avancée", L"Erweiterte Filtersyntax aktivieren", L"Включить расширенный синтаксис фильтра", L"啟用進階篩選語法", L"Activar sintaxis de filtro avanzado", L"Attiva sintassi filtro avanzata", L"Дэвшилтэт шүүлтүүрийн хэлбэрийг идэвхжүүлэх", L"Ŝalti altnivelan filtrilan sintakson", L"啟進篩之式", L"เปิดไวยากรณ์ตัวกรองขั้นสูง", L"I-enable ang advanced filter syntax", L"Gelişmiş filtre sözdizimini etkinleştir", L"Įjungti išplėstinę filtro sintaksę", L"Aktiver avansert filtersyntaks", L"Bật cú pháp bộ lọc nâng cao", L"啟用進階篩選語法"},
        {L"Off", L"关闭", L"Mitfi", L"オフ", L"Désactivé", L"Aus", L"Выкл.", L"關閉", L"Desactivado", L"Disattivato", L"Унтраах", L"Malŝaltita", L"闔", L"ปิด", L"Naka-off", L"Kapalı", L"Išjungta", L"Av", L"Tắt", L"關閉"},
        {L"Standard", L"标准", L"Standard", L"標準", L"Standard", L"Standard", L"Стандартный", L"標準", L"Estándar", L"Standard", L"Стандарт", L"Norma", L"常", L"มาตรฐาน", L"Pamantayan", L"Standart", L"Standartinis", L"Standard", L"Tiêu chuẩn", L"標準"},
        {L"Advanced", L"高级", L"Avvanzat", L"高度", L"Avancé", L"Erweitert", L"Расширенный", L"進階", L"Avanzado", L"Avanzato", L"Ахисан", L"Altnivela", L"高", L"ขั้นสูง", L"Advanced", L"Gelişmiş", L"Išplėstinis", L"Avansert", L"Nâng cao", L"進階"},
        {L"Low", L"低", L"Baxx", L"低", L"Faible", L"Niedrig", L"Низкий", L"低", L"Bajo", L"Basso", L"Бага", L"Malalta", L"低", L"ต่ำ", L"Mababa", L"Düşük", L"Žemas", L"Lav", L"Thấp", L"低"},
        {L"Medium", L"中", L"Medju", L"中", L"Moyen", L"Mittel", L"Средний", L"中", L"Medio", L"Medio", L"Дунд", L"Meza", L"中", L"กลาง", L"Katamtaman", L"Orta", L"Vidutinis", L"Middels", L"Trung bình", L"中"},
        {L"High", L"高", L"Għoli", L"高", L"Élevé", L"Hoch", L"Высокий", L"高", L"Alto", L"Alto", L"Өндөр", L"Alta", L"高", L"สูง", L"Mataas", L"Yüksek", L"Aukštas", L"Høy", L"Cao", L"高"},
        {L"Simple", L"简洁版", L"Sempliċi", L"簡易", L"Simple", L"Einfach", L"Простой", L"簡潔版", L"Simple", L"Semplice", L"Энгийн", L"Simpla", L"簡", L"แบบง่าย", L"Simple", L"Basit", L"Paprastas", L"Enkel", L"Đơn giản", L"簡潔版"},
        {L"Teacher", L"教师版", L"Għall-għalliem", L"教師用", L"Enseignant", L"Lehrkraft", L"Учитель", L"教師版", L"Profesor", L"Insegnante", L"Багш", L"Instruisto", L"師", L"ครู", L"Guro", L"Öğretmen", L"Mokytojas", L"Lærer", L"Giáo viên", L"教師版"},
        {L"Parent", L"家长版", L"Ġenitur", L"保護者用", L"Parent", L"Eltern", L"Родитель", L"家長版", L"Padre", L"Genitore", L"Эцэг эх", L"Gepatro", L"親", L"ผู้ปกครอง", L"Magulang", L"Veli", L"Tėvas ar mama", L"Foresatt", L"Phụ huynh", L"家長版"},
        {L"Complete", L"完整版", L"Komplet", L"完全", L"Complet", L"Vollständig", L"Полный", L"完整版", L"Completo", L"Completo", L"Бүрэн", L"Kompleta", L"全", L"สมบูรณ์", L"Kumpleto", L"Tam", L"Visas", L"Fullstendig", L"Đầy đủ", L"完整版"},
        {L"All time", L"全部时间", L"Il-ħin kollu", L"全期間", L"Toute la période", L"Gesamter Zeitraum", L"За всё время", L"全部時間", L"Todo el tiempo", L"Tutto il periodo", L"Бүх хугацаа", L"Ĉiu tempo", L"通時", L"ตลอดเวลา", L"Lahat ng oras", L"Tüm zamanlar", L"Visas laikas", L"Hele perioden", L"Toàn bộ thời gian", L"全部時間"},
        {L"This week", L"本周", L"Din il-ġimgħa", L"今週", L"Cette semaine", L"Diese Woche", L"Эта неделя", L"本週", L"Esta semana", L"Questa settimana", L"Энэ долоо хоног", L"Ĉi tiu semajno", L"本週", L"สัปดาห์นี้", L"Ngayong linggo", L"Bu hafta", L"Ši savaitė", L"Denne uken", L"Tuần này", L"本週"},
        {L"This month", L"本月", L"Dan ix-xahar", L"今月", L"Ce mois-ci", L"Dieser Monat", L"Этот месяц", L"本月", L"Este mes", L"Questo mese", L"Энэ сар", L"Ĉi tiu monato", L"本月", L"เดือนนี้", L"Ngayong buwan", L"Bu ay", L"Šis mėnuo", L"Denne måneden", L"Tháng này", L"本月"},
        {L"Create today's lesson", L"创建今日课次", L"Oħloq il-lezzjoni tal-lum", L"今日の授業を作成", L"Créer la séance du jour", L"Heutige Unterrichtsstunde erstellen", L"Создать урок на сегодня", L"建立今日課次", L"Crear la clase de hoy", L"Crea la lezione di oggi", L"Өнөөдрийн хичээл үүсгэх", L"Krei hodiaŭan lecionon", L"立今日課", L"สร้างคาบเรียนวันนี้", L"Gumawa ng aralin ngayon", L"Bugünün dersini oluştur", L"Sukurti šiandienos pamoką", L"Opprett dagens økt", L"Tạo buổi học hôm nay", L"建立今日課堂"},
        {L"Switch lesson", L"切换课次", L"Ibdel lezzjoni", L"授業を切り替え", L"Changer de séance", L"Unterrichtsstunde wechseln", L"Сменить урок", L"切換課次", L"Cambiar clase", L"Cambia lezione", L"Хичээл солих", L"Ŝanĝi lecionon", L"易課", L"สลับคาบเรียน", L"Lumipat ng aralin", L"Dersi değiştir", L"Keisti pamoką", L"Bytt økt", L"Chuyển buổi học", L"切換課堂"},
        {L"Student profile", L"学生档案", L"Profil tal-istudent", L"学生プロフィール", L"Profil de l'élève", L"Schülerprofil", L"Профиль ученика", L"學生檔案", L"Perfil del estudiante", L"Profilo studente", L"Сурагчийн танилцуулга", L"Profilo de lernanto", L"弟子檔", L"โปรไฟล์นักเรียน", L"Profile ng estudyante", L"Öğrenci profili", L"Mokinio profilis", L"Elevprofil", L"Hồ sơ học sinh", L"學生檔案"},
        {L"Advanced filter", L"高级筛选", L"Filtru avvanzat", L"高度なフィルター", L"Filtre avancé", L"Erweiterter Filter", L"Расширенный фильтр", L"進階篩選", L"Filtro avanzado", L"Filtro avanzato", L"Дэвшилтэт шүүлтүүр", L"Altnivela filtrilo", L"進篩", L"ตัวกรองขั้นสูง", L"Advanced filter", L"Gelişmiş filtre", L"Išplėstinis filtras", L"Avansert filter", L"Bộ lọc nâng cao", L"進階篩選"},
        {L"Statistics range", L"统计范围", L"Medda tal-istatistika", L"統計範囲", L"Période statistique", L"Statistikzeitraum", L"Диапазон статистики", L"統計範圍", L"Rango de estadísticas", L"Intervallo statistiche", L"Статистикийн хүрээ", L"Statistika intervalo", L"計數之域", L"ช่วงสถิติ", L"Saklaw ng istatistika", L"İstatistik aralığı", L"Statistikos intervalas", L"Statistikkområde", L"Phạm vi thống kê", L"統計範圍"},
        {L"Risk students", L"风险学生", L"Studenti f'riskju", L"要注意生徒", L"Élèves à risque", L"Risikoschüler", L"Учащиеся группы риска", L"風險學生", L"Estudiantes en riesgo", L"Studenti a rischio", L"Эрсдэлтэй сурагчид", L"Riskaj lernantoj", L"危弟", L"นักเรียนกลุ่มเสี่ยง", L"Mga estudyanteng may panganib", L"Riskli öğrenciler", L"Rizikos mokiniai", L"Risikostudenter", L"Học sinh có nguy cơ", L"風險學生"},
        {L"Backup manager", L"备份管理中心", L"Maniġer tal-backup", L"バックアップ管理", L"Gestionnaire de sauvegardes", L"Sicherungsverwaltung", L"Менеджер резервных копий", L"備份管理中心", L"Gestor de copias", L"Gestore backup", L"Нөөцийн менежер", L"Sekurkopi-administrilo", L"備份司", L"ตัวจัดการข้อมูลสำรอง", L"Backup manager", L"Yedek yöneticisi", L"Atsarginių kopijų tvarkyklė", L"Sikkerhetskopi-behandler", L"Trình quản lý sao lưu", L"備份管理中心"},
        {L"Command palette", L"命令面板", L"Paletta tal-kmand", L"コマンドパレット", L"Palette de commandes", L"Befehlspalette", L"Палитра команд", L"命令面板", L"Paleta de comandos", L"Tavolozza comandi", L"Командын самбар", L"Komanda paletro", L"命令板", L"จานคำสั่ง", L"Command palette", L"Komut paleti", L"Komandų paletė", L"Kommandopalett", L"Bảng lệnh", L"指令面板"},
        {L"Shortcut center", L"快捷键中心", L"Ċentru shortcuts", L"ショートカットセンター", L"Centre des raccourcis", L"Tastenkürzelzentrale", L"Центр сочетаний клавиш", L"快速鍵中心", L"Centro de atajos", L"Centro scorciatoie", L"Товчлолын төв", L"Ŝparvoja centro", L"捷鍵司", L"ศูนย์คีย์ลัด", L"Sentro ng shortcut", L"Kısayol merkezi", L"Spartųjų klavišų centras", L"Snarveissenter", L"Trung tâm phím tắt", L"快速鍵中心"},
        {L"Lesson", L"课次", L"Lezzjoni", L"授業", L"Séance", L"Unterrichtsstunde", L"Урок", L"課次", L"Clase", L"Lezione", L"Хичээл", L"Leciono", L"課", L"คาบเรียน", L"Aralin", L"Ders", L"Pamoka", L"Økt", L"Buổi học", L"課堂"},
        {L"Lesson name:", L"课次名称：", L"Isem tal-lezzjoni:", L"授業名:", L"Nom de la séance :", L"Name der Unterrichtsstunde:", L"Название урока:", L"課次名稱：", L"Nombre de la clase:", L"Nome della lezione:", L"Хичээлийн нэр:", L"Leciona nomo:", L"課名：", L"ชื่อคาบเรียน:", L"Pangalan ng aralin:", L"Ders adı:", L"Pamokos pavadinimas:", L"Øktnavn:", L"Tên buổi học:", L"課堂名稱："},
        {L"Recent history", L"最近记录", L"Storja riċenti", L"最近の履歴", L"Historique récent", L"Letzte Historie", L"Недавняя история", L"最近紀錄", L"Historial reciente", L"Cronologia recente", L"Сүүлийн түүх", L"Lastatempa historio", L"近錄", L"ประวัติล่าสุด", L"Kamakailang kasaysayan", L"Son geçmiş", L"Naujausia istorija", L"Nylig historikk", L"Lịch sử gần đây", L"最近紀錄"},
        {L"Lesson completion check", L"课次完成检查", L"Kontroll tat-tlestija tal-lezzjoni", L"授業完了チェック", L"Vérification de séance", L"Unterrichtsprüfung", L"Проверка урока", L"課次完成檢查", L"Comprobación de clase", L"Controllo lezione", L"Хичээлийн шалгалт", L"Leciona kompletiga kontrolo", L"課成檢", L"ตรวจสอบความครบถ้วนของคาบ", L"Pagsusuri ng aralin", L"Ders tamamlama denetimi", L"Pamokos patikra", L"Kontroll av økt", L"Kiểm tra buổi học", L"課堂完成檢查"},
        {L"Quick roll call", L"快速点名", L"Sejħa rapida", L"クイック点呼", L"Appel rapide", L"Schnelle Anwesenheit", L"Быстрая перекличка", L"快速點名", L"Pase de lista rápido", L"Appello rapido", L"Шуурхай бүртгэл", L"Rapida nomvoko", L"速點", L"เช็กชื่อด่วน", L"Mabilis na roll call", L"Hızlı yoklama", L"Greita patikra", L"Hurtig opprop", L"Điểm danh nhanh", L"快速點名"},
        {L"Reviewed", L"已检查", L"Iċċekkjati", L"確認済み", L"Vérifiés", L"Geprüft", L"Проверено", L"已檢查", L"Revisados", L"Verificati", L"Шалгасан", L"Kontrolitaj", L"已檢", L"ตรวจแล้ว", L"Nasuri", L"İncelenen", L"Patikrinta", L"Kontrollert", L"Đã kiểm tra", L"已檢查"},
        {L"Remaining", L"剩余", L"Fadal", L"残り", L"Restants", L"Verbleibend", L"Осталось", L"剩餘", L"Restantes", L"Rimanenti", L"Үлдсэн", L"Restantaj", L"餘", L"คงเหลือ", L"Natitira", L"Kalan", L"Liko", L"Gjenstår", L"Còn lại", L"剩餘"},
        {L"Course:", L"课程：", L"Kors:", L"コース：", L"Cours :", L"Kurs:", L"Курс:", L"課程：", L"Curso:", L"Corso:", L"Хичээл:", L"Kurso:", L"課程：", L"รายวิชา:", L"Kurso:", L"Ders:", L"Kursas:", L"Kurs:", L"Khóa học:", L"課程："},
        {L"Lesson:", L"课次：", L"Lezzjoni:", L"授業：", L"Séance :", L"Unterricht:", L"Урок:", L"課次：", L"Clase:", L"Lezione:", L"Хичээл:", L"Leciono:", L"課：", L"คาบเรียน:", L"Aralin:", L"Ders:", L"Pamoka:", L"Økt:", L"Buổi học:", L"課堂："},
        {L"Current status:", L"当前状态：", L"Status kurrenti:", L"現在の状態：", L"Statut actuel :", L"Aktueller Status:", L"Текущий статус:", L"目前狀態：", L"Estado actual:", L"Stato attuale:", L"Одоогийн төлөв:", L"Nuna stato:", L"今態：", L"สถานะปัจจุบัน:", L"Kasalukuyang katayuan:", L"Geçerli durum:", L"Dabartinė būsena:", L"Gjeldende status:", L"Trạng thái hiện tại:", L"目前狀態："},
        {L"Unmarked", L"未标记", L"Mhux immarkat", L"未マーク", L"Non marqué", L"Nicht markiert", L"Не отмечено", L"未標記", L"Sin marcar", L"Non segnato", L"Тэмдэглээгүй", L"Nemarkita", L"未標", L"ยังไม่ระบุ", L"Hindi pa namarkahan", L"İşaretlenmedi", L"Nepažymėta", L"Ikke markert", L"Chưa đánh dấu", L"未標記"},
        {L"Notes:", L"备注：", L"Noti:", L"メモ：", L"Notes :", L"Notizen:", L"Примечания:", L"備註：", L"Notas:", L"Note:", L"Тэмдэглэл:", L"Notoj:", L"註：", L"หมายเหตุ:", L"Tala:", L"Notlar:", L"Pastabos:", L"Merknader:", L"Ghi chú:", L"備註："},
        {L"Previous", L"上一位", L"Preċedenti", L"前へ", L"Précédent", L"Zurück", L"Предыдущий", L"上一位", L"Anterior", L"Precedente", L"Өмнөх", L"Antaŭa", L"前", L"ก่อนหน้า", L"Nakaraan", L"Önceki", L"Ankstesnis", L"Forrige", L"Trước", L"上一位"},
        {L"Skip", L"跳过", L"Aqbeż", L"スキップ", L"Passer", L"Überspringen", L"Пропустить", L"略過", L"Omitir", L"Salta", L"Алгасах", L"Preterlasi", L"略", L"ข้าม", L"Laktawan", L"Atla", L"Praleisti", L"Hopp over", L"Bỏ qua", L"略過"},
        {L"Finish", L"完成", L"Lesti", L"完了", L"Terminer", L"Fertig", L"Завершить", L"完成", L"Finalizar", L"Termina", L"Дуусгах", L"Fini", L"畢", L"เสร็จสิ้น", L"Tapusin", L"Bitir", L"Baigti", L"Fullfør", L"Hoàn tất", L"完成"},
        {L"Cancel", L"取消", L"Ikkanċella", L"キャンセル", L"Annuler", L"Abbrechen", L"Отмена", L"取消", L"Cancelar", L"Annulla", L"Цуцлах", L"Nuligi", L"罷", L"ยกเลิก", L"Kanselahin", L"İptal", L"Atšaukti", L"Avbryt", L"Hủy", L"取消"},
        {L"No students in the roster.", L"当前课程名单中没有学生。", L"M'hemmx studenti fil-lista.", L"名簿に生徒がいません。", L"La liste ne contient aucun élève.", L"Die Liste enthält keine Schüler.", L"В списке нет учащихся.", L"目前課程名單中沒有學生。", L"No hay estudiantes en la lista.", L"Nessuno studente nell'elenco.", L"Жагсаалтад сурагч алга.", L"Neniuj lernantoj en la listo.", L"名簿無生。", L"ไม่มีนักเรียนในรายชื่อ", L"Walang estudyante sa talaan.", L"Listede öğrenci yok.", L"Sąraše nėra mokinių.", L"Ingen elever i listen.", L"Không có học sinh trong danh sách.", L"目前課程名單中沒有學生。"},
        {L"Other status requires a note.", L"选择其他状态时必须填写备注。", L"L-istatus Ieħor jeħtieġ nota.", L"その他にはメモが必要です。", L"Le statut Autre exige une note.", L"Für Sonstiges ist eine Notiz erforderlich.", L"Для статуса Другое нужна заметка.", L"選擇其他狀態時必須填寫備註。", L"El estado Otro requiere una nota.", L"Lo stato Altro richiede una nota.", L"Бусад төлөвт тэмдэглэл шаардлагатай.", L"Alia stato postulas noton.", L"他態須註。", L"สถานะอื่นต้องมีหมายเหตุ", L"Kailangan ng tala para sa Iba pa.", L"Diğer durumu için not gerekir.", L"Būsenai Kita reikia pastabos.", L"Annen status krever merknad.", L"Trạng thái Khác cần ghi chú.", L"其他狀態必須填寫備註。"},
        {L"Duplicate lesson records must be resolved before quick roll call.", L"开始快速点名前必须先处理重复课次记录。", L"Ir-rekords duplikati għandhom jiġu solvuti qabel is-sejħa rapida.", L"クイック点呼の前に重複記録を解決してください。", L"Corrigez les doublons avant l'appel rapide.", L"Doppelte Einträge müssen vorher bereinigt werden.", L"Перед перекличкой устраните дубликаты.", L"快速點名前必須先處理重複課次紀錄。", L"Resuelva los registros duplicados antes del pase rápido.", L"Risolvi i record duplicati prima dell'appello rapido.", L"Шуурхай бүртгэлээс өмнө давхардлыг засна уу.", L"Solvu duoblajn registrojn antaŭ la rapida nomvoko.", L"速點前必去重錄。", L"ต้องแก้ไขรายการซ้ำก่อนเช็กชื่อด่วน", L"Ayusin muna ang dobleng tala bago ang mabilis na roll call.", L"Hızlı yoklamadan önce yinelenen kayıtları çözün.", L"Prieš greitą patikrą pašalinkite dublikatus.", L"Løs duplikater før hurtig opprop.", L"Hãy xử lý bản ghi trùng trước khi điểm danh nhanh.", L"快速點名前必須先處理重複課堂紀錄。"},
        {L"Discard quick roll call changes?", L"放弃本次快速点名的修改吗？", L"Twarrab il-bidliet tas-sejħa rapida?", L"クイック点呼の変更を破棄しますか？", L"Abandonner les modifications de l'appel rapide ?", L"Änderungen der Schnellkontrolle verwerfen?", L"Отменить изменения быстрой переклички?", L"放棄本次快速點名的修改嗎？", L"¿Descartar los cambios del pase rápido?", L"Scartare le modifiche dell'appello rapido?", L"Шуурхай бүртгэлийн өөрчлөлтийг цуцлах уу?", L"Forĵeti la ŝanĝojn de rapida nomvoko?", L"棄速點之改乎？", L"ยกเลิกการเปลี่ยนแปลงเช็กชื่อด่วนหรือไม่", L"Itapon ang mga pagbabago sa mabilis na roll call?", L"Hızlı yoklama değişiklikleri atılsın mı?", L"Atmesti greitos patikros pakeitimus?", L"Forkaste endringer fra hurtig opprop?", L"Hủy thay đổi điểm danh nhanh?", L"放棄本次快速點名的修改嗎？"},
        {L"students have not been reviewed. Finish and keep the partial results?", L"名学生尚未检查。是否完成并保留部分结果？", L"studenti għadhom ma ġewx iċċekkjati. Tispiċċa u żżomm ir-riżultati parzjali?", L"名が未確認です。部分結果を保持して完了しますか？", L"élèves restent à vérifier. Terminer et conserver les résultats partiels ?", L"Schüler sind ungeprüft. Beenden und Teilergebnisse behalten?", L"учащихся не проверено. Завершить и сохранить частичные результаты?", L"名學生尚未檢查。是否完成並保留部分結果？", L"estudiantes no han sido revisados. ¿Finalizar y conservar los resultados parciales?", L"studenti non sono stati verificati. Terminare e conservare i risultati parziali?", L"сурагч шалгагдаагүй. Дуусгаж хэсэгчилсэн үр дүнг хадгалах уу?", L"lernantoj ne estas kontrolitaj. Fini kaj konservi la partajn rezultojn?", L"生未檢。畢而存其半果乎？", L"คนยังไม่ได้ตรวจ จะเสร็จสิ้นและเก็บผลบางส่วนหรือไม่", L"estudyante ang hindi pa nasuri. Tapusin at panatilihin ang bahagyang resulta?", L"öğrenci incelenmedi. Bitirip kısmi sonuçlar tutulsun mu?", L"mokiniai nepatikrinti. Baigti ir palikti dalinius rezultatus?", L"elever er ikke kontrollert. Fullføre og beholde delresultatene?", L"học sinh chưa được kiểm tra. Hoàn tất và giữ kết quả một phần?", L"名學生尚未檢查。是否完成並保留部分結果？"},
        {L"Quick roll call changes were applied.", L"快速点名修改已应用。", L"Il-bidliet tas-sejħa rapida ġew applikati.", L"クイック点呼の変更を適用しました。", L"Les modifications de l'appel rapide ont été appliquées.", L"Die Änderungen wurden übernommen.", L"Изменения быстрой переклички применены.", L"快速點名修改已套用。", L"Se aplicaron los cambios del pase rápido.", L"Le modifiche dell'appello rapido sono state applicate.", L"Шуурхай бүртгэлийн өөрчлөлтийг хэрэгжүүллээ.", L"La ŝanĝoj de rapida nomvoko estis aplikitaj.", L"速點之改已施。", L"ใช้การเปลี่ยนแปลงเช็กชื่อด่วนแล้ว", L"Nailapat ang mga pagbabago sa mabilis na roll call.", L"Hızlı yoklama değişiklikleri uygulandı.", L"Greitos patikros pakeitimai pritaikyti.", L"Endringene fra hurtig opprop er brukt.", L"Đã áp dụng thay đổi điểm danh nhanh.", L"快速點名修改已套用。"},
        {L"Keyboard: 1 Present, 2 Absent, 3 Late, 4 Other, Left Previous, Right Skip", L"键盘：1 出席，2 缺席，3 迟到，4 其他，左方向键上一位，右方向键跳过", L"Tastiera: 1 Preżenti, 2 Assenti, 3 Tard, 4 Ieħor, Xellug Qabel, Lemin Aqbeż", L"キー：1 出席、2 欠席、3 遅刻、4 その他、左 前へ、右 スキップ", L"Clavier : 1 Présent, 2 Absent, 3 Retard, 4 Autre, Gauche Précédent, Droite Passer", L"Tasten: 1 Anwesend, 2 Abwesend, 3 Verspätet, 4 Sonstiges, Links Zurück, Rechts Überspringen", L"Клавиши: 1 Присутствует, 2 Отсутствует, 3 Опоздал, 4 Другое, Влево Назад, Вправо Пропустить", L"鍵盤：1 出席，2 缺席，3 遲到，4 其他，左鍵上一位，右鍵略過", L"Teclado: 1 Presente, 2 Ausente, 3 Tarde, 4 Otro, Izquierda Anterior, Derecha Omitir", L"Tasti: 1 Presente, 2 Assente, 3 Ritardo, 4 Altro, Sinistra Precedente, Destra Salta", L"Товч: 1 Ирсэн, 2 Тасалсан, 3 Хоцорсон, 4 Бусад, Зүүн Өмнөх, Баруун Алгасах", L"Klavoj: 1 Ĉeestas, 2 Forestas, 3 Malfrua, 4 Alia, Maldekstre Antaŭa, Dekstre Preterlasi", L"鍵：一出席，二缺席，三遲，四他，左前，右略", L"แป้น: 1 มา, 2 ขาด, 3 สาย, 4 อื่น, ซ้าย ก่อนหน้า, ขวา ข้าม", L"Key: 1 Present, 2 Absent, 3 Late, 4 Iba pa, Kaliwa Nakaraan, Kanan Laktawan", L"Tuşlar: 1 Var, 2 Yok, 3 Geç, 4 Diğer, Sol Önceki, Sağ Atla", L"Klavišai: 1 Yra, 2 Nėra, 3 Vėluoja, 4 Kita, Kairė Ankstesnis, Dešinė Praleisti", L"Taster: 1 Tilstede, 2 Fravær, 3 Sen, 4 Annet, Venstre Forrige, Høyre Hopp over", L"Phím: 1 Có mặt, 2 Vắng, 3 Muộn, 4 Khác, Trái Trước, Phải Bỏ qua", L"鍵盤：1 出席，2 缺席，3 遲到，4 其他，左鍵上一位，右鍵略過"},
        {L"Type a command: save, import, export, backup, stats, risk, filter, lesson, rollcall, review, profile, settings", L"输入命令：save、import、export、backup、stats、risk、filter、lesson、rollcall、点名、review、profile、settings", L"Ikteb kmand: save, import, export, backup, stats, risk, filter, lesson, rollcall, review, profile, settings", L"コマンドを入力：save、import、export、backup、stats、risk、filter、lesson、rollcall、review、profile、settings", L"Saisissez une commande : save, import, export, backup, stats, risk, filter, lesson, rollcall, review, profile, settings", L"Befehl eingeben: save, import, export, backup, stats, risk, filter, lesson, rollcall, review, profile, settings", L"Введите команду: save, import, export, backup, stats, risk, filter, lesson, rollcall, review, profile, settings", L"輸入命令：save、import、export、backup、stats、risk、filter、lesson、rollcall、點名、review、profile、settings", L"Escriba un comando: save, import, export, backup, stats, risk, filter, lesson, rollcall, review, profile, settings", L"Digita un comando: save, import, export, backup, stats, risk, filter, lesson, rollcall, review, profile, settings", L"Тушаал оруулна уу: save, import, export, backup, stats, risk, filter, lesson, rollcall, review, profile, settings", L"Tajpu komandon: save, import, export, backup, stats, risk, filter, lesson, rollcall, review, profile, settings", L"輸命：save、import、export、backup、stats、risk、filter、lesson、rollcall、review、profile、settings", L"พิมพ์คำสั่ง: save, import, export, backup, stats, risk, filter, lesson, rollcall, review, profile, settings", L"Mag-type ng command: save, import, export, backup, stats, risk, filter, lesson, rollcall, review, profile, settings", L"Komut girin: save, import, export, backup, stats, risk, filter, lesson, rollcall, review, profile, settings", L"Įveskite komandą: save, import, export, backup, stats, risk, filter, lesson, rollcall, review, profile, settings", L"Skriv en kommando: save, import, export, backup, stats, risk, filter, lesson, rollcall, review, profile, settings", L"Nhập lệnh: save, import, export, backup, stats, risk, filter, lesson, rollcall, review, profile, settings", L"輸入指令：save、import、export、backup、stats、risk、filter、lesson、rollcall、點名、review、profile、settings"},
        {L"Default Course", L"默认课程", L"Kors awtomatiku", L"既定コース", L"Cours par défaut", L"Standardkurs", L"Курс по умолчанию", L"預設課程", L"Curso predeterminado", L"Corso predefinito", L"Үндсэн хичээл", L"Defaŭlta kurso", L"常課", L"รายวิชาเริ่มต้น", L"Default na kurso", L"Varsayılan ders", L"Numatytasis kursas", L"Standardkurs", L"Khóa học mặc định", L"預設課程"}
    };

    for (const auto& entry : entries) {
        if (key != entry.key) continue;
        switch (g_language) {
        case UiLanguage::ChineseSimplified: out = entry.zh; break;
        case UiLanguage::Maltese: out = entry.mt; break;
        case UiLanguage::Japanese: out = entry.ja; break;
        case UiLanguage::French: out = entry.fr; break;
        case UiLanguage::German: out = entry.de; break;
        case UiLanguage::Russian: out = entry.ru; break;
        case UiLanguage::ChineseTraditional: out = entry.zht; break;
        case UiLanguage::Spanish: out = entry.es; break;
        case UiLanguage::Italian: out = entry.it; break;
        case UiLanguage::Mongolian: out = entry.mn; break;
        case UiLanguage::Esperanto: out = entry.eo; break;
        case UiLanguage::ClassicalChinese: out = entry.lzh; break;
        case UiLanguage::Thai: out = entry.th; break;
        case UiLanguage::Filipino: out = entry.fil; break;
        case UiLanguage::Turkish: out = entry.tr; break;
        case UiLanguage::Lithuanian: out = entry.lt; break;
        case UiLanguage::Norwegian: out = entry.no; break;
        case UiLanguage::Vietnamese: out = entry.vi; break;
        case UiLanguage::ChineseTraditionalHongKong: out = entry.zhhk; break;
        default: return false;
        }
        return true;
    }
    return false;
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

std::wstring Tr(const wchar_t* english, const wchar_t* chinese) {
    std::wstring key = english;
    if (g_language == UiLanguage::English) return key;
    std::wstring additional;
    if (TranslateFeatureText(key, additional)) return additional;
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
    // Every user-visible call site supplies a Chinese fallback. Keep non-English
    // modes readable while a legacy string is being promoted into a language pack.
    return chinese && *chinese ? std::wstring(chinese) : key;
}

std::wstring ParticleEffectsLabel() {
    switch (g_language) {
    case UiLanguage::ChineseSimplified: return L"\u542f\u7528\u7c92\u5b50\u7279\u6548";
    case UiLanguage::Maltese: return L"Ixg\u0127el effetti tal-parti\u010belli";
    case UiLanguage::Japanese: return L"\u30d1\u30fc\u30c6\u30a3\u30af\u30eb\u52b9\u679c\u3092\u6709\u52b9\u5316";
    case UiLanguage::French: return L"Activer les effets de particules";
    case UiLanguage::German: return L"Partikeleffekte aktivieren";
    case UiLanguage::Russian: return L"\u0412\u043a\u043b\u044e\u0447\u0438\u0442\u044c \u044d\u0444\u0444\u0435\u043a\u0442\u044b \u0447\u0430\u0441\u0442\u0438\u0446";
    case UiLanguage::ChineseTraditional: return L"\u555f\u7528\u7c92\u5b50\u7279\u6548";
    case UiLanguage::Spanish: return L"Activar efectos de part\u00edculas";
    case UiLanguage::Italian: return L"Attiva effetti particellari";
    case UiLanguage::Mongolian: return L"\u0411\u04e9\u04e9\u043c\u0441\u0438\u0439\u043d \u044d\u0444\u0444\u0435\u043a\u0442\u0438\u0439\u0433 \u0438\u0434\u044d\u0432\u0445\u0436\u04af\u04af\u043b\u044d\u0445";
    case UiLanguage::Esperanto: return L"\u015calti partiklo-efektojn";
    case UiLanguage::ClassicalChinese: return L"\u555f\u7c92\u5b50\u4e4b\u6548";
    case UiLanguage::Thai: return L"\u0e40\u0e1b\u0e34\u0e14\u0e40\u0e2d\u0e1f\u0e40\u0e1f\u0e01\u0e15\u0e4c\u0e2d\u0e19\u0e38\u0e20\u0e32\u0e04";
    case UiLanguage::Filipino: return L"I-enable ang particle effects";
    case UiLanguage::Turkish: return L"Par\u00e7ac\u0131k efektlerini etkinle\u015ftir";
    case UiLanguage::Lithuanian: return L"\u012ejungti daleli\u0173 efektus";
    case UiLanguage::Norwegian: return L"Sl\u00e5 p\u00e5 partikkeleffekter";
    case UiLanguage::Vietnamese: return L"B\u1eadt hi\u1ec7u \u1ee9ng h\u1ea1t";
    case UiLanguage::ChineseTraditionalHongKong: return L"\u555f\u7528\u7c92\u5b50\u7279\u6548";
    default: return L"Enable particle effects";
    }
}

std::wstring AnimationLevelName(AnimationLevel level) {
    switch (level) {
    case AnimationLevel::Off: return Tr(L"Off", L"\u5173\u95ed");
    case AnimationLevel::Standard: return Tr(L"Standard", L"\u6807\u51c6");
    default: return Tr(L"Advanced", L"\u9ad8\u7ea7");
    }
}

std::wstring ParticleLevelName(ParticleLevel level) {
    switch (level) {
    case ParticleLevel::Low: return Tr(L"Low", L"\u4f4e");
    case ParticleLevel::High: return Tr(L"High", L"\u9ad8");
    default: return Tr(L"Medium", L"\u4e2d");
    }
}

std::wstring ReportTemplateName(ReportTemplate templateType) {
    switch (templateType) {
    case ReportTemplate::Simple: return Tr(L"Simple", L"\u7b80\u6d01\u7248");
    case ReportTemplate::Teacher: return Tr(L"Teacher", L"\u6559\u5e08\u7248");
    case ReportTemplate::Parent: return Tr(L"Parent", L"\u5bb6\u957f\u7248");
    default: return Tr(L"Complete", L"\u5b8c\u6574\u7248");
    }
}

std::wstring StatsRangeName(StatsRange range) {
    switch (range) {
    case StatsRange::ThisWeek: return Tr(L"This week", L"\u672c\u5468");
    case StatsRange::ThisMonth: return Tr(L"This month", L"\u672c\u6708");
    default: return Tr(L"All time", L"\u5168\u90e8");
    }
}

HFONT CreateUiFont(int height, int weight) {
    return CreateFontW(height, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, g_fontFamily.c_str());
}

int ComboItemHeight() {
    if (g_textScale == TextScale::Small) return 28;
    if (g_textScale == TextScale::Large) return 36;
    return 30;
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

int ClampSettingInt(const std::string& value, int fallback, int minValue, int maxValue) {
    try {
        return std::clamp(std::stoi(value), minValue, maxValue);
    } catch (...) {
        return fallback;
    }
}

std::string BoolSetting(bool value) {
    return value ? "1" : "0";
}

void SaveSettings() {
    auto path = SettingsFilePath();
    if (path.empty()) return;
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    if (!file) return;
    file << "language=" << LanguageToString(g_language) << "\n";
    file << "advanced_personalization=" << BoolSetting(g_advancedPersonalizationEnabled) << "\n";
    file << "theme=" << (int)g_theme << "\n";
    file << "accent_tone=" << (int)g_accentTone << "\n";
    file << "text_scale=" << (int)g_textScale << "\n";
    file << "layout_density=" << (int)g_layoutDensity << "\n";
    file << "show_stats_cards=" << BoolSetting(g_showStatsCards) << "\n";
    file << "show_footer_hint=" << BoolSetting(g_showFooterHint) << "\n";
    file << "show_table_grid=" << BoolSetting(g_showTableGrid) << "\n";
    file << "autosave_interval=" << g_autosaveIntervalSeconds << "\n";
    file << "font=" << WideToUtf8(g_fontFamily) << "\n";
    file << "default_save_dir=" << WideToUtf8(g_defaultSaveDir) << "\n";
    file << "particles=" << (g_particlesEnabled ? "1" : "0") << "\n";
    file << "animation_level=" << (int)g_animationLevel << "\n";
    file << "particle_level=" << (int)g_particleLevel << "\n";
    file << "report_template=" << (int)g_reportTemplate << "\n";
    file << "risk_alerts=" << BoolSetting(g_riskAlertsEnabled) << "\n";
    file << "autosave_prompt=" << BoolSetting(g_autosavePromptEnabled) << "\n";
    file << "command_palette=" << BoolSetting(g_commandPaletteEnabled) << "\n";
    file << "advanced_filter=" << BoolSetting(g_advancedFilterEnabled) << "\n";
    file << "shortcut_save=" << WideToUtf8(g_shortcutSave) << "\n";
    file << "shortcut_import=" << WideToUtf8(g_shortcutImport) << "\n";
    file << "shortcut_undo=" << WideToUtf8(g_shortcutUndo) << "\n";
    file << "shortcut_redo=" << WideToUtf8(g_shortcutRedo) << "\n";
    file << "shortcut_command=" << WideToUtf8(g_shortcutCommand) << "\n";
    file << "shortcut_fullscreen=" << WideToUtf8(g_shortcutFullscreen) << "\n";
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
        else if (key == "advanced_personalization") g_advancedPersonalizationEnabled = value != "0";
        else if (key == "theme") g_theme = (value == "light" || value == "1") ? UiTheme::Light : UiTheme::Dark;
        else if (key == "accent_tone") g_accentTone = (AccentTone)ClampSettingInt(value, 0, 0, 2);
        else if (key == "text_scale") g_textScale = (TextScale)ClampSettingInt(value, 1, 0, 2);
        else if (key == "layout_density") g_layoutDensity = (LayoutDensity)ClampSettingInt(value, 1, 0, 2);
        else if (key == "show_stats_cards") g_showStatsCards = value != "0";
        else if (key == "show_footer_hint") g_showFooterHint = value != "0";
        else if (key == "show_table_grid") g_showTableGrid = value != "0";
        else if (key == "autosave_interval") {
            int seconds = ClampSettingInt(value, 30, 0, 300);
            g_autosaveIntervalSeconds = seconds == 0 || seconds == 30 || seconds == 60 || seconds == 300 ? seconds : 30;
        }
        else if (key == "font" && !value.empty()) g_fontFamily = Utf8ToWide(value);
        else if (key == "default_save_dir") g_defaultSaveDir = Utf8ToWide(value);
        else if (key == "particles") g_particlesEnabled = value != "0";
        else if (key == "animation_level") g_animationLevel = (AnimationLevel)ClampSettingInt(value, (int)AnimationLevel::Advanced, 0, 2);
        else if (key == "particle_level") g_particleLevel = (ParticleLevel)ClampSettingInt(value, (int)ParticleLevel::Medium, 0, 2);
        else if (key == "report_template") g_reportTemplate = (ReportTemplate)ClampSettingInt(value, (int)ReportTemplate::Complete, 0, 3);
        else if (key == "risk_alerts") g_riskAlertsEnabled = value != "0";
        else if (key == "autosave_prompt") g_autosavePromptEnabled = value != "0";
        else if (key == "command_palette") g_commandPaletteEnabled = value != "0";
        else if (key == "advanced_filter") g_advancedFilterEnabled = value != "0";
        else if (key == "shortcut_save") g_shortcutSave = NormalizeShortcut(Utf8ToWide(value), L"Ctrl+S");
        else if (key == "shortcut_import") g_shortcutImport = NormalizeShortcut(Utf8ToWide(value), L"Ctrl+O");
        else if (key == "shortcut_undo") g_shortcutUndo = NormalizeShortcut(Utf8ToWide(value), L"Ctrl+Z");
        else if (key == "shortcut_redo") g_shortcutRedo = NormalizeShortcut(Utf8ToWide(value), L"Ctrl+Y");
        else if (key == "shortcut_command") g_shortcutCommand = NormalizeShortcut(Utf8ToWide(value), L"Ctrl+K");
        else if (key == "shortcut_fullscreen") g_shortcutFullscreen = NormalizeShortcut(Utf8ToWide(value), L"F11");
    }
    if (!g_advancedPersonalizationEnabled) {
        g_theme = UiTheme::Dark;
        g_accentTone = AccentTone::Neutral;
        g_textScale = TextScale::Standard;
        g_layoutDensity = LayoutDensity::Comfortable;
        g_showStatsCards = true;
        g_showFooterHint = true;
        g_showTableGrid = true;
        g_autosaveIntervalSeconds = 30;
    }
}

void ResetSettings() {
    auto path = SettingsFilePath();
    if (!path.empty()) {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
    g_language = UiLanguage::English;
    g_theme = UiTheme::Dark;
    g_accentTone = AccentTone::Neutral;
    g_textScale = TextScale::Standard;
    g_layoutDensity = LayoutDensity::Comfortable;
    g_fontFamily = L"Segoe UI";
    g_defaultSaveDir.clear();
    g_particlesEnabled = true;
    g_animationLevel = AnimationLevel::Advanced;
    g_particleLevel = ParticleLevel::Medium;
    g_reportTemplate = ReportTemplate::Complete;
    g_riskAlertsEnabled = true;
    g_autosavePromptEnabled = true;
    g_commandPaletteEnabled = true;
    g_advancedFilterEnabled = true;
    g_advancedPersonalizationEnabled = false;
    g_showStatsCards = true;
    g_showFooterHint = true;
    g_showTableGrid = true;
    g_autosaveIntervalSeconds = 30;
    g_shortcutSave = L"Ctrl+S";
    g_shortcutImport = L"Ctrl+O";
    g_shortcutUndo = L"Ctrl+Z";
    g_shortcutRedo = L"Ctrl+Y";
    g_shortcutCommand = L"Ctrl+K";
    g_shortcutFullscreen = L"F11";
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
    ss << "ATTENDANCE_V5\n";
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
    if (header != "ATTENDANCE_V2" && header != "ATTENDANCE_V3" && header != "ATTENDANCE_V4" && header != "ATTENDANCE_V5") return false;

    std::string countLine;
    if (header == "ATTENDANCE_V3" || header == "ATTENDANCE_V4" || header == "ATTENDANCE_V5") {
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
        if (header == "ATTENDANCE_V4" || header == "ATTENDANCE_V5") {
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
    int totalInRange = 0;
    for (const auto& record : g_records) {
        if (!RecordMatchesStatsRange(record)) continue;
        ++totalInRange;
        if (record.status == L"Present") ++present;
        else if (record.status == L"Absent") ++absent;
        else if (record.status == L"Late") ++late;
        else ++other;
    }

    double attendanceRate = totalInRange == 0 ? 0.0 : (present * 100.0 / totalInRange);
    double issueRate = totalInRange == 0 ? 0.0 : ((absent + late) * 100.0 / totalInRange);
    std::wstringstream ss;
    ss << StatsRangeName(g_statsRange) << L"  |  "
       << Tr(L"Total", L"\u603b\u6570") << L" " << totalInRange
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
    totalCard << StatsRangeName(g_statsRange) << L"\n" << totalInRange;
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
        const std::wstring displayName = sheet.name == L"Default Course"
            ? Tr(L"Default Course", L"默认课程")
            : sheet.name;
        SendMessageW(g_courseCombo, CB_ADDSTRING, 0, (LPARAM)displayName.c_str());
    }
    SendMessageW(g_courseCombo, CB_SETCURSEL, g_activeSheet, 0);
    RedrawWindow(g_courseCombo, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
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

int ThemedMenuContentTop(const ThemedMenuState* state) {
    (void)state;
    return 0;
}

int ThemedMenuContentBottom(const ThemedMenuState* state) {
    if (!state) return 0;
    return state->height;
}

int ThemedMenuHitTest(ThemedMenuState* state, int y) {
    if (!state || y < ThemedMenuContentTop(state) || y >= ThemedMenuContentBottom(state)) return -1;
    int contentY = y - ThemedMenuContentTop(state) + state->scrollY;
    for (int i = 0; i < (int)state->items.size(); ++i) {
        int top = ThemedMenuItemTop(state, i);
        int height = state->items[i].separator ? THEMED_MENU_SEPARATOR_H : THEMED_MENU_ITEM_H;
        if (contentY >= top && contentY < top + height) {
            return state->items[i].separator ? -1 : i;
        }
    }
    return -1;
}

void ScrollThemedMenu(HWND hwnd, ThemedMenuState* state, int delta) {
    if (!hwnd || !state || !state->scrollable || delta == 0) return;
    int next = std::clamp(state->scrollY + delta, 0, state->maxScrollY);
    if (next == state->scrollY) return;
    state->scrollY = next;
    if (!state->keyboardMode) state->hover = -1;
    StartAnimation(hwnd, AnimChannel::Hover, 0.0, 80);
    InvalidateRect(hwnd, nullptr, FALSE);
}

int FindThemedMenuItem(const ThemedMenuState* state, int start, int direction) {
    if (!state || state->items.empty() || direction == 0) return -1;
    int index = start;
    for (int tries = 0; tries < (int)state->items.size(); ++tries) {
        index += direction;
        if (index < 0 || index >= (int)state->items.size()) return -1;
        if (!state->items[index].separator) return index;
    }
    return -1;
}

int FirstThemedMenuItem(const ThemedMenuState* state) {
    return FindThemedMenuItem(state, -1, 1);
}

int LastThemedMenuItem(const ThemedMenuState* state) {
    return state ? FindThemedMenuItem(state, (int)state->items.size(), -1) : -1;
}

void EnsureThemedMenuItemVisible(HWND hwnd, ThemedMenuState* state, int index) {
    if (!hwnd || !state || index < 0 || index >= (int)state->items.size() || !state->scrollable) return;
    int top = ThemedMenuItemTop(state, index);
    int bottom = top + (state->items[index].separator ? THEMED_MENU_SEPARATOR_H : THEMED_MENU_ITEM_H);
    int viewportHeight = std::max(1, ThemedMenuContentBottom(state) - ThemedMenuContentTop(state));
    int next = state->scrollY;
    if (top < next) next = top;
    else if (bottom > next + viewportHeight) next = bottom - viewportHeight;
    next = std::clamp(next, 0, state->maxScrollY);
    if (next != state->scrollY) {
        state->scrollY = next;
        InvalidateRect(hwnd, nullptr, FALSE);
    }
}

void SelectThemedMenuItem(HWND hwnd, ThemedMenuState* state, int index) {
    if (!hwnd || !state || index < 0 || index >= (int)state->items.size() || state->items[index].separator) return;
    state->keyboardMode = true;
    state->hover = index;
    EnsureThemedMenuItemVisible(hwnd, state, index);
    StartAnimation(hwnd, AnimChannel::Hover, 1.0, 90);
    InvalidateRect(hwnd, nullptr, FALSE);
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
    double reveal = GetAnimationValue(hwnd, AnimChannel::Reveal, 1.0);
    int slideOffset = -(int)std::lround((1.0 - reveal) * THEMED_MENU_SLIDE_PX);
    int top = ThemedMenuItemTop(state, index) - state->scrollY + ThemedMenuContentTop(state) + slideOffset;
    return RECT{6, top, rc.right - 6, top + THEMED_MENU_ITEM_H};
}

void ApplyThemedMenuReveal(HWND hwnd, ThemedMenuState* state) {
    if (!hwnd || !state) return;
    InvalidateRect(hwnd, nullptr, FALSE);
}

void PaintThemedPopupMenuContent(HWND hwnd, HDC hdc, ThemedMenuState* state) {
    RECT rc{};
    GetClientRect(hwnd, &rc);
    double reveal = GetAnimationValue(hwnd, AnimChannel::Reveal, 1.0);
    COLORREF fill = BlendColor(COLOR_BG, COLOR_ROW_ALT, reveal);
    COLORREF border = BlendColor(COLOR_PANEL, COLOR_BORDER, reveal);
    COLORREF hover = COLOR_HOVER;
    int blurRadius = (int)std::lround(reveal * 10.0);

    HBRUSH bg = CreateSolidBrush(fill);
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);

    for (int i = 0; i < blurRadius; i += 3) {
        RECT glow{rc.left + i, rc.top + i, rc.right - i, rc.bottom - i};
        HPEN glowPen = CreatePen(PS_SOLID, 1, BlendColor(COLOR_BG, COLOR_SELECTED, reveal * (1.0 - i / 12.0)));
        HGDIOBJ oldGlowPen = SelectObject(hdc, glowPen);
        HGDIOBJ oldGlowBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
        Rectangle(hdc, glow.left, glow.top, glow.right, glow.bottom);
        SelectObject(hdc, oldGlowBrush);
        SelectObject(hdc, oldGlowPen);
        DeleteObject(glowPen);
    }

    HGDIOBJ oldFont = SelectObject(hdc, g_font);
    SetBkMode(hdc, TRANSPARENT);

    const int contentTop = ThemedMenuContentTop(state);
    const int contentBottom = ThemedMenuContentBottom(state);
    const int slideOffset = -(int)std::lround((1.0 - reveal) * THEMED_MENU_SLIDE_PX);
    SaveDC(hdc);
    IntersectClipRect(hdc, 0, contentTop, rc.right, contentBottom);
    for (int i = 0; i < (int)state->items.size(); ++i) {
        double itemProgress = EaseOut(reveal);
        int top = ThemedMenuItemTop(state, i) - state->scrollY + contentTop + slideOffset;
        int itemHeight = state->items[i].separator ? THEMED_MENU_SEPARATOR_H : THEMED_MENU_ITEM_H;
        if (top + itemHeight <= contentTop || top >= contentBottom) continue;
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
        textRc.right -= state->scrollable ? 26 : 16;
        SetTextColor(hdc, BlendColor(COLOR_MUTED, COLOR_TEXT, itemProgress));
        DrawTextW(hdc, state->items[i].text.c_str(), -1, &textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }
    RestoreDC(hdc, -1);

    if (state->scrollable && state->contentHeight > 0) {
        RECT track{rc.right - 9, rc.top + 8, rc.right - 5, rc.bottom - 8};
        HBRUSH trackBrush = CreateSolidBrush(COLOR_PANEL);
        FillRect(hdc, &track, trackBrush);
        DeleteObject(trackBrush);

        int trackHeight = std::max(1, (int)(track.bottom - track.top));
        int thumbHeight = std::clamp(
            (int)std::lround(trackHeight * (state->height / (double)state->contentHeight)),
            34,
            trackHeight);
        int thumbTravel = std::max(0, trackHeight - thumbHeight);
        int thumbTop = track.top + (state->maxScrollY > 0
            ? (int)std::lround(thumbTravel * (state->scrollY / (double)state->maxScrollY))
            : 0);
        RECT thumb{track.left, thumbTop, track.right, thumbTop + thumbHeight};
        HBRUSH thumbBrush = CreateSolidBrush(COLOR_ACCENT);
        FillRect(hdc, &thumb, thumbBrush);
        DeleteObject(thumbBrush);
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
            POINT mouse{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            bool actuallyMoved = !state->hasMousePoint
                || mouse.x != state->lastMouse.x || mouse.y != state->lastMouse.y;
            state->lastMouse = mouse;
            state->hasMousePoint = true;
            if (state->keyboardMode && !actuallyMoved) return 0;
            state->keyboardMode = false;
            int next = ThemedMenuHitTest(state, mouse.y);
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
    case WM_MOUSEWHEEL:
        if (state && !state->closing && state->scrollable) {
            state->keyboardMode = false;
            state->hover = -1;
            int wheel = GET_WHEEL_DELTA_WPARAM(wParam);
            int rows = wheel / WHEEL_DELTA;
            if (rows == 0) rows = wheel > 0 ? 1 : -1;
            ScrollThemedMenu(hwnd, state, -rows * THEMED_MENU_ITEM_H * 2);
            return 0;
        }
        break;
    case WM_LBUTTONDOWN:
        if (state && !state->closing) {
            POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            RECT rc{};
            GetClientRect(hwnd, &rc);
            if (!PtInRect(&rc, pt)) {
                CloseThemedPopupMenu(hwnd, state, 0);
                return 0;
            }
            if (state->scrollable && pt.x >= rc.right - 16) {
                state->keyboardMode = false;
                state->hover = -1;
                ScrollThemedMenu(hwnd, state, pt.y < rc.bottom / 2 ? -state->height : state->height);
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
        if (state && !state->closing) {
            if (wParam == VK_RETURN || wParam == VK_SPACE) {
                if (state->hover >= 0 && state->hover < (int)state->items.size() && !state->items[state->hover].separator) {
                    CloseThemedPopupMenu(hwnd, state, state->items[state->hover].command);
                }
                return 0;
            }
            if (wParam == VK_HOME) {
                SelectThemedMenuItem(hwnd, state, FirstThemedMenuItem(state));
                return 0;
            }
            if (wParam == VK_END) {
                SelectThemedMenuItem(hwnd, state, LastThemedMenuItem(state));
                return 0;
            }
            if (wParam == VK_UP || wParam == VK_DOWN || wParam == VK_PRIOR || wParam == VK_NEXT) {
                int direction = (wParam == VK_UP || wParam == VK_PRIOR) ? -1 : 1;
                int steps = (wParam == VK_PRIOR || wParam == VK_NEXT) ? 6 : 1;
                int index = state->hover;
                if (index < 0) {
                    index = direction > 0 ? FirstThemedMenuItem(state) : LastThemedMenuItem(state);
                } else {
                    for (int i = 0; i < steps; ++i) {
                        int next = FindThemedMenuItem(state, index, direction);
                        if (next < 0) break;
                        index = next;
                    }
                }
                SelectThemedMenuItem(hwnd, state, index);
                return 0;
            }
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
    wc.style = CS_HREDRAW | CS_VREDRAW;
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
    return std::min(width, 460);
}

int ShowThemedPopupMenu(HWND button, const std::vector<ThemedMenuItem>& items) {
    if (!button || items.empty()) return 0;
    ThemedMenuState state;
    state.items = items;
    state.mainScrollX = g_scrollX;
    state.mainScrollY = g_scrollY;
    ++g_openThemedPopupMenus;
    RegisterThemedMenuClass();

    RECT buttonRc{};
    GetWindowRect(button, &buttonRc);
    int width = MeasureThemedMenuWidth(items);
    state.contentHeight = ThemedMenuTotalHeight(&state);
    int height = state.contentHeight;
    int x = buttonRc.right - width;
    int y = buttonRc.bottom + 4;

    HMONITOR monitor = MonitorFromRect(&buttonRc, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfoW(monitor, &mi)) {
        int workHeight = (int)(mi.rcWork.bottom - mi.rcWork.top);
        int maxHeight = std::max(160, std::min(THEMED_MENU_MAX_H, workHeight - 16));
        if (height > maxHeight) {
            state.scrollable = true;
            height = maxHeight;
            state.maxScrollY = std::max(0, state.contentHeight - height);
        }
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
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
        L"AttendanceThemedPopupMenu", L"",
        WS_POPUP,
        state.finalX, state.finalY, width, height,
        g_hwnd, nullptr, GetModuleHandleW(nullptr), &state
    );
    if (!popup) {
        --g_openThemedPopupMenus;
        return 0;
    }

    StartWindowReveal(popup, 180);
    ShowWindow(popup, SW_SHOWNOACTIVATE);
    ApplyThemedMenuReveal(popup, &state);
    SetCapture(popup);
    SetFocus(popup);

    MSG msg{};
    while (!state.done && IsWindow(popup) && GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if ((msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN || msg.message == WM_MOUSEWHEEL)
            && msg.hwnd != popup) {
            SendMessageW(popup, msg.message, msg.wParam, msg.lParam);
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    --g_openThemedPopupMenus;
    if (IsWindow(popup)) DestroyWindow(popup);
    InvalidateRect(button, nullptr, FALSE);
    if (g_scrollX != state.mainScrollX || g_scrollY != state.mainScrollY) {
        g_scrollX = state.mainScrollX;
        g_scrollY = state.mainScrollY;
        ResizeLayout(g_hwnd);
        InvalidateRect(g_hwnd, nullptr, FALSE);
    }
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
    if (!g_advancedPersonalizationEnabled) g_theme = UiTheme::Dark;
    if (g_theme == UiTheme::Light) {
        COLOR_BG = RGB(255, 255, 255);
        COLOR_PANEL = RGB(246, 246, 246);
        COLOR_INPUT = RGB(250, 250, 250);
        COLOR_TEXT = RGB(0, 0, 0);
        COLOR_MUTED = RGB(58, 58, 58);
        COLOR_DANGER = RGB(150, 74, 74);
        COLOR_BORDER = RGB(194, 194, 194);
        COLOR_HOVER = RGB(232, 232, 232);
        COLOR_CARD = RGB(250, 250, 250);
        COLOR_ROW_ALT = RGB(240, 240, 240);
        COLOR_SELECTED = RGB(220, 228, 230);
        COLOR_CHECK_FILL = RGB(255, 255, 255);
        if (g_accentTone == AccentTone::Sage) COLOR_ACCENT = RGB(76, 112, 96);
        else if (g_accentTone == AccentTone::Cyan) COLOR_ACCENT = RGB(52, 112, 128);
        else COLOR_ACCENT = RGB(78, 78, 78);
    } else {
        COLOR_BG = RGB(0, 0, 0);
        COLOR_PANEL = RGB(18, 18, 18);
        COLOR_INPUT = RGB(26, 26, 26);
        COLOR_TEXT = RGB(255, 255, 255);
        COLOR_MUTED = RGB(224, 224, 224);
        COLOR_DANGER = RGB(176, 112, 112);
        COLOR_BORDER = RGB(64, 64, 64);
        COLOR_HOVER = RGB(38, 38, 38);
        COLOR_CARD = RGB(16, 16, 16);
        COLOR_ROW_ALT = RGB(14, 14, 14);
        COLOR_SELECTED = RGB(42, 42, 42);
        COLOR_CHECK_FILL = RGB(0, 0, 0);
        if (g_accentTone == AccentTone::Sage) COLOR_ACCENT = RGB(142, 166, 154);
        else if (g_accentTone == AccentTone::Cyan) COLOR_ACCENT = RGB(112, 174, 188);
        else COLOR_ACCENT = RGB(188, 188, 188);
    }
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
    wchar_t className[32]{};
    GetClassNameW(child, className, 32);
    if (lstrcmpiW(className, L"ComboBox") == 0) {
        int itemHeight = ComboItemHeight();
        SendMessageW(child, CB_SETITEMHEIGHT, (WPARAM)-1, itemHeight);
        SendMessageW(child, CB_SETITEMHEIGHT, 0, itemHeight);
    }
    return TRUE;
}

void RecreateFonts() {
    HFONT oldFont = g_font;
    HFONT oldTitleFont = g_titleFont;
    HFONT oldSmallFont = g_smallFont;

    double scale = g_textScale == TextScale::Small ? 0.90
        : g_textScale == TextScale::Large ? 1.15 : 1.0;
    g_font = CreateUiFont((int)std::lround(20 * scale), FW_NORMAL);
    g_titleFont = CreateUiFont((int)std::lround(30 * scale), FW_SEMIBOLD);
    g_smallFont = CreateUiFont((int)std::lround(17 * scale), FW_NORMAL);

    const HWND roots[] = {g_hwnd, g_settingsWindow, g_chartWindow, g_quickRollCallWindow};
    for (HWND root : roots) {
        if (root && IsWindow(root)) EnumChildWindows(root, ApplyFontToChild, 0);
    }
    if (g_hwnd && IsWindow(g_hwnd)) {
        SendMessageW(GetDlgItem(g_hwnd, IDC_TITLE), WM_SETFONT, (WPARAM)g_titleFont, TRUE);
        SendMessageW(GetDlgItem(g_hwnd, IDC_SUBTITLE), WM_SETFONT, (WPARAM)g_smallFont, TRUE);
        SendMessageW(GetDlgItem(g_hwnd, IDC_STATS), WM_SETFONT, (WPARAM)g_smallFont, TRUE);
        SendMessageW(GetDlgItem(g_hwnd, IDC_HINT), WM_SETFONT, (WPARAM)g_smallFont, TRUE);
        SendMessageW(GetDlgItem(g_hwnd, IDC_STAT_TOTAL), WM_SETFONT, (WPARAM)g_smallFont, TRUE);
        SendMessageW(GetDlgItem(g_hwnd, IDC_STAT_ATTENDANCE), WM_SETFONT, (WPARAM)g_smallFont, TRUE);
        SendMessageW(GetDlgItem(g_hwnd, IDC_STAT_ISSUES), WM_SETFONT, (WPARAM)g_smallFont, TRUE);
        SendMessageW(GetDlgItem(g_hwnd, IDC_STAT_VISIBLE), WM_SETFONT, (WPARAM)g_smallFont, TRUE);
    }
    if (g_settingsWindow && IsWindow(g_settingsWindow)) {
        SendMessageW(GetDlgItem(g_settingsWindow, IDC_SETTINGS_TITLE), WM_SETFONT, (WPARAM)g_titleFont, TRUE);
        SendMessageW(GetDlgItem(g_settingsWindow, IDC_SETTINGS_ADVANCED_TITLE), WM_SETFONT, (WPARAM)g_titleFont, TRUE);
    }
    if (g_quickRollCallWindow && IsWindow(g_quickRollCallWindow)) {
        SendMessageW(GetDlgItem(g_quickRollCallWindow, IDC_QUICK_TITLE), WM_SETFONT, (WPARAM)g_titleFont, TRUE);
        SendMessageW(GetDlgItem(g_quickRollCallWindow, IDC_QUICK_STUDENT), WM_SETFONT, (WPARAM)g_titleFont, TRUE);
        SendMessageW(GetDlgItem(g_quickRollCallWindow, IDC_QUICK_HINT), WM_SETFONT, (WPARAM)g_smallFont, TRUE);
    }

    if (oldFont) DeleteObject(oldFont);
    if (oldTitleFont) DeleteObject(oldTitleFont);
    if (oldSmallFont) DeleteObject(oldSmallFont);
}

BOOL CALLBACK ApplyThemeToChild(HWND child, LPARAM) {
    SetWindowTheme(child, g_theme == UiTheme::Dark ? L"DarkMode_Explorer" : L"Explorer", nullptr);
    wchar_t className[32]{};
    GetClassNameW(child, className, 32);
    if (lstrcmpiW(className, L"ComboBox") == 0) ApplyComboDropDownTheme(child);
    InvalidateRect(child, nullptr, FALSE);
    return TRUE;
}

void ApplyThemedControls(HWND root) {
    if (!root) return;
    SetWindowTheme(root, g_theme == UiTheme::Dark ? L"DarkMode_Explorer" : L"Explorer", nullptr);
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
    BOOL dark = g_theme == UiTheme::Dark ? TRUE : FALSE;
    if (g_hwnd) DwmSetWindowAttribute(g_hwnd, 20, &dark, sizeof(dark));
    if (g_settingsWindow) DwmSetWindowAttribute(g_settingsWindow, 20, &dark, sizeof(dark));
    if (g_chartWindow) DwmSetWindowAttribute(g_chartWindow, 20, &dark, sizeof(dark));
    ApplyGlassTitleBar(g_hwnd);
    ApplyGlassTitleBar(g_settingsWindow);
    ApplyGlassTitleBar(g_chartWindow);
    ApplyGlassTitleBar(g_quickRollCallWindow);
    ApplyThemedControls(g_hwnd);
    ApplyThemedControls(g_settingsWindow);
    ApplyThemedControls(g_chartWindow);
    ApplyThemedControls(g_quickRollCallWindow);
    if (g_settingsWindow) RedrawWindow(g_settingsWindow, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME | RDW_NOERASE);
    if (g_chartWindow) RedrawWindow(g_chartWindow, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME | RDW_NOERASE);
    if (g_quickRollCallWindow) RedrawWindow(g_quickRollCallWindow, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME | RDW_NOERASE);
    if (g_hwnd) {
        ShowWindow(GetDlgItem(g_hwnd, IDC_STAT_TOTAL), g_showStatsCards ? SW_SHOW : SW_HIDE);
        ShowWindow(GetDlgItem(g_hwnd, IDC_STAT_ATTENDANCE), g_showStatsCards ? SW_SHOW : SW_HIDE);
        ShowWindow(GetDlgItem(g_hwnd, IDC_STAT_ISSUES), g_showStatsCards ? SW_SHOW : SW_HIDE);
        ShowWindow(GetDlgItem(g_hwnd, IDC_STAT_VISIBLE), g_showStatsCards ? SW_SHOW : SW_HIDE);
        ShowWindow(GetDlgItem(g_hwnd, IDC_HINT), g_showFooterHint ? SW_SHOW : SW_HIDE);
        KillTimer(g_hwnd, 1);
        if (g_autosaveIntervalSeconds > 0) {
            SetTimer(g_hwnd, 1, (UINT)g_autosaveIntervalSeconds * 1000, nullptr);
        }
        InvalidateRect(g_hwnd, nullptr, FALSE);
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
    for (const auto& r : records) {
        if (!RecordMatchesStatsRange(r)) continue;
        ++stats.total;
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

std::string BuildPptSlide1(const AttendanceSheet& sheet, const PptStats& stats) {
    int id = 3;
    double rate = stats.total ? stats.present * 100.0 / stats.total : 0.0;
    std::wstringstream rateText;
    rateText.setf(std::ios::fixed);
    rateText.precision(1);
    rateText << rate << L"%";
    std::wstring title = Tr(L"Attendance Manager", L"\u70b9\u540d\u7ba1\u7406\u5668");
    if (g_reportTemplate == ReportTemplate::Simple) title = Tr(L"Attendance Summary", L"\u51fa\u52e4\u6458\u8981");
    else if (g_reportTemplate == ReportTemplate::Teacher) title = Tr(L"Course Attendance Report", L"\u8bfe\u7a0b\u51fa\u52e4\u62a5\u544a");
    else if (g_reportTemplate == ReportTemplate::Parent) title = Tr(L"Student Attendance Report", L"\u5b66\u751f\u51fa\u52e4\u62a5\u544a");

    std::wstring footer = Tr(L"Generated by AttendanceApp", L"\u7531 AttendanceApp \u751f\u6210");
    if (g_reportTemplate == ReportTemplate::Teacher || g_reportTemplate == ReportTemplate::Complete) {
        footer.clear();
        if (!sheet.teacher.empty()) footer += Tr(L"Teacher/owner:", L"\u6559\u5e08/\u8d1f\u8d23\u4eba\uff1a") + L" " + sheet.teacher + L"  ";
        if (!sheet.location.empty()) footer += Tr(L"Location:", L"\u5730\u70b9\uff1a") + L" " + sheet.location + L"  ";
        if (!sheet.notes.empty()) footer += Tr(L"Course notes:", L"\u8bfe\u7a0b\u5907\u6ce8\uff1a") + L" " + sheet.notes;
        if (footer.empty()) footer = Tr(L"Teacher template with course details enabled.", L"\u6559\u5e08\u7248\u62a5\u544a\u5df2\u542f\u7528\u8bfe\u7a0b\u8be6\u60c5\u3002");
    } else if (g_reportTemplate == ReportTemplate::Parent) {
        footer = Tr(L"Roster students", L"\u540d\u5355\u5b66\u751f") + L" " + std::to_wstring(sheet.students.size()) + L" | "
            + Tr(L"Attendance", L"\u51fa\u52e4\u7387") + L" " + rateText.str();
    }

    std::ostringstream ss;
    ss << SlideBackground();
    ss << ShapeText(id++, 760000, 520000, 8000000, 340000, ReportTemplateName(g_reportTemplate) + L" | " + StatsRangeName(g_statsRange), 1300, "BDBDBD", true);
    ss << ShapeText(id++, 740000, 900000, 8400000, 700000, title, 3400, "FFFFFF", true);
    ss << ShapeText(id++, 760000, 1600000, 7600000, 420000, sheet.name, 1600, "E0E0E0");
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

    ss << ShapeText(id++, 760000, 5950000, 10400000, 340000, footer, 1050, "B8B8B8");
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
        if (!RecordMatchesStatsRange(r)) continue;
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
    entries.push_back({"ppt/slides/slide1.xml", BuildPptSlide1(sheet, stats)});
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
    file << "<p><strong>" << HtmlCell(Tr(L"Report template", L"\u62a5\u544a\u6a21\u677f")) << ":</strong> "
         << HtmlCell(ReportTemplateName(g_reportTemplate)) << "</p>";
    bool showCourseDetails = g_reportTemplate == ReportTemplate::Teacher || g_reportTemplate == ReportTemplate::Complete;
    bool showRosterSummary = g_reportTemplate != ReportTemplate::Simple;
    bool showOtherColumn = g_reportTemplate != ReportTemplate::Parent && g_reportTemplate != ReportTemplate::Simple;
    if (showCourseDetails && (!g_sheets[g_activeSheet].teacher.empty() || !g_sheets[g_activeSheet].location.empty() || !g_sheets[g_activeSheet].notes.empty())) {
        file << "<p>";
        if (!g_sheets[g_activeSheet].teacher.empty()) file << "<strong>" << HtmlCell(Tr(L"Teacher/owner:", L"\u6559\u5e08/\u8d1f\u8d23\u4eba\uff1a")) << "</strong> " << HtmlCell(g_sheets[g_activeSheet].teacher) << "<br>";
        if (!g_sheets[g_activeSheet].location.empty()) file << "<strong>" << HtmlCell(Tr(L"Location:", L"\u5730\u70b9\uff1a")) << "</strong> " << HtmlCell(g_sheets[g_activeSheet].location) << "<br>";
        if (!g_sheets[g_activeSheet].notes.empty()) file << "<strong>" << HtmlCell(Tr(L"Course notes:", L"\u8bfe\u7a0b\u5907\u6ce8\uff1a")) << "</strong> " << HtmlCell(g_sheets[g_activeSheet].notes);
        file << "</p>";
    }
    if (showRosterSummary && !g_sheets[g_activeSheet].students.empty()) {
        file << "<p><strong>" << HtmlCell(Tr(L"Manage students", L"\u7ba1\u7406\u5b66\u751f")) << ":</strong> " << g_sheets[g_activeSheet].students.size() << "</p>";
    }
    file << "<table><tr><th>" << HtmlCell(Tr(L"Date/Time", L"\u65e5\u671f\u65f6\u95f4")) << "</th><th>"
         << HtmlCell(Tr(L"Name", L"\u59d3\u540d")) << "</th><th>"
         << HtmlCell(Tr(L"Status", L"\u72b6\u6001")) << "</th>";
    if (showOtherColumn) file << "<th>" << HtmlCell(Tr(L"Other", L"\u5176\u4ed6")) << "</th>";
    file << "</tr>";
    for (const auto& r : g_records) {
        file << "<tr><td>" << HtmlCell(r.dateTime) << "</td><td>" << HtmlCell(r.name)
             << "</td><td>" << HtmlCell(r.status) << "</td>";
        if (showOtherColumn) file << "<td>" << HtmlCell(r.other) << "</td>";
        file << "</tr>";
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
    int totalInRange = present + absent + late + other;
    std::vector<std::pair<std::wstring, int>> studentIssues;
    for (const auto& record : g_records) {
        if (!RecordMatchesStatsRange(record)) continue;
        if (record.status != L"Absent" && record.status != L"Late") continue;
        auto it = std::find_if(studentIssues.begin(), studentIssues.end(), [&](const auto& item) { return item.first == record.name; });
        if (it == studentIssues.end()) studentIssues.push_back({record.name, 1});
        else ++it->second;
    }
    std::sort(studentIssues.begin(), studentIssues.end(), [](const auto& a, const auto& b) { return a.second > b.second; });

    std::wstringstream ss;
    ss << Tr(L"Statistics summary", L"\u7edf\u8ba1\u6458\u8981") << L"\n\n";
    ss << Tr(L"Courses", L"\u8bfe\u7a0b") << L": " << g_sheets[g_activeSheet].name << L"\n";
    ss << Tr(L"Range", L"\u8303\u56f4") << L": " << StatsRangeName(g_statsRange) << L"\n";
    ss << Tr(L"Total", L"\u603b\u6570") << L": " << totalInRange
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

void ApplyFilterText(const std::wstring& filter) {
    if (g_filterEdit) SetText(g_filterEdit, filter);
    g_filterText = LowerText(filter);
    RefreshList();
}

std::vector<std::wstring> CurrentLessonKeys() {
    std::vector<std::wstring> lessons;
    for (const auto& record : g_records) {
        if (record.dateTime.empty()) continue;
        if (std::find(lessons.begin(), lessons.end(), record.dateTime) == lessons.end()) {
            lessons.push_back(record.dateTime);
        }
    }
    std::sort(lessons.begin(), lessons.end(), std::greater<std::wstring>());
    return lessons;
}

void CreateTodayLessonFromRoster() {
    SyncActiveSheet();
    const auto& students = g_sheets[g_activeSheet].students;
    if (students.empty()) {
        ShowMessage(Tr(L"No students in the roster.", L"\u540d\u5355\u4e2d\u6ca1\u6709\u5b66\u751f\u3002"));
        return;
    }
    std::wstring lesson = TodayDateKey() + L" " + Tr(L"Lesson", L"\u8bfe\u6b21");
    if (!PromptText(Tr(L"Create today's lesson", L"\u521b\u5efa\u4eca\u65e5\u8bfe\u6b21"), Tr(L"Lesson name:", L"\u8bfe\u6b21\u540d\u79f0\uff1a"), lesson)) return;
    lesson = TrimWide(lesson);
    if (lesson.empty()) return;

    PushUndo();
    MarkDirty();
    int added = 0;
    for (const auto& student : students) {
        auto duplicate = std::find_if(g_records.begin(), g_records.end(), [&](const AttendanceRecord& record) {
            return record.name == student && record.dateTime == lesson;
        });
        if (duplicate == g_records.end()) {
            g_records.push_back({lesson, student, L"Absent", L""});
            ++added;
        }
    }
    SetText(g_dateEdit, lesson);
    ApplyFilterText(L"date:" + LowerText(lesson));
    std::wstringstream ss;
    ss << Tr(L"Lesson records created.", L"\u8bfe\u6b21\u8bb0\u5f55\u5df2\u521b\u5efa\u3002") << L" (" << added << L")";
    ShowMessage(ss.str(), Tr(L"Create today's lesson", L"\u521b\u5efa\u4eca\u65e5\u8bfe\u6b21"));
}

void SwitchLessonFilter() {
    auto lessons = CurrentLessonKeys();
    if (lessons.empty()) {
        ShowMessage(Tr(L"No lessons were found.", L"\u672a\u627e\u5230\u8bfe\u6b21\u3002"));
        return;
    }
    std::wstringstream list;
    for (int i = 0; i < (int)std::min<size_t>(lessons.size(), 20); ++i) {
        list << (i + 1) << L". " << lessons[i] << L"\n";
    }
    std::wstring choice;
    if (!PromptText(Tr(L"Switch lesson", L"\u5207\u6362\u8bfe\u6b21"), list.str() + L"\n" + Tr(L"Enter lesson number:", L"\u8f93\u5165\u8bfe\u6b21\u7f16\u53f7\uff1a"), choice)) return;
    int index = 0;
    try { index = std::stoi(choice) - 1; } catch (...) { return; }
    if (index < 0 || index >= (int)lessons.size()) return;
    SetText(g_dateEdit, lessons[index]);
    ApplyFilterText(L"date:" + LowerText(lessons[index]));
}

void ShowStudentProfile() {
    SyncActiveSheet();
    std::wstring name = GetText(g_nameEdit);
    int selected = VisibleToRecordIndex(ListView_GetNextItem(g_list, -1, LVNI_SELECTED));
    if (selected >= 0) name = g_records[selected].name;
    if (!PromptText(Tr(L"Student profile", L"\u5b66\u751f\u6863\u6848"), Tr(L"Student name:", L"\u5b66\u751f\u59d3\u540d\uff1a"), name)) return;
    name = TrimWide(name);
    if (name.empty()) return;

    int total = 0, present = 0, absent = 0, late = 0, other = 0;
    std::vector<std::wstring> history;
    for (const auto& sheet : g_sheets) {
        for (const auto& record : sheet.records) {
            if (!ContainsText(record.name, LowerText(name))) continue;
            ++total;
            if (record.status == L"Present") ++present;
            else if (record.status == L"Absent") ++absent;
            else if (record.status == L"Late") ++late;
            else ++other;
            if (history.size() < 12) {
                history.push_back(sheet.name + L" | " + record.dateTime + L" | " + Tr(record.status.c_str(), record.status.c_str()));
            }
        }
    }
    double rate = total ? present * 100.0 / total : 0.0;
    std::wstringstream ss;
    ss << Tr(L"Student profile", L"\u5b66\u751f\u6863\u6848") << L": " << name << L"\n\n"
       << Tr(L"Total", L"\u603b\u6570") << L": " << total << L"\n"
       << Tr(L"Present", L"\u51fa\u5e2d") << L": " << present << L"\n"
       << Tr(L"Absent", L"\u7f3a\u5e2d") << L": " << absent << L"\n"
       << Tr(L"Late", L"\u8fdf\u5230") << L": " << late << L"\n"
       << Tr(L"Other", L"\u5176\u4ed6") << L": " << other << L"\n"
       << Tr(L"Attendance", L"\u51fa\u52e4\u7387") << L": " << std::fixed << std::setprecision(1) << rate << L"%\n";
    if (!history.empty()) {
        ss << L"\n" << Tr(L"Recent history", L"\u6700\u8fd1\u8bb0\u5f55") << L":\n";
        for (const auto& line : history) ss << L"- " << line << L"\n";
    }
    ShowMessage(ss.str(), Tr(L"Student profile", L"\u5b66\u751f\u6863\u6848"));
}

void ShowAdvancedFilterHelp() {
    std::wstring filter = GetText(g_filterEdit);
    if (filter.empty()) filter = L"name: status: from:2026-07-01 to:2026-07-31";
    std::wstring help =
        Tr(L"Advanced filter examples:", L"\u9ad8\u7ea7\u7b5b\u9009\u793a\u4f8b\uff1a") + L"\n"
        L"name:Alice\nstatus:Absent\nfrom:2026-07-01 to:2026-07-31\nthisweek\nthismonth\ndate:2026-07-09\n\n"
        + Tr(L"Edit filter:", L"\u7f16\u8f91\u7b5b\u9009\uff1a");
    if (PromptText(Tr(L"Advanced filter", L"\u9ad8\u7ea7\u7b5b\u9009"), help, filter)) {
        ApplyFilterText(filter);
    }
}

void ShowStatsRangeMenu(HWND button) {
    int command = ShowThemedPopupMenu(button, {
        {1, StatsRangeName(StatsRange::All), false},
        {2, StatsRangeName(StatsRange::ThisWeek), false},
        {3, StatsRangeName(StatsRange::ThisMonth), false}
    });
    if (command == 1) g_statsRange = StatsRange::All;
    else if (command == 2) g_statsRange = StatsRange::ThisWeek;
    else if (command == 3) g_statsRange = StatsRange::ThisMonth;
    else return;
    RefreshList();
}

void ShowRiskStudents() {
    if (!g_riskAlertsEnabled) {
        ShowMessage(Tr(L"Risk student reminders are disabled in Settings.", L"\u98ce\u9669\u5b66\u751f\u63d0\u9192\u5df2\u5728\u8bbe\u7f6e\u4e2d\u5173\u95ed\u3002"));
        return;
    }
    std::map<std::wstring, std::pair<int, int>> stats;
    for (const auto& sheet : g_sheets) {
        for (const auto& record : sheet.records) {
            auto& entry = stats[record.name];
            ++entry.first;
            if (record.status == L"Absent" || record.status == L"Late") ++entry.second;
        }
    }
    std::vector<std::pair<std::wstring, std::pair<int, int>>> risky;
    for (const auto& item : stats) {
        double issueRate = item.second.first ? item.second.second * 100.0 / item.second.first : 0.0;
        if (item.second.second >= 3 || issueRate >= 20.0) risky.push_back(item);
    }
    std::sort(risky.begin(), risky.end(), [](const auto& a, const auto& b) {
        return a.second.second == b.second.second ? a.first < b.first : a.second.second > b.second.second;
    });
    std::wstringstream ss;
    ss << Tr(L"Risk students", L"\u98ce\u9669\u5b66\u751f") << L"\n\n";
    if (risky.empty()) {
        ss << Tr(L"No risk students found.", L"\u672a\u53d1\u73b0\u98ce\u9669\u5b66\u751f\u3002");
    } else {
        for (int i = 0; i < (int)std::min<size_t>(risky.size(), 20); ++i) {
            double issueRate = risky[i].second.first ? risky[i].second.second * 100.0 / risky[i].second.first : 0.0;
            ss << L"- " << risky[i].first << L": " << risky[i].second.second << L"/" << risky[i].second.first
               << L" (" << std::fixed << std::setprecision(1) << issueRate << L"%)\n";
        }
    }
    ShowMessage(ss.str(), Tr(L"Risk students", L"\u98ce\u9669\u5b66\u751f"));
}

std::filesystem::path BackupDirectoryPath() {
    auto marker = AppDataFilePath(L"backups\\marker.tmp");
    return marker.empty() ? std::filesystem::path{} : marker.parent_path();
}

std::wstring TimestampForFileName() {
    SYSTEMTIME st{};
    GetLocalTime(&st);
    wchar_t buffer[32]{};
    swprintf_s(buffer, L"%04d%02d%02d-%02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    return buffer;
}

std::vector<std::filesystem::path> ListBackupFiles() {
    std::vector<std::filesystem::path> files;
    auto dir = BackupDirectoryPath();
    if (dir.empty() || !std::filesystem::exists(dir)) return files;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == L".attd") files.push_back(entry.path());
    }
    std::sort(files.begin(), files.end(), std::greater<std::filesystem::path>());
    return files;
}

void ShowBackupManager() {
    auto files = ListBackupFiles();
    std::wstringstream ss;
    ss << Tr(L"Backup manager", L"\u5907\u4efd\u7ba1\u7406\u4e2d\u5fc3") << L"\n\n";
    if (files.empty()) {
        ss << Tr(L"No backup file was found.", L"\u672a\u627e\u5230\u5907\u4efd\u6587\u4ef6\u3002") << L"\n";
        ShowMessage(ss.str(), Tr(L"Backup manager", L"\u5907\u4efd\u7ba1\u7406\u4e2d\u5fc3"));
        return;
    }
    for (int i = 0; i < (int)std::min<size_t>(files.size(), 15); ++i) {
        ss << (i + 1) << L". " << files[i].filename().wstring() << L"\n";
    }
    std::wstring choice;
    if (!PromptText(Tr(L"Backup manager", L"\u5907\u4efd\u7ba1\u7406\u4e2d\u5fc3"), ss.str() + L"\n" + Tr(L"Enter backup number to restore, or leave blank to cancel:", L"\u8f93\u5165\u8981\u6062\u590d\u7684\u5907\u4efd\u7f16\u53f7\uff0c\u7559\u7a7a\u53d6\u6d88\uff1a"), choice)) return;
    choice = TrimWide(choice);
    if (choice.empty()) return;
    int index = 0;
    try { index = std::stoi(choice) - 1; } catch (...) { return; }
    if (index < 0 || index >= (int)files.size()) return;
    if (!ConfirmDiscardUnsaved(Tr(L"Restore backup", L"\u6062\u590d\u5907\u4efd"))) return;
    LoadAttendanceFile(files[index].wstring(), true);
}

void ShowReportTemplateMenu(HWND button) {
    int command = ShowThemedPopupMenu(button, {
        {1, ReportTemplateName(ReportTemplate::Simple), false},
        {2, ReportTemplateName(ReportTemplate::Teacher), false},
        {3, ReportTemplateName(ReportTemplate::Parent), false},
        {4, ReportTemplateName(ReportTemplate::Complete), false}
    });
    if (command >= 1 && command <= 4) {
        g_reportTemplate = (ReportTemplate)(command - 1);
        SaveSettings();
    }
}

void ShowShortcutCenter() {
    std::wstring config = ShortcutConfigText();
    std::wstring prompt = Tr(
        L"Edit shortcuts as action=key; use Ctrl+letter or F keys:",
        L"\u6309 action=key \u7f16\u8f91\u5feb\u6377\u952e\uff0c\u652f\u6301 Ctrl+\u5b57\u6bcd \u6216 F \u952e\uff1a"
    );
    if (!PromptText(Tr(L"Shortcut center", L"\u5feb\u6377\u952e\u4e2d\u5fc3"), prompt, config)) return;
    if (ApplyShortcutConfigText(config)) {
        SaveSettings();
        ShowMessage(Tr(L"Shortcuts updated.", L"\u5feb\u6377\u952e\u5df2\u66f4\u65b0\u3002"), Tr(L"Shortcut center", L"\u5feb\u6377\u952e\u4e2d\u5fc3"));
    } else {
        ShowMessage(Tr(L"No shortcut changes were applied.", L"\u672a\u5e94\u7528\u5feb\u6377\u952e\u66f4\u6539\u3002"), Tr(L"Shortcut center", L"\u5feb\u6377\u952e\u4e2d\u5fc3"));
    }
}

int FindQuickRollCallRecord(const QuickRollCallState& state, const std::wstring& student) {
    int found = -1;
    for (int i = 0; i < (int)state.workingRecords.size(); ++i) {
        const auto& record = state.workingRecords[i];
        if (record.dateTime != state.lesson || record.name != student) continue;
        if (found >= 0) return -2;
        found = i;
    }
    return found;
}

std::wstring LocalizedQuickRollCallStatus(const std::wstring& status) {
    if (status == L"Present") return Tr(L"Present", L"出席");
    if (status == L"Absent") return Tr(L"Absent", L"缺席");
    if (status == L"Late") return Tr(L"Late", L"迟到");
    if (status == L"Other") return Tr(L"Other", L"其他");
    if (status.empty()) return Tr(L"Unmarked", L"未标记");
    return status;
}

void ResizeQuickRollCallLayout(HWND hwnd) {
    RECT rc{};
    GetClientRect(hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    if (width <= 0 || height <= 0) return;

    int pad = 28;
    int innerW = std::max(320, width - pad * 2);
    int gap = 12;
    int statusW = std::max(110, (innerW - gap * 3) / 4);
    int navW = std::max(110, (innerW - gap * 3) / 4);

    HDWP defer = BeginDeferWindowPos(17);
    auto move = [&](int id, int x, int y, int w, int h) {
        HWND control = GetDlgItem(hwnd, id);
        if (control && defer) {
            defer = DeferWindowPos(defer, control, nullptr, x, y, w, h,
                SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
        }
    };

    move(IDC_QUICK_TITLE, pad, 20, innerW, 38);
    move(IDC_QUICK_COURSE, pad, 64, innerW * 2 / 3, 26);
    move(IDC_QUICK_LESSON, pad, 92, innerW * 2 / 3, 26);
    move(IDC_QUICK_PROGRESS, pad + innerW * 2 / 3, 64, innerW / 3, 54);
    move(IDC_QUICK_STUDENT, pad, 130, innerW, 54);
    move(IDC_QUICK_STATUS, pad, 187, innerW, 30);
    move(IDC_QUICK_NOTES_LABEL, pad, 232, innerW, 24);
    move(IDC_QUICK_NOTES, pad, 260, innerW, 52);

    int statusY = 334;
    move(IDC_QUICK_PRESENT, pad, statusY, statusW, 54);
    move(IDC_QUICK_ABSENT, pad + statusW + gap, statusY, statusW, 54);
    move(IDC_QUICK_LATE, pad + (statusW + gap) * 2, statusY, statusW, 54);
    move(IDC_QUICK_OTHER, pad + (statusW + gap) * 3, statusY, innerW - (statusW + gap) * 3, 54);

    int navY = std::max(statusY + 74, height - 112);
    move(IDC_QUICK_PREVIOUS, pad, navY, navW, 40);
    move(IDC_QUICK_SKIP, pad + navW + gap, navY, navW, 40);
    move(IDC_QUICK_FINISH, pad + (navW + gap) * 2, navY, navW, 40);
    move(IDC_QUICK_CANCEL, pad + (navW + gap) * 3, navY, innerW - (navW + gap) * 3, 40);
    move(IDC_QUICK_HINT, pad, navY + 48, innerW, 28);
    if (defer) EndDeferWindowPos(defer);
    RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_NOERASE);
}

void RefreshQuickRollCallWindow(HWND hwnd) {
    auto* state = reinterpret_cast<QuickRollCallState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!state || state->students.empty()) return;
    state->index = std::min(state->index, state->students.size() - 1);

    const std::wstring& student = state->students[state->index];
    int recordIndex = FindQuickRollCallRecord(*state, student);
    std::wstring status;
    std::wstring notes;
    if (recordIndex >= 0) {
        status = state->workingRecords[recordIndex].status;
        notes = state->workingRecords[recordIndex].other;
    }

    size_t reviewed = (size_t)std::count(state->reviewed.begin(), state->reviewed.end(), true);
    size_t remaining = state->reviewed.size() - reviewed;
    std::wstringstream progress;
    progress << Tr(L"Reviewed", L"已检查") << L" " << reviewed << L" / " << state->students.size()
             << L"\n" << Tr(L"Remaining", L"剩余") << L" " << remaining;

    SetText(GetDlgItem(hwnd, IDC_QUICK_COURSE), Tr(L"Course:", L"课程：") + L" " + state->course);
    SetText(GetDlgItem(hwnd, IDC_QUICK_LESSON), Tr(L"Lesson:", L"课次：") + L" " + state->lesson);
    SetText(GetDlgItem(hwnd, IDC_QUICK_PROGRESS), progress.str());
    SetText(GetDlgItem(hwnd, IDC_QUICK_STUDENT), student);
    SetText(GetDlgItem(hwnd, IDC_QUICK_STATUS),
        Tr(L"Current status:", L"当前状态：") + L" " + LocalizedQuickRollCallStatus(status));
    SetText(GetDlgItem(hwnd, IDC_QUICK_NOTES), notes);
    SendMessageW(GetDlgItem(hwnd, IDC_QUICK_NOTES), EM_SETSEL, 0, 0);
    InvalidateRect(hwnd, nullptr, FALSE);
}

bool ApplyQuickRollCallStatus(HWND hwnd, const std::wstring& status) {
    auto* state = reinterpret_cast<QuickRollCallState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!state || state->students.empty()) return false;

    std::wstring notes = TrimWide(GetText(GetDlgItem(hwnd, IDC_QUICK_NOTES)));
    if (status == L"Other" && notes.empty()) {
        ThemedMessageBox(hwnd, Tr(L"Other status requires a note.", L"选择其他状态时必须填写备注。"),
            Tr(L"Quick roll call", L"快速点名"), false);
        SetFocus(GetDlgItem(hwnd, IDC_QUICK_NOTES));
        return false;
    }

    const std::wstring& student = state->students[state->index];
    int recordIndex = FindQuickRollCallRecord(*state, student);
    if (recordIndex == -2) {
        ThemedMessageBox(hwnd,
            Tr(L"Duplicate lesson records must be resolved before quick roll call.", L"开始快速点名前必须先处理重复课次记录。"),
            Tr(L"Quick roll call", L"快速点名"), false);
        return false;
    }

    if (recordIndex < 0) {
        state->workingRecords.push_back({state->lesson, student, status, notes});
        state->changed = true;
    } else {
        auto& record = state->workingRecords[recordIndex];
        if (record.status != status || record.other != notes) {
            record.status = status;
            record.other = notes;
            state->changed = true;
        }
    }
    state->reviewed[state->index] = true;
    if (state->index + 1 < state->students.size()) ++state->index;
    RefreshQuickRollCallWindow(hwnd);
    return true;
}

void SkipQuickRollCallStudent(HWND hwnd) {
    auto* state = reinterpret_cast<QuickRollCallState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!state || state->students.empty()) return;
    state->reviewed[state->index] = true;
    if (state->index + 1 < state->students.size()) ++state->index;
    RefreshQuickRollCallWindow(hwnd);
}

void PreviousQuickRollCallStudent(HWND hwnd) {
    auto* state = reinterpret_cast<QuickRollCallState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!state || state->students.empty()) return;
    if (state->index > 0) --state->index;
    RefreshQuickRollCallWindow(hwnd);
}

void CancelQuickRollCall(HWND hwnd) {
    auto* state = reinterpret_cast<QuickRollCallState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!state) return;
    if (state->changed && !state->allowClose) {
        int result = ThemedMessageBox(hwnd,
            Tr(L"Discard quick roll call changes?", L"放弃本次快速点名的修改吗？"),
            Tr(L"Quick roll call", L"快速点名"), true);
        if (result != IDYES) return;
    }
    state->allowClose = true;
    DestroyWindow(hwnd);
}

void FinishQuickRollCall(HWND hwnd) {
    auto* state = reinterpret_cast<QuickRollCallState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!state) return;
    size_t unreviewed = (size_t)std::count(state->reviewed.begin(), state->reviewed.end(), false);
    if (unreviewed > 0) {
        std::wstringstream warning;
        warning << unreviewed << L" "
                << Tr(L"students have not been reviewed. Finish and keep the partial results?",
                      L"名学生尚未检查。是否完成并保留部分结果？");
        if (ThemedMessageBox(hwnd, warning.str(), Tr(L"Quick roll call", L"快速点名"), true) != IDYES) return;
    }

    bool changed = state->changed;
    if (changed) {
        PushUndo();
        g_records = state->workingRecords;
        MarkDirty();
        SyncActiveSheet();
        RefreshList();
    }
    state->allowClose = true;
    DestroyWindow(hwnd);
    if (changed) {
        ShowMessage(Tr(L"Quick roll call changes were applied.", L"快速点名修改已应用。"),
            Tr(L"Quick roll call", L"快速点名"));
    }
}

LRESULT CALLBACK QuickRollCallProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* state = reinterpret_cast<QuickRollCallState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE: {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        state = reinterpret_cast<QuickRollCallState*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)state);

        MakeSettingsControl(hwnd, L"STATIC", Tr(L"Quick roll call", L"快速点名").c_str(), SS_LEFT | SS_NOPREFIX, IDC_QUICK_TITLE);
        MakeSettingsControl(hwnd, L"STATIC", L"", SS_LEFT | SS_NOPREFIX | SS_ENDELLIPSIS, IDC_QUICK_COURSE);
        MakeSettingsControl(hwnd, L"STATIC", L"", SS_LEFT | SS_NOPREFIX | SS_ENDELLIPSIS, IDC_QUICK_LESSON);
        MakeSettingsControl(hwnd, L"STATIC", L"", SS_RIGHT | SS_NOPREFIX, IDC_QUICK_PROGRESS);
        MakeSettingsControl(hwnd, L"STATIC", L"", SS_CENTER | SS_NOPREFIX, IDC_QUICK_STUDENT);
        MakeSettingsControl(hwnd, L"STATIC", L"", SS_CENTER | SS_NOPREFIX, IDC_QUICK_STATUS);
        MakeSettingsControl(hwnd, L"STATIC", Tr(L"Notes:", L"备注：").c_str(), SS_LEFT | SS_NOPREFIX, IDC_QUICK_NOTES_LABEL);
        MakeSettingsControl(hwnd, L"EDIT", L"", WS_TABSTOP | ES_AUTOHSCROLL, IDC_QUICK_NOTES);
        MakeSettingsControl(hwnd, L"BUTTON", Tr(L"Present", L"出席").c_str(), BS_PUSHBUTTON | BS_OWNERDRAW | WS_TABSTOP, IDC_QUICK_PRESENT);
        MakeSettingsControl(hwnd, L"BUTTON", Tr(L"Absent", L"缺席").c_str(), BS_PUSHBUTTON | BS_OWNERDRAW | WS_TABSTOP, IDC_QUICK_ABSENT);
        MakeSettingsControl(hwnd, L"BUTTON", Tr(L"Late", L"迟到").c_str(), BS_PUSHBUTTON | BS_OWNERDRAW | WS_TABSTOP, IDC_QUICK_LATE);
        MakeSettingsControl(hwnd, L"BUTTON", Tr(L"Other", L"其他").c_str(), BS_PUSHBUTTON | BS_OWNERDRAW | WS_TABSTOP, IDC_QUICK_OTHER);
        MakeSettingsControl(hwnd, L"BUTTON", Tr(L"Previous", L"上一位").c_str(), BS_PUSHBUTTON | BS_OWNERDRAW | WS_TABSTOP, IDC_QUICK_PREVIOUS);
        MakeSettingsControl(hwnd, L"BUTTON", Tr(L"Skip", L"跳过").c_str(), BS_PUSHBUTTON | BS_OWNERDRAW | WS_TABSTOP, IDC_QUICK_SKIP);
        MakeSettingsControl(hwnd, L"BUTTON", Tr(L"Finish", L"完成").c_str(), BS_PUSHBUTTON | BS_OWNERDRAW | WS_TABSTOP, IDC_QUICK_FINISH);
        MakeSettingsControl(hwnd, L"BUTTON", Tr(L"Cancel", L"取消").c_str(), BS_PUSHBUTTON | BS_OWNERDRAW | WS_TABSTOP, IDC_QUICK_CANCEL);
        MakeSettingsControl(hwnd, L"STATIC",
            Tr(L"Keyboard: 1 Present, 2 Absent, 3 Late, 4 Other, Left Previous, Right Skip",
               L"键盘：1 出席，2 缺席，3 迟到，4 其他，左方向键上一位，右方向键跳过").c_str(),
            SS_CENTER | SS_NOPREFIX | SS_ENDELLIPSIS, IDC_QUICK_HINT);

        SendMessageW(GetDlgItem(hwnd, IDC_QUICK_TITLE), WM_SETFONT, (WPARAM)g_titleFont, TRUE);
        SendMessageW(GetDlgItem(hwnd, IDC_QUICK_STUDENT), WM_SETFONT, (WPARAM)g_titleFont, TRUE);
        SendMessageW(GetDlgItem(hwnd, IDC_QUICK_HINT), WM_SETFONT, (WPARAM)g_smallFont, TRUE);
        ApplyThemedControls(hwnd);
        ResizeQuickRollCallLayout(hwnd);
        RefreshQuickRollCallWindow(hwnd);
        SetFocus(GetDlgItem(hwnd, IDC_QUICK_PRESENT));
        return 0;
    }
    case WM_SIZE:
        ResizeQuickRollCallLayout(hwnd);
        return 0;
    case WM_COMMAND:
        if (HIWORD(wParam) != BN_CLICKED) break;
        switch (LOWORD(wParam)) {
        case IDC_QUICK_PRESENT: ApplyQuickRollCallStatus(hwnd, L"Present"); return 0;
        case IDC_QUICK_ABSENT: ApplyQuickRollCallStatus(hwnd, L"Absent"); return 0;
        case IDC_QUICK_LATE: ApplyQuickRollCallStatus(hwnd, L"Late"); return 0;
        case IDC_QUICK_OTHER: ApplyQuickRollCallStatus(hwnd, L"Other"); return 0;
        case IDC_QUICK_PREVIOUS: PreviousQuickRollCallStudent(hwnd); return 0;
        case IDC_QUICK_SKIP: SkipQuickRollCallStudent(hwnd); return 0;
        case IDC_QUICK_FINISH: FinishQuickRollCall(hwnd); return 0;
        case IDC_QUICK_CANCEL: CancelQuickRollCall(hwnd); return 0;
        }
        break;
    case WM_KEYDOWN:
        if (wParam == L'1' || wParam == VK_NUMPAD1) { ApplyQuickRollCallStatus(hwnd, L"Present"); return 0; }
        if (wParam == L'2' || wParam == VK_NUMPAD2) { ApplyQuickRollCallStatus(hwnd, L"Absent"); return 0; }
        if (wParam == L'3' || wParam == VK_NUMPAD3) { ApplyQuickRollCallStatus(hwnd, L"Late"); return 0; }
        if (wParam == L'4' || wParam == VK_NUMPAD4) { ApplyQuickRollCallStatus(hwnd, L"Other"); return 0; }
        if (wParam == VK_LEFT) { PreviousQuickRollCallStudent(hwnd); return 0; }
        if (wParam == VK_RIGHT) { SkipQuickRollCallStudent(hwnd); return 0; }
        if (wParam == VK_ESCAPE) { CancelQuickRollCall(hwnd); return 0; }
        break;
    case WM_DRAWITEM: {
        auto* draw = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (DrawButtonItem(draw)) return TRUE;
        break;
    }
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
        PaintAppBackground(hwnd, memDc);
        BitBlt(hdc, 0, 0, width, height, memDc, 0, 0, SRCCOPY);
        SelectObject(memDc, oldBitmap);
        DeleteObject(bitmap);
        DeleteDC(memDc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        HWND control = (HWND)lParam;
        int id = GetDlgCtrlID(control);
        SetTextColor(hdc, id == IDC_QUICK_TITLE || id == IDC_QUICK_STUDENT ? COLOR_TEXT : COLOR_MUTED);
        SetBkMode(hdc, OPAQUE);
        SetBkColor(hdc, COLOR_BG);
        return (LRESULT)g_bgBrush;
    }
    case WM_CTLCOLOREDIT: {
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
    case WM_CLOSE:
        CancelQuickRollCall(hwnd);
        return 0;
    case WM_DESTROY:
        RemoveAnimationState(hwnd);
        g_quickRollCallWindow = nullptr;
        EnableWindow(g_hwnd, TRUE);
        SetForegroundWindow(g_hwnd);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        delete state;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void ShowQuickRollCall() {
    if (g_quickRollCallWindow && IsWindow(g_quickRollCallWindow)) {
        ShowWindow(g_quickRollCallWindow, SW_SHOW);
        SetForegroundWindow(g_quickRollCallWindow);
        return;
    }

    SyncActiveSheet();
    EnsureSheets();
    std::wstring lesson = TrimWide(GetText(g_dateEdit));
    if (lesson.empty()) {
        if (!PromptText(Tr(L"Quick roll call", L"快速点名"), Tr(L"Lesson name:", L"课次名称："), lesson)) return;
        lesson = TrimWide(lesson);
        if (lesson.empty()) return;
        SetText(g_dateEdit, lesson);
    }

    std::vector<std::wstring> students;
    std::set<std::wstring> seen;
    for (const auto& rawName : g_sheets[g_activeSheet].students) {
        std::wstring name = TrimWide(rawName);
        std::wstring key = LowerText(name);
        if (!name.empty() && seen.insert(key).second) students.push_back(name);
    }
    if (students.empty()) {
        ShowMessage(Tr(L"No students in the roster.", L"当前课程名单中没有学生。"),
            Tr(L"Quick roll call", L"快速点名"));
        return;
    }

    std::map<std::wstring, int> counts;
    for (const auto& record : g_records) {
        if (record.dateTime == lesson) ++counts[record.name];
    }
    std::vector<std::wstring> duplicates;
    for (const auto& student : students) {
        if (counts[student] > 1) duplicates.push_back(student);
    }
    if (!duplicates.empty()) {
        std::wstringstream message;
        message << Tr(L"Duplicate lesson records must be resolved before quick roll call.",
                      L"开始快速点名前必须先处理重复课次记录。") << L"\n\n";
        for (const auto& student : duplicates) message << L"- " << student << L"\n";
        ShowMessage(message.str(), Tr(L"Quick roll call", L"快速点名"));
        return;
    }

    auto* state = new QuickRollCallState;
    state->lesson = lesson;
    state->course = g_sheets[g_activeSheet].name;
    state->students = std::move(students);
    state->workingRecords = g_records;
    state->reviewed.assign(state->students.size(), false);

    HINSTANCE instance = GetModuleHandleW(nullptr);
    const wchar_t* className = L"AttendanceQuickRollCallWindow";
    static bool registered = false;
    if (!registered) {
        WNDCLASSW wc{};
        wc.lpfnWndProc = QuickRollCallProc;
        wc.hInstance = instance;
        wc.lpszClassName = className;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON));
        wc.hbrBackground = nullptr;
        RegisterClassW(&wc);
        registered = true;
    }

    EnableWindow(g_hwnd, FALSE);
    g_quickRollCallWindow = CreateWindowExW(
        WS_EX_CONTROLPARENT,
        className,
        Tr(L"Quick roll call", L"快速点名").c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 900, 570,
        g_hwnd, nullptr, instance, state
    );
    if (!g_quickRollCallWindow) {
        EnableWindow(g_hwnd, TRUE);
        delete state;
        return;
    }
    ApplyGlassTitleBar(g_quickRollCallWindow);
    CenterOwnedWindow(g_quickRollCallWindow, g_hwnd);
    ShowWindow(g_quickRollCallWindow, SW_SHOW);
    UpdateWindow(g_quickRollCallWindow);
    SetForegroundWindow(g_quickRollCallWindow);
}

void RunCommandPalette() {
    if (!g_commandPaletteEnabled) return;
    std::wstring command;
    if (!PromptText(Tr(L"Command palette", L"\u547d\u4ee4\u9762\u677f"), Tr(L"Type a command: save, import, export, backup, stats, risk, filter, lesson, rollcall, review, profile, settings", L"\u8f93\u5165\u547d\u4ee4\uff1asave\u3001import\u3001export\u3001backup\u3001stats\u3001risk\u3001filter\u3001lesson\u3001rollcall\u3001\u70b9\u540d\u3001review\u3001profile\u3001settings"), command)) return;
    command = LowerText(TrimWide(command));
    if (command.find(L"save") != std::wstring::npos || command.find(L"\u4fdd\u5b58") != std::wstring::npos) SaveAttendance();
    else if (command.find(L"import") != std::wstring::npos || command.find(L"\u5bfc\u5165") != std::wstring::npos) ImportAttendance();
    else if (command.find(L"export") != std::wstring::npos || command.find(L"\u5bfc\u51fa") != std::wstring::npos) ExportCsv();
    else if (command.find(L"backup") != std::wstring::npos || command.find(L"\u5907\u4efd") != std::wstring::npos) BackupNow();
    else if (command.find(L"stats") != std::wstring::npos || command.find(L"\u7edf\u8ba1") != std::wstring::npos) ShowStatisticsSummary();
    else if (command.find(L"risk") != std::wstring::npos || command.find(L"\u98ce\u9669") != std::wstring::npos) ShowRiskStudents();
    else if (command.find(L"filter") != std::wstring::npos || command.find(L"\u7b5b\u9009") != std::wstring::npos) ShowAdvancedFilterHelp();
    else if (command.find(L"rollcall") != std::wstring::npos || command.find(L"\u70b9\u540d") != std::wstring::npos) ShowQuickRollCall();
    else if (command.find(L"review") != std::wstring::npos || command.find(L"\u68c0\u67e5") != std::wstring::npos) ReviewLessonCompletion();
    else if (command.find(L"lesson") != std::wstring::npos || command.find(L"\u8bfe\u6b21") != std::wstring::npos) CreateTodayLessonFromRoster();
    else if (command.find(L"profile") != std::wstring::npos || command.find(L"\u6863\u6848") != std::wstring::npos) ShowStudentProfile();
    else if (command.find(L"settings") != std::wstring::npos || command.find(L"\u8bbe\u7f6e") != std::wstring::npos) ShowSettingsWindow();
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
    std::string payload = EncodeAttd(SerializeWorkbook());
    file << payload;
    file.close();

    auto backupDir = BackupDirectoryPath();
    std::filesystem::path timestamped;
    if (!backupDir.empty()) {
        timestamped = backupDir / (L"backup-" + TimestampForFileName() + L".attd");
        std::ofstream history(timestamped, std::ios::binary);
        if (history) history << payload;
    }
    std::wstring message = Tr(L"Backup created:", L"\u5907\u4efd\u5df2\u521b\u5efa\uff1a") + L"\n" + path.wstring();
    if (!timestamped.empty()) message += L"\n" + timestamped.wstring();
    ShowMessage(message);
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
    if (!g_autosavePromptEnabled) return;
    auto path = AppDataFilePath(L"autosave.attd");
    if (path.empty() || !std::filesystem::exists(path)) return;
    std::wstring choice = L"1";
    std::wstring restoreTitle = Tr(L"Restore Autosave", L"\u6062\u590d\u81ea\u52a8\u4fdd\u5b58");
    std::wstring restoreMsg =
        Tr(L"An autosaved attendance file was found.", L"\u627e\u5230\u81ea\u52a8\u4fdd\u5b58\u7684\u70b9\u540d\u6587\u4ef6\u3002") + L"\n\n"
        L"1. " + Tr(L"Restore", L"\u6062\u590d") + L"\n"
        L"2. " + Tr(L"View file location", L"\u67e5\u770b\u6587\u4ef6\u4f4d\u7f6e") + L"\n"
        L"3. " + Tr(L"Ignore this time", L"\u672c\u6b21\u5ffd\u7565") + L"\n"
        L"4. " + Tr(L"Delete autosave", L"\u5220\u9664\u81ea\u52a8\u4fdd\u5b58") + L"\n\n"
        + Tr(L"Choose an option:", L"\u9009\u62e9\u64cd\u4f5c\uff1a");
    if (!PromptText(restoreTitle, restoreMsg, choice)) {
        g_allowAutosaveOverwrite = false;
        return;
    }
    choice = TrimWide(choice);
    if (choice == L"1") {
        LoadAttendanceFile(path.wstring(), false);
    } else if (choice == L"2") {
        ShowMessage(path.wstring(), Tr(L"Autosave file", L"\u81ea\u52a8\u4fdd\u5b58\u6587\u4ef6"));
        g_allowAutosaveOverwrite = false;
    } else if (choice == L"4") {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    } else {
        g_allowAutosaveOverwrite = false;
    }
}

void ShowShortcuts() {
    std::wstring text =
        g_shortcutFullscreen + L": " + Tr(L"Fullscreen", L"\u5168\u5c4f") + L"\n" +
        g_shortcutSave + L": " + Tr(L"Save .attd", L"\u4fdd\u5b58 .attd") + L"\n" +
        g_shortcutImport + L": " + Tr(L"Import .attd", L"\u5bfc\u5165 .attd") + L"\n" +
        g_shortcutUndo + L": " + Tr(L"Undo", L"\u64a4\u9500") + L"\n" +
        g_shortcutRedo + L": " + Tr(L"Redo", L"\u91cd\u505a") + L"\n" +
        g_shortcutCommand + L": " + Tr(L"Command palette", L"\u547d\u4ee4\u9762\u677f") + L"\n" +
        Tr(L"Double-click row: Edit selected record", L"\u53cc\u51fb\u884c\uff1a\u7f16\u8f91\u9009\u4e2d\u8bb0\u5f55") + L"\n" +
        Tr(L"Ctrl/Shift click: Multi-select rows", L"Ctrl/Shift \u70b9\u51fb\uff1a\u591a\u9009\u884c");
    ShowMessage(text, Tr(L"Keyboard Shortcuts", L"\u5feb\u6377\u952e"));
}

void CountStatuses(int& present, int& absent, int& late, int& other) {
    present = absent = late = other = 0;
    for (const auto& r : g_records) {
        if (!RecordMatchesStatsRange(r)) continue;
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
        HBRUSH bg = CreateSolidBrush(COLOR_PANEL);
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
        g_chartWindow = nullptr;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void ShowStatsChart() {
    if (g_chartWindow && IsWindow(g_chartWindow)) {
        InvalidateRect(g_chartWindow, nullptr, FALSE);
        SetForegroundWindow(g_chartWindow);
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
    g_chartWindow = CreateWindowExW(
        0, className, chartTitle.c_str(),
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 760, 420,
        g_hwnd, nullptr, instance, nullptr
    );
    ApplyGlassTitleBar(g_chartWindow);
    CenterOwnedWindow(g_chartWindow, g_hwnd);
    StartWindowReveal(g_chartWindow, 190);
    ShowWindow(g_chartWindow, SW_SHOW);
    UpdateWindow(g_chartWindow);
}

void ReviewLessonCompletion() {
    SyncActiveSheet();
    const std::wstring lesson = TrimWide(GetText(g_dateEdit));
    if (lesson.empty()) {
        ShowMessage(Tr(L"Enter a lesson name or date first.", L"请先输入课次名称或日期。"));
        return;
    }

    const auto& sheet = g_sheets[g_activeSheet];
    std::map<std::wstring, int> recordCounts;
    int recordsInLesson = 0;
    for (const auto& record : g_records) {
        if (record.dateTime != lesson) continue;
        ++recordsInLesson;
        ++recordCounts[record.name];
    }

    std::vector<std::wstring> roster = sheet.students;
    if (roster.empty()) {
        for (const auto& item : recordCounts) roster.push_back(item.first);
    }
    std::vector<std::wstring> missing;
    for (const auto& student : roster) {
        if (recordCounts.find(student) == recordCounts.end()) missing.push_back(student);
    }
    std::vector<std::wstring> duplicates;
    for (const auto& item : recordCounts) {
        if (item.second > 1) duplicates.push_back(item.first);
    }

    std::wstringstream report;
    report << Tr(L"Lesson completion check", L"课次完成检查") << L"\n\n"
           << Tr(L"Lesson", L"课次") << L": " << lesson << L"\n"
           << Tr(L"Roster", L"名单人数") << L": " << roster.size() << L"\n"
           << Tr(L"Records", L"记录数") << L": " << recordsInLesson << L"\n";
    if (roster.empty()) {
        report << L"\n" << Tr(L"No roster or lesson records are available to check.", L"当前没有名单或课次记录可供检查。") << L"\n";
    } else if (missing.empty()) {
        report << L"\n" << Tr(L"Every roster student has a record.", L"名单中的每位学生都已有记录。") << L"\n";
    } else {
        report << L"\n" << Tr(L"Missing records", L"缺少记录") << L" (" << missing.size() << L"): ";
        for (size_t i = 0; i < missing.size(); ++i) {
            if (i) report << L", ";
            report << missing[i];
        }
        report << L"\n";
    }
    if (!duplicates.empty()) {
        report << L"\n" << Tr(L"Duplicate records", L"重复记录") << L" (" << duplicates.size() << L"): ";
        for (size_t i = 0; i < duplicates.size(); ++i) {
            if (i) report << L", ";
            report << duplicates[i];
        }
    }
    ShowMessage(report.str(), Tr(L"Lesson completion check", L"课次完成检查"));
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
    std::wstring createLesson = Tr(L"Create today's lesson", L"\u521b\u5efa\u4eca\u65e5\u8bfe\u6b21");
    std::wstring switchLesson = Tr(L"Switch lesson", L"\u5207\u6362\u8bfe\u6b21");
    std::wstring quickRollCall = Tr(L"Quick roll call", L"\u5feb\u901f\u70b9\u540d");
    std::wstring lessonCompletion = Tr(L"Lesson completion check", L"课次完成检查");
    std::wstring studentProfile = Tr(L"Student profile", L"\u5b66\u751f\u6863\u6848");
    std::wstring advancedFilter = Tr(L"Advanced filter", L"\u9ad8\u7ea7\u7b5b\u9009");
    std::wstring statsRange = Tr(L"Statistics range", L"\u7edf\u8ba1\u8303\u56f4") + L": " + StatsRangeName(g_statsRange);
    std::wstring riskStudents = Tr(L"Risk students", L"\u98ce\u9669\u5b66\u751f");
    std::wstring backupManager = Tr(L"Backup manager", L"\u5907\u4efd\u7ba1\u7406\u4e2d\u5fc3");
    std::wstring reportTemplate = Tr(L"Report template", L"\u62a5\u544a\u6a21\u677f") + L": " + ReportTemplateName(g_reportTemplate);
    std::wstring commandPalette = Tr(L"Command palette", L"\u547d\u4ee4\u9762\u677f");
    std::wstring shortcutCenter = Tr(L"Shortcut center", L"\u5feb\u6377\u952e\u4e2d\u5fc3");
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
        {IDM_LESSON_CREATE_TODAY, createLesson, false},
        {IDM_LESSON_SWITCH, switchLesson, false},
        {IDM_QUICK_ROLL_CALL, quickRollCall, false},
        {IDM_LESSON_COMPLETION, lessonCompletion, false},
        {IDM_STUDENT_PROFILE, studentProfile, false},
        {IDM_COURSE_DETAILS, courseDetails, false},
        {0, L"", true},
        {IDM_ADVANCED_FILTER, advancedFilter, false},
        {IDM_PRINT_HTML, print, false},
        {IDM_EXPORT_PPTX, pptx, false},
        {IDM_REPORT_TEMPLATE, reportTemplate, false},
        {IDM_STATS_CHART, chart, false},
        {IDM_STATS_SUMMARY, summary, false},
        {IDM_STATS_RANGE, statsRange, false},
        {IDM_RISK_STUDENTS, riskStudents, false},
        {0, L"", true},
        {IDM_UNDO, undo, false},
        {IDM_REDO, redo, false},
        {IDM_SHORTCUTS, shortcuts, false},
        {IDM_SHORTCUT_CENTER, shortcutCenter, false},
        {IDM_COMMAND_PALETTE, commandPalette, false},
        {0, L"", true},
        {IDM_BACKUP_NOW, backup, false},
        {IDM_BACKUP_MANAGER, backupManager, false},
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
    case IDM_LESSON_CREATE_TODAY: CreateTodayLessonFromRoster(); break;
    case IDM_LESSON_SWITCH: SwitchLessonFilter(); break;
    case IDM_QUICK_ROLL_CALL: ShowQuickRollCall(); break;
    case IDM_LESSON_COMPLETION: ReviewLessonCompletion(); break;
    case IDM_STUDENT_PROFILE: ShowStudentProfile(); break;
    case IDM_COURSE_DETAILS: EditCourseDetails(); break;
    case IDM_ADVANCED_FILTER: ShowAdvancedFilterHelp(); break;
    case IDM_PRINT_HTML: ExportPrintHtml(); break;
    case IDM_EXPORT_PPTX: ExportPptx(); break;
    case IDM_REPORT_TEMPLATE: ShowReportTemplateMenu(button); break;
    case IDM_STATS_CHART: ShowStatsChart(); break;
    case IDM_STATS_SUMMARY: ShowStatisticsSummary(); break;
    case IDM_STATS_RANGE: ShowStatsRangeMenu(button); break;
    case IDM_RISK_STUDENTS: ShowRiskStudents(); break;
    case IDM_UNDO: UndoLast(); break;
    case IDM_REDO: RedoLast(); break;
    case IDM_SHORTCUTS: ShowShortcuts(); break;
    case IDM_SHORTCUT_CENTER: ShowShortcutCenter(); break;
    case IDM_COMMAND_PALETTE: RunCommandPalette(); break;
    case IDM_BACKUP_NOW: BackupNow(); break;
    case IDM_BACKUP_MANAGER: ShowBackupManager(); break;
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
    if (plainText.rfind("ATTENDANCE_V5\n", 0) != 0) {
        std::filesystem::path backupPath = std::filesystem::path(path).wstring() + L".pre-v5.bak";
        if (!std::filesystem::exists(backupPath)) {
            std::error_code ec;
            std::filesystem::copy_file(std::filesystem::path(path), backupPath, std::filesystem::copy_options::skip_existing, ec);
        }
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
        SendMessageW(hwnd, CB_SETITEMHEIGHT, (WPARAM)-1, ComboItemHeight());
        SendMessageW(hwnd, CB_SETITEMHEIGHT, 0, ComboItemHeight());
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
        SendMessageW(hwnd, CB_SETITEMHEIGHT, (WPARAM)-1, ComboItemHeight());
        SendMessageW(hwnd, CB_SETITEMHEIGHT, 0, ComboItemHeight());
        SendMessageW(hwnd, CB_SETMINVISIBLE, 12, 0);
        EnableComboPaint(hwnd);
    }
    return hwnd;
}

void FillSettingsCombos(HWND hwnd) {
    HWND language = GetDlgItem(hwnd, IDC_SETTINGS_LANGUAGE);
    HWND font = GetDlgItem(hwnd, IDC_SETTINGS_FONT);
    HWND animation = GetDlgItem(hwnd, IDC_SETTINGS_ANIMATION_LEVEL);
    HWND particle = GetDlgItem(hwnd, IDC_SETTINGS_PARTICLE_LEVEL);
    HWND report = GetDlgItem(hwnd, IDC_SETTINGS_REPORT_TEMPLATE);
    HWND theme = GetDlgItem(hwnd, IDC_SETTINGS_THEME);
    HWND accent = GetDlgItem(hwnd, IDC_SETTINGS_ACCENT);
    HWND textScale = GetDlgItem(hwnd, IDC_SETTINGS_TEXT_SCALE);
    HWND density = GetDlgItem(hwnd, IDC_SETTINGS_DENSITY);
    HWND autosave = GetDlgItem(hwnd, IDC_SETTINGS_AUTOSAVE_INTERVAL);

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

    SendMessageW(animation, CB_RESETCONTENT, 0, 0);
    for (int i = 0; i <= 2; ++i) {
        std::wstring label = AnimationLevelName((AnimationLevel)i);
        SendMessageW(animation, CB_ADDSTRING, 0, (LPARAM)label.c_str());
    }
    SendMessageW(animation, CB_SETCURSEL, (WPARAM)g_animationLevel, 0);

    SendMessageW(particle, CB_RESETCONTENT, 0, 0);
    for (int i = 0; i <= 2; ++i) {
        std::wstring label = ParticleLevelName((ParticleLevel)i);
        SendMessageW(particle, CB_ADDSTRING, 0, (LPARAM)label.c_str());
    }
    SendMessageW(particle, CB_SETCURSEL, (WPARAM)g_particleLevel, 0);

    SendMessageW(report, CB_RESETCONTENT, 0, 0);
    for (int i = 0; i <= 3; ++i) {
        std::wstring label = ReportTemplateName((ReportTemplate)i);
        SendMessageW(report, CB_ADDSTRING, 0, (LPARAM)label.c_str());
    }
    SendMessageW(report, CB_SETCURSEL, (WPARAM)g_reportTemplate, 0);

    SendMessageW(theme, CB_RESETCONTENT, 0, 0);
    SendMessageW(theme, CB_ADDSTRING, 0, (LPARAM)Tr(L"Pure black", L"\u7eaf\u9ed1").c_str());
    SendMessageW(theme, CB_ADDSTRING, 0, (LPARAM)Tr(L"Pure white", L"\u7eaf\u767d").c_str());
    SendMessageW(theme, CB_SETCURSEL, (WPARAM)g_theme, 0);

    SendMessageW(accent, CB_RESETCONTENT, 0, 0);
    SendMessageW(accent, CB_ADDSTRING, 0, (LPARAM)Tr(L"Neutral", L"\u4e2d\u6027\u7070").c_str());
    SendMessageW(accent, CB_ADDSTRING, 0, (LPARAM)Tr(L"Muted sage", L"\u4f4e\u9971\u548c\u9f20\u5c3e\u8349").c_str());
    SendMessageW(accent, CB_ADDSTRING, 0, (LPARAM)Tr(L"Muted cyan", L"\u4f4e\u9971\u548c\u9752\u8272").c_str());
    SendMessageW(accent, CB_SETCURSEL, (WPARAM)g_accentTone, 0);

    SendMessageW(textScale, CB_RESETCONTENT, 0, 0);
    SendMessageW(textScale, CB_ADDSTRING, 0, (LPARAM)Tr(L"Small", L"\u5c0f").c_str());
    SendMessageW(textScale, CB_ADDSTRING, 0, (LPARAM)Tr(L"Standard", L"\u6807\u51c6").c_str());
    SendMessageW(textScale, CB_ADDSTRING, 0, (LPARAM)Tr(L"Large", L"\u5927").c_str());
    SendMessageW(textScale, CB_SETCURSEL, (WPARAM)g_textScale, 0);

    SendMessageW(density, CB_RESETCONTENT, 0, 0);
    SendMessageW(density, CB_ADDSTRING, 0, (LPARAM)Tr(L"Compact", L"\u7d27\u51d1").c_str());
    SendMessageW(density, CB_ADDSTRING, 0, (LPARAM)Tr(L"Comfortable", L"\u8212\u9002").c_str());
    SendMessageW(density, CB_ADDSTRING, 0, (LPARAM)Tr(L"Spacious", L"\u5bbd\u677e").c_str());
    SendMessageW(density, CB_SETCURSEL, (WPARAM)g_layoutDensity, 0);

    SendMessageW(autosave, CB_RESETCONTENT, 0, 0);
    SendMessageW(autosave, CB_ADDSTRING, 0, (LPARAM)Tr(L"Off", L"\u5173\u95ed").c_str());
    SendMessageW(autosave, CB_ADDSTRING, 0, (LPARAM)Tr(L"30 seconds", L"30 \u79d2").c_str());
    SendMessageW(autosave, CB_ADDSTRING, 0, (LPARAM)Tr(L"1 minute", L"1 \u5206\u949f").c_str());
    SendMessageW(autosave, CB_ADDSTRING, 0, (LPARAM)Tr(L"5 minutes", L"5 \u5206\u949f").c_str());
    int autosaveIndex = g_autosaveIntervalSeconds == 0 ? 0 : g_autosaveIntervalSeconds == 60 ? 2 : g_autosaveIntervalSeconds == 300 ? 3 : 1;
    SendMessageW(autosave, CB_SETCURSEL, autosaveIndex, 0);

    RedrawWindow(language, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
    RedrawWindow(font, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
    RedrawWindow(animation, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
    RedrawWindow(particle, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
    RedrawWindow(report, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
    RedrawWindow(theme, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
    RedrawWindow(accent, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
    RedrawWindow(textScale, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
    RedrawWindow(density, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
    RedrawWindow(autosave, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
    EnableWindow(particle, g_particlesEnabled ? TRUE : FALSE);
}

void ApplySettingsLanguage(HWND hwnd) {
    std::wstring settingsTitle = Tr(L"Settings", L"\u8bbe\u7f6e");
    SetWindowTextW(hwnd, settingsTitle.c_str());
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_TITLE), Tr(L"Interface Settings", L"\u754c\u9762\u8bbe\u7f6e"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_LANG_LABEL), Tr(L"Language", L"\u8bed\u8a00"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_FONT_LABEL), Tr(L"Interface Font", L"\u754c\u9762\u5b57\u4f53"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_ANIMATION_LABEL), Tr(L"Animation level", L"\u52a8\u753b\u5f3a\u5ea6"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_PARTICLE_LABEL), Tr(L"Particle density", L"\u7c92\u5b50\u5bc6\u5ea6"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_REPORT_LABEL), Tr(L"Report template", L"\u62a5\u544a\u6a21\u677f"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_PARTICLES), ParticleEffectsLabel());
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_RISK_ALERTS), Tr(L"Enable risk student reminders", L"\u542f\u7528\u98ce\u9669\u5b66\u751f\u63d0\u9192"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_AUTOSAVE_PROMPT), Tr(L"Prompt for autosave recovery", L"\u542f\u52a8\u65f6\u63d0\u793a\u6062\u590d\u81ea\u52a8\u4fdd\u5b58"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_COMMAND_PALETTE), Tr(L"Enable Ctrl+K command palette", L"\u542f\u7528 Ctrl+K \u547d\u4ee4\u9762\u677f"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_ADVANCED_FILTER), Tr(L"Enable advanced filter syntax", L"\u542f\u7528\u9ad8\u7ea7\u7b5b\u9009\u8bed\u6cd5"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_EXPERIMENTAL), Tr(L"Advanced personalization (Experimental)", L"\u9ad8\u7ea7\u4e2a\u6027\u5316\uff08\u5b9e\u9a8c\u6027\uff09"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_ADVANCED_TITLE), Tr(L"Advanced personalization", L"\u9ad8\u7ea7\u4e2a\u6027\u5316"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_THEME_LABEL), Tr(L"Theme", L"\u4e3b\u9898"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_ACCENT_LABEL), Tr(L"Accent tone", L"\u5f3a\u8c03\u8272\u8c03"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_TEXT_SCALE_LABEL), Tr(L"Text size", L"\u6587\u5b57\u5927\u5c0f"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_DENSITY_LABEL), Tr(L"Interface density", L"\u754c\u9762\u5bc6\u5ea6"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_AUTOSAVE_LABEL), Tr(L"Autosave interval", L"\u81ea\u52a8\u4fdd\u5b58\u95f4\u9694"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_SHOW_STATS), Tr(L"Show statistics cards", L"\u663e\u793a\u7edf\u8ba1\u5361\u7247"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_SHOW_HINT), Tr(L"Show bottom usage hint", L"\u663e\u793a\u5e95\u90e8\u4f7f\u7528\u63d0\u793a"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_TABLE_GRID), Tr(L"Show table grid lines", L"\u663e\u793a\u8868\u683c\u7f51\u683c\u7ebf"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_APPLY), Tr(L"Apply", L"\u5e94\u7528"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_CLOSE), Tr(L"Close", L"\u5173\u95ed"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_RESET), Tr(L"Reset All Settings", L"\u91cd\u7f6e\u6240\u6709\u8bbe\u7f6e"));
    SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_PARTICLES), BM_SETCHECK, g_particlesEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_RISK_ALERTS), BM_SETCHECK, g_riskAlertsEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_AUTOSAVE_PROMPT), BM_SETCHECK, g_autosavePromptEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_COMMAND_PALETTE), BM_SETCHECK, g_commandPaletteEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_ADVANCED_FILTER), BM_SETCHECK, g_advancedFilterEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_EXPERIMENTAL), BM_SETCHECK, g_advancedPersonalizationEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_SHOW_STATS), BM_SETCHECK, g_showStatsCards ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_SHOW_HINT), BM_SETCHECK, g_showFooterHint ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_TABLE_GRID), BM_SETCHECK, g_showTableGrid ? BST_CHECKED : BST_UNCHECKED, 0);
    FillSettingsCombos(hwnd);
}

void ApplySettingsFromWindow(HWND hwnd) {
    int language = (int)SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_LANGUAGE), CB_GETCURSEL, 0, 0);
    int fontIndex = (int)SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_FONT), CB_GETCURSEL, 0, 0);
    int animationIndex = (int)SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_ANIMATION_LEVEL), CB_GETCURSEL, 0, 0);
    int particleIndex = (int)SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_PARTICLE_LEVEL), CB_GETCURSEL, 0, 0);
    int reportIndex = (int)SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_REPORT_TEMPLATE), CB_GETCURSEL, 0, 0);
    int themeIndex = (int)SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_THEME), CB_GETCURSEL, 0, 0);
    int accentIndex = (int)SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_ACCENT), CB_GETCURSEL, 0, 0);
    int textScaleIndex = (int)SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_TEXT_SCALE), CB_GETCURSEL, 0, 0);
    int densityIndex = (int)SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_DENSITY), CB_GETCURSEL, 0, 0);
    int autosaveIndex = (int)SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_AUTOSAVE_INTERVAL), CB_GETCURSEL, 0, 0);

    if (language < 0 || language > (int)UiLanguage::ChineseTraditionalHongKong) language = 0;
    g_language = (UiLanguage)language;
    if (animationIndex >= 0 && animationIndex <= 2) g_animationLevel = (AnimationLevel)animationIndex;
    if (particleIndex >= 0 && particleIndex <= 2) g_particleLevel = (ParticleLevel)particleIndex;
    if (reportIndex >= 0 && reportIndex <= 3) g_reportTemplate = (ReportTemplate)reportIndex;
    if (g_advancedPersonalizationEnabled) {
        if (themeIndex >= 0 && themeIndex <= 1) g_theme = (UiTheme)themeIndex;
        if (accentIndex >= 0 && accentIndex <= 2) g_accentTone = (AccentTone)accentIndex;
        if (textScaleIndex >= 0 && textScaleIndex <= 2) g_textScale = (TextScale)textScaleIndex;
        if (densityIndex >= 0 && densityIndex <= 2) g_layoutDensity = (LayoutDensity)densityIndex;
        static const int autosaveSeconds[] = {0, 30, 60, 300};
        if (autosaveIndex >= 0 && autosaveIndex <= 3) g_autosaveIntervalSeconds = autosaveSeconds[autosaveIndex];
    } else {
        g_theme = UiTheme::Dark;
    }

    if (fontIndex >= 0) {
        std::wstring fontName = GetComboSelectedText(GetDlgItem(hwnd, IDC_SETTINGS_FONT));
        if (!fontName.empty()) g_fontFamily = fontName;
    }
    ApplyVisualSettings();
    EnumChildWindows(hwnd, ApplyFontToChild, 0);
    SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_TITLE), WM_SETFONT, (WPARAM)g_titleFont, TRUE);
    ApplySettingsLanguage(hwnd);
    SaveSettings();
    InvalidateRect(hwnd, nullptr, FALSE);
}

void ResizeSettingsLayout(HWND hwnd) {
    int pad = 24;
    int labelW = 170;
    int rowH = 34;
    int x = pad;
    int y = 22;
    int comboX = x + labelW;
    int comboW = 260;

    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_TITLE), x, y, labelW + comboW, 34, TRUE);
    y += 56;
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_LANG_LABEL), x, y + 6, labelW, rowH, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_LANGUAGE), comboX, y, comboW, 430, TRUE);
    y += 48;
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_FONT_LABEL), x, y + 6, labelW, rowH, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_FONT), comboX, y, comboW, 300, TRUE);
    y += 48;
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_ANIMATION_LABEL), x, y + 6, labelW, rowH, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_ANIMATION_LEVEL), comboX, y, comboW, 140, TRUE);
    y += 48;
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_PARTICLE_LABEL), x, y + 6, labelW, rowH, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_PARTICLE_LEVEL), comboX, y, comboW, 140, TRUE);
    y += 48;
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_REPORT_LABEL), x, y + 6, labelW, rowH, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_REPORT_TEMPLATE), comboX, y, comboW, 180, TRUE);
    y += 48;
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_PARTICLES), x, y, labelW + comboW, 30, TRUE);
    y += 38;
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_RISK_ALERTS), x, y, labelW + comboW, 30, TRUE);
    y += 38;
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_AUTOSAVE_PROMPT), x, y, labelW + comboW, 30, TRUE);
    y += 38;
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_COMMAND_PALETTE), x, y, labelW + comboW, 30, TRUE);
    y += 38;
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_ADVANCED_FILTER), x, y, labelW + comboW, 30, TRUE);
    y += 36;
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_EXPERIMENTAL), x, y, labelW + comboW, 30, TRUE);
    y += 42;
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_RESET), x, y, labelW + comboW, 36, TRUE);
    y += 44;
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_APPLY), comboX + comboW - 230, y, 110, 38, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_CLOSE), comboX + comboW - 110, y, 110, 38, TRUE);

    int advancedX = 584;
    int advancedComboX = advancedX + 170;
    int advancedY = 22;
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_ADVANCED_TITLE), advancedX, advancedY, 430, 34, TRUE);
    advancedY += 56;
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_THEME_LABEL), advancedX, advancedY + 6, 170, rowH, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_THEME), advancedComboX, advancedY, comboW, 120, TRUE);
    advancedY += 48;
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_ACCENT_LABEL), advancedX, advancedY + 6, 170, rowH, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_ACCENT), advancedComboX, advancedY, comboW, 140, TRUE);
    advancedY += 48;
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_TEXT_SCALE_LABEL), advancedX, advancedY + 6, 170, rowH, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_TEXT_SCALE), advancedComboX, advancedY, comboW, 140, TRUE);
    advancedY += 48;
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_DENSITY_LABEL), advancedX, advancedY + 6, 170, rowH, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_DENSITY), advancedComboX, advancedY, comboW, 140, TRUE);
    advancedY += 48;
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_AUTOSAVE_LABEL), advancedX, advancedY + 6, 170, rowH, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_AUTOSAVE_INTERVAL), advancedComboX, advancedY, comboW, 160, TRUE);
    advancedY += 54;
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_SHOW_STATS), advancedX, advancedY, 430, 30, TRUE);
    advancedY += 38;
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_SHOW_HINT), advancedX, advancedY, 430, 30, TRUE);
    advancedY += 38;
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_TABLE_GRID), advancedX, advancedY, 430, 30, TRUE);
}

void ResetAdvancedPersonalizationValues() {
    g_theme = UiTheme::Dark;
    g_accentTone = AccentTone::Neutral;
    g_textScale = TextScale::Standard;
    g_layoutDensity = LayoutDensity::Comfortable;
    g_showStatsCards = true;
    g_showFooterHint = true;
    g_showTableGrid = true;
    g_autosaveIntervalSeconds = 30;
}

void SetSettingsAdvancedExpanded(HWND hwnd, bool expanded) {
    const int advancedIds[] = {
        IDC_SETTINGS_ADVANCED_TITLE, IDC_SETTINGS_THEME_LABEL, IDC_SETTINGS_THEME,
        IDC_SETTINGS_ACCENT_LABEL, IDC_SETTINGS_ACCENT, IDC_SETTINGS_TEXT_SCALE_LABEL,
        IDC_SETTINGS_TEXT_SCALE, IDC_SETTINGS_DENSITY_LABEL, IDC_SETTINGS_DENSITY,
        IDC_SETTINGS_AUTOSAVE_LABEL, IDC_SETTINGS_AUTOSAVE_INTERVAL,
        IDC_SETTINGS_SHOW_STATS, IDC_SETTINGS_SHOW_HINT, IDC_SETTINGS_TABLE_GRID
    };
    for (int id : advancedIds) {
        HWND control = GetDlgItem(hwnd, id);
        if (control) ShowWindow(control, expanded ? SW_SHOW : SW_HIDE);
    }
    RECT rc{};
    GetWindowRect(hwnd, &rc);
    int targetWidth = expanded ? 1060 : 560;
    SetWindowPos(hwnd, nullptr, rc.left, rc.top, targetWidth, 690,
        SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    CenterOwnedWindow(hwnd, g_hwnd);
    ResizeSettingsLayout(hwnd);
    InvalidateRect(hwnd, nullptr, FALSE);
}

LRESULT CALLBACK SettingsProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        MakeSettingsControl(hwnd, L"STATIC", L"", 0, IDC_SETTINGS_TITLE);
        MakeSettingsControl(hwnd, L"STATIC", L"", 0, IDC_SETTINGS_LANG_LABEL);
        MakeSettingsControl(hwnd, L"COMBOBOX", L"", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP, IDC_SETTINGS_LANGUAGE);
        MakeSettingsControl(hwnd, L"STATIC", L"", 0, IDC_SETTINGS_FONT_LABEL);
        MakeSettingsControl(hwnd, L"COMBOBOX", L"", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP, IDC_SETTINGS_FONT);
        MakeSettingsControl(hwnd, L"STATIC", L"", 0, IDC_SETTINGS_ANIMATION_LABEL);
        MakeSettingsControl(hwnd, L"COMBOBOX", L"", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP, IDC_SETTINGS_ANIMATION_LEVEL);
        MakeSettingsControl(hwnd, L"STATIC", L"", 0, IDC_SETTINGS_PARTICLE_LABEL);
        MakeSettingsControl(hwnd, L"COMBOBOX", L"", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP, IDC_SETTINGS_PARTICLE_LEVEL);
        MakeSettingsControl(hwnd, L"STATIC", L"", 0, IDC_SETTINGS_REPORT_LABEL);
        MakeSettingsControl(hwnd, L"COMBOBOX", L"", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP, IDC_SETTINGS_REPORT_TEMPLATE);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, IDC_SETTINGS_PARTICLES);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, IDC_SETTINGS_RISK_ALERTS);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, IDC_SETTINGS_AUTOSAVE_PROMPT);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, IDC_SETTINGS_COMMAND_PALETTE);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, IDC_SETTINGS_ADVANCED_FILTER);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, IDC_SETTINGS_EXPERIMENTAL);

        MakeSettingsControl(hwnd, L"STATIC", L"", 0, IDC_SETTINGS_ADVANCED_TITLE);
        MakeSettingsControl(hwnd, L"STATIC", L"", 0, IDC_SETTINGS_THEME_LABEL);
        MakeSettingsControl(hwnd, L"COMBOBOX", L"", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP, IDC_SETTINGS_THEME);
        MakeSettingsControl(hwnd, L"STATIC", L"", 0, IDC_SETTINGS_ACCENT_LABEL);
        MakeSettingsControl(hwnd, L"COMBOBOX", L"", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP, IDC_SETTINGS_ACCENT);
        MakeSettingsControl(hwnd, L"STATIC", L"", 0, IDC_SETTINGS_TEXT_SCALE_LABEL);
        MakeSettingsControl(hwnd, L"COMBOBOX", L"", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP, IDC_SETTINGS_TEXT_SCALE);
        MakeSettingsControl(hwnd, L"STATIC", L"", 0, IDC_SETTINGS_DENSITY_LABEL);
        MakeSettingsControl(hwnd, L"COMBOBOX", L"", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP, IDC_SETTINGS_DENSITY);
        MakeSettingsControl(hwnd, L"STATIC", L"", 0, IDC_SETTINGS_AUTOSAVE_LABEL);
        MakeSettingsControl(hwnd, L"COMBOBOX", L"", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP, IDC_SETTINGS_AUTOSAVE_INTERVAL);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, IDC_SETTINGS_SHOW_STATS);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, IDC_SETTINGS_SHOW_HINT);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, IDC_SETTINGS_TABLE_GRID);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_PUSHBUTTON | BS_OWNERDRAW | WS_TABSTOP, IDC_SETTINGS_RESET);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_PUSHBUTTON | BS_OWNERDRAW | WS_TABSTOP, IDC_SETTINGS_APPLY);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_PUSHBUTTON | BS_OWNERDRAW | WS_TABSTOP, IDC_SETTINGS_CLOSE);
        SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_TITLE), WM_SETFONT, (WPARAM)g_titleFont, TRUE);
        SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_ADVANCED_TITLE), WM_SETFONT, (WPARAM)g_titleFont, TRUE);
        ApplySettingsLanguage(hwnd);
        ResizeSettingsLayout(hwnd);
        SetSettingsAdvancedExpanded(hwnd, g_advancedPersonalizationEnabled);
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
        if ((LOWORD(wParam) == IDC_SETTINGS_LANGUAGE || LOWORD(wParam) == IDC_SETTINGS_FONT)
            && HIWORD(wParam) == CBN_SELCHANGE) {
            CommitComboSelectionNow((HWND)lParam);
            ApplySettingsFromWindow(hwnd);
            return 0;
        }
        if ((LOWORD(wParam) == IDC_SETTINGS_ANIMATION_LEVEL
            || LOWORD(wParam) == IDC_SETTINGS_PARTICLE_LEVEL
            || LOWORD(wParam) == IDC_SETTINGS_REPORT_TEMPLATE
            || LOWORD(wParam) == IDC_SETTINGS_THEME
            || LOWORD(wParam) == IDC_SETTINGS_ACCENT
            || LOWORD(wParam) == IDC_SETTINGS_TEXT_SCALE
            || LOWORD(wParam) == IDC_SETTINGS_DENSITY
            || LOWORD(wParam) == IDC_SETTINGS_AUTOSAVE_INTERVAL) && HIWORD(wParam) == CBN_SELCHANGE) {
            CommitComboSelectionNow((HWND)lParam);
            ApplySettingsFromWindow(hwnd);
            return 0;
        }
        if (LOWORD(wParam) == IDC_SETTINGS_EXPERIMENTAL && HIWORD(wParam) == BN_CLICKED) {
            HWND toggle = GetDlgItem(hwnd, IDC_SETTINGS_EXPERIMENTAL);
            if (!g_advancedPersonalizationEnabled) {
                int result = ThemedMessageBox(hwnd,
                    Tr(L"This feature is experimental and may make the software unstable. Enable it?",
                       L"\u6b64\u529f\u80fd\u4e3a\u5b9e\u9a8c\u6027\uff0c\u53ef\u80fd\u4f1a\u9020\u6210\u8f6f\u4ef6\u7684\u4e0d\u7a33\u5b9a\u3002\u662f\u5426\u542f\u7528\uff1f"),
                    Tr(L"Experimental feature", L"\u5b9e\u9a8c\u6027\u529f\u80fd"), true);
                if (result != IDYES) {
                    SendMessageW(toggle, BM_SETCHECK, BST_UNCHECKED, 0);
                    InvalidateRect(toggle, nullptr, FALSE);
                    return 0;
                }
                g_advancedPersonalizationEnabled = true;
                SendMessageW(toggle, BM_SETCHECK, BST_CHECKED, 0);
                SaveSettings();
                SetSettingsAdvancedExpanded(hwnd, true);
                FillSettingsCombos(hwnd);
            } else {
                g_advancedPersonalizationEnabled = false;
                ResetAdvancedPersonalizationValues();
                SendMessageW(toggle, BM_SETCHECK, BST_UNCHECKED, 0);
                ApplyVisualSettings();
                SaveSettings();
                ApplySettingsLanguage(hwnd);
                SetSettingsAdvancedExpanded(hwnd, false);
            }
            InvalidateRect(toggle, nullptr, FALSE);
            return 0;
        }
        if ((LOWORD(wParam) == IDC_SETTINGS_PARTICLES
            || LOWORD(wParam) == IDC_SETTINGS_RISK_ALERTS
            || LOWORD(wParam) == IDC_SETTINGS_AUTOSAVE_PROMPT
            || LOWORD(wParam) == IDC_SETTINGS_COMMAND_PALETTE
            || LOWORD(wParam) == IDC_SETTINGS_ADVANCED_FILTER) && HIWORD(wParam) == BN_CLICKED) {
            int id = LOWORD(wParam);
            HWND toggle = GetDlgItem(hwnd, id);
            if (id == IDC_SETTINGS_PARTICLES) g_particlesEnabled = !g_particlesEnabled;
            else if (id == IDC_SETTINGS_RISK_ALERTS) g_riskAlertsEnabled = !g_riskAlertsEnabled;
            else if (id == IDC_SETTINGS_AUTOSAVE_PROMPT) g_autosavePromptEnabled = !g_autosavePromptEnabled;
            else if (id == IDC_SETTINGS_COMMAND_PALETTE) g_commandPaletteEnabled = !g_commandPaletteEnabled;
            else if (id == IDC_SETTINGS_ADVANCED_FILTER) g_advancedFilterEnabled = !g_advancedFilterEnabled;
            bool checked = id == IDC_SETTINGS_PARTICLES ? g_particlesEnabled
                : id == IDC_SETTINGS_RISK_ALERTS ? g_riskAlertsEnabled
                : id == IDC_SETTINGS_AUTOSAVE_PROMPT ? g_autosavePromptEnabled
                : id == IDC_SETTINGS_COMMAND_PALETTE ? g_commandPaletteEnabled
                : g_advancedFilterEnabled;
            SendMessageW(toggle, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
            if (id == IDC_SETTINGS_PARTICLES) {
                EnableWindow(GetDlgItem(hwnd, IDC_SETTINGS_PARTICLE_LEVEL), checked ? TRUE : FALSE);
            }
            SaveSettings();
            InvalidateRect(toggle, nullptr, FALSE);
            return 0;
        }
        if ((LOWORD(wParam) == IDC_SETTINGS_SHOW_STATS
            || LOWORD(wParam) == IDC_SETTINGS_SHOW_HINT
            || LOWORD(wParam) == IDC_SETTINGS_TABLE_GRID) && HIWORD(wParam) == BN_CLICKED) {
            int id = LOWORD(wParam);
            if (id == IDC_SETTINGS_SHOW_STATS) g_showStatsCards = !g_showStatsCards;
            else if (id == IDC_SETTINGS_SHOW_HINT) g_showFooterHint = !g_showFooterHint;
            else g_showTableGrid = !g_showTableGrid;
            bool checked = id == IDC_SETTINGS_SHOW_STATS ? g_showStatsCards
                : id == IDC_SETTINGS_SHOW_HINT ? g_showFooterHint : g_showTableGrid;
            SendMessageW((HWND)lParam, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
            ApplyVisualSettings();
            SaveSettings();
            InvalidateRect((HWND)lParam, nullptr, FALSE);
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
                SetSettingsAdvancedExpanded(hwnd, false);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }
        break;
    case WM_MEASUREITEM: {
        auto* measure = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
        if (measure->CtlType == ODT_COMBOBOX) {
            measure->itemHeight = ComboItemHeight();
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
        int id = GetDlgCtrlID(control);
        SetTextColor(hdc, (id == IDC_SETTINGS_TITLE || id == IDC_SETTINGS_ADVANCED_TITLE) ? COLOR_TEXT : COLOR_MUTED);
        SetBkMode(hdc, OPAQUE);
        SetBkColor(hdc, COLOR_BG);
        return (LRESULT)g_bgBrush;
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
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
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
        wc.style = CS_HREDRAW | CS_VREDRAW;
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
        WS_EX_CONTROLPARENT,
        className,
        settingsTitle.c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 560, 690,
        g_hwnd, nullptr, instance, nullptr
    );
    ApplyGlassTitleBar(g_settingsWindow);
    ApplyThemedControls(g_settingsWindow);
    CenterOwnedWindow(g_settingsWindow, g_hwnd);
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

    const bool compact = g_layoutDensity == LayoutDensity::Compact;
    const bool spacious = g_layoutDensity == LayoutDensity::Spacious;
    int pad = compact ? 20 : spacious ? 36 : 28;
    auto move = [&](int id, int x, int y, int width, int height) {
        HWND child = GetDlgItem(hwnd, id);
        if (child) MoveWindow(child, x - g_scrollX, y - g_scrollY, width, height, FALSE);
    };
    auto moveHwnd = [&](HWND child, int x, int y, int width, int height) {
        if (child) MoveWindow(child, x - g_scrollX, y - g_scrollY, width, height, FALSE);
    };

    int titleY = compact ? 14 : spacious ? 28 : 20;
    move(IDC_TITLE, pad, titleY, 430, 38);
    move(IDC_SUBTITLE, pad, titleY + 42, std::min(720, layoutW - pad * 2), 26);

    int courseW = 300;
    int courseBtnW = 124;
    int courseX = std::max(pad + 520, layoutW - pad - courseW - courseBtnW - 12);
    moveHwnd(g_courseCombo, courseX, titleY + 6, courseW, 430);
    move(IDC_COURSE_OPTIONS, courseX + courseW + 12, titleY + 6, courseBtnW, 38);

    int statsY = compact ? 90 : spacious ? 120 : 104;
    int statsGap = compact ? 12 : spacious ? 22 : 16;
    int statsH = g_showStatsCards ? (compact ? 78 : spacious ? 108 : 92) : 0;
    int statW = std::max(180, (layoutW - pad * 2 - statsGap * 3) / 4);
    int statX = pad;
    move(IDC_STAT_TOTAL, statX, statsY, statW, statsH);
    move(IDC_STAT_ATTENDANCE, statX + (statW + statsGap), statsY, statW, statsH);
    move(IDC_STAT_ISSUES, statX + (statW + statsGap) * 2, statsY, statW, statsH);
    move(IDC_STAT_VISIBLE, statX + (statW + statsGap) * 3, statsY, statW, statsH);

    int sectionGap = compact ? 12 : spacious ? 24 : 18;
    int filterY = g_showStatsCards ? statsY + statsH + sectionGap : statsY;
    int listX = pad;
    int listW = std::max(720, layoutW - pad * 2);
    int clearW = 136;
    int filterLabelW = 74;
    int filterW = std::min(460, std::max(240, listW / 3));
    int fieldH = compact ? 32 : spacious ? 44 : 36;
    int fieldLabelOffset = std::max(4, (fieldH - 24) / 2);
    move(IDC_FILTER_LABEL, listX, filterY + fieldLabelOffset, filterLabelW, 24);
    moveHwnd(g_filterEdit, listX + filterLabelW, filterY, filterW, fieldH);
    move(IDC_CLEAR_FILTER, listX + filterLabelW + filterW + 12, filterY, clearW, fieldH);

    int dockPrimaryH = g_layoutDensity == LayoutDensity::Compact ? 50
        : g_layoutDensity == LayoutDensity::Spacious ? 64 : 56;
    int drawerH = g_layoutDensity == LayoutDensity::Compact ? 36
        : g_layoutDensity == LayoutDensity::Spacious ? 44 : 40;
    int bottomPad = compact ? 22 : spacious ? 36 : 28;
    int dockGap = compact ? 10 : spacious ? 22 : 16;
    int composerGap = compact ? 54 : spacious ? 82 : 66;
    int listGap = compact ? 42 : spacious ? 64 : 52;
    int listBottomGap = compact ? 14 : spacious ? 28 : 20;
    int drawerY = std::max(filterY + 430, layoutH - drawerH - bottomPad);
    int dockY = drawerY - dockPrimaryH - dockGap;
    int composerY = dockY - composerGap;
    int listY = filterY + listGap;
    int listH = std::max(260, composerY - listY - listBottomGap);
    moveHwnd(g_list, listX, listY, listW, listH);

    int composerW = std::min(1060, layoutW - pad * 2);
    int composerX = pad + (layoutW - pad * 2 - composerW) / 2;
    int labelY = composerY - (compact ? 16 : spacious ? 22 : 18);
    int dateW = std::max(210, composerW / 4);
    int nameW = std::max(240, composerW / 4);
    int otherW = composerW - dateW - nameW - 32;
    move(2001, composerX, labelY, dateW, 18);
    int editorH = compact ? 34 : spacious ? 46 : 38;
    moveHwnd(g_dateEdit, composerX, composerY, dateW, editorH);
    move(2002, composerX + dateW + 16, labelY, nameW, 18);
    moveHwnd(g_nameEdit, composerX + dateW + 16, composerY, nameW, editorH);
    move(2003, composerX + dateW + nameW + 32, labelY, otherW, 18);
    moveHwnd(g_otherEdit, composerX + dateW + nameW + 32, composerY, otherW, editorH);

    int primaryGap = compact ? 8 : spacious ? 18 : 12;
    int primaryW = std::clamp((layoutW - pad * 2 - primaryGap * 5) / 6, 128, 168);
    int primaryTotalW = primaryW * 6 + primaryGap * 5;
    int primaryX = pad + (layoutW - pad * 2 - primaryTotalW) / 2;
    move(IDC_PRESENT, primaryX, dockY, primaryW, dockPrimaryH);
    move(IDC_ABSENT, primaryX + (primaryW + primaryGap), dockY, primaryW, dockPrimaryH);
    move(IDC_LATE, primaryX + (primaryW + primaryGap) * 2, dockY, primaryW, dockPrimaryH);
    move(IDC_OTHER_STATUS, primaryX + (primaryW + primaryGap) * 3, dockY, primaryW, dockPrimaryH);
    move(IDC_SAVE, primaryX + (primaryW + primaryGap) * 4, dockY, primaryW, dockPrimaryH);
    move(IDC_EXPORT_CSV, primaryX + (primaryW + primaryGap) * 5, dockY, primaryW, dockPrimaryH);

    int drawerGap = compact ? 6 : spacious ? 14 : 10;
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

    int footerStatsW = std::min(720, std::max(420, layoutW - pad * 2));
    int footerStatsX = std::max(6, layoutW - footerStatsW - 6);
    int hintW = std::max(260, footerStatsX - 18);
    move(IDC_HINT, 6, layoutH - 20, hintW, 18);
    move(IDC_STATS, footerStatsX, layoutH - 20, footerStatsW, 18);

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
    UINT redrawFlags = RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_NOERASE;
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
}

void ApplyDarkMode(HWND hwnd) {
    BOOL value = g_theme == UiTheme::Dark ? TRUE : FALSE;
    if (hwnd) DwmSetWindowAttribute(hwnd, 20, &value, sizeof(value));
    if (hwnd == g_hwnd) ApplyGlassTitleBar(hwnd);
    ApplyThemedControls(hwnd);
    if (g_list) {
        SetWindowTheme(g_list, g_theme == UiTheme::Dark ? L"DarkMode_Explorer" : L"Explorer", nullptr);
        HWND header = ListView_GetHeader(g_list);
        if (header) {
            SetWindowTheme(header, g_theme == UiTheme::Dark ? L"DarkMode_Explorer" : L"Explorer", nullptr);
            InvalidateRect(header, nullptr, FALSE);
        }
        ListView_SetBkColor(g_list, COLOR_PANEL);
        ListView_SetTextBkColor(g_list, COLOR_PANEL);
        ListView_SetTextColor(g_list, COLOR_TEXT);
        DWORD listStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP;
        if (g_showTableGrid) listStyle |= LVS_EX_GRIDLINES;
        ListView_SetExtendedListViewStyle(g_list, listStyle);
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
    case IDC_QUICK_PRESENT:
    case IDC_QUICK_ABSENT:
    case IDC_QUICK_LATE:
    case IDC_QUICK_OTHER:
    case IDC_QUICK_FINISH:
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
    if (id == IDC_SETTINGS_PARTICLES || id == IDC_SETTINGS_RISK_ALERTS
        || id == IDC_SETTINGS_AUTOSAVE_PROMPT || id == IDC_SETTINGS_COMMAND_PALETTE
        || id == IDC_SETTINGS_ADVANCED_FILTER || id == IDC_SETTINGS_EXPERIMENTAL
        || id == IDC_SETTINGS_SHOW_STATS || id == IDC_SETTINGS_SHOW_HINT
        || id == IDC_SETTINGS_TABLE_GRID) return;
    bool particlesEnabled = g_particlesEnabled && IsCoreParticleButton(id);
    bool rippleEnabled = g_animationLevel != AnimationLevel::Off;
    if (!rippleEnabled && !particlesEnabled) {
        g_buttonEffects.erase(hwnd);
        return;
    }

    ButtonEffectState state;
    state.origin = origin;
    state.startMs = AnimationNowMs();
    state.rippleEnabled = rippleEnabled;
    state.durationMs = rippleEnabled ? 400u : 0u;

    if (particlesEnabled) {
        int minCount = 30;
        int maxCount = 50;
        if (g_particleLevel == ParticleLevel::Low) {
            minCount = 12;
            maxCount = 22;
        } else if (g_particleLevel == ParticleLevel::High) {
            minCount = 50;
            maxCount = 72;
        }
        int count = RandomInt(minCount, maxCount);
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
            particle.color = RandomInt(0, 12) == 0
                ? COLOR_ACCENT
                : (g_theme == UiTheme::Dark ? RGB(255, 255, 255) : RGB(0, 0, 0));
            state.particles.push_back(particle);
            state.durationMs = std::max(state.durationMs, particle.lifetimeMs);
        }
    }

    g_buttonEffects[hwnd] = std::move(state);
    RestartAnimation(hwnd, AnimChannel::Effect, 1.0, g_buttonEffects[hwnd].durationMs + 50);
    RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
}

void DrawButtonEffects(const DRAWITEMSTRUCT* draw, const RECT& buttonRc, COLORREF baseFill) {
    auto it = g_buttonEffects.find(draw->hwndItem);
    if (it == g_buttonEffects.end()) return;

    uint64_t now = AnimationNowMs();
    ButtonEffectState& state = it->second;
    uint64_t elapsedMs = now > state.startMs ? now - state.startMs : 0;
    if (elapsedMs > state.durationMs + 40) {
        g_buttonEffects.erase(it);
        return;
    }

    HRGN clip = CreateRoundRectRgn(buttonRc.left, buttonRc.top, buttonRc.right + 1, buttonRc.bottom + 1, 6, 6);
    SelectClipRgn(draw->hDC, clip);

    double rippleProgress = std::clamp(elapsedMs / 400.0, 0.0, 1.0);
    if (state.rippleEnabled && rippleProgress < 1.0) {
        double eased = EaseOut(rippleProgress);
        int radius = (int)std::lround(80.0 * eased);
        double strength = 0.30 * (1.0 - rippleProgress);
        COLORREF rippleTarget = g_theme == UiTheme::Dark ? RGB(255, 255, 255) : RGB(0, 0, 0);
        COLORREF rippleColor = BlendColor(baseFill, rippleTarget, strength);
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
        COLORREF color = BlendColor(particle.color, COLOR_MUTED, progress);
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
    if (!draw || draw->CtlType != ODT_BUTTON) return false;
    bool* value = nullptr;
    if (draw->CtlID == IDC_SETTINGS_PARTICLES) value = &g_particlesEnabled;
    else if (draw->CtlID == IDC_SETTINGS_RISK_ALERTS) value = &g_riskAlertsEnabled;
    else if (draw->CtlID == IDC_SETTINGS_AUTOSAVE_PROMPT) value = &g_autosavePromptEnabled;
    else if (draw->CtlID == IDC_SETTINGS_COMMAND_PALETTE) value = &g_commandPaletteEnabled;
    else if (draw->CtlID == IDC_SETTINGS_ADVANCED_FILTER) value = &g_advancedFilterEnabled;
    else if (draw->CtlID == IDC_SETTINGS_EXPERIMENTAL) value = &g_advancedPersonalizationEnabled;
    else if (draw->CtlID == IDC_SETTINGS_SHOW_STATS) value = &g_showStatsCards;
    else if (draw->CtlID == IDC_SETTINGS_SHOW_HINT) value = &g_showFooterHint;
    else if (draw->CtlID == IDC_SETTINGS_TABLE_GRID) value = &g_showTableGrid;
    else return false;

    wchar_t text[160]{};
    GetWindowTextW(draw->hwndItem, text, 160);

    RECT rc = draw->rcItem;
    HBRUSH bg = CreateSolidBrush(COLOR_BG);
    FillRect(draw->hDC, &rc, bg);
    DeleteObject(bg);

    bool checked = *value;
    double hover = GetAnimationValue(draw->hwndItem, AnimChannel::Hover, 0.0);
    bool focused = (draw->itemState & ODS_FOCUS) != 0;
    COLORREF border = BlendColor(COLOR_BORDER, COLOR_ACCENT, focused ? 1.0 : hover);

    int box = 18;
    int boxTop = (int)rc.top + std::max(0, ((int)(rc.bottom - rc.top) - box) / 2);
    RECT boxRc{
        rc.left + 2,
        boxTop,
        rc.left + 2 + box,
        boxTop + box
    };

    HBRUSH fill = CreateSolidBrush(COLOR_CHECK_FILL);
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

    COLORREF fill = BlendColor(COLOR_INPUT, COLOR_HOVER, hover);
    COLORREF border = BlendColor(COLOR_BORDER, COLOR_ACCENT, focused ? 1.0 : hover);
    COLORREF buttonText = BlendColor(COLOR_MUTED, COLOR_TEXT, std::clamp(0.45 + hover * 0.55, 0.0, 1.0));

    bool isDanger = draw->CtlID == IDC_DELETE || draw->CtlID == IDC_SETTINGS_RESET;
    bool isGreen = draw->CtlID == IDC_PRESENT || draw->CtlID == IDC_SAVE || draw->CtlID == IDC_ALL_PRESENT || draw->CtlID == IDC_SETTINGS_APPLY;
    bool isBlue = draw->CtlID == IDC_IMPORT || draw->CtlID == IDC_NEW || draw->CtlID == IDC_EXPORT_CSV;

    if (isDanger) {
        COLORREF dangerBase = BlendColor(COLOR_INPUT, COLOR_DANGER, g_theme == UiTheme::Dark ? 0.12 : 0.08);
        fill = BlendColor(dangerBase, COLOR_DANGER, hover * 0.18);
        border = BlendColor(COLOR_BORDER, COLOR_DANGER, focused ? 1.0 : 0.45 + hover * 0.55);
        buttonText = COLOR_TEXT;
    } else if (isGreen) {
        COLORREF success = g_theme == UiTheme::Dark ? RGB(146, 166, 156) : RGB(74, 112, 92);
        fill = BlendColor(COLOR_INPUT, success, 0.08 + hover * 0.12);
        border = BlendColor(COLOR_BORDER, success, focused ? 1.0 : 0.35 + hover * 0.65);
    } else if (isBlue) {
        fill = BlendColor(COLOR_INPUT, COLOR_ACCENT, 0.04 + hover * 0.10);
        border = BlendColor(COLOR_BORDER, COLOR_ACCENT, focused ? 1.0 : hover);
    }

    fill = BlendColor(fill, COLOR_SELECTED, press * 0.45);

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
    HGDIOBJ oldFont = g_font ? SelectObject(draw->hDC, g_font) : nullptr;
    DrawTextW(draw->hDC, text, -1, &buttonRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    if (oldFont) SelectObject(draw->hDC, oldFont);
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
    COLORREF rowFill = selected ? COLOR_SELECTED : COLOR_INPUT;
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
    HGDIOBJ oldFont = g_font ? SelectObject(draw->hDC, g_font) : nullptr;
    DrawTextW(draw->hDC, text.c_str(), -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    if (oldFont) SelectObject(draw->hDC, oldFont);
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
    COLORREF fill = enabled ? BlendColor(COLOR_INPUT, COLOR_HOVER, active) : COLOR_PANEL;
    COLORREF border = BlendColor(COLOR_BORDER, COLOR_ACCENT, focused ? 1.0 : active);
    COLORREF arrowFill = BlendColor(COLOR_PANEL, COLOR_SELECTED, active);
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
        InvalidateRect(hwnd, nullptr, FALSE);
        return result;
    }
    case WM_APP_ANIMATION_TICK: {
        RedrawAnimatedControl(hwnd);
        COMBOBOXINFO info{};
        info.cbSize = sizeof(info);
        if (GetComboBoxInfo(hwnd, &info) && info.hwndList) {
            RedrawWindow(info.hwndList, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
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

    const wchar_t* themeName = g_theme == UiTheme::Dark ? L"DarkMode_Explorer" : L"Explorer";
    SetWindowTheme(combo, themeName, nullptr);
    if (info.hwndItem) SetWindowTheme(info.hwndItem, themeName, nullptr);
    if (info.hwndList) {
        SetWindowTheme(info.hwndList, themeName, nullptr);
        InvalidateRect(info.hwndList, nullptr, FALSE);
        RedrawWindow(info.hwndList, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME | RDW_NOERASE);
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
    COLORREF fill = COLOR_PANEL;
    COLORREF border = COLOR_BORDER;
    HBRUSH bg = CreateSolidBrush(fill);
    FillRect(hdc, &full, bg);
    DeleteObject(bg);

    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, COLOR_TEXT);
    HGDIOBJ oldFont = g_font ? SelectObject(hdc, g_font) : nullptr;

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
            HBRUSH fill = CreateSolidBrush(COLOR_CARD);
            HPEN pen = CreatePen(PS_SOLID, 1, COLOR_CARD);
            HGDIOBJ oldBrush = SelectObject(hdc, fill);
            HGDIOBJ oldPen = SelectObject(hdc, pen);
            RoundRect(hdc, cardRc.left, cardRc.top, cardRc.right, cardRc.bottom, 8, 8);
            SelectObject(hdc, oldPen);
            SelectObject(hdc, oldBrush);
            DeleteObject(pen);
            DeleteObject(fill);

            for (int glow = 0; glow < 8; ++glow) {
                double amount = 0.050 * (1.0 - glow / 8.0);
                COLORREF glowTarget = g_theme == UiTheme::Dark ? RGB(255, 255, 255) : RGB(0, 0, 0);
                COLORREF glowColor = BlendColor(COLOR_CARD, glowTarget, amount);
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
        InvalidateRect(hwnd, nullptr, FALSE);
        if (g_autosaveIntervalSeconds > 0) {
            SetTimer(hwnd, 1, (UINT)g_autosaveIntervalSeconds * 1000, nullptr);
        }
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
        if (g_openThemedPopupMenus > 0) return 0;
        ScrollMainWindow(hwnd, SB_VERT, 0, GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;
    case WM_MOUSEHWHEEL:
        if (g_openThemedPopupMenus > 0) return 0;
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
            SetTimer(hwnd, 2, 120, nullptr);
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
            KillTimer(hwnd, 2);
            SetText(g_filterEdit, L"");
            KillTimer(hwnd, 2);
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
        if (ShortcutMatches(g_shortcutSave, wParam)) {
            SaveAttendance();
            return 0;
        }
        if (ShortcutMatches(g_shortcutImport, wParam)) {
            ImportAttendance();
            return 0;
        }
        if (ShortcutMatches(g_shortcutUndo, wParam)) {
            UndoLast();
            return 0;
        }
        if (ShortcutMatches(g_shortcutRedo, wParam)) {
            RedoLast();
            return 0;
        }
        if (ShortcutMatches(g_shortcutCommand, wParam)) {
            RunCommandPalette();
            return 0;
        }
        if (ShortcutMatches(g_shortcutFullscreen, wParam)) {
            ToggleFullscreen(hwnd);
            return 0;
        }
        break;
    case WM_TIMER:
        if (wParam == 1) AutoSaveNow();
        else if (wParam == 2) {
            KillTimer(hwnd, 2);
            RefreshList();
        }
        return 0;
    case WM_MEASUREITEM: {
        auto* measure = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
        if (measure->CtlType == ODT_COMBOBOX) {
            measure->itemHeight = ComboItemHeight();
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
        SetBkMode(hdc, OPAQUE);
        SetBkColor(hdc, COLOR_BG);
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
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        KillTimer(hwnd, 2);
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
        WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN,
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
        if (g_quickRollCallWindow && IsWindow(g_quickRollCallWindow)
            && (msg.hwnd == g_quickRollCallWindow || IsChild(g_quickRollCallWindow, msg.hwnd))) {
            if (msg.message == WM_KEYDOWN) {
                HWND notes = GetDlgItem(g_quickRollCallWindow, IDC_QUICK_NOTES);
                bool editingNotes = GetFocus() == notes;
                bool globalKey = msg.wParam == VK_ESCAPE;
                bool rollCallKey = msg.wParam == L'1' || msg.wParam == L'2' || msg.wParam == L'3' || msg.wParam == L'4'
                    || msg.wParam == VK_NUMPAD1 || msg.wParam == VK_NUMPAD2 || msg.wParam == VK_NUMPAD3 || msg.wParam == VK_NUMPAD4
                    || msg.wParam == VK_LEFT || msg.wParam == VK_RIGHT;
                if (globalKey || (!editingNotes && rollCallKey)) {
                    SendMessageW(g_quickRollCallWindow, WM_KEYDOWN, msg.wParam, msg.lParam);
                    continue;
                }
            }
            if (IsDialogMessageW(g_quickRollCallWindow, &msg)) continue;
        }
        if (g_settingsWindow && IsWindow(g_settingsWindow)
            && (msg.hwnd == g_settingsWindow || IsChild(g_settingsWindow, msg.hwnd))) {
            if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
                SendMessageW(g_settingsWindow, WM_KEYDOWN, msg.wParam, msg.lParam);
                continue;
            }
            if (IsDialogMessageW(g_settingsWindow, &msg)) continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}
