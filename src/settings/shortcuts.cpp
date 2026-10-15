#include "settings/shortcuts.h"
#include "animation/scheduler.h"
#include "core/text.h"
#include "model/history.h"
#include "app/documents.h"
#include "app/commands.h"
#include "ui/layout.h"
#include "settings/state.h"
#include "ui/state.h"
#include "core/constants.h"

namespace attendance {

bool ShortcutKeyPartMatches(const std::wstring& key, WPARAM wParam);

std::wstring NormalizeShortcut(const std::wstring& value, const std::wstring& fallback){
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
                size_t consumed = 0;
                int number = std::stoi(key.substr(1), &consumed);
                if (consumed == key.size() - 1 && number >= 1 && number <= 24) return L"F" + std::to_wstring(number);
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

bool ShortcutKeyPartMatches(const std::wstring& key, WPARAM wParam){
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

bool ShortcutMatches(const std::wstring& shortcut, WPARAM key){
    std::wstring normalized = NormalizeShortcut(shortcut, L"");
    if (normalized.empty()) return false;
    bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    bool altDown = (GetKeyState(VK_MENU) & 0x8000) != 0;
    if (normalized.rfind(L"Ctrl+", 0) == 0) {
        return ctrlDown && !altDown && ShortcutKeyPartMatches(normalized.substr(5), key);
    }
    return !ctrlDown && !altDown && ShortcutKeyPartMatches(normalized, key);
}

std::wstring ShortcutConfigText(){
    return L"save=" + Settings().shortcutSave
        + L"; import=" + Settings().shortcutImport
        + L"; undo=" + Settings().shortcutUndo
        + L"; redo=" + Settings().shortcutRedo
        + L"; command=" + Settings().shortcutCommand
        + L"; fullscreen=" + Settings().shortcutFullscreen;
}

bool ApplyShortcutConfigText(const std::wstring& config){
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
            target = &Settings().shortcutSave;
            fallback = L"Ctrl+S";
        } else if (key == L"import" || key == L"open" || key == L"o") {
            target = &Settings().shortcutImport;
            fallback = L"Ctrl+O";
        } else if (key == L"undo" || key == L"z") {
            target = &Settings().shortcutUndo;
            fallback = L"Ctrl+Z";
        } else if (key == L"redo" || key == L"y") {
            target = &Settings().shortcutRedo;
            fallback = L"Ctrl+Y";
        } else if (key == L"command" || key == L"palette" || key == L"command_palette" || key == L"commandpalette") {
            target = &Settings().shortcutCommand;
            fallback = L"Ctrl+K";
        } else if (key == L"fullscreen" || key == L"full" || key == L"f11") {
            target = &Settings().shortcutFullscreen;
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

bool ExecuteMainShortcut(WPARAM key){
    if (ShortcutMatches(Settings().shortcutSave, key)) SaveAttendance();
    else if (ShortcutMatches(Settings().shortcutImport, key)) ImportAttendance();
    else if (ShortcutMatches(Settings().shortcutUndo, key)) UndoLast();
    else if (ShortcutMatches(Settings().shortcutRedo, key)) RedoLast();
    else if (Settings().commandPaletteEnabled && ShortcutMatches(Settings().shortcutCommand, key)) RunCommandPalette();
    else if (ShortcutMatches(Settings().shortcutFullscreen, key)) ToggleFullscreen(Ui().hwnd);
    else return false;
    return true;
}

bool HandleMainShortcutMessage(const MSG& msg){
    if (msg.message != WM_KEYDOWN && msg.message != WM_SYSKEYDOWN) return false;
    if (!msg.hwnd || GetAncestor(msg.hwnd, GA_ROOT) != Ui().hwnd) return false;
    return ExecuteMainShortcut(msg.wParam);
}

} // namespace attendance
