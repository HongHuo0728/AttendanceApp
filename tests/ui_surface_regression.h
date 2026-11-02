#pragma once
#include "animation/particles.h"
#include "app/main_window.h"
#include "settings/fonts.h"
#include "ui/appearance.h"
#include "ui/dialogs.h"
#include "ui/layout.h"
#include "ui/roll_call.h"
#include "ui/statistics.h"
#include "ui/table.h"

namespace attendance {
LRESULT CALLBACK InputDialogProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MessageDialogProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK QuickRollCallProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ChartProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ThemedMenuProc(HWND, UINT, WPARAM, LPARAM);
RECT ThemedMenuScrollThumb(const ThemedMenuState *);
int ThemedMenuTotalHeight(const ThemedMenuState *);
void PaintStatsChart(HWND, HDC);
void ResizeQuickRollCallLayout(HWND);
void ResizeInputDialog(HWND);
SIZE CalculateMessageDialogWindowSize(HWND, const std::wstring &);
uint32_t EffectiveAnimationDuration(AnimChannel, uint32_t);
} // namespace attendance

struct MainActionProbe {
  int clicks = 0;
  HWND source = nullptr;
};
LRESULT CALLBACK ProbeMainAction(HWND hwnd, UINT message, WPARAM wParam,
                                 LPARAM lParam, UINT_PTR, DWORD_PTR data) {
  if (message == WM_COMMAND && HIWORD(wParam) == BN_CLICKED && lParam) {
    auto &probe = *reinterpret_cast<MainActionProbe *>(data);
    ++probe.clicks;
    probe.source = reinterpret_cast<HWND>(lParam);
    return 0;
  }
  return DefSubclassProc(hwnd, message, wParam, lParam);
}

RECT SurfaceChildBox(HWND root, int id) {
  RECT box{};
  HWND child = GetDlgItem(root, id);
  if (!child)
    throw std::runtime_error("surface fixture control exists");
  GetWindowRect(child, &box);
  MapWindowPoints(nullptr, root, (POINT *)&box, 2);
  return box;
}
_Ret_notnull_ HWND SurfaceFixture(const wchar_t *name, WNDPROC proc, HWND owner,
                                  int width, int height, void *state = nullptr,
                                  DWORD extra = 0) {
  WNDCLASSW wc{};
  wc.hInstance = GetModuleHandleW(nullptr);
  wc.lpfnWndProc = proc;
  wc.lpszClassName = name;
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  RegisterClassW(&wc);
  HWND window = CreateWindowExW(
      extra, name, L"UI regression",
      WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_VSCROLL, -32000, -32000, width,
      height, owner, nullptr, wc.hInstance, state);
  if (!window)
    throw std::runtime_error("native surface fixture creation failed");
  return window;
}
void TestUiSurfaces() {
  auto savedUi = Ui();
  auto savedDocument = Document();
  auto savedSettings = CaptureSettings();
  Ui() = UiState{};
  Document() = DocumentState{};
  ApplySettingsSnapshot(SettingsSnapshot{});
  Settings().animationLevel = AnimationLevel::Off;
  Settings().autosaveIntervalSeconds = 0;
  Document().sheets = {MakeSheet(L"UI regression course")};
  for (size_t i = 0; i < StatusKeys.size(); ++i) {
    auto name = L"Student " + std::to_wstring(i + 1);
    Document().sheets[0].students.push_back(name);
    Document().Records().push_back(
        {L"2026-11-02 UI lesson", name, StatusKeys[i], L"Sample note"});
  }
  HWND root = SurfaceFixture(L"AttendanceSurfaceRegression", WindowProc,
                             nullptr, 1600, 900);
  Check(root != nullptr, "real main-window fixture");
  Ui().hwnd = root;
  SetText(Ui().dateEdit, L"2026-11-02 UI lesson");
  RefreshList();
  HWND tab = GetNextDlgTabItem(root, nullptr, FALSE);
  for (int id :
       {IDC_COURSE_COMBO, IDC_COURSE_OPTIONS, IDC_FILTER,      IDC_CLEAR_FILTER,
        IDC_LIST,         IDC_DATE,           IDC_NAME,        IDC_OTHER,
        IDC_PRESENT,      IDC_ABSENT,         IDC_LATE,        IDC_OTHER_STATUS,
        IDC_LEAVE,        IDC_PENDING,        IDC_SAVE,        IDC_EXPORT_CSV,
        IDC_ADD_UPDATE,   IDC_EDIT_SELECTED,  IDC_ALL_PRESENT, IDC_NEW,
        IDC_IMPORT,       IDC_DELETE,         IDC_TOOLS,       IDC_SETTINGS}) {
    Check(tab == GetDlgItem(root, id),
          "main tab order follows the visible layout and includes all actions");
    tab = GetNextDlgTabItem(root, tab, FALSE);
  }
  MainActionProbe actionProbe;
  SetWindowSubclass(root, ProbeMainAction, 101, (DWORD_PTR)&actionProbe);
  for (int id : {IDC_PRESENT, IDC_ABSENT, IDC_LATE, IDC_OTHER_STATUS, IDC_LEAVE,
                 IDC_PENDING, IDC_SAVE, IDC_EXPORT_CSV, IDC_ADD_UPDATE,
                 IDC_EDIT_SELECTED, IDC_ALL_PRESENT, IDC_NEW, IDC_IMPORT,
                 IDC_DELETE, IDC_TOOLS, IDC_SETTINGS}) {
    HWND button = GetDlgItem(root, id);
    int previous = actionProbe.clicks;
    MSG enter{};
    enter.hwnd = button;
    enter.message = WM_KEYDOWN;
    enter.wParam = VK_RETURN;
    Check(HandleMainShortcutMessage(enter) &&
              actionProbe.clicks == previous + 1 &&
              actionProbe.source == button,
          "Enter activates each main action exactly once");
  }
  MSG editEnter{};
  editEnter.hwnd = Ui().nameEdit;
  editEnter.message = WM_KEYDOWN;
  editEnter.wParam = VK_RETURN;
  Check(!HandleMainShortcutMessage(editEnter),
        "Enter in an input does not trigger a main action");
  RemoveWindowSubclass(root, ProbeMainAction, 101);
  for (auto density : {LayoutDensity::Compact, LayoutDensity::Comfortable,
                       LayoutDensity::Spacious}) {
    Settings().layoutDensity = density;
    ApplyVisualSettings();
    Check((HFONT)SendMessageW(GetDlgItem(root, IDC_TITLE), WM_GETFONT, 0, 0) ==
                  Ui().titleFont &&
              (HFONT)SendMessageW(GetDlgItem(root, IDC_SUBTITLE), WM_GETFONT, 0,
                                  0) == Ui().smallFont,
          "density changes preserve the title and subtitle font hierarchy");
    Check(SendMessageW(Ui().courseCombo, CB_GETITEMHEIGHT, (WPARAM)-1, 0) ==
              ComboItemHeight(),
          "density changes update closed dropdown height");
  }
  int cases = 0;
  for (int language = 0; language < 20; ++language) {
    Settings().language = (UiLanguage)language;
    for (auto scale :
         {TextScale::Small, TextScale::Standard, TextScale::Large}) {
      Settings().textScale = scale;
      RecreateFonts();
      ApplyMainLanguage();
      RefreshCourseCombo();
      for (auto density : {LayoutDensity::Compact, LayoutDensity::Comfortable,
                           LayoutDensity::Spacious}) {
        Settings().layoutDensity = density;
        for (auto size : {SIZE{1600, 900}, SIZE{1180, 740}, SIZE{720, 520}}) {
          SetWindowPos(root, nullptr, -32000, -32000, size.cx, size.cy,
                       SWP_NOZORDER | SWP_NOACTIVATE);
          ResizeLayout(root);
          RECT list = SurfaceChildBox(root, IDC_LIST),
               label = SurfaceChildBox(root, 2001),
               edit = SurfaceChildBox(root, IDC_DATE);
          RECT primary = SurfaceChildBox(root, IDC_PRESENT),
               drawer = SurfaceChildBox(root, IDC_SETTINGS),
               footer = SurfaceChildBox(root, IDC_STATS);
          Check(list.bottom + 10 <= label.top && label.bottom + 4 <= edit.top,
                "table, composer labels and inputs do not overlap");
          Check(edit.bottom + 8 <= primary.top &&
                    primary.bottom + 8 <= drawer.top &&
                    drawer.bottom <= footer.top,
                "main actions and footer do not overlap");
          Check(list.bottom - list.top >= 260 &&
                    footer.bottom + Ui().scrollY <= Ui().contentH,
                "minimum table and complete scrollable content");
          RECT title = SurfaceChildBox(root, IDC_TITLE),
               subtitle = SurfaceChildBox(root, IDC_SUBTITLE),
               course = SurfaceChildBox(root, IDC_COURSE_COMBO);
          Check(title.right + 12 <= course.left &&
                    subtitle.right + 12 <= course.left,
                "header text does not overlap course selector");
          for (int id :
               {IDC_PRESENT, IDC_ABSENT, IDC_LATE, IDC_OTHER_STATUS, IDC_LEAVE,
                IDC_PENDING, IDC_SAVE, IDC_EXPORT_CSV, IDC_ADD_UPDATE,
                IDC_EDIT_SELECTED, IDC_ALL_PRESENT, IDC_NEW, IDC_IMPORT,
                IDC_DELETE, IDC_TOOLS, IDC_SETTINGS}) {
            RECT box = SurfaceChildBox(root, id);
            Check(WrappedTextHeight(GetDlgItem(root, id),
                                    box.right - box.left - 24) +
                          12 <=
                      box.bottom - box.top,
                  "all translated action labels fit");
            Check(box.left + Ui().scrollX >= 0 &&
                      box.right + Ui().scrollX <= Ui().contentW,
                  "translated actions fit the scrollable width");
          }
          ScrollMainWindow(root, SB_VERT, SB_BOTTOM);
          RECT viewport{};
          GetClientRect(root, &viewport);
          Check(SurfaceChildBox(root, IDC_SETTINGS).bottom <= viewport.bottom,
                "bottom actions reachable after scroll");
          ScrollMainWindow(root, SB_VERT, SB_TOP);
          ++cases;
        }
      }
      auto *quickState = new QuickRollCallState;
      quickState->course = Document().sheets[0].name;
      quickState->lesson = L"2026-11-02 UI lesson";
      quickState->students = Document().sheets[0].students;
      quickState->workingRecords = Document().Records();
      quickState->reviewed.assign(quickState->students.size(), false);
      for (size_t i = 0; i < quickState->students.size(); ++i)
        quickState->recordByStudent[quickState->students[i]] = (int)i;
      HWND quick =
          SurfaceFixture(L"AttendanceQuickSurfaceRegression", QuickRollCallProc,
                         root, 900, 780, quickState, WS_EX_CONTROLPARENT);
      Check(quick != nullptr, "quick roll call fixture");
      Ui().quickRollCallWindow = quick;
      for (int id : {IDC_QUICK_TITLE, IDC_QUICK_COURSE, IDC_QUICK_LESSON,
                     IDC_QUICK_PROGRESS, IDC_QUICK_STUDENT, IDC_QUICK_STATUS,
                     IDC_QUICK_NOTES_LABEL, IDC_QUICK_HINT}) {
        auto box = SurfaceChildBox(quick, id);
        Check(WrappedTextHeight(GetDlgItem(quick, id), box.right - box.left) <=
                  box.bottom - box.top,
              "translated roll call text fits");
      }
      Check(SurfaceChildBox(quick, IDC_QUICK_PROGRESS).bottom <
                SurfaceChildBox(quick, IDC_QUICK_STUDENT).top,
            "two-line progress stays above student");
      for (int id : {IDC_QUICK_PRESENT, IDC_QUICK_ABSENT, IDC_QUICK_LATE,
                     IDC_QUICK_OTHER, IDC_QUICK_LEAVE, IDC_QUICK_PENDING}) {
        SetText(GetDlgItem(quick, IDC_QUICK_NOTES), L"UI note");
        SendMessageW(quick, WM_COMMAND, MAKEWPARAM(id, BN_CLICKED), 0);
      }
      Check(std::count(quickState->reviewed.begin(), quickState->reviewed.end(),
                       true) == 6,
            "all six quick roll call states work");
      DestroyWindow(quick);

      auto chartLayout = MeasureStatsChartLayout(root, 800);
      for (size_t i = 0; i < 6; ++i) {
        Check(chartLayout.labels[i].right + 8 <= chartLayout.bars[i].left &&
                  chartLayout.bars[i].right + 8 <= chartLayout.counts[i].left,
              "chart columns do not overlap");
        Check(chartLayout.counts[i].right <= 776 &&
                  chartLayout.bars[i].bottom < chartLayout.contentHeight,
              "all six chart rows fit measured content");
        if (i)
          Check(chartLayout.bars[i - 1].bottom + 8 <= chartLayout.bars[i].top,
                "chart rows remain separate");
      }
    }
  }
  Settings().language = UiLanguage::English;
  SetWindowPos(root, nullptr, -32000, -32000, 720, 520,
               SWP_NOZORDER | SWP_NOACTIVATE);
  ResizeLayout(root);
  ScrollMainWindow(root, SB_VERT, SB_TOP);
  ScrollMainWindow(root, SB_HORZ, SB_TOP);
  SendMessageW(root, WM_MOUSEWHEEL, MAKEWPARAM(0, (WORD)-WHEEL_DELTA), 0);
  int wheelDown = Ui().scrollY;
  Check(wheelDown > 0, "mouse wheel reaches lower main-window content");
  SendMessageW(root, WM_MOUSEWHEEL, MAKEWPARAM(0, WHEEL_DELTA), 0);
  Check(Ui().scrollY < wheelDown, "main mouse wheel scrolls back upward");
  SendMessageW(root, WM_MOUSEWHEEL, MAKEWPARAM(0, (WORD)(-WHEEL_DELTA / 4)), 0);
  Check(Ui().scrollY > 0 && Ui().scrollY < wheelDown,
        "high-resolution main wheel deltas keep smaller scroll increments");
  ScrollMainWindow(root, SB_VERT, SB_TOP);
  SendMessageW(Ui().nameEdit, WM_MOUSEWHEEL, MAKEWPARAM(0, (WORD)-WHEEL_DELTA),
               0);
  Check(Ui().scrollY > 0, "wheel over main input scrolls the form downward");
  auto beforeWheelRecords = Document().Records();
  for (int copy = 0; copy < 12; ++copy)
    Document().Records().insert(Document().Records().end(),
                                beforeWheelRecords.begin(),
                                beforeWheelRecords.end());
  RefreshList();
  ScrollMainWindow(root, SB_VERT, SB_TOP);
  ListView_EnsureVisible(Ui().list, 0, FALSE);
  SendMessageW(Ui().list, WM_MOUSEWHEEL, MAKEWPARAM(0, (WORD)-WHEEL_DELTA), 0);
  Check(ListView_GetTopIndex(Ui().list) > 0 && Ui().scrollY == 0,
        "table wheel scrolls records before moving the outer form");
  ListView_EnsureVisible(Ui().list, ListView_GetItemCount(Ui().list) - 1,
                         FALSE);
  SendMessageW(Ui().list, WM_MOUSEWHEEL, MAKEWPARAM(0, (WORD)-WHEEL_DELTA), 0);
  Check(Ui().scrollY > 0,
        "wheel at the table boundary reaches lower main actions");
  Document().Records() = std::move(beforeWheelRecords);
  RefreshList();
  SendMessageW(root, WM_MOUSEHWHEEL, MAKEWPARAM(0, WHEEL_DELTA), 0);
  int wheelRight = Ui().scrollX;
  Check(wheelRight > 0, "positive horizontal wheel scrolls right");
  SendMessageW(root, WM_MOUSEHWHEEL, MAKEWPARAM(0, (WORD)-WHEEL_DELTA), 0);
  Check(Ui().scrollX < wheelRight, "negative horizontal wheel scrolls left");
  EnsureMainControlVisible(GetDlgItem(root, IDC_SETTINGS));
  RECT smallClient{};
  GetClientRect(root, &smallClient);
  RECT visibleAction = SurfaceChildBox(root, IDC_SETTINGS);
  Check(
      visibleAction.left >= 0 && visibleAction.right <= smallClient.right &&
          visibleAction.top >= 0 && visibleAction.bottom <= smallClient.bottom,
      "keyboard focus makes main actions reachable in both scroll directions");
  Settings().textScale = TextScale::Large;
  RecreateFonts();
  auto chartLayout = MeasureStatsChartLayout(root, 800);
  HWND chart =
      SurfaceFixture(L"AttendanceChartSurfaceRegression", ChartProc, root, 820,
                     chartLayout.contentHeight + 50, nullptr, WS_EX_LAYERED);
  Check(chart != nullptr, "statistics chart fixture");
  Ui().chartWindow = chart;
  RECT chartClient{};
  GetClientRect(chart, &chartClient);
  auto renderedLayout = MeasureStatsChartLayout(chart, chartClient.right);
  HDC screen = GetDC(root), dc = CreateCompatibleDC(screen);
  HBITMAP bitmap = CreateCompatibleBitmap(screen, chartClient.right,
                                          renderedLayout.contentHeight);
  HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
  PaintStatsChart(chart, dc);
  auto pending = renderedLayout.bars[5];
  Check(GetPixel(dc, pending.right - 3, (pending.top + pending.bottom) / 2) ==
            RGB(100, 100, 120),
        "pending chart bar reaches full width at animation completion");
  SelectObject(dc, oldBitmap);
  DeleteObject(bitmap);
  DeleteDC(dc);
  ReleaseDC(root, screen);
  Settings().animationLevel = AnimationLevel::Advanced;

  ThemedMenuState menuState;
  for (int i = 0; i < 40; ++i)
    menuState.items.push_back(
        {i + 1, L"Menu item " + std::to_wstring(i + 1), false});
  HWND menu = SurfaceFixture(L"AttendanceMenuSurfaceRegression", ThemedMenuProc,
                             root, 330, 360, &menuState);
  Check(menu != nullptr, "scrollable menu fixture");
  RECT menuClient{};
  GetClientRect(menu, &menuClient);
  menuState.width = menuClient.right;
  menuState.height = menuClient.bottom;
  menuState.contentHeight = ThemedMenuTotalHeight(&menuState);
  menuState.maxScrollY = menuState.contentHeight - menuState.height;
  menuState.scrollable = true;
  RECT thumb = ThemedMenuScrollThumb(&menuState);
  int thumbX = menuState.width - 7, thumbY = thumb.top + 2;
  SendMessageW(menu, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(thumbX, thumbY));
  SendMessageW(menu, WM_MOUSEMOVE, MK_LBUTTON,
               MAKELPARAM(thumbX, thumbY + 100));
  SendMessageW(menu, WM_LBUTTONUP, 0, MAKELPARAM(thumbX, thumbY + 100));
  Check(menuState.scrollY > 0 && !menuState.closing && menuState.selected == 0,
        "dragging menu scrollbar scrolls without activating a command");
  SendMessageW(menu, WM_LBUTTONDOWN, MK_LBUTTON,
               MAKELPARAM(thumbX, menuState.height - 10));
  SendMessageW(menu, WM_LBUTTONUP, 0,
               MAKELPARAM(thumbX, menuState.height - 10));
  Check(!menuState.closing && menuState.selected == 0,
        "menu scrollbar page clicks keep the menu open");
  DestroyWindow(menu);
  StartWindowReveal(chart, 190);
  StopAnimationThread();
  {
    std::lock_guard<std::mutex> lock(Animations().animMutex);
    Animations().animations[chart][(int)AnimChannel::Reveal].current = 1;
    Animations().animations[chart][(int)AnimChannel::Reveal].active = false;
    Animations().animations[chart][(int)AnimChannel::Close].current = .5;
  }
  SendMessageW(chart, WM_APP_ANIMATION_TICK, 0, 0);
  BYTE alpha = 0;
  DWORD flags = 0;
  COLORREF key = 0;
  Check(GetLayeredWindowAttributes(chart, &key, &alpha, &flags) && alpha > 0 &&
            alpha < 255,
        "chart closing presents a partial-opacity frame");
  DestroyWindow(chart);

  Settings().animationLevel = AnimationLevel::Standard;
  Check(EffectiveAnimationDuration(AnimChannel::Effect, 1250) == 1250 &&
            EffectiveAnimationDuration(AnimChannel::SmoothScroll, 240) == 240,
        "standard motion keeps complete particle and smooth-scroll lifetimes");
  Settings().animationLevel = AnimationLevel::Off;
  POINT origin{10, 10};
  TriggerButtonFeedback(GetDlgItem(root, IDC_PRESENT), origin);
  Check(!Animations().buttonEffects[GetDlgItem(root, IDC_PRESENT)].active,
        "animation off does not leave frozen particles");
  Settings().animationLevel = AnimationLevel::Advanced;

  std::wstring longText;
  for (int i = 0; i < 80; ++i)
    longText += L"Long translated explanation with every line reachable.\r\n";
  auto size = CalculateMessageDialogWindowSize(root, longText);
  MessageDialogState message{L"Test", longText, false, IDOK, false};
  HWND dialog =
      SurfaceFixture(L"AttendanceMessageSurfaceRegression", MessageDialogProc,
                     root, size.cx, size.cy, &message, WS_EX_CONTROLPARENT);
  Check(dialog != nullptr, "long message fixture");
  RECT text = SurfaceChildBox(dialog, IDC_MESSAGE_TEXT),
       ok = SurfaceChildBox(dialog, IDC_MESSAGE_OK);
  Check(text.bottom + 12 <= ok.top &&
            MeasuredTextWidth(GetDlgItem(dialog, IDC_MESSAGE_OK)) + 24 <=
                ok.right - ok.left,
        "message text and localized action stay separate");
  HWND body = GetDlgItem(dialog, IDC_MESSAGE_TEXT);
  SendMessageW(body, WM_VSCROLL, SB_BOTTOM, 0);
  Check(SendMessageW(body, EM_GETFIRSTVISIBLELINE, 0, 0) > 0,
        "long messages can scroll to their final lines");
  DestroyWindow(dialog);
  InputDialogState input{L"Test",
                         L"A wrapped translated prompt explaining how to edit "
                         L"several shortcut commands at once.",
                         L"sample", false};
  dialog = SurfaceFixture(L"AttendanceInputSurfaceRegression", InputDialogProc,
                          root, 480, 300, &input, WS_EX_CONTROLPARENT);
  Check(dialog != nullptr, "wrapped input fixture");
  ResizeInputDialog(dialog);
  auto prompt = SurfaceChildBox(dialog, 0);
  auto field = SurfaceChildBox(dialog, IDC_INPUT_EDIT);
  ok = SurfaceChildBox(dialog, IDC_INPUT_OK);
  Check(prompt.bottom + 8 <= field.top && field.bottom + 12 <= ok.top,
        "input prompt wraps without covering input or actions");
  DestroyWindow(dialog);

  HDC digitsScreen = GetDC(root);
  dc = CreateCompatibleDC(digitsScreen);
  bitmap = CreateCompatibleBitmap(digitsScreen, 240, 100);
  oldBitmap = SelectObject(dc, bitmap);
  RECT digitsRect{0, 0, 240, 100};
  FillRect(dc, &digitsRect, (HBRUSH)GetStockObject(BLACK_BRUSH));
  HGDIOBJ oldFont = SelectObject(dc, Ui().font);
  SetTextColor(dc, RGB(255, 255, 255));
  SetBkMode(dc, TRANSPARENT);
  DrawStableDigits(dc, L"Reviewed 1 / 6\nRemaining 5", digitsRect, DT_RIGHT);
  bool upper = false, lower = false;
  for (int y = 0; y < 100; ++y)
    for (int x = 0; x < 240; ++x)
      if (GetPixel(dc, x, y) != RGB(0, 0, 0)) {
        if (y < 50)
          upper = true;
        else
          lower = true;
      }
  Check(upper && lower, "stable numeric text renders both progress lines");
  SelectObject(dc, oldFont);
  SelectObject(dc, oldBitmap);
  DeleteObject(bitmap);
  DeleteDC(dc);
  ReleaseDC(root, digitsScreen);

  Settings().animationLevel = AnimationLevel::Advanced;
  TestSharedComboPopup(Ui().courseCombo, false);
  StopOperationFeedback();
  StopAnimationThread();
  KillTimer(root, 1);
  KillTimer(root, 2);
  SetWindowLongPtrW(root, GWLP_WNDPROC, (LONG_PTR)DefWindowProcW);
  DestroyWindow(root);
  if (Ui().font)
    DeleteObject(Ui().font);
  if (Ui().titleFont)
    DeleteObject(Ui().titleFont);
  if (Ui().smallFont)
    DeleteObject(Ui().smallFont);
  if (Ui().bgBrush)
    DeleteObject(Ui().bgBrush);
  if (Ui().inputBrush)
    DeleteObject(Ui().inputBrush);
  if (Ui().panelBrush)
    DeleteObject(Ui().panelBrush);
  Ui() = savedUi;
  Document() = savedDocument;
  ApplySettingsSnapshot(savedSettings);
  std::cout << "UI surface geometry: " << cases
            << " main layouts; 60 roll calls/charts; dialogs, progress, "
               "settings persistence and motion regressions passed\n";
}
