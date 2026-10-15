#include "ui/roll_call.h"
#include "model/workbook.h"
#include "ui/common.h"
#include "animation/scheduler.h"
#include "ui/title_bar.h"
#include "core/text.h"
#include "ui/dialogs.h"
#include "localization/strings.h"
#include "app/validation.h"
#include "ui/table.h"
#include "ui/theme.h"
#include "model/history.h"
#include "ui/controls.h"
#include "ui/drawing.h"
#include "model/state.h"
#include "ui/state.h"
#include "core/constants.h"

namespace attendance {

int FindQuickRollCallRecord(const QuickRollCallState& state, const std::wstring& student);
std::wstring LocalizedQuickRollCallStatus(const std::wstring& status);
void ResizeQuickRollCallLayout(HWND hwnd);
bool ApplyQuickRollCallStatus(HWND hwnd, const std::wstring& status);
void SkipQuickRollCallStudent(HWND hwnd);
void PreviousQuickRollCallStudent(HWND hwnd);
void CancelQuickRollCall(HWND hwnd);
void FinishQuickRollCall(HWND hwnd);
LRESULT CALLBACK QuickRollCallProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int FindQuickRollCallRecord(const QuickRollCallState& state, const std::wstring& student){
    auto found = state.recordByStudent.find(student);
    return found == state.recordByStudent.end() ? -1 : found->second;
}

std::wstring LocalizedQuickRollCallStatus(const std::wstring& status){
    if (status == L"Present") return Tr(L"Present", L"出席");
    if (status == L"Absent") return Tr(L"Absent", L"缺席");
    if (status == L"Late") return Tr(L"Late", L"迟到");
    if (status == L"Other") return Tr(L"Other", L"其他");
    if (status.empty()) return Tr(L"Unmarked", L"未标记");
    return status;
}

void ResizeQuickRollCallLayout(HWND hwnd){
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

void RefreshQuickRollCallWindow(HWND hwnd){
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

bool ApplyQuickRollCallStatus(HWND hwnd, const std::wstring& status){
    auto* state = reinterpret_cast<QuickRollCallState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!state || state->students.empty()) return false;

    std::wstring notes = TrimWide(GetText(GetDlgItem(hwnd, IDC_QUICK_NOTES)));
    if (status == L"Other" && notes.empty()) {
        ThemedMessageBox(hwnd, Tr(L"Other status requires a note.", L"选择其他状态时必须填写备注。"),
            Tr(L"Quick roll call", L"快速点名"), false);
        SetFocus(GetDlgItem(hwnd, IDC_QUICK_NOTES));
        return false;
    }
    if (!ValidateInputField(state->lesson, MAX_RECORD_FIELD_BYTES)
        || !ValidateInputField(notes, MAX_RECORD_FIELD_BYTES)) return false;

    const std::wstring& student = state->students[state->index];
    int recordIndex = FindQuickRollCallRecord(*state, student);
    if (recordIndex == -2) {
        ThemedMessageBox(hwnd,
            Tr(L"Duplicate lesson records must be resolved before quick roll call.", L"开始快速点名前必须先处理重复课次记录。"),
            Tr(L"Quick roll call", L"快速点名"), false);
        return false;
    }

    if (recordIndex < 0) {
        const size_t pendingAdds = state->workingRecords.size() >= Document().Records().size()
            ? state->workingRecords.size() - Document().Records().size() + 1 : 1;
        if (!CanAppendRecords(pendingAdds)) return false;
        state->workingRecords.push_back({state->lesson, student, status, notes});
        state->recordByStudent[student] = static_cast<int>(state->workingRecords.size() - 1);
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

void SkipQuickRollCallStudent(HWND hwnd){
    auto* state = reinterpret_cast<QuickRollCallState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!state || state->students.empty()) return;
    state->reviewed[state->index] = true;
    if (state->index + 1 < state->students.size()) ++state->index;
    RefreshQuickRollCallWindow(hwnd);
}

void PreviousQuickRollCallStudent(HWND hwnd){
    auto* state = reinterpret_cast<QuickRollCallState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!state || state->students.empty()) return;
    if (state->index > 0) --state->index;
    RefreshQuickRollCallWindow(hwnd);
}

void CancelQuickRollCall(HWND hwnd){
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

void FinishQuickRollCall(HWND hwnd){
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
        Document().Records() = std::move(state->workingRecords);
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

LRESULT CALLBACK QuickRollCallProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
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
        HWND notesEdit = MakeSettingsControl(hwnd, L"EDIT", L"", WS_TABSTOP | ES_AUTOHSCROLL, IDC_QUICK_NOTES);
        SetAccessibleName(notesEdit, Tr(L"Notes:", L"\u5907\u6ce8\uff1a"));
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

        SendMessageW(GetDlgItem(hwnd, IDC_QUICK_TITLE), WM_SETFONT, (WPARAM)Ui().titleFont, TRUE);
        SendMessageW(GetDlgItem(hwnd, IDC_QUICK_STUDENT), WM_SETFONT, (WPARAM)Ui().titleFont, TRUE);
        SendMessageW(GetDlgItem(hwnd, IDC_QUICK_HINT), WM_SETFONT, (WPARAM)Ui().smallFont, TRUE);
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
        SetTextColor(hdc, id == IDC_QUICK_TITLE || id == IDC_QUICK_STUDENT ? Ui().COLOR_TEXT : Ui().COLOR_MUTED);
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
    case WM_ERASEBKGND:
        return 1;
    case WM_APP_ANIMATION_TICK:
        AcknowledgeAnimationTick(hwnd);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_CLOSE:
        CancelQuickRollCall(hwnd);
        return 0;
    case WM_DESTROY:
        RemoveAnimationState(hwnd);
        Ui().quickRollCallWindow = nullptr;
        EnableWindow(Ui().hwnd, TRUE);
        SetForegroundWindow(Ui().hwnd);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        delete state;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void ShowQuickRollCall(){
    if (Ui().quickRollCallWindow && IsWindow(Ui().quickRollCallWindow)) {
        ShowWindow(Ui().quickRollCallWindow, SW_SHOW);
        SetForegroundWindow(Ui().quickRollCallWindow);
        return;
    }

    SyncActiveSheet();
    EnsureSheets();
    std::wstring lesson = TrimWide(GetText(Ui().dateEdit));
    if (lesson.empty()) {
        if (!PromptText(Tr(L"Quick roll call", L"快速点名"), Tr(L"Lesson name:", L"课次名称："), lesson)) return;
        lesson = TrimWide(lesson);
        if (lesson.empty()) return;
        SetText(Ui().dateEdit, lesson);
    }

    std::vector<std::wstring> students;
    std::set<std::wstring> seen;
    for (const auto& rawName : Document().sheets[Document().activeSheet].students) {
        std::wstring name = TrimWide(rawName);
        std::wstring key = LowerText(name);
        if (!name.empty() && seen.insert(key).second) students.push_back(name);
    }
    if (students.empty()) {
        ShowMessage(Tr(L"No students in the roster.", L"当前课程名单中没有学生。"),
            Tr(L"Quick roll call", L"快速点名"));
        return;
    }

    std::unordered_map<std::wstring, int> counts;
    for (const auto& record : Document().Records()) {
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
    state->course = Document().sheets[Document().activeSheet].name;
    state->students = std::move(students);
    state->workingRecords = Document().Records();
    state->recordByStudent.reserve(counts.size());
    for (int i = 0; i < static_cast<int>(state->workingRecords.size()); ++i) {
        const auto& record = state->workingRecords[i];
        if (record.dateTime != lesson) continue;
        auto result = state->recordByStudent.emplace(record.name, i);
        if (!result.second) result.first->second = -2;
    }
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

    EnableWindow(Ui().hwnd, FALSE);
    Ui().quickRollCallWindow = CreateWindowExW(
        WS_EX_CONTROLPARENT,
        className,
        Tr(L"Quick roll call", L"快速点名").c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 900, 570,
        Ui().hwnd, nullptr, instance, state
    );
    if (!Ui().quickRollCallWindow) {
        EnableWindow(Ui().hwnd, TRUE);
        delete state;
        return;
    }
    ApplyGlassTitleBar(Ui().quickRollCallWindow);
    CenterOwnedWindow(Ui().quickRollCallWindow, Ui().hwnd);
    ShowWindow(Ui().quickRollCallWindow, SW_SHOW);
    UpdateWindow(Ui().quickRollCallWindow);
    SetForegroundWindow(Ui().quickRollCallWindow);
}

} // namespace attendance
