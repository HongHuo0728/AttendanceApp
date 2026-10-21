#include "ui/choice.h"
#include "core/text.h"
#include "localization/strings.h"
#include "ui/common.h"
#include "ui/controls.h"
#include "ui/state.h"
#include "ui/theme.h"
#include "ui/title_bar.h"
namespace attendance {
namespace {
struct Choice {
  const std::vector<std::wstring> *items;
  std::vector<int> visible;
  int selected = -1;
  bool done = false;
};
void Filter(HWND hwnd, Choice &s) {
  auto list = GetDlgItem(hwnd, 8011);
  auto query = GetText(GetDlgItem(hwnd, 8010));
  SendMessageW(list, WM_SETREDRAW, FALSE, 0);
  SendMessageW(list, LB_RESETCONTENT, 0, 0);
  s.visible.clear();
  for (size_t i = 0; i < s.items->size(); ++i)
    if (query.empty() || ContainsText((*s.items)[i], query)) {
      SendMessageW(list, LB_ADDSTRING, 0,
                   reinterpret_cast<LPARAM>((*s.items)[i].c_str()));
      s.visible.push_back(static_cast<int>(i));
    }
  SendMessageW(list, LB_SETCURSEL, 0, 0);
  SendMessageW(list, WM_SETREDRAW, TRUE, 0);
  InvalidateRect(list, nullptr, TRUE);
  EnableWindow(GetDlgItem(hwnd, 8012), !s.visible.empty());
}
LRESULT CALLBACK ChoiceProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  auto *s = reinterpret_cast<Choice *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
  if (msg == WM_CREATE) {
    s = static_cast<Choice *>(
        reinterpret_cast<CREATESTRUCTW *>(lp)->lpCreateParams);
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(s));
    auto search = MakeSettingsControl(hwnd, L"EDIT", L"",
                                      WS_TABSTOP | ES_AUTOHSCROLL, 8010);
    SetAccessibleName(search, Tr(L"Search", L"搜索"));
    SendMessageW(search, EM_SETCUEBANNER, TRUE,
                 reinterpret_cast<LPARAM>(Tr(L"Search", L"搜索").c_str()));
    MakeSettingsControl(hwnd, L"LISTBOX", L"",
                        WS_TABSTOP | WS_BORDER | WS_VSCROLL | LBS_NOTIFY |
                            LBS_NOINTEGRALHEIGHT,
                        8011);
    SetAccessibleName(GetDlgItem(hwnd, 8011), Tr(L"Choose item", L"选择项目"));
    MakeSettingsControl(hwnd, L"BUTTON", Tr(L"OK", L"确定").c_str(),
                        WS_TABSTOP | BS_DEFPUSHBUTTON, 8012);
    MakeSettingsControl(hwnd, L"BUTTON", Tr(L"Cancel", L"取消").c_str(),
                        WS_TABSTOP, 8013);
    Filter(hwnd, *s);
    return 0;
  }
  if (msg == WM_SIZE) {
    RECT rc{};
    GetClientRect(hwnd, &rc);
    int w = rc.right, h = rc.bottom;
    MoveWindow(GetDlgItem(hwnd, 8010), 20, 20, w - 40, 36, TRUE);
    MoveWindow(GetDlgItem(hwnd, 8011), 20, 70, w - 40, std::max(30, h - 140),
               TRUE);
    MoveWindow(GetDlgItem(hwnd, 8012), w - 260, h - 54, 110, 34, TRUE);
    MoveWindow(GetDlgItem(hwnd, 8013), w - 140, h - 54, 120, 34, TRUE);
    return 0;
  }
  if (msg == WM_GETMINMAXINFO) {
    auto *mm = reinterpret_cast<MINMAXINFO *>(lp);
    mm->ptMinTrackSize = {480, 320};
    return 0;
  }
  if (msg == WM_COMMAND && s) {
    const int id = LOWORD(wp);
    if (id == 8010 && HIWORD(wp) == EN_CHANGE) {
      Filter(hwnd, *s);
      return 0;
    }
    if (id == 8013 || id == IDCANCEL) {
      s->done = true;
      return 0;
    }
    if (id == 8012 || id == IDOK || (id == 8011 && HIWORD(wp) == LBN_DBLCLK)) {
      auto i = SendMessageW(GetDlgItem(hwnd, 8011), LB_GETCURSEL, 0, 0);
      if (i >= 0 && static_cast<size_t>(i) < s->visible.size()) {
        s->selected = s->visible[i];
        s->done = true;
      }
      return 0;
    }
  }
  if (msg == WM_CLOSE && s) {
    s->done = true;
    return 0;
  }
  if (msg == WM_CTLCOLORSTATIC || msg == WM_CTLCOLORLISTBOX ||
      msg == WM_CTLCOLOREDIT) {
    auto dc = reinterpret_cast<HDC>(wp);
    SetTextColor(dc, Ui().COLOR_TEXT);
    SetBkColor(dc, Ui().COLOR_INPUT);
    return reinterpret_cast<LRESULT>(Ui().inputBrush);
  }
  if (msg == WM_ERASEBKGND) {
    RECT r{};
    GetClientRect(hwnd, &r);
    FillRect(reinterpret_cast<HDC>(wp), &r, Ui().bgBrush);
    return 1;
  }
  return DefWindowProcW(hwnd, msg, wp, lp);
}
} // namespace
int ChooseItem(const std::wstring &title,
               const std::vector<std::wstring> &items) {
  if (items.empty())
    return -1;
  WNDCLASSW wc{};
  wc.hInstance = GetModuleHandleW(nullptr);
  wc.lpfnWndProc = ChoiceProc;
  wc.lpszClassName = L"AttendanceChoice";
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  RegisterClassW(&wc);
  Choice state{&items, {}, -1, false};
  HWND hwnd = CreateWindowExW(
      WS_EX_CONTROLPARENT, wc.lpszClassName, title.c_str(),
      WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 680,
      520, Ui().hwnd, nullptr, wc.hInstance, &state);
  if (!hwnd)
    return -1;
  EnableWindow(Ui().hwnd, FALSE);
  ApplyGlassTitleBar(hwnd);
  ApplyThemedControls(hwnd);
  CenterOwnedWindow(hwnd, Ui().hwnd);
  ShowWindow(hwnd, SW_SHOW);
  SetFocus(GetDlgItem(hwnd, 8010));
  MSG msg{};
  while (!state.done && IsWindow(hwnd)) {
    int result = GetMessageW(&msg, nullptr, 0, 0);
    if (result <= 0) {
      if (result == 0)
        PostQuitMessage(static_cast<int>(msg.wParam));
      break;
    }
    if (!IsDialogMessageW(hwnd, &msg)) {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
  }
  DestroyWindow(hwnd);
  EnableWindow(Ui().hwnd, TRUE);
  SetForegroundWindow(Ui().hwnd);
  return state.selected;
}
} // namespace attendance
