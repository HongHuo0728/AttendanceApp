#pragma once

#include "core/types.h"

namespace attendance {

struct UiState {
    HWND hwnd = nullptr;
    HWND dateEdit = nullptr;
    HWND nameEdit = nullptr;
    HWND otherEdit = nullptr;
    HWND list = nullptr;
    HWND courseCombo = nullptr;
    HWND filterEdit = nullptr;
    HWND settingsWindow = nullptr;
    HWND chartWindow = nullptr;
    HWND quickRollCallWindow = nullptr;
    HFONT font = nullptr;
    HFONT titleFont = nullptr;
    HFONT smallFont = nullptr;
    HBRUSH bgBrush = nullptr;
    HBRUSH inputBrush = nullptr;
    HBRUSH panelBrush = nullptr;
    bool fullscreen = false;
    bool liveResizing = false;
    WINDOWPLACEMENT previousPlacement{sizeof(WINDOWPLACEMENT)};
    DWORD previousStyle = 0;
    int scrollX = 0;
    int scrollY = 0;
    int settingsScrollY = 0;
    int contentW = 0;
    int contentH = 0;
    int openThemedPopupMenus = 0;
    bool languageUpdateActive = false;
    COLORREF COLOR_BG = RGB(0, 0, 0);
    COLORREF COLOR_PANEL = RGB(18, 18, 18);
    COLORREF COLOR_INPUT = RGB(26, 26, 26);
    COLORREF COLOR_TEXT = RGB(255, 255, 255);
    COLORREF COLOR_MUTED = RGB(224, 224, 224);
    COLORREF COLOR_ACCENT = RGB(188, 188, 188);
    COLORREF COLOR_DANGER = RGB(176, 112, 112);
    COLORREF COLOR_BORDER = RGB(64, 64, 64);
    COLORREF COLOR_HOVER = RGB(38, 38, 38);
    COLORREF COLOR_CARD = RGB(16, 16, 16);
    COLORREF COLOR_ROW_ALT = RGB(14, 14, 14);
    COLORREF COLOR_SELECTED = RGB(42, 42, 42);
    COLORREF COLOR_CHECK_FILL = RGB(0, 0, 0);
    ThemeTransitionState themeTransition;
    std::map<HWND, ComboListScrollState> comboListScrollStates;
};

UiState& Ui();

} // namespace attendance
