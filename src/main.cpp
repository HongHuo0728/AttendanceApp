#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <shlobj.h>
#include <uxtheme.h>

#include <algorithm>
#include <cstdint>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "../include/resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")

struct AttendanceRecord {
    std::wstring dateTime;
    std::wstring name;
    std::wstring status;
    std::wstring other;
};

struct AttendanceSheet {
    std::wstring name;
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
static constexpr int IDC_TITLE = 2004;
static constexpr int IDC_SUBTITLE = 2005;
static constexpr int IDC_STATS = 2006;
static constexpr int IDC_HINT = 2007;

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
static constexpr int IDC_INPUT_EDIT = 5001;
static constexpr int IDC_INPUT_OK = 5002;
static constexpr int IDC_INPUT_CANCEL = 5003;

static HWND g_hwnd = nullptr;
static HWND g_dateEdit = nullptr;
static HWND g_nameEdit = nullptr;
static HWND g_otherEdit = nullptr;
static HWND g_list = nullptr;
static HWND g_courseCombo = nullptr;
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
static int g_activeSheet = 0;
static std::vector<std::string> g_undoStack;
static std::vector<std::string> g_redoStack;
static bool g_fullscreen = false;
static WINDOWPLACEMENT g_previousPlacement{sizeof(g_previousPlacement)};
static DWORD g_previousStyle = 0;

struct InputDialogState {
    std::wstring title;
    std::wstring prompt;
    std::wstring value;
    bool accepted = false;
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
    Spanish
};
enum class UiTheme { Dark, Light };

static UiLanguage g_language = UiLanguage::English;
static UiTheme g_theme = UiTheme::Dark;
static std::wstring g_fontFamily = L"Segoe UI";

static COLORREF COLOR_BG = RGB(24, 26, 32);
static COLORREF COLOR_PANEL = RGB(34, 37, 45);
static COLORREF COLOR_INPUT = RGB(44, 48, 58);
static COLORREF COLOR_TEXT = RGB(235, 238, 245);
static COLORREF COLOR_MUTED = RGB(168, 174, 190);
static COLORREF COLOR_ACCENT = RGB(64, 156, 255);
static COLORREF COLOR_DANGER = RGB(238, 91, 91);

void ResizeLayout(HWND hwnd);
void ApplyVisualSettings();
void ApplyDarkMode(HWND hwnd);
std::string WideToUtf8(const std::wstring& input);
std::wstring Utf8ToWide(const std::string& input);
bool LoadAttendanceFile(const std::wstring& path, bool showSuccess);
void PushUndo();
HWND MakeSettingsControl(HWND parent, const wchar_t* cls, const wchar_t* text, DWORD style, int id);
std::wstring Tr(const wchar_t* english, const wchar_t* chinese);

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
        MakeSettingsControl(hwnd, L"BUTTON", okText.c_str(), BS_PUSHBUTTON | WS_TABSTOP, IDC_INPUT_OK);
        MakeSettingsControl(hwnd, L"BUTTON", cancelText.c_str(), BS_PUSHBUTTON | WS_TABSTOP, IDC_INPUT_CANCEL);
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
            DestroyWindow(hwnd);
            return 0;
        }
        if (LOWORD(wParam) == IDC_INPUT_CANCEL) {
            DestroyWindow(hwnd);
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
    case WM_ERASEBKGND: {
        RECT rc{};
        GetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, g_bgBrush);
        return 1;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
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
    default: return L"English";
    }
}

std::wstring Tr(const wchar_t* english, const wchar_t*) {
    std::wstring key = english;
    if (g_language == UiLanguage::English) return key;

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
        {L"Present", L"\u51fa\u5e2d", L"Pre\u017centi", L"\u51fa\u5e2d", L"Pr\u00e9sent", L"Anwesend", L"\u041f\u0440\u0438\u0441\u0443\u0442\u0441\u0442\u0432\u0443\u0435\u0442", L"\u51fa\u5e2d", L"Presente"},
        {L"Absent", L"\u7f3a\u5e2d", L"Assenti", L"\u6b20\u5e2d", L"Absent", L"Abwesend", L"\u041e\u0442\u0441\u0443\u0442\u0441\u0442\u0432\u0443\u0435\u0442", L"\u7f3a\u5e2d", L"Ausente"},
        {L"Late", L"\u8fdf\u5230", L"Tard", L"\u9045\u523b", L"En retard", L"Versp\u00e4tet", L"\u041e\u043f\u043e\u0437\u0434\u0430\u043b", L"\u9072\u5230", L"Tarde"},
        {L"Total", L"\u603b\u6570", L"Total", L"\u5408\u8a08", L"Total", L"Gesamt", L"\u0412\u0441\u0435\u0433\u043e", L"\u7e3d\u6578", L"Total"},
        {L"Attendance", L"\u51fa\u52e4\u7387", L"Attendenza", L"\u51fa\u5e2d\u7387", L"Pr\u00e9sence", L"Anwesenheit", L"\u041f\u043e\u0441\u0435\u0449\u0430\u0435\u043c\u043e\u0441\u0442\u044c", L"\u51fa\u52e4\u7387", L"Asistencia"},
        {L"Absent/Late", L"\u7f3a\u5e2d/\u8fdf\u5230", L"Assenti/Tard", L"\u6b20\u5e2d/\u9045\u523b", L"Absent/retard", L"Abwesend/versp\u00e4tet", L"\u041e\u0442\u0441\u0443\u0442./\u043e\u043f\u043e\u0437\u0434.", L"\u7f3a\u5e2d/\u9072\u5230", L"Ausente/tarde"},
        {L"Update Selected", L"\u66f4\u65b0\u9009\u4e2d", L"A\u0121\u0121orna mag\u0127\u017cul", L"\u9078\u629e\u3092\u66f4\u65b0", L"Mettre \u00e0 jour", L"Auswahl aktualisieren", L"\u041e\u0431\u043d\u043e\u0432\u0438\u0442\u044c", L"\u66f4\u65b0\u9078\u53d6", L"Actualizar selecci\u00f3n"},
        {L"Edit Selected", L"\u7f16\u8f91\u9009\u4e2d", L"Editja mag\u0127\u017cul", L"\u9078\u629e\u3092\u7de8\u96c6", L"Modifier", L"Auswahl bearbeiten", L"\u0418\u0437\u043c\u0435\u043d\u0438\u0442\u044c", L"\u7de8\u8f2f\u9078\u53d6", L"Editar selecci\u00f3n"},
        {L"Mark All Present", L"\u5168\u5458\u51fa\u5e2d", L"Kollha pre\u017centi", L"\u5168\u54e1\u51fa\u5e2d", L"Tout pr\u00e9sent", L"Alle anwesend", L"\u0412\u0441\u0435 \u043f\u0440\u0438\u0441\u0443\u0442\u0441\u0442\u0432\u0443\u044e\u0442", L"\u5168\u54e1\u51fa\u5e2d", L"Todos presentes"},
        {L"Create New", L"\u65b0\u5efa\u70b9\u540d", L"\u0120did", L"\u65b0\u898f\u4f5c\u6210", L"Nouveau", L"Neu erstellen", L"\u0421\u043e\u0437\u0434\u0430\u0442\u044c", L"\u65b0\u589e\u9ede\u540d", L"Crear nuevo"},
        {L"Delete Options", L"\u5220\u9664\u9009\u9879", L"G\u0127a\u017cliet ta' t\u0127assir", L"\u524a\u9664\u30aa\u30d7\u30b7\u30e7\u30f3", L"Options de suppression", L"L\u00f6schoptionen", L"\u041f\u0430\u0440\u0430\u043c\u0435\u0442\u0440\u044b \u0443\u0434\u0430\u043b\u0435\u043d\u0438\u044f", L"\u522a\u9664\u9078\u9805", L"Opciones de borrar"},
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
        {L"Print / export PDF", L"\u6253\u5370 / \u5bfc\u51fa PDF", L"Stampa / esporta PDF", L"\u5370\u5237 / PDF \u51fa\u529b", L"Imprimer / exporter PDF", L"Drucken / PDF exportieren", L"\u041f\u0435\u0447\u0430\u0442\u044c / PDF", L"\u5217\u5370 / \u532f\u51fa PDF", L"Imprimir / exportar PDF"},
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
        {L"Could not write the database file.", L"\u65e0\u6cd5\u5199\u5165\u6570\u636e\u5e93\u6587\u4ef6\u3002", L"Ma setax jinkiteb id-database.", L"\u30c7\u30fc\u30bf\u30d9\u30fc\u30b9\u30d5\u30a1\u30a4\u30eb\u3092\u66f8\u304d\u8fbc\u3081\u307e\u305b\u3093\u3002", L"Impossible d'\u00e9crire la base.", L"Datenbankdatei konnte nicht geschrieben werden.", L"\u041d\u0435 \u0443\u0434\u0430\u043b\u043e\u0441\u044c \u0437\u0430\u043f\u0438\u0441\u0430\u0442\u044c \u0411\u0414.", L"\u7121\u6cd5\u5beb\u5165\u8cc7\u6599\u5eab\u6a94\u6848\u3002", L"No se pudo escribir la base."},
        {L"Database mirror exported to:", L"\u6570\u636e\u5e93\u955c\u50cf\u5df2\u5bfc\u51fa\u5230\uff1a", L"Kopja tad-database esportata lejn:", L"\u30c7\u30fc\uタ\u30d9\u30fc\u30b9\u30df\u30e9\u30fc\u306e\u51fa\u529b\u5148:", L"Miroir de base export\u00e9 vers :", L"Datenbankspiegel exportiert nach:", L"\u0417\u0435\u0440\u043a\u0430\u043b\u043e \u0411\u0414 \u044d\u043a\u0441\u043f\u043e\u0440\u0442\u0438\u0440\u043e\u0432\u0430\u043d\u043e:", L"\u8cc7\u6599\u5eab\u93e1\u50cf\u5df2\u532f\u51fa\u5230\uff1a", L"Espejo exportado a:"},
        {L"No autosave file was found.", L"\u672a\u627e\u5230\u81ea\u52a8\u4fdd\u5b58\u6587\u4ef6\u3002", L"Ma nstabx autosave.", L"\u81ea\u52d5\u4fdd\u5b58\u30d5\u30a1\u30a4\u30eb\u306f\u3042\u308a\u307e\u305b\u3093\u3002", L"Aucune autosauvegarde trouv\u00e9e.", L"Keine Autosave-Datei gefunden.", L"\u0410\u0432\u0442\u043e\u0441\u043e\u0445\u0440. \u043d\u0435 \u043d\u0430\u0439\u0434\u0435\u043d\u043e.", L"\u672a\u627e\u5230\u81ea\u52d5\u5132\u5b58\u6a94\u6848\u3002", L"No se encontr\u00f3 autoguardado."},
        {L"An autosaved attendance file was found. Restore it now?", L"\u627e\u5230\u81ea\u52a8\u4fdd\u5b58\u7684\u70b9\u540d\u6587\u4ef6\u3002\u662f\u5426\u73b0\u5728\u6062\u590d\uff1f", L"Instab fajl autosave. Tirrestawrah issa?", L"\u81ea\u52d5\u4fdd\u5b58\u304c\u898b\u3064\u304b\u308a\u307e\u3057\u305f\u3002\u5fa9\u5143\u3057\u307e\u3059\u304b\uff1f", L"Autosauvegarde trouv\u00e9e. Restaurer ?", L"Autosave gefunden. Jetzt wiederherstellen?", L"\u041d\u0430\u0439\u0434\u0435\u043d\u043e \u0430\u0432\u0442\u043e\u0441\u043e\u0445\u0440. \u0412\u043e\u0441\u0441\u0442\u0430\u043d\u043e\u0432\u0438\u0442\u044c?", L"\u627e\u5230\u81ea\u52d5\u5132\u5b58\u6a94\u3002\u662f\u5426\u73fe\u5728\u9084\u539f\uff1f", L"Se encontr\u00f3 autoguardado. \u00bfRestaurar?"},
        {L"Restore Autosave", L"\u6062\u590d\u81ea\u52a8\u4fdd\u5b58", L"Irrestawra autosave", L"\u81ea\u52d5\u4fdd\u5b58\u5fa9\u5143", L"Restaurer autosauvegarde", L"Autosave wiederherstellen", L"\u0412\u043e\u0441\u0441\u0442. \u0430\u0432\u0442\u043e\u0441\u043e\u0445\u0440.", L"\u9084\u539f\u81ea\u52d5\u5132\u5b58", L"Restaurar autoguardado"},
        {L"Keyboard Shortcuts", L"\u5feb\u6377\u952e", L"Shortcuts", L"\u30b7\u30e7\u30fc\u30c8\u30ab\u30c3\u30c8", L"Raccourcis clavier", L"Tastenk\u00fcrzel", L"\u0413\u043e\u0440\u044f\u0447\u0438\u0435 \u043a\u043b\u0430\u0432\u0438\u0448\u0438", L"\u5feb\u6377\u9375", L"Atajos de teclado"},
        {L"Statistics Chart", L"\u7edf\u8ba1\u56fe\u8868", L"\u010aart tal-istatistika", L"\u7d71\u8a08\u30b0\u30e9\u30d5", L"Graphique statistique", L"Statistikdiagramm", L"\u0413\u0440\u0430\u0444\u0438\u043a", L"\u7d71\u8a08\u5716\u8868", L"Gr\u00e1fico estad\u00edstico"},
        {L"Print / Save as PDF", L"\u6253\u5370 / \u53e6\u5b58\u4e3a PDF", L"Stampa / PDF", L"\u5370\u5237 / PDF\u4fdd\u5b58", L"Imprimer / PDF", L"Drucken / PDF speichern", L"\u041f\u0435\u0447\u0430\u0442\u044c / PDF", L"\u5217\u5370 / \u53e6\u5b58 PDF", L"Imprimir / guardar PDF"},
        {L"Fullscreen", L"\u5168\u5c4f", L"Skrin s\u0127i\u0127", L"\u5168\u753b\u9762", L"Plein \u00e9cran", L"Vollbild", L"\u041f\u043e\u043b\u043d\u044b\u0439 \u044d\u043a\u0440\u0430\u043d", L"\u5168\u87a2\u5e55", L"Pantalla completa"},
        {L"Double-click row: Edit selected record", L"\u53cc\u51fb\u884c\uff1a\u7f16\u8f91\u9009\u4e2d\u8bb0\u5f55", L"Ikklikkja darbtejn: editja", L"\u884c\u3092\u30c0\u30d6\u30eb\u30af\u30ea\u30c3\u30af\uff1a\u7de8\u96c6", L"Double-clic : modifier", L"Doppelklick: bearbeiten", L"\u0414\u0432\u043e\u0439\u043d\u043e\u0439 \u0449\u0435\u043b\u0447\u043e\u043a: \u043f\u0440\u0430\u0432\u043a\u0430", L"\u96d9\u64ca\u884c\uff1a\u7de8\u8f2f\u9078\u53d6\u8a18\u9304", L"Doble clic: editar"},
        {L"Ctrl/Shift click: Multi-select rows", L"Ctrl/Shift \u70b9\u51fb\uff1a\u591a\u9009\u884c", L"Ctrl/Shift: ag\u0127\u017cel aktar", L"Ctrl/Shift\uff1a\u8907\u6570\u9078\u629e", L"Ctrl/Shift : multi-s\u00e9lection", L"Strg/Umschalt: Mehrfachauswahl", L"Ctrl/Shift: \u043c\u0443\u043b\u044c\u0442\u0438\u0432\u044b\u0431\u043e\u0440", L"Ctrl/Shift \u9ede\u64ca\uff1a\u591a\u9078\u884c", L"Ctrl/Shift: selecci\u00f3n m\u00faltiple"},
        {L"Date / Time", L"\u65e5\u671f / \u65f6\u95f4", L"Data / \u0126in", L"\u65e5\u6642", L"Date / heure", L"Datum / Uhrzeit", L"\u0414\u0430\u0442\u0430 / \u0432\u0440\u0435\u043c\u044f", L"\u65e5\u671f / \u6642\u9593", L"Fecha / hora"},
        {L"Status", L"\u72b6\u6001", L"Status", L"\u72b6\u614b", L"Statut", L"Status", L"\u0421\u0442\u0430\u0442\u0443\u0441", L"\u72c0\u614b", L"Estado"},
        {L"Interface Settings", L"\u754c\u9762\u8bbe\u7f6e", L"Settings tal-interface", L"\u30a4\u30f3\u30bf\u30fc\u30d5\u30a7\u30fc\u30b9\u8a2d\u5b9a", L"Param\u00e8tres de l'interface", L"Oberfl\u00e4cheneinstellungen", L"\u041d\u0430\u0441\u0442\u0440\u043e\u0439\u043a\u0438 \u0438\u043d\u0442\u0435\u0440\u0444\u0435\u0439\u0441\u0430", L"\u介面設定", L"Configuraci\u00f3n de interfaz"},
        {L"Language", L"\u8bed\u8a00", L"Lingwa", L"\u8a00\u8a9e", L"Langue", L"Sprache", L"\u042f\u0437\u044b\u043a", L"\u8a9e\u8a00", L"Idioma"},
        {L"Style", L"\u98ce\u683c", L"Stil", L"\u30b9\u30bf\u30a4\u30eb", L"Style", L"Stil", L"\u0421\u0442\u0438\u043b\u044c", L"\u98a8\u683c", L"Estilo"},
        {L"Interface Font", L"\u754c\u9762\u5b57\u4f53", L"Font tal-interface", L"\u30a4\u30f3\u30bf\u30fc\u30d5\u30a7\u30fc\u30b9\u30d5\u30a9\u30f3\u30c8", L"Police de l'interface", L"Schriftart", L"\u0428\u0440\u0438\u0444\u0442", L"\u介面字型", L"Fuente de interfaz"},
        {L"Apply", L"\u5e94\u7528", L"Applika", L"\u9069\u7528", L"Appliquer", L"Anwenden", L"\u041f\u0440\u0438\u043c\u0435\u043d\u0438\u0442\u044c", L"\u5957\u7528", L"Aplicar"},
        {L"Close", L"\u5173\u95ed", L"Ag\u0127laq", L"\u9589\u3058\u308b", L"Fermer", L"Schlie\u00dfen", L"\u0417\u0430\u043a\u0440\u044b\u0442\u044c", L"\u95dc\u9589", L"Cerrar"},
        {L"Reset All Settings", L"\u91cd\u7f6e\u6240\u6709\u8bbe\u7f6e", L"Irrisettja kollox", L"\u3059\u3079\u3066\u30ea\u30bb\u30c3\u30c8", L"Tout r\u00e9initialiser", L"Alles zur\u00fccksetzen", L"\u0421\u0431\u0440\u043e\u0441\u0438\u0442\u044c \u0432\u0441\u0451", L"\u91cd\u8a2d\u6240\u6709\u8a2d\u5b9a", L"Restablecer todo"},
        {L"Dark", L"\u6df1\u8272", L"Skur", L"\u30c0\u30fc\u30af", L"Sombre", L"Dunkel", L"\u0422\u0451\u043c\u043d\u0430\u044f", L"\u6df1\u8272", L"Oscuro"},
        {L"Light", L"\u6d45\u8272", L"\u010aar", L"\u30e9\u30a4\u30c8", L"Clair", L"Hell", L"\u0421\u0432\u0435\u0442\u043b\u0430\u044f", L"\u6dfa\u8272", L"Claro"},
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
    return UiLanguage::English;
}

void SaveSettings() {
    auto path = SettingsFilePath();
    if (path.empty()) return;
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    if (!file) return;
    file << "language=" << LanguageToString(g_language) << "\n";
    file << "theme=" << (g_theme == UiTheme::Light ? "light" : "dark") << "\n";
    file << "font=" << WideToUtf8(g_fontFamily) << "\n";
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
        else if (key == "theme") g_theme = value == "light" ? UiTheme::Light : UiTheme::Dark;
        else if (key == "font" && !value.empty()) g_fontFamily = Utf8ToWide(value);
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
    g_fontFamily = L"Segoe UI";
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
        g_sheets.push_back({L"Default Course", g_records});
        g_activeSheet = 0;
    }
}

void SyncActiveSheet() {
    EnsureSheets();
    if (g_activeSheet >= 0 && g_activeSheet < (int)g_sheets.size()) {
        g_sheets[g_activeSheet].records = g_records;
    }
}

std::string SerializeWorkbook() {
    SyncActiveSheet();
    std::ostringstream ss;
    ss << "ATTENDANCE_V2\n";
    ss << g_sheets.size() << "\n";
    for (const auto& sheet : g_sheets) {
        ss << Escape(WideToUtf8(sheet.name)) << "\n";
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
    if (header == "ATTENDANCE_V1") {
        std::vector<AttendanceRecord> records;
        if (!DeserializeRecords(plainText, records)) return false;
        output = {{L"Default Course", std::move(records)}};
        return true;
    }
    if (header != "ATTENDANCE_V2") return false;

    std::string countLine;
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
        if (!std::getline(ss, recordCountLine)) return false;
        size_t recordCount = 0;
        try {
            recordCount = std::stoull(recordCountLine);
        } catch (...) {
            return false;
        }

        AttendanceSheet sheet;
        sheet.name = Utf8ToWide(Unescape(nameLine));
        if (sheet.name.empty()) sheet.name = L"Course";
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

    if (sheets.empty()) sheets.push_back({L"Default Course", {}});
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
    MessageBoxW(g_hwnd, text.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
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
    wchar_t rateText[160]{};
    swprintf_s(rateText, L"    %s %.1f%%    %s %.1f%%",
        Tr(L"Attendance", L"\u51fa\u52e4\u7387").c_str(), attendanceRate,
        Tr(L"Absent/Late", L"\u7f3a\u5e2d/\u8fdf\u5230").c_str(), issueRate);

    std::wstringstream ss;
    ss << Tr(L"Total", L"\u603b\u6570") << L" " << g_records.size()
       << L"    " << Tr(L"Present", L"\u51fa\u5e2d") << L" " << present
       << L"    " << Tr(L"Absent", L"\u7f3a\u5e2d") << L" " << absent
       << L"    " << Tr(L"Late", L"\u8fdf\u5230") << L" " << late
       << L"    " << Tr(L"Other", L"\u5176\u4ed6") << L" " << other
       << rateText;
    SetText(GetDlgItem(g_hwnd, IDC_STATS), ss.str());
}

void RefreshList() {
    ListView_DeleteAllItems(g_list);
    for (int i = 0; i < (int)g_records.size(); ++i) {
        LVITEMW item{};
        item.mask = LVIF_TEXT;
        item.iItem = i;
        item.pszText = const_cast<wchar_t*>(g_records[i].dateTime.c_str());
        ListView_InsertItem(g_list, &item);
        ListView_SetItemText(g_list, i, 1, const_cast<wchar_t*>(g_records[i].name.c_str()));
        ListView_SetItemText(g_list, i, 2, const_cast<wchar_t*>(g_records[i].status.c_str()));
        ListView_SetItemText(g_list, i, 3, const_cast<wchar_t*>(g_records[i].other.c_str()));
    }
    UpdateStats();
}

void RefreshCourseCombo() {
    if (!g_courseCombo) return;
    EnsureSheets();
    SendMessageW(g_courseCombo, CB_RESETCONTENT, 0, 0);
    for (const auto& sheet : g_sheets) {
        SendMessageW(g_courseCombo, CB_ADDSTRING, 0, (LPARAM)sheet.name.c_str());
    }
    SendMessageW(g_courseCombo, CB_SETCURSEL, g_activeSheet, 0);
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
    g_sheets.push_back({name, {}});
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
    if (MessageBoxW(g_hwnd, deleteCourseMsg.c_str(), deleteCourseTitle.c_str(), MB_YESNO | MB_ICONWARNING) != IDYES) {
        return;
    }
    PushUndo();
    g_sheets.erase(g_sheets.begin() + g_activeSheet);
    g_activeSheet = std::max(0, g_activeSheet - 1);
    g_records = g_sheets[g_activeSheet].records;
    RefreshCourseCombo();
    RefreshList();
}

void ShowCourseMenu(HWND button) {
    HMENU menu = CreatePopupMenu();
    std::wstring addCourseText = Tr(L"Add course/class", L"\u65b0\u589e\u8bfe\u7a0b/\u73ed\u7ea7");
    std::wstring renameCourseText = Tr(L"Rename current course/class", L"\u91cd\u547d\u540d\u5f53\u524d\u8bfe\u7a0b/\u73ed\u7ea7");
    std::wstring deleteCourseText = Tr(L"Delete current course/class", L"\u5220\u9664\u5f53\u524d\u8bfe\u7a0b/\u73ed\u7ea7");
    AppendMenuW(menu, MF_STRING, IDM_COURSE_ADD, addCourseText.c_str());
    AppendMenuW(menu, MF_STRING, IDM_COURSE_RENAME, renameCourseText.c_str());
    AppendMenuW(menu, MF_STRING, IDM_COURSE_DELETE, deleteCourseText.c_str());
    RECT rc{};
    GetWindowRect(button, &rc);
    int command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTALIGN | TPM_TOPALIGN, rc.right, rc.bottom + 4, 0, g_hwnd, nullptr);
    DestroyMenu(menu);
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
    if (g_theme == UiTheme::Dark) {
        COLOR_BG = RGB(24, 26, 32);
        COLOR_PANEL = RGB(34, 37, 45);
        COLOR_INPUT = RGB(44, 48, 58);
        COLOR_TEXT = RGB(235, 238, 245);
        COLOR_MUTED = RGB(168, 174, 190);
        COLOR_ACCENT = RGB(64, 156, 255);
        COLOR_DANGER = RGB(238, 91, 91);
    } else {
        COLOR_BG = RGB(245, 247, 251);
        COLOR_PANEL = RGB(255, 255, 255);
        COLOR_INPUT = RGB(255, 255, 255);
        COLOR_TEXT = RGB(24, 28, 36);
        COLOR_MUTED = RGB(91, 99, 118);
        COLOR_ACCENT = RGB(24, 119, 242);
        COLOR_DANGER = RGB(196, 54, 54);
    }
    ResetBrushes();
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
    }
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
    SetText(GetDlgItem(g_hwnd, IDC_TITLE), Tr(L"Attendance Manager", L"点名管理器"));
    SetText(GetDlgItem(g_hwnd, IDC_SUBTITLE),
        Tr(L"Create, edit, export, save, import, and batch clean .attd roll calls.",
           L"创建、编辑、导出、保存、导入，并批量整理 .attd 点名记录。"));
    SetText(GetDlgItem(g_hwnd, IDC_HINT),
        Tr(L"Tip: double-click a row to edit. Ctrl/Shift supports multi-select.",
           L"提示：双击记录可编辑，Ctrl/Shift 可多选。"));
    SetText(GetDlgItem(g_hwnd, 2001), Tr(L"Date/Time", L"日期时间"));
    SetText(GetDlgItem(g_hwnd, 2002), Tr(L"Name", L"姓名"));
    SetText(GetDlgItem(g_hwnd, 2003), Tr(L"Other", L"其他"));
    SetText(GetDlgItem(g_hwnd, IDC_PRESENT), Tr(L"Present", L"出席"));
    SetText(GetDlgItem(g_hwnd, IDC_ABSENT), Tr(L"Absent", L"缺席"));
    SetText(GetDlgItem(g_hwnd, IDC_LATE), Tr(L"Late", L"迟到"));
    SetText(GetDlgItem(g_hwnd, IDC_OTHER_STATUS), Tr(L"Other", L"其他"));
    SetText(GetDlgItem(g_hwnd, IDC_ADD_UPDATE), Tr(L"Update Selected", L"更新选中"));
    SetText(GetDlgItem(g_hwnd, IDC_EDIT_SELECTED), Tr(L"Edit Selected", L"编辑选中"));
    SetText(GetDlgItem(g_hwnd, IDC_ALL_PRESENT), Tr(L"Mark All Present", L"全员出席"));
    SetText(GetDlgItem(g_hwnd, IDC_NEW), Tr(L"Create New", L"新建点名"));
    SetText(GetDlgItem(g_hwnd, IDC_DELETE), Tr(L"Delete Options", L"删除选项"));
    SetText(GetDlgItem(g_hwnd, IDC_SAVE), Tr(L"Save .attd", L"保存 .attd"));
    SetText(GetDlgItem(g_hwnd, IDC_IMPORT), Tr(L"Import .attd", L"导入 .attd"));
    SetText(GetDlgItem(g_hwnd, IDC_EXPORT_CSV), Tr(L"Export CSV", L"导出 CSV"));
    SetText(GetDlgItem(g_hwnd, IDC_SETTINGS), Tr(L"Settings", L"设置"));
    SetText(GetDlgItem(g_hwnd, IDC_COURSE_OPTIONS), Tr(L"Courses", L"\u8bfe\u7a0b"));
    SetText(GetDlgItem(g_hwnd, IDC_TOOLS), Tr(L"Tools", L"\u5de5\u5177"));

    SetListColumnText(0, Tr(L"Date / Time", L"日期 / 时间"));
    SetListColumnText(1, Tr(L"Name", L"姓名"));
    SetListColumnText(2, Tr(L"Status", L"状态"));
    SetListColumnText(3, Tr(L"Other", L"其他"));
    UpdateStats();
}

void ApplyVisualSettings() {
    ApplyThemePalette();
    RecreateFonts();
    ApplyMainLanguage();
    if (g_list) {
        ApplyDarkMode(g_hwnd);
        ListView_SetBkColor(g_list, COLOR_PANEL);
        ListView_SetTextBkColor(g_list, COLOR_PANEL);
        ListView_SetTextColor(g_list, COLOR_TEXT);
    }
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
        ShowMessage(L"Please enter a date and time.");
        return;
    }
    if (name.empty()) {
        ShowMessage(L"Please enter a name.");
        return;
    }
    if (status == L"Other" && other.empty()) {
        ShowMessage(L"Please fill the Other field.");
        return;
    }

    int selected = ListView_GetNextItem(g_list, -1, LVNI_SELECTED);
    AttendanceRecord record{dateTime, name, status, other};
    PushUndo();
    if (selected >= 0 && selected < (int)g_records.size()) {
        g_records[selected] = record;
    } else {
        g_records.push_back(record);
    }
    RefreshList();
}

void LoadRecordIntoEditor(int index) {
    if (index < 0 || index >= (int)g_records.size()) {
        ShowMessage(L"Please select a record to edit.");
        return;
    }

    const auto& record = g_records[index];
    SetText(g_dateEdit, record.dateTime);
    SetText(g_nameEdit, record.name);
    SetText(g_otherEdit, record.other);
    ListView_SetItemState(g_list, index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(g_list, index, FALSE);
}

void UpdateSelectedRecord() {
    int selected = ListView_GetNextItem(g_list, -1, LVNI_SELECTED);
    if (selected < 0 || selected >= (int)g_records.size()) {
        AddOrUpdateRecord(L"Present");
        return;
    }

    std::wstring dateTime = GetText(g_dateEdit);
    std::wstring name = GetText(g_nameEdit);
    std::wstring other = GetText(g_otherEdit);
    if (dateTime.empty()) {
        ShowMessage(L"Please enter a date and time.");
        return;
    }
    if (name.empty()) {
        ShowMessage(L"Please enter a name.");
        return;
    }

    PushUndo();
    g_records[selected].dateTime = dateTime;
    g_records[selected].name = name;
    g_records[selected].other = g_records[selected].status == L"Other" ? other : L"";
    RefreshList();
    ListView_SetItemState(g_list, selected, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
}

void MarkAllPresent() {
    if (g_records.empty()) {
        ShowMessage(L"There are no records to mark.");
        return;
    }
    if (MessageBoxW(g_hwnd, L"Mark every record as Present?", L"All Present", MB_YESNO | MB_ICONQUESTION) == IDYES) {
        PushUndo();
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
        if (selected >= 0 && selected < (int)g_records.size()) rows.push_back(selected);
    }
    return rows;
}

void EraseRows(std::vector<int> rows) {
    PushUndo();
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
        ShowMessage(L"Please select one or more records to delete.");
        return;
    }

    std::wstringstream ss;
    ss << L"Delete " << rows.size() << L" selected record(s)?";
    if (MessageBoxW(g_hwnd, ss.str().c_str(), L"Delete Selected", MB_YESNO | MB_ICONWARNING) == IDYES) {
        EraseRows(rows);
    }
}

void DeleteRecordsByStatus(const std::wstring& status) {
    std::vector<int> rows;
    for (int i = 0; i < (int)g_records.size(); ++i) {
        if (g_records[i].status == status) rows.push_back(i);
    }

    if (rows.empty()) {
        ShowMessage(L"No matching records found.");
        return;
    }

    std::wstringstream ss;
    ss << L"Delete all " << status << L" records? (" << rows.size() << L")";
    if (MessageBoxW(g_hwnd, ss.str().c_str(), L"Batch Delete", MB_YESNO | MB_ICONWARNING) == IDYES) {
        EraseRows(rows);
    }
}

void ClearAllRecords() {
    if (g_records.empty()) {
        ShowMessage(L"There are no records to clear.");
        return;
    }
    if (MessageBoxW(g_hwnd, L"Clear every attendance record in this sheet?", L"Clear All", MB_YESNO | MB_ICONWARNING) == IDYES) {
        PushUndo();
        g_records.clear();
        RefreshList();
    }
}

void ShowDeleteMenu(HWND button) {
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, IDM_DELETE_SELECTED, L"Delete selected records");
    AppendMenuW(menu, MF_STRING, IDM_DELETE_ABSENT, L"Delete all Absent records");
    AppendMenuW(menu, MF_STRING, IDM_DELETE_LATE, L"Delete all Late records");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, IDM_CLEAR_ALL, L"Clear all records");

    RECT rc{};
    GetWindowRect(button, &rc);
    int command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTALIGN | TPM_TOPALIGN, rc.right, rc.bottom + 4, 0, g_hwnd, nullptr);
    DestroyMenu(menu);

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
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    return GetSaveFileNameW(&ofn) ? fileName : L"";
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
        ShowMessage(L"There are no records to export.");
        return;
    }

    std::wstring path = SaveCsvFileDialog();
    if (path.empty()) return;

    std::ofstream file(std::filesystem::path(path), std::ios::binary);
    if (!file) {
        ShowMessage(L"Could not export the CSV file.");
        return;
    }

    file << "\xEF\xBB\xBF";
    file << "Date/Time,Name,Status,Other\n";
    for (const auto& record : g_records) {
        file << CsvCell(record.dateTime) << ','
             << CsvCell(record.name) << ','
             << CsvCell(record.status) << ','
             << CsvCell(record.other) << '\n';
    }
    ShowMessage(L"CSV exported successfully.");
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
    g_activeSheet = std::min(g_activeSheet, (int)g_sheets.size() - 1);
    if (g_activeSheet < 0) g_activeSheet = 0;
    g_records = g_sheets[g_activeSheet].records;
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
    file << "<!doctype html><meta charset='utf-8'><title>Attendance</title>";
    file << "<style>body{font-family:Segoe UI,Arial;margin:32px}table{border-collapse:collapse;width:100%}"
            "th,td{border:1px solid #999;padding:8px;text-align:left}th{background:#eee}"
            "@media print{button{display:none}}</style>";
    file << "<button onclick='window.print()'>" << HtmlCell(Tr(L"Print / Save as PDF", L"\u6253\u5370 / \u53e6\u5b58\u4e3a PDF")) << "</button>";
    file << "<h1>" << HtmlCell(g_sheets[g_activeSheet].name) << "</h1>";
    file << "<table><tr><th>Date/Time</th><th>Name</th><th>Status</th><th>Other</th></tr>";
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
    PushUndo();
    std::string line;
    int added = 0;
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
        g_records.push_back({CurrentDateTimeText(), name, L"Absent", L""});
        ++added;
    }
    RefreshList();
    std::wstringstream ss;
    ss << Tr(L"Imported", L"\u5df2\u5bfc\u5165") << L" " << added << L" " << Tr(L"students into the current course.", L"\u540d\u5b66\u751f\u5230\u5f53\u524d\u8bfe\u7a0b\u3002");
    ShowMessage(ss.str());
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
    file << "course\tdate_time\tname\tstatus\tother\n";
    for (const auto& sheet : g_sheets) {
        for (const auto& r : sheet.records) {
            file << CsvCell(sheet.name) << '\t' << CsvCell(r.dateTime) << '\t' << CsvCell(r.name)
                 << '\t' << CsvCell(r.status) << '\t' << CsvCell(r.other) << '\n';
        }
    }
    std::wstring msg = Tr(L"Database mirror exported to:", L"\u6570\u636e\u5e93\u955c\u50cf\u5df2\u5bfc\u51fa\u5230\uff1a") + L"\n" + path.wstring();
    ShowMessage(msg);
}

void AutoSaveNow() {
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
    LoadAttendanceFile(path.wstring(), true);
}

void PromptRestoreAutosave() {
    auto path = AppDataFilePath(L"autosave.attd");
    if (path.empty() || !std::filesystem::exists(path)) return;
    std::wstring restoreMsg = Tr(L"An autosaved attendance file was found. Restore it now?", L"\u627e\u5230\u81ea\u52a8\u4fdd\u5b58\u7684\u70b9\u540d\u6587\u4ef6\u3002\u662f\u5426\u73b0\u5728\u6062\u590d\uff1f");
    std::wstring restoreTitle = Tr(L"Restore Autosave", L"\u6062\u590d\u81ea\u52a8\u4fdd\u5b58");
    if (MessageBoxW(
        g_hwnd,
        restoreMsg.c_str(),
        restoreTitle.c_str(),
        MB_YESNO | MB_ICONQUESTION
    ) == IDYES) {
        LoadAttendanceFile(path.wstring(), false);
    }
}

void ShowShortcuts() {
    std::wstring text =
        L"F11: " + Tr(L"Fullscreen", L"\u5168\u5c4f") + L"\n" +
        L"Ctrl+S: " + Tr(L"Save .attd", L"\u4fdd\u5b58 .attd") + L"\n" +
        L"Ctrl+O: " + Tr(L"Import .attd", L"\u5bfc\u5165 .attd") + L"\n" +
        L"Ctrl+Z: " + Tr(L"Undo", L"\u64a4\u9500") + L"\n" +
        L"Ctrl+Y: " + Tr(L"Redo", L"\u91cd\u505a") + L"\n" +
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
    COLORREF colors[] = {RGB(40, 167, 119), RGB(222, 78, 78), RGB(230, 168, 47), RGB(108, 117, 125)};
    int maxValue = std::max(1, std::max(std::max(present, absent), std::max(late, other)));

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
        HBRUSH bg = CreateSolidBrush(g_theme == UiTheme::Dark ? RGB(45, 49, 60) : RGB(224, 229, 238));
        FillRect(hdc, &barBg, bg);
        DeleteObject(bg);

        int width = (int)(barMaxW * (values[i] / (double)maxValue));
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
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
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
    ShowWindow(g_chartWindow, SW_SHOW);
    UpdateWindow(g_chartWindow);
}

void ShowToolsMenu(HWND button) {
    HMENU menu = CreatePopupMenu();
    std::wstring roster = Tr(L"Import student roster (CSV)", L"\u5bfc\u5165\u5b66\u751f\u540d\u5355 (CSV)");
    std::wstring print = Tr(L"Print / export PDF", L"\u6253\u5370 / \u5bfc\u51fa PDF");
    std::wstring chart = Tr(L"Statistics chart", L"\u7edf\u8ba1\u56fe\u8868");
    std::wstring undo = Tr(L"Undo", L"\u64a4\u9500");
    std::wstring redo = Tr(L"Redo", L"\u91cd\u505a");
    std::wstring shortcuts = Tr(L"Keyboard shortcuts", L"\u5feb\u6377\u952e");
    std::wstring db = Tr(L"Export database mirror", L"\u5bfc\u51fa\u6570\u636e\u5e93\u955c\u50cf");
    std::wstring autosave = Tr(L"Open autosave", L"\u6253\u5f00\u81ea\u52a8\u4fdd\u5b58");
    AppendMenuW(menu, MF_STRING, IDM_IMPORT_ROSTER, roster.c_str());
    AppendMenuW(menu, MF_STRING, IDM_PRINT_HTML, print.c_str());
    AppendMenuW(menu, MF_STRING, IDM_STATS_CHART, chart.c_str());
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, IDM_UNDO, undo.c_str());
    AppendMenuW(menu, MF_STRING, IDM_REDO, redo.c_str());
    AppendMenuW(menu, MF_STRING, IDM_SHORTCUTS, shortcuts.c_str());
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, IDM_EXPORT_DB, db.c_str());
    AppendMenuW(menu, MF_STRING, IDM_OPEN_AUTOSAVE, autosave.c_str());
    RECT rc{};
    GetWindowRect(button, &rc);
    int command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTALIGN | TPM_TOPALIGN, rc.right, rc.bottom + 4, 0, g_hwnd, nullptr);
    DestroyMenu(menu);
    switch (command) {
    case IDM_IMPORT_ROSTER: ImportRosterCsv(); break;
    case IDM_PRINT_HTML: ExportPrintHtml(); break;
    case IDM_STATS_CHART: ShowStatsChart(); break;
    case IDM_UNDO: UndoLast(); break;
    case IDM_REDO: RedoLast(); break;
    case IDM_SHORTCUTS: ShowShortcuts(); break;
    case IDM_EXPORT_DB: ExportDatabaseMirror(); break;
    case IDM_OPEN_AUTOSAVE: OpenAutosave(); break;
    }
}

void SaveAttendance() {
    std::wstring path = SaveFileDialog();
    if (path.empty()) return;

    std::ofstream file(std::filesystem::path(path), std::ios::binary);
    if (!file) {
        ShowMessage(L"Could not save the file.");
        return;
    }
    file << EncodeAttd(SerializeWorkbook());
    ShowMessage(L"Saved successfully.");
}

bool LoadAttendanceFile(const std::wstring& path, bool showSuccess) {
    std::ifstream file(std::filesystem::path(path), std::ios::binary);
    if (!file) {
        ShowMessage(L"Could not open the file.");
        return false;
    }

    std::string fileText((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::string plainText;
    std::vector<AttendanceSheet> imported;
    if (!DecodeAttd(fileText, plainText) || !DeserializeWorkbook(plainText, imported)) {
        ShowMessage(L"This .attd file could not be decoded.");
        return false;
    }

    g_sheets = std::move(imported);
    g_activeSheet = 0;
    g_records = g_sheets[g_activeSheet].records;
    g_undoStack.clear();
    g_redoStack.clear();
    RefreshCourseCombo();
    RefreshList();
    if (showSuccess) ShowMessage(L"Imported successfully.");
    return true;
}

void ImportAttendance() {
    std::wstring path = OpenFileDialog();
    if (path.empty()) return;
    LoadAttendanceFile(path, true);
}

HWND MakeControl(const wchar_t* cls, const wchar_t* text, DWORD style, int id) {
    HWND hwnd = CreateWindowExW(
        0, cls, text,
        WS_CHILD | WS_VISIBLE | style,
        0, 0, 100, 30,
        g_hwnd, (HMENU)(intptr_t)id, GetModuleHandleW(nullptr), nullptr
    );
    SendMessageW(hwnd, WM_SETFONT, (WPARAM)g_font, TRUE);
    return hwnd;
}

HWND MakeButton(const wchar_t* text, int id) {
    return MakeControl(L"BUTTON", text, BS_PUSHBUTTON | BS_OWNERDRAW, id);
}

HWND MakeSettingsControl(HWND parent, const wchar_t* cls, const wchar_t* text, DWORD style, int id) {
    HWND hwnd = CreateWindowExW(
        0, cls, text,
        WS_CHILD | WS_VISIBLE | style,
        0, 0, 100, 30,
        parent, (HMENU)(intptr_t)id, GetModuleHandleW(nullptr), nullptr
    );
    SendMessageW(hwnd, WM_SETFONT, (WPARAM)g_font, TRUE);
    return hwnd;
}

void FillSettingsCombos(HWND hwnd) {
    HWND language = GetDlgItem(hwnd, IDC_SETTINGS_LANGUAGE);
    HWND theme = GetDlgItem(hwnd, IDC_SETTINGS_THEME);
    HWND font = GetDlgItem(hwnd, IDC_SETTINGS_FONT);

    SendMessageW(language, CB_RESETCONTENT, 0, 0);
    for (int i = 0; i <= (int)UiLanguage::Spanish; ++i) {
        SendMessageW(language, CB_ADDSTRING, 0, (LPARAM)LanguageName((UiLanguage)i));
    }
    SendMessageW(language, CB_SETCURSEL, (WPARAM)g_language, 0);

    SendMessageW(theme, CB_RESETCONTENT, 0, 0);
    std::wstring dark = Tr(L"Dark", L"\u6df1\u8272");
    std::wstring light = Tr(L"Light", L"\u6d45\u8272");
    SendMessageW(theme, CB_ADDSTRING, 0, (LPARAM)dark.c_str());
    SendMessageW(theme, CB_ADDSTRING, 0, (LPARAM)light.c_str());
    SendMessageW(theme, CB_SETCURSEL, g_theme == UiTheme::Light ? 1 : 0, 0);

    SendMessageW(font, CB_RESETCONTENT, 0, 0);
    const wchar_t* fonts[] = {L"Segoe UI", L"Microsoft YaHei UI", L"Arial", L"Calibri", L"Consolas"};
    int selected = 0;
    for (int i = 0; i < 5; ++i) {
        SendMessageW(font, CB_ADDSTRING, 0, (LPARAM)fonts[i]);
        if (g_fontFamily == fonts[i]) selected = i;
    }
    SendMessageW(font, CB_SETCURSEL, selected, 0);
}

void ApplySettingsLanguage(HWND hwnd) {
    std::wstring settingsTitle = Tr(L"Settings", L"\u8bbe\u7f6e");
    SetWindowTextW(hwnd, settingsTitle.c_str());
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_TITLE), Tr(L"Interface Settings", L"界面设置"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_LANG_LABEL), Tr(L"Language", L"语言"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_THEME_LABEL), Tr(L"Style", L"风格"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_FONT_LABEL), Tr(L"Interface Font", L"界面字体"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_APPLY), Tr(L"Apply", L"应用"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_CLOSE), Tr(L"Close", L"关闭"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_RESET), Tr(L"Reset All Settings", L"\u91cd\u7f6e\u6240\u6709\u8bbe\u7f6e"));
    FillSettingsCombos(hwnd);
}

void ApplySettingsFromWindow(HWND hwnd) {
    int language = (int)SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_LANGUAGE), CB_GETCURSEL, 0, 0);
    int theme = (int)SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_THEME), CB_GETCURSEL, 0, 0);
    int fontIndex = (int)SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_FONT), CB_GETCURSEL, 0, 0);

    if (language < 0 || language > (int)UiLanguage::Spanish) language = 0;
    g_language = (UiLanguage)language;
    g_theme = theme == 1 ? UiTheme::Light : UiTheme::Dark;

    wchar_t fontName[128]{};
    if (fontIndex >= 0) {
        SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_FONT), CB_GETLBTEXT, fontIndex, (LPARAM)fontName);
        if (fontName[0]) g_fontFamily = fontName;
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
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_LANGUAGE), comboX, y, comboW, 220, TRUE);
    y += 48;
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_THEME_LABEL), x, y + 6, labelW, rowH, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_THEME), comboX, y, comboW, 220, TRUE);
    y += 48;
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_FONT_LABEL), x, y + 6, labelW, rowH, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS_FONT), comboX, y, comboW, 220, TRUE);
    y += 56;
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
        MakeSettingsControl(hwnd, L"STATIC", L"", 0, IDC_SETTINGS_THEME_LABEL);
        MakeSettingsControl(hwnd, L"STATIC", L"", 0, IDC_SETTINGS_FONT_LABEL);
        MakeSettingsControl(hwnd, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP, IDC_SETTINGS_LANGUAGE);
        MakeSettingsControl(hwnd, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP, IDC_SETTINGS_THEME);
        MakeSettingsControl(hwnd, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP, IDC_SETTINGS_FONT);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_PUSHBUTTON | WS_TABSTOP, IDC_SETTINGS_RESET);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_PUSHBUTTON | WS_TABSTOP, IDC_SETTINGS_APPLY);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_PUSHBUTTON | WS_TABSTOP, IDC_SETTINGS_CLOSE);
        SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_TITLE), WM_SETFONT, (WPARAM)g_titleFont, TRUE);
        ApplySettingsLanguage(hwnd);
        ResizeSettingsLayout(hwnd);
        return 0;
    }
    case WM_SIZE:
        ResizeSettingsLayout(hwnd);
        return 0;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_SETTINGS_APPLY) {
            ApplySettingsFromWindow(hwnd);
            return 0;
        }
        if (LOWORD(wParam) == IDC_SETTINGS_CLOSE) {
            DestroyWindow(hwnd);
            return 0;
        }
        if (LOWORD(wParam) == IDC_SETTINGS_RESET) {
            if (MessageBoxW(hwnd, L"Reset all settings and delete the AppData configuration file?", L"Reset Settings", MB_YESNO | MB_ICONWARNING) == IDYES) {
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
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        HWND control = (HWND)lParam;
        SetTextColor(hdc, GetDlgCtrlID(control) == IDC_SETTINGS_TITLE ? COLOR_TEXT : COLOR_MUTED);
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
    case WM_ERASEBKGND: {
        RECT rc{};
        GetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, g_bgBrush);
        return 1;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
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
    RECT rc{};
    GetClientRect(hwnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    int pad = 24;
    int top = 22;
    int labelW = 92;
    int editH = 36;
    int buttonH = 42;
    int leftW = std::min(460, std::max(380, w / 3));
    int x = pad;

    MoveWindow(GetDlgItem(hwnd, IDC_TITLE), x, top, leftW, 38, TRUE);
    int rightX = pad + leftW + 24;
    MoveWindow(g_courseCombo, rightX, top, 260, 34, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_COURSE_OPTIONS), rightX + 272, top, 128, 34, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_STATS), rightX + 416, top + 4, std::max(320, w - (rightX + 440)), 30, TRUE);
    top += 48;
    MoveWindow(GetDlgItem(hwnd, IDC_SUBTITLE), x, top, leftW, 26, TRUE);
    top += 44;

    MoveWindow(GetDlgItem(hwnd, 2001), x, top + 6, labelW, 24, TRUE);
    MoveWindow(g_dateEdit, x + labelW, top, leftW - labelW, editH, TRUE);
    top += 52;
    MoveWindow(GetDlgItem(hwnd, 2002), x, top + 6, labelW, 24, TRUE);
    MoveWindow(g_nameEdit, x + labelW, top, leftW - labelW, editH, TRUE);
    top += 52;
    MoveWindow(GetDlgItem(hwnd, 2003), x, top + 6, labelW, 24, TRUE);
    MoveWindow(g_otherEdit, x + labelW, top, leftW - labelW, editH, TRUE);
    top += 64;

    int half = (leftW - 12) / 2;
    MoveWindow(GetDlgItem(hwnd, IDC_PRESENT), x, top, half, buttonH, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_ABSENT), x + half + 12, top, half, buttonH, TRUE);
    top += 50;
    MoveWindow(GetDlgItem(hwnd, IDC_LATE), x, top, half, buttonH, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_OTHER_STATUS), x + half + 12, top, half, buttonH, TRUE);
    top += 60;
    MoveWindow(GetDlgItem(hwnd, IDC_ADD_UPDATE), x, top, half, buttonH + 2, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_EDIT_SELECTED), x + half + 12, top, half, buttonH + 2, TRUE);
    top += 58;
    MoveWindow(GetDlgItem(hwnd, IDC_ALL_PRESENT), x, top, leftW, buttonH, TRUE);
    top += 52;
    MoveWindow(GetDlgItem(hwnd, IDC_NEW), x, top, half, buttonH, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_DELETE), x + half + 12, top, half, buttonH, TRUE);
    top += 52;
    MoveWindow(GetDlgItem(hwnd, IDC_SAVE), x, top, half, buttonH, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_IMPORT), x + half + 12, top, half, buttonH, TRUE);
    top += 52;
    MoveWindow(GetDlgItem(hwnd, IDC_EXPORT_CSV), x, top, leftW, buttonH, TRUE);

    int bottomBarY = std::max(top + 56, h - 56);
    MoveWindow(GetDlgItem(hwnd, IDC_HINT), x, bottomBarY + 10, std::max(260, w - 230), 26, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_TOOLS), std::max(x, w - 326), bottomBarY + 4, 146, 38, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_SETTINGS), std::max(x, w - 170), bottomBarY + 4, 146, 38, TRUE);

    int listX = pad + leftW + 24;
    int listW = std::max(300, w - listX - pad);
    int listY = pad + 52;
    int listH = std::max(300, bottomBarY - listY - 14);
    MoveWindow(g_list, listX, listY, listW, listH, TRUE);

    int widths[4] = {
        std::max(210, listW / 5),
        std::max(220, listW / 4),
        std::max(150, listW / 7),
        std::max(260, listW - (listW / 5) - (listW / 4) - (listW / 7) - 24)
    };
    for (int i = 0; i < 4; ++i) ListView_SetColumnWidth(g_list, i, widths[i]);
}

void ApplyDarkMode(HWND hwnd) {
    BOOL value = g_theme == UiTheme::Dark;
    DwmSetWindowAttribute(hwnd, 20, &value, sizeof(value));
    SetWindowTheme(g_list, g_theme == UiTheme::Dark ? L"DarkMode_Explorer" : L"Explorer", nullptr);
    ListView_SetBkColor(g_list, COLOR_PANEL);
    ListView_SetTextBkColor(g_list, COLOR_PANEL);
    ListView_SetTextColor(g_list, COLOR_TEXT);
    ListView_SetExtendedListViewStyle(g_list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);
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

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_hwnd = hwnd;
        ApplyThemePalette();
        RecreateFonts();

        HWND title = MakeControl(L"STATIC", L"Attendance Manager", 0, IDC_TITLE);
        HWND subtitle = MakeControl(L"STATIC", L"Create, edit, export, save, import, and batch clean .attd roll calls.", 0, IDC_SUBTITLE);
        HWND stats = MakeControl(L"STATIC", L"Total 0    Present 0    Absent 0    Late 0    Other 0", 0, IDC_STATS);
        HWND hint = MakeControl(L"STATIC", L"Tip: double-click a row to edit. Ctrl/Shift supports multi-select.", 0, IDC_HINT);
        SendMessageW(title, WM_SETFONT, (WPARAM)g_titleFont, TRUE);
        SendMessageW(subtitle, WM_SETFONT, (WPARAM)g_smallFont, TRUE);
        SendMessageW(stats, WM_SETFONT, (WPARAM)g_smallFont, TRUE);
        SendMessageW(hint, WM_SETFONT, (WPARAM)g_smallFont, TRUE);
        g_courseCombo = MakeControl(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP, IDC_COURSE_COMBO);
        MakeButton(L"Courses", IDC_COURSE_OPTIONS);
        MakeControl(L"STATIC", L"Date/Time", 0, 2001);
        MakeControl(L"STATIC", L"Name", 0, 2002);
        MakeControl(L"STATIC", L"Other", 0, 2003);
        g_dateEdit = MakeControl(L"EDIT", CurrentDateTimeText().c_str(), WS_TABSTOP | ES_AUTOHSCROLL, IDC_DATE);
        g_nameEdit = MakeControl(L"EDIT", L"", WS_TABSTOP | ES_AUTOHSCROLL, IDC_NAME);
        g_otherEdit = MakeControl(L"EDIT", L"", WS_TABSTOP | ES_AUTOHSCROLL, IDC_OTHER);

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

        g_list = MakeControl(WC_LISTVIEWW, L"", LVS_REPORT | WS_TABSTOP | WS_BORDER, IDC_LIST);
        InitListColumns();
        EnsureSheets();
        RefreshCourseCombo();
        ApplyDarkMode(hwnd);
        ApplyMainLanguage();
        ResizeLayout(hwnd);
        SetTimer(hwnd, 1, 30000, nullptr);
        return 0;
    }
    case WM_SIZE:
        ResizeLayout(hwnd);
        return 0;
    case WM_COMMAND: {
        if (LOWORD(wParam) == IDC_COURSE_COMBO && HIWORD(wParam) == CBN_SELCHANGE) {
            int index = (int)SendMessageW(g_courseCombo, CB_GETCURSEL, 0, 0);
            SwitchCourse(index);
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
        case IDC_NEW:
            if (MessageBoxW(hwnd, L"Create a new attendance sheet and clear current records?", L"New Attendance", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                PushUndo();
                g_records.clear();
                SyncActiveSheet();
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
    case WM_DRAWITEM: {
        auto* draw = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (draw->CtlType == ODT_BUTTON) {
            wchar_t text[128]{};
            GetWindowTextW(draw->hwndItem, text, 128);
            bool pressed = (draw->itemState & ODS_SELECTED) != 0;
            bool focused = (draw->itemState & ODS_FOCUS) != 0;
            COLORREF fill = pressed ? RGB(52, 92, 140) : COLOR_INPUT;
            COLORREF border = focused ? COLOR_ACCENT : RGB(76, 82, 96);
            COLORREF buttonText = COLOR_TEXT;
            if (draw->CtlID == IDC_DELETE) {
                fill = pressed ? RGB(128, 51, 57) : RGB(88, 48, 55);
                border = focused ? COLOR_DANGER : RGB(140, 74, 82);
                buttonText = RGB(255, 245, 245);
            } else if (draw->CtlID == IDC_PRESENT || draw->CtlID == IDC_SAVE || draw->CtlID == IDC_ALL_PRESENT) {
                fill = pressed ? RGB(36, 103, 86) : RGB(35, 79, 72);
                border = focused ? RGB(90, 220, 176) : RGB(64, 135, 120);
                buttonText = RGB(242, 255, 250);
            } else if (draw->CtlID == IDC_IMPORT || draw->CtlID == IDC_NEW || draw->CtlID == IDC_EXPORT_CSV) {
                fill = pressed ? RGB(45, 87, 143) : RGB(44, 70, 116);
                border = focused ? COLOR_ACCENT : RGB(77, 117, 180);
                buttonText = RGB(245, 249, 255);
            }

            HBRUSH brush = CreateSolidBrush(fill);
            FillRect(draw->hDC, &draw->rcItem, brush);
            DeleteObject(brush);

            HPEN pen = CreatePen(PS_SOLID, 1, border);
            HGDIOBJ oldPen = SelectObject(draw->hDC, pen);
            HGDIOBJ oldBrush = SelectObject(draw->hDC, GetStockObject(HOLLOW_BRUSH));
            Rectangle(draw->hDC, draw->rcItem.left, draw->rcItem.top, draw->rcItem.right, draw->rcItem.bottom);
            SelectObject(draw->hDC, oldBrush);
            SelectObject(draw->hDC, oldPen);
            DeleteObject(pen);

            SetBkMode(draw->hDC, TRANSPARENT);
            SetTextColor(draw->hDC, buttonText);
            DrawTextW(draw->hDC, text, -1, &draw->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            return TRUE;
        }
        break;
    }
    case WM_NOTIFY: {
        auto* hdr = reinterpret_cast<NMHDR*>(lParam);
        if (hdr->idFrom == IDC_LIST && hdr->code == LVN_ITEMCHANGED) {
            auto* item = reinterpret_cast<NMLISTVIEW*>(lParam);
            if ((item->uNewState & LVIS_SELECTED) && item->iItem >= 0 && item->iItem < (int)g_records.size()) {
                const auto& record = g_records[item->iItem];
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
        else if (GetDlgCtrlID(control) == IDC_STATS) SetTextColor(hdc, COLOR_ACCENT);
        else SetTextColor(hdc, COLOR_MUTED);
        SetBkColor(hdc, COLOR_BG);
        return (LRESULT)g_bgBrush;
    }
    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, COLOR_TEXT);
        SetBkColor(hdc, COLOR_INPUT);
        return (LRESULT)g_inputBrush;
    }
    case WM_ERASEBKGND: {
        RECT rc{};
        GetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, g_bgBrush);
        return 1;
    }
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        AutoSaveNow();
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
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1600, 900,
        nullptr, nullptr, instance, nullptr
    );

    if (!hwnd) return 1;
    SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON)));
    SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON)));
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
