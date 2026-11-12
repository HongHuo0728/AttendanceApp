#pragma once
#include "core/csv.h"
#include "ui/popup_menu.h"

namespace attendance {
int UpdateThemedMenuTypeAhead(ThemedMenuState&, wchar_t, uint64_t);
}

void TestInputMaintenance(HWND dialog, InputDialogState& input) {
  auto motion = Settings().animationLevel;
  Settings().animationLevel = AnimationLevel::Off;
  HWND edit = GetDlgItem(dialog, IDC_INPUT_EDIT);
  ShowWindow(dialog, SW_SHOWNOACTIVATE);
  SetActiveWindow(dialog);
  SetFocus(edit);
  SendMessageW(edit, WM_IME_STARTCOMPOSITION, 0, 0);
  MSG enter{}; enter.hwnd = edit; enter.message = WM_KEYDOWN; enter.wParam = VK_RETURN;
  Check(DispatchImeInput(enter) && !input.accepted && IsWindow(dialog),
        "IME Enter is dispatched to the composing edit without submitting");
  SendMessageW(edit, WM_IME_ENDCOMPOSITION, 0, 0);
  Check(!DispatchImeInput(enter), "normal Enter resumes after composition");
  SetFocus(GetDlgItem(dialog, IDC_INPUT_CANCEL));
  Check(GetFocus() == GetDlgItem(dialog, IDC_INPUT_CANCEL), "cancel keyboard fixture focus");
  SendMessageW(dialog, WM_KEYDOWN, VK_RETURN, 0);
  Check(!input.accepted && !IsWindow(dialog),
        "Enter on Cancel dismisses without accepting edited text");
  Settings().animationLevel = motion;
}

struct MenuMaintenanceProbe {
  HWND combo = nullptr;
  bool observed = false, bounded = false, wrapped = true, spaceKeepsOpen = false;
};
void CALLBACK InspectMaintenanceMenu(HWND owner, UINT, UINT_PTR, DWORD) {
  auto* probe = (MenuMaintenanceProbe*)GetPropW(owner, L"Test.MenuMaintenance");
  if (!probe) return;
  HWND popup = (HWND)GetPropW(probe->combo, L"Attendance.ComboPopup");
  if (!popup) return;
  auto* state = (ThemedMenuState*)GetWindowLongPtrW(popup, GWLP_USERDATA);
  if (!state || state->closing) return;
  probe->observed = true;
  RECT box{}; GetWindowRect(popup, &box);
  MONITORINFO monitor{sizeof(monitor)};
  GetMonitorInfoW(MonitorFromWindow(popup, MONITOR_DEFAULTTONEAREST), &monitor);
  probe->bounded = box.left >= monitor.rcWork.left && box.right <= monitor.rcWork.right &&
                   box.top >= monitor.rcWork.top && box.bottom <= monitor.rcWork.bottom;
  HDC dc = GetDC(popup); auto old = SelectObject(dc, Ui().font);
  for (size_t i = 0; i < state->items.size(); ++i) {
    RECT text{0,0,state->width-76,0};
    DrawTextW(dc,state->items[i].text.c_str(),-1,&text,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);
    if (text.bottom+16 > state->itemHeights[i]) probe->wrapped = false;
  }
  SelectObject(dc, old); ReleaseDC(popup, dc);
  SendMessageW(popup, WM_KEYDOWN, VK_SPACE, 0);
  probe->spaceKeepsOpen = IsWindow(popup) && !state->closing;
  SendMessageW(popup, WM_KEYDOWN, VK_ESCAPE, 0);
}
void TestPopupMaintenance(HWND combo) {
  auto shortcut = Settings().shortcutFullscreen;
  Settings().shortcutFullscreen = L"A";
  MSG typing{}; typing.hwnd=Ui().nameEdit; typing.message=WM_KEYDOWN; typing.wParam=L'A';
  Check(!HandleMainShortcutMessage(typing), "typing a name does not invoke a single-letter global shortcut");
  Settings().shortcutFullscreen = shortcut;
  RECT old{}; GetWindowRect(combo, &old);
  MapWindowPoints(nullptr, GetParent(combo), (POINT*)&old, 2);
  MONITORINFO monitor{sizeof(monitor)};
  GetMonitorInfoW(MonitorFromWindow(combo,MONITOR_DEFAULTTONEAREST),&monitor);
  SetWindowPos(combo,nullptr,0,0,monitor.rcWork.right-monitor.rcWork.left+200,40,
               SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
  std::wstring longLabel;
  for(int i=0;i<60;++i) longLabel += L"Long course title ";
  MenuMaintenanceProbe probe; probe.combo = combo;
  HWND owner = GetAncestor(combo,GA_ROOT);
  auto motion = Settings().animationLevel;
  Settings().animationLevel = AnimationLevel::Off;
  SetPropW(owner,L"Test.MenuMaintenance",&probe);
  SetTimer(owner,905,16,InspectMaintenanceMenu);
  int selected = ShowThemedPopupMenu(combo,{{1,longLabel,false},{2,L"Second course",false}},{true,0});
  KillTimer(owner,905); RemovePropW(owner,L"Test.MenuMaintenance");
  SetWindowPos(combo,nullptr,old.left,old.top,old.right-old.left,old.bottom-old.top,
               SWP_NOZORDER|SWP_NOACTIVATE);
  Settings().animationLevel = motion;
  Check(probe.observed && probe.bounded && probe.wrapped && probe.spaceKeepsOpen && selected == 0,
        "wide-anchor popup fits its monitor, wraps text and keeps Space for type-ahead");
  std::cout << "2.1.2 native popup bounds, wrapping, IME and focused-Cancel regressions passed\n";
}

void Test212() {
  auto previousDocument = Document();
  bool previousAdvanced = Settings().advancedFilterEnabled;
  std::vector<std::wstring> names;
  Check(ParseRosterCsv("\xEF\xBB\xBFName\r\n\"Morgan, Alex\"\r\n\"A \"\"quoted\"\" name\"\r\nStudent\r\nStudent\r\n", names) &&
        names == std::vector<std::wstring>({L"Morgan, Alex", L"A \"quoted\" name", L"Student"}),
        "roster BOM, CRLF, quotes, deduplication and header-like real names");
  for (const std::string invalid : {"Name\n\"unclosed\nAlice", "Name\nAl\"ice", "Name\n\"Alice\"junk", "Name\n\xFF"}) {
    names = {L"must be cleared"};
    Check(!ParseRosterCsv(invalid, names) && names.empty(),
          "malformed roster rejected without partial results");
  }
  Check(ParseRosterCsv("\n , ,\nName\nAlice\n", names) && names == std::vector<std::wstring>{L"Alice"},
        "leading blank CSV records do not consume the header");
  Check(CsvCell(L"Alice") == "Alice" && CsvCell(L"A,\"B\"") == "\"A,\"\"B\"\"\"",
        "ordinary CSV values retain RFC quoting");
  for (const auto* dangerous : {L"=1+1", L"+1", L"-1", L"@SUM(A1)", L"  =1", L"\ttext", L"\rtext", L"\ntext", L"\uFF1D1+1", L"\uFF20SUM(A1)"}) {
    auto csv = CsvCell(dangerous);
    Check(csv.rfind("\"'", 0) == 0 && csv.back() == '"',
          "spreadsheet-facing CSV prefixes risky text inside a quoted cell");
  }
  Check(TsvCell(L"=1+1") == "=1+1", "database export remains lossless");
  std::wstring decoded;
  Check(!DecodeUtf8Field(std::string("A\0B", 3), 100, decoded) && decoded.empty() &&
        !WorkbookFieldIsValid(std::wstring(L"A\0B", 3), 100),
        "embedded NUL rejected before reaching Win32 text controls");
  Check(XmlCell(L"A\x01<&\xFFFE") == "A\xEF\xBF\xBD&lt;&amp;\xEF\xBF\xBD" &&
        XmlCell(L"A\tB\r\nC") == "A\tB\r\nC",
        "XML replaces illegal controls while retaining valid whitespace");

  Document() = DocumentState{};
  Document().sheets = {MakeSheet(L"Filter regression")};
  Check(!CourseNameAvailable(L"Filter regression") &&
        CourseNameAvailable(L"Filter regression", 0) &&
        !CourseNameAvailable(L"  ") && CourseNameAvailable(L"Another course"),
        "course add and rename share duplicate and blank-name validation");
  Settings().advancedFilterEnabled = true;
  AttendanceRecord valid{L"2026-11-12 lesson", L"Alex", L"Present", L""};
  for (const auto* query : {L"from:2026-11-01", L"to:2026-11-30", L"from:2026-11-01 to:2026-11-30"}) {
    Document().filterText = query;
    Check(RecordMatchesFilter(valid) &&
          !RecordMatchesFilter({L"undated", L"Alex", L"Present", L""}) &&
          !RecordMatchesFilter({L"2026-11-99", L"Alex", L"Present", L""}),
          "bounded date searches exclude invalid dates and free-form lesson labels");
  }
  Document().filterText = L"from:2026-99-01";
  Check(!RecordMatchesFilter(valid), "invalid filter boundary does not act as a lexical date");
  Document().Records() = {valid};
  auto before = SerializeWorkbook();
  Check(!ChangeRecords({0}, L"Present", std::wstring(L"A\0B", 3), NoteMode::Replace) &&
        SerializeWorkbook() == before && Document().undoStack.empty(),
        "invalid input does not mutate the workbook or undo history");

  ThemedMenuState menu;
  menu.items = {{1,L"Alpha",false},{2,L"Alpine",false},{3,L"Beta",false},{4,L"Segoe UI",false}};
  menu.hover = 0;
  int match = UpdateThemedMenuTypeAhead(menu, L'a', 2000);
  Check(match == 1, "first character finds the next matching item");
  menu.hover = match;
  match = UpdateThemedMenuTypeAhead(menu, L'a', 2010);
  Check(match == 0, "repeating a character cycles matching items");
  menu.hover = match;
  Check(UpdateThemedMenuTypeAhead(menu, L'b', 2020) == 2,
        "unmatched prefix restarts at the new character");
  menu.hover = 2;
  for (wchar_t ch : std::wstring(L"Segoe U")) {
    match = UpdateThemedMenuTypeAhead(menu, ch, 4000 + menu.typeAhead.size());
    Check(match == 3, "font type-ahead accepts spaces without selecting early");
    menu.hover = match;
  }
  Document() = previousDocument;
  Settings().advancedFilterEnabled = previousAdvanced;
  std::cout << "2.1.2 CSV, input boundaries, XML, date-filter and menu typing regressions passed\n";
}
