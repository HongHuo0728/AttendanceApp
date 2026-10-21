#pragma once

#include "core/platform.h"

namespace attendance {

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
    std::unordered_map<std::wstring, int> recordByStudent;
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

enum class AttendanceLoadIntent { OpenFile, RestoreAutosave, RestoreBackup, CommandLine };

struct SettingToggleBinding {
    int id;
    bool* value;
};

struct ThemePalette {
    COLORREF background = RGB(0, 0, 0);
    COLORREF panel = RGB(18, 18, 18);
    COLORREF input = RGB(26, 26, 26);
    COLORREF text = RGB(255, 255, 255);
    COLORREF muted = RGB(224, 224, 224);
    COLORREF accent = RGB(188, 188, 188);
    COLORREF danger = RGB(176, 112, 112);
    COLORREF border = RGB(64, 64, 64);
    COLORREF hover = RGB(38, 38, 38);
    COLORREF card = RGB(16, 16, 16);
    COLORREF rowAlt = RGB(14, 14, 14);
    COLORREF selected = RGB(42, 42, 42);
    COLORREF checkFill = RGB(0, 0, 0);
};

struct ThemeTransitionState {
    ThemePalette start;
    ThemePalette target;
    UiTheme targetTheme = UiTheme::Dark;
    bool active = false;
};

enum class AnimChannel : int {
    Hover = 1,
    Press = 2,
    ComboOpen = 3,
    Reveal = 4,
    Close = 5,
    ListReveal = 6,
    Effect = 7,
    ThemeTransition = 8,
    SmoothScroll = 9
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
    bool active = false;
    std::vector<ButtonParticle> particles;
};

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
    double smoothScrollStartY = 0.0;
    double smoothScrollCurrentY = 0.0;
    double smoothScrollTargetY = 0.0;
    uint64_t smoothScrollStartMs = 0;
    uint32_t smoothScrollDurationMs = 220;
    HDC bufferDc = nullptr;
    HBITMAP bufferBitmap = nullptr;
    HGDIOBJ bufferOldBitmap = nullptr;
    int bufferWidth = 0;
    int bufferHeight = 0;
};

enum class AccentState : int {
    Disabled = 0,
    BlurBehind = 3,
    AcrylicBlurBehind = 4
};

struct AccentPolicy {
    AccentState state = AccentState::Disabled;
    DWORD flags = 0;
    DWORD gradientColor = 0;
    DWORD animationId = 0;
};

struct WindowCompositionAttributeData {
    int attribute = 0;
    void* data = nullptr;
    SIZE_T size = 0;
};

using SetWindowCompositionAttributeFn = BOOL(WINAPI*)(HWND, WindowCompositionAttributeData*);

struct TitleBarVisualState {
    COLORREF caption = RGB(18, 18, 18);
    COLORREF text = RGB(255, 255, 255);
    COLORREF border = RGB(64, 64, 64);
    bool dark = true;
};

struct SettingsSnapshot {
    UiLanguage language = UiLanguage::English;
    UiTheme theme = UiTheme::Dark;
    AccentTone accentTone = AccentTone::Neutral;
    TextScale textScale = TextScale::Standard;
    LayoutDensity layoutDensity = LayoutDensity::Comfortable;
    std::wstring fontFamily = L"Segoe UI";
    std::wstring defaultSaveDir;
    AnimationLevel animationLevel = AnimationLevel::Advanced;
    ParticleLevel particleLevel = ParticleLevel::Medium;
    ReportTemplate reportTemplate = ReportTemplate::Complete;
    bool particlesEnabled = true;
    bool riskAlertsEnabled = true;
    bool autosavePromptEnabled = true;
    bool commandPaletteEnabled = true;
    bool advancedFilterEnabled = true;
    bool smoothScrollingEnabled = true;
    bool showStatsCards = true;
    bool showFooterHint = true;
    bool showTableGrid = true;
    int autosaveIntervalSeconds = 30;
    int scrollAnimationFps = 60;
    std::wstring shortcutSave = L"Ctrl+S";
    std::wstring shortcutImport = L"Ctrl+O";
    std::wstring shortcutUndo = L"Ctrl+Z";
    std::wstring shortcutRedo = L"Ctrl+Y";
    std::wstring shortcutCommand = L"Ctrl+K";
    std::wstring shortcutFullscreen = L"F11";
};

enum class WorkbookValidationIssue {
    None,
    TooManySheets,
    TooManyStudents,
    TooManyRecords,
    FieldTooLarge
};

struct ZipEntry {
    std::string name;
    std::string data;
    uint32_t crc = 0;
    uint32_t offset = 0;
};

struct PptStats {
    int present = 0;
    int absent = 0;
    int late = 0;
    int other = 0;
    int total = 0;
    int leave = 0;
    int pending = 0;
    int Eligible() const { return total - leave - pending; }
};

enum class AutosaveResult { Skipped, Saved, Failed };

struct ComboListScrollState {
    HWND combo = nullptr;
    double startPx = 0.0;
    double currentPx = 0.0;
    double targetPx = 0.0;
    uint64_t startMs = 0;
    uint32_t durationMs = 220;
    int hoverIndex = -1;
    bool active = false;
    HDC bufferDc = nullptr;
    HBITMAP bufferBitmap = nullptr;
    HGDIOBJ bufferOldBitmap = nullptr;
    int bufferWidth = 0;
    int bufferHeight = 0;
    std::vector<std::wstring> cachedItems;
};

} // namespace attendance
