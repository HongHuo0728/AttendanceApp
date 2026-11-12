#include "ui/feedback.h"
#include "ui/motion.h"
#pragma once
#include "animation/state.h"
#include "ui/common.h"
#include "ui/controls.h"
#include "ui/redraw.h"
#include "ui/settings_window.h"
#include "ui/state.h"
#include "ui/theme.h"

namespace attendance {
void ResizeSettingsLayout(HWND);
void ApplySettingsFromWindow(HWND);
void SyncSettingsWindowState(HWND);
} // namespace attendance

struct PaintProbe {
  int text = 0, paint = 0, reset = 0, theme = 0, position = 0;
};

struct ComboPopupProbe {
  HWND combo = nullptr;
  bool commit = false, shared = false, nativeHidden = false;
  bool opened = false, closed = false, timedOut = false;
  int phase = 0;
  uint64_t deadline = 0;
  double closeStart = 0;
};
void CALLBACK ObserveComboPopup(HWND owner, UINT, UINT_PTR, DWORD) {
  auto *probe = (ComboPopupProbe *)GetPropW(owner, L"Test.ComboPopupProbe");
  if (!probe)
    return;
  HWND popup = (HWND)GetPropW(probe->combo, L"Attendance.ComboPopup");
  if (!popup || !IsWindow(popup))
    return;
  if (AnimationNowMs() > probe->deadline) {
    probe->timedOut = true;
    DestroyWindow(popup);
    return;
  }
  double arrow = GetAnimationValue(probe->combo, AnimChannel::ComboOpen, 0);
  if (probe->phase == 0) {
    if (arrow == 0)
      return;
    wchar_t type[64]{};
    GetClassNameW(popup, type, 64);
    probe->shared =
        lstrcmpW(type, L"AttendanceThemedPopupMenu") == 0 &&
        GetWindow(popup, GW_OWNER) == GetAncestor(probe->combo, GA_ROOT);
    COMBOBOXINFO info{sizeof(info)};
    probe->nativeHidden = GetComboBoxInfo(probe->combo, &info) &&
                          info.hwndList && !IsWindowVisible(info.hwndList);
    probe->opened = arrow > 0 && arrow < 1 &&
                    SendMessageW(probe->combo, CB_GETDROPPEDSTATE, 0, 0);
    if (probe->commit) {
      SendMessageW(popup, WM_KEYDOWN, VK_END, 0);
      SendMessageW(popup, WM_KEYDOWN, VK_RETURN, 0);
    } else
      SendMessageW(popup, WM_KEYDOWN, VK_ESCAPE, 0);
    probe->closeStart =
        GetAnimationValue(probe->combo, AnimChannel::ComboOpen, 0);
    probe->phase = 1;
  } else if (arrow > 0 && arrow < probe->closeStart)
    probe->closed = true;
}
void TestSharedComboPopup(HWND combo, bool commit) {
  HWND owner = GetAncestor(combo, GA_ROOT);
  int previous = (int)SendMessageW(combo, CB_GETCURSEL, 0, 0);
  auto motion = Settings().animationLevel;
  Settings().animationLevel = AnimationLevel::Advanced;
  RemoveAnimationState(combo);
  ComboPopupProbe probe;
  probe.combo = combo;
  probe.commit = commit;
  probe.deadline = AnimationNowMs() + 3000;
  SetPropW(owner, L"Test.ComboPopupProbe", &probe);
  SetTimer(owner, 904, 16, ObserveComboPopup);
  SendMessageW(combo, CB_SHOWDROPDOWN, TRUE, 0);
  KillTimer(owner, 904);
  RemovePropW(owner, L"Test.ComboPopupProbe");
  Check(probe.shared && probe.nativeHidden && !probe.timedOut,
        "combos use the Tools popup class and their own top-level owner");
  Check(probe.opened && probe.closed,
        "combo arrows have real intermediate values on opening and closing");
  Check(!SendMessageW(combo, CB_GETDROPPEDSTATE, 0, 0),
        "shared combo clears expanded state on dismissal");
  int expected =
      commit ? (int)SendMessageW(combo, CB_GETCOUNT, 0, 0) - 1 : previous;
  Check(SendMessageW(combo, CB_GETCURSEL, 0, 0) == expected,
        "shared combo keyboard selection commits and Escape cancels");
  Settings().animationLevel = motion;
  std::cout << "shared combo popup, owner, selection and two-way arrow "
               "animation passed\n";
}
LRESULT CALLBACK ProbeControl(HWND hwnd, UINT msg, WPARAM w, LPARAM l,
                              UINT_PTR id, DWORD_PTR data) {
  auto &probe = *reinterpret_cast<PaintProbe *>(data);
  if (msg == WM_SETTEXT)
    ++probe.text;
  if (msg == WM_PAINT)
    ++probe.paint;
  if (msg == CB_RESETCONTENT)
    ++probe.reset;
  if (msg == WM_THEMECHANGED)
    ++probe.theme;
  if (msg == WM_WINDOWPOSCHANGED)
    ++probe.position;
  if (msg == WM_NCDESTROY)
    RemoveWindowSubclass(hwnd, ProbeControl, id);
  return DefSubclassProc(hwnd, msg, w, l);
}

void TestUiRedraw() {
  UiState previousUi = Ui();
  Ui() = UiState{};
  auto oldLanguage = Settings().language;
  auto oldDensity = Settings().layoutDensity;
  auto oldTheme = Settings().theme;
  int oldInterval = Settings().autosaveIntervalSeconds;
  auto oldScale = Settings().textScale;
  INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_LISTVIEW_CLASSES};
  InitCommonControlsEx(&controls);
  WNDCLASSW wc{};
  wc.hInstance = GetModuleHandleW(nullptr);
  wc.lpfnWndProc = DefWindowProcW;
  wc.lpszClassName = L"AttendancePaintRegression";
  RegisterClassW(&wc);
  // Off-screen native fixtures exercise message behavior without touching a
  // user's app.
  HWND root = CreateWindowExW(0, wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW,
                              -32000, -32000, 1040, 860, nullptr, nullptr,
                              wc.hInstance, nullptr);
  if (!root)
    throw std::runtime_error("paint regression root");
  Ui().hwnd = root;
  // Exercise the same manifest activation context as the released application.
  // LVS_EX_DOUBLEBUFFER silently has no effect with legacy common controls.
  HWND listProbe =
      CreateWindowExW(0, WC_LISTVIEWW, L"", WS_CHILD | LVS_REPORT, 0, 0, 100,
                      60, root, nullptr, wc.hInstance, nullptr);
  if (!listProbe)
    throw std::runtime_error("native list fixture creation failed");
  struct ControlVersion {
    DWORD size, major, minor, build, platform;
  };
  auto controlModule =
      reinterpret_cast<HMODULE>(GetClassLongPtrW(listProbe, GCLP_HMODULE));
  if (!controlModule)
    throw std::runtime_error("native list control module unavailable");
  auto versionFunction = reinterpret_cast<HRESULT(WINAPI *)(ControlVersion *)>(
      GetProcAddress(controlModule, "DllGetVersion"));
  ControlVersion controlVersion{sizeof(ControlVersion), 0, 0, 0, 0};
  Check(versionFunction && SUCCEEDED(versionFunction(&controlVersion)) &&
            controlVersion.major >= 6,
        "release manifest activates common controls with native double "
        "buffering");
  DestroyWindow(listProbe);
  ValidateRect(root, nullptr);
  {
    RedrawScope hidden(root);
  }
  Check(!(GetWindowLongPtrW(root, GWL_STYLE) & WS_VISIBLE),
        "hidden redraw scope never shows a window");
  HWND startupLabel =
      CreateWindowExW(0, L"STATIC", L"Startup label", WS_CHILD | WS_VISIBLE, 0,
                      0, 100, 30, root, nullptr, wc.hInstance, nullptr);
  if (!startupLabel)
    throw std::runtime_error("startup label creation");
  ValidateRect(startupLabel, nullptr);
  {
    RedrawScope hiddenLayout(root);
    MoveWindow(startupLabel, 120, 60, 320, 40, FALSE);
  }
  RECT startupRect{};
  GetClientRect(startupLabel, &startupRect);
  Check(startupRect.right == 320 && startupRect.bottom == 40 &&
            !(GetWindowLongPtrW(root, GWL_STYLE) & WS_VISIBLE),
        "hidden layout moves children without showing the parent");
  DestroyWindow(startupLabel);
  SetWindowLongPtrW(root, GWL_STYLE,
                    GetWindowLongPtrW(root, GWL_STYLE) | WS_VISIBLE);
  {
    RedrawScope outer(root);
    {
      RedrawScope inner(root);
    }
    Check(!(GetWindowLongPtrW(root, GWL_STYLE) & WS_VISIBLE),
          "nested scope cannot resume an outer paint");
  }
  Check((GetWindowLongPtrW(root, GWL_STYLE) & WS_VISIBLE) != 0,
        "outer scope resumes visible fixture");
  ShowWindow(root, SW_HIDE);
  HWND label =
      CreateWindowExW(0, L"STATIC", L"unchanged", WS_CHILD | WS_VISIBLE, 0, 0,
                      150, 30, root, nullptr, wc.hInstance, nullptr);
  PaintProbe labelProbe;
  SetWindowSubclass(label, ProbeControl, 99,
                    reinterpret_cast<DWORD_PTR>(&labelProbe));
  ValidateRect(label, nullptr);
  SetText(label, L"unchanged");
  Check(labelProbe.text == 0 && !GetUpdateRect(label, nullptr, FALSE),
        "unchanged statistics do not repaint");
  SetText(label, L"changed");
  Check(labelProbe.text == 1 && labelProbe.paint == 0,
        "changed text is queued without synchronous paint");
  HWND edit = CreateWindowExW(0, L"EDIT", L"editable",
                              WS_CHILD | WS_VISIBLE | WS_BORDER, 0, 40, 150, 30,
                              root, nullptr, wc.hInstance, nullptr);
  ValidateRect(edit, nullptr);
  RedrawAnimatedControl(edit);
  Check(!GetUpdateRect(edit, nullptr, FALSE),
        "edit border animation does not invalidate native text");
  // Buffered frames present only once and preserve pixels outside their
  // rectangle.
  HDC dc = CreateCompatibleDC(nullptr);
  HBITMAP bitmap = CreateBitmap(24, 18, 1, 32, nullptr);
  HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
  RECT all{0, 0, 24, 18};
  FillRect(dc, &all, (HBRUSH)GetStockObject(BLACK_BRUSH));
  RECT rect{4, 3, 20, 15};
  HBRUSH red = CreateSolidBrush(RGB(255, 0, 0));
  {
    BufferedPaint frame(dc, rect);
    FillRect(frame.DC(), &rect, red);
    if (frame.DC() != dc)
      Check(GetPixel(dc, 5, 5) == RGB(0, 0, 0),
            "incomplete buffered frame is not presented");
    frame.Commit();
  }
  Check(GetPixel(dc, 5, 5) == RGB(255, 0, 0) &&
            GetPixel(dc, 0, 0) == RGB(0, 0, 0),
        "buffer commit and clipping");
  DeleteObject(red);
  SelectObject(dc, oldBitmap);
  DeleteObject(bitmap);
  DeleteDC(dc);

  Settings().language = UiLanguage::English;
  Settings().theme = UiTheme::Dark;
  ApplyThemePalette();
  RecreateFonts();
  PrewarmSettingsWindow();
  HWND settings = Ui().settingsWindow;
  if (!settings)
    throw std::runtime_error("settings creation failed");
  Check(settings && !(GetWindowLongPtrW(settings, GWL_STYLE) & WS_VISIBLE),
        "settings prewarm stays hidden after creation and layout");
  Check(GetText(SettingsControl(settings, IDC_SETTINGS_TITLE)) == L"Settings",
        "single settings title");
  HWND fpsInput = SettingsControl(settings, IDC_SETTINGS_SCROLL_FPS);
  RECT fpsText{}, fpsClient{};
  SendMessageW(fpsInput, EM_GETRECT, 0, (LPARAM)&fpsText);
  GetClientRect(fpsInput, &fpsClient);
  std::cout << "FPS text rect=" << fpsText.top << "," << fpsText.bottom
            << " client=" << fpsClient.bottom
            << " style=" << GetWindowLongPtrW(fpsInput, GWL_STYLE) << "\n";
  Check(fpsText.top > 2 && fpsText.bottom < fpsClient.bottom &&
            (GetWindowLongPtrW(
                 SettingsControl(settings, IDC_SETTINGS_SCROLL_FPS_UNIT),
                 GWL_STYLE) &
             SS_CENTERIMAGE),
        "numeric settings input and its unit share vertical alignment");
  bool obsoleteTitle = false;
  EnumChildWindows(
      settings,
      [](HWND child, LPARAM data) -> BOOL {
        if (GetText(child).find(L"Advanced personalization") !=
            std::wstring::npos)
          *reinterpret_cast<bool *>(data) = true;
        return TRUE;
      },
      reinterpret_cast<LPARAM>(&obsoleteTitle));
  Check(!obsoleteTitle, "obsolete personalization section removed");
  for (int width : {520, 1040}) {
    SetWindowPos(settings, nullptr, -32000, -32000, width, 700,
                 SWP_NOZORDER | SWP_NOACTIVATE);
    ResizeSettingsLayout(settings);
    RECT client{};
    GetClientRect(settings, &client);
    for (int id : {IDC_SETTINGS_LANGUAGE,        IDC_SETTINGS_FONT,
                   IDC_SETTINGS_THEME,           IDC_SETTINGS_ACCENT,
                   IDC_SETTINGS_TEXT_SCALE,      IDC_SETTINGS_DENSITY,
                   IDC_SETTINGS_ANIMATION_LEVEL, IDC_SETTINGS_PARTICLE_LEVEL,
                   IDC_SETTINGS_REPORT_TEMPLATE, IDC_SETTINGS_AUTOSAVE_INTERVAL,
                   IDC_SETTINGS_SCROLL_FPS,      IDC_SETTINGS_PARTICLES,
                   IDC_SETTINGS_SMOOTH_SCROLL,   IDC_SETTINGS_SHOW_STATS,
                   IDC_SETTINGS_SHOW_HINT,       IDC_SETTINGS_TABLE_GRID,
                   IDC_SETTINGS_RISK_ALERTS,     IDC_SETTINGS_AUTOSAVE_PROMPT,
                   IDC_SETTINGS_COMMAND_PALETTE, IDC_SETTINGS_ADVANCED_FILTER,
                   IDC_SETTINGS_RESET,           IDC_SETTINGS_CLOSE}) {
      HWND child = SettingsControl(settings, id);
      RECT box{};
      GetWindowRect(child, &box);
      MapWindowPoints(nullptr, settings, reinterpret_cast<POINT *>(&box), 2);
      Check(child && box.left >= 0 && box.right <= client.right &&
                box.right > box.left,
            "unified settings controls stay within narrow and wide layouts");
    }
    SCROLLINFO info{sizeof(info)};
    info.fMask = SIF_ALL;
    GetScrollInfo(settings, SB_VERT, &info);
    Check(info.nMax >= (int)info.nPage,
          "settings overflow remains reachable by scrolling");
    RECT firstReset{}, firstClose{};
    GetWindowRect(SettingsControl(settings, IDC_SETTINGS_RESET), &firstReset);
    GetWindowRect(SettingsControl(settings, IDC_SETTINGS_CLOSE), &firstClose);
    MapWindowPoints(nullptr, settings, (POINT *)&firstReset, 2);
    MapWindowPoints(nullptr, settings, (POINT *)&firstClose, 2);
    Check(firstReset.top >= Ui().settingsBodyBottom &&
              firstClose.bottom <= client.bottom,
          "settings actions visible before scrolling");
    SendMessageW(settings, WM_VSCROLL, SB_BOTTOM, 0);
    RECT close{};
    GetWindowRect(SettingsControl(settings, IDC_SETTINGS_CLOSE), &close);
    MapWindowPoints(nullptr, settings, reinterpret_cast<POINT *>(&close), 2);
    Check(close.top >= 0 && close.bottom <= client.bottom,
          "fixed settings actions remain visible during scrolling");
  }
  ShowWindow(settings, SW_SHOWNOACTIVATE);
  auto previousMotion = Settings().animationLevel;
  bool previousSmooth = Settings().smoothScrollingEnabled;
  Settings().animationLevel = AnimationLevel::Advanced;
  Settings().smoothScrollingEnabled = true;
  Ui().settingsScrollY = 0;
  Ui().settingsScrollCurrent = Ui().settingsScrollTarget = 0;
  Ui().settingsScrollStartMs = 0;
  ResizeSettingsLayout(settings);
  SendMessageW(settings, WM_VSCROLL, SB_BOTTOM, 0);
  StopAnimationThread();
  Check(Ui().settingsScrollTarget > 0 &&
            Ui().settingsScrollY < Ui().settingsScrollTarget,
        "settings scroll schedules continuous motion");
  Ui().settingsScrollStartMs = AnimationNowMs() - 100;
  SendMessageW(settings, WM_APP_ANIMATION_TICK, 0, 0);
  Check(Ui().settingsScrollY > 0 &&
            Ui().settingsScrollY < Ui().settingsScrollTarget,
        "settings scroll has a real intermediate position");
  Settings().smoothScrollingEnabled = false;
  SendMessageW(settings, WM_VSCROLL, SB_TOP, 0);
  StopAnimationThread();
  Check(Ui().settingsScrollY == 0 && !Ui().settingsScrollStartMs,
        "smooth scrolling off immediately reaches its target");
  HWND cacheCombo = SettingsControl(settings, IDC_SETTINGS_THEME);
  PaintProbe scrollProbe;
  RemoveAnimationState(settings);
  ResizeSettingsLayout(settings);
  SendMessageW(settings, WM_APP_ANIMATION_TICK, 0, 0);
  SetWindowSubclass(cacheCombo, ProbeControl, 97, (DWORD_PTR)&scrollProbe);
  ValidateRect(settings, nullptr);
  ValidateRect(cacheCombo, nullptr);
  for (int frame = 0; frame < 5; ++frame)
    SendMessageW(settings, WM_APP_ANIMATION_TICK, 0, 0);
  Check(!(GetWindowLongPtrW(settings, GWL_EXSTYLE) &
          (WS_EX_COMPOSITED | WS_EX_LAYERED)),
        "settled settings release layered/composited redirection before "
        "scrolling");
  Check(
      scrollProbe.position == 0 && !GetUpdateRect(cacheCombo, nullptr, FALSE),
      "settled reveal ticks neither move controls nor invalidate form fields");
  int selection = (int)SendMessageW(cacheCombo, CB_GETCURSEL, 0, 0);
  SendMessageW(cacheCombo, WM_MOUSEWHEEL, MAKEWPARAM(0, (WORD)-WHEEL_DELTA), 0);
  Check(Ui().settingsScrollY > 0 &&
            (int)SendMessageW(cacheCombo, CB_GETCURSEL, 0, 0) == selection,
        "wheel over a closed settings combo scrolls the form without changing "
        "its value");
  SCROLLINFO scrollRange{sizeof(scrollRange)};
  scrollRange.fMask = SIF_ALL;
  GetScrollInfo(settings, SB_VERT, &scrollRange);
  int scrollMax = std::max(0, scrollRange.nMax - (int)scrollRange.nPage + 1);
  uint64_t scrollStart = PerformanceNowUs(), scrollPeak = 0;
  HWND content = GetParent(cacheCombo);
  Check(
      GetParent(GetParent(content)) == settings,
      "settings fields live inside a clipped viewport and one content window");
  PaintProbe contentProbe;
  scrollProbe.position = 0;
  SetWindowSubclass(content, ProbeControl, 96, (DWORD_PTR)&contentProbe);
  for (int frame = 0; frame < 120; ++frame) {
    Ui().settingsScrollY = scrollMax * frame / 119;
    uint64_t frameStart = PerformanceNowUs();
    ResizeSettingsLayout(settings);
    scrollPeak = std::max(scrollPeak, PerformanceNowUs() - frameStart);
  }
  std::cout << "settings positioning mean_us="
            << (PerformanceNowUs() - scrollStart) / 120
            << " max_us=" << scrollPeak << "\n";
  Check(scrollProbe.position == 0 && contentProbe.position > 0,
        "settings scroll moves the content window without moving individual "
        "fields");
  std::cout << "settings scroll content_moves=" << contentProbe.position
            << " field_moves=" << scrollProbe.position << "\n";
  RemoveWindowSubclass(content, ProbeControl, 96);
  SendMessageW(settings, WM_VSCROLL, SB_TOP, 0);
  RemoveWindowSubclass(cacheCombo, ProbeControl, 97);
  Settings().animationLevel = previousMotion;
  Settings().smoothScrollingEnabled = previousSmooth;
  ShowWindow(settings, SW_HIDE);
  RemoveAnimationState(settings);
  SendMessageW(settings, WM_APP_ANIMATION_TICK, 0, 0);
  Check(!IsWindowVisible(settings),
        "late animation ticks cannot reveal hidden settings");
  // Language/font/layout refreshes must never expose the cached hidden dialog.
  for (auto language :
       {UiLanguage::English, UiLanguage::ChineseSimplified,
        UiLanguage::ChineseTraditionalHongKong, UiLanguage::German}) {
    Settings().language = language;
    ApplyLanguageAtomically(settings);
    SyncSettingsWindowState(settings);
    Check(!(GetWindowLongPtrW(settings, GWL_STYLE) & WS_VISIBLE),
          "language refresh preserves cached window visibility");
  }
  HWND tab = GetNextDlgTabItem(settings, nullptr, FALSE);
  Check(GetText(SettingsControl(settings, IDC_SETTINGS_THEME_LABEL)) ==
                L"Design" &&
            Tr(L"Untranslated test label", L"测试回退") ==
                L"Untranslated test label",
        "German settings labels translate and missing keys use English");
  SendMessageW(SettingsControl(settings, IDC_SETTINGS_LANGUAGE), CB_SETCURSEL,
               static_cast<WPARAM>(UiLanguage::ChineseSimplified), 0);
  ApplySettingsFromWindow(settings);
  Check(Settings().language == UiLanguage::ChineseSimplified &&
            GetText(SettingsControl(settings, IDC_SETTINGS_TITLE)) == L"设置" &&
            !(GetWindowLongPtrW(settings, GWL_STYLE) & WS_VISIBLE),
        "Apply refreshes changed language and preserves hidden visibility");
  for (int id : {IDC_SETTINGS_LANGUAGE, IDC_SETTINGS_FONT, IDC_SETTINGS_THEME,
                 IDC_SETTINGS_ACCENT, IDC_SETTINGS_TEXT_SCALE,
                 IDC_SETTINGS_DENSITY, IDC_SETTINGS_ANIMATION_LEVEL}) {
    Check(tab == SettingsControl(settings, id),
          "settings tab order follows the unified form");
    tab = GetNextDlgTabItem(settings, tab, FALSE);
  }
  Ui().settingsScrollY = 0;
  ResizeSettingsLayout(settings);
  EnsureSettingsControlVisible(SettingsControl(settings, IDC_SETTINGS_CLOSE));
  RECT action{}, viewport{};
  GetWindowRect(SettingsControl(settings, IDC_SETTINGS_CLOSE), &action);
  GetClientRect(settings, &viewport);
  MapWindowPoints(nullptr, settings, reinterpret_cast<POINT *>(&action), 2);
  Check(action.top >= 0 && action.bottom <= viewport.bottom,
        "keyboard focus scrolls actions into view");
  PaintProbe comboProbe, positionProbe;
  HWND theme = SettingsControl(settings, IDC_SETTINGS_THEME);
  SetWindowSubclass(theme, ProbeControl, 99,
                    reinterpret_cast<DWORD_PTR>(&comboProbe));
  SetWindowSubclass(settings, ProbeControl, 99,
                    reinterpret_cast<DWORD_PTR>(&positionProbe));
  SendMessageW(theme, WM_APP_COMMIT_COMBO_SELECTION, 1, 0);
  Check(Settings().theme == UiTheme::Light && Ui().themeTransition.active,
        "theme change retains transition");
  Check(comboProbe.reset == 0 && positionProbe.position == 0,
        "theme option neither rebuilds combos nor moves settings");
  StopAnimationThread();
  auto frameAt = [&](double progress) {
    {
      std::lock_guard<std::mutex> lock(Animations().animMutex);
      Animations().animations[root][(int)AnimChannel::ThemeTransition].current =
          progress;
    }
    UpdateThemeTransitionFrame();
    return GetRValue(Ui().COLOR_BG);
  };
  const int start = frameAt(0.0), middle = frameAt(0.5);
  Check(start == 0 && middle > start && middle < 255,
        "theme palette has a real intermediate frame");
  BeginThemeTransition(UiTheme::Dark);
  StopAnimationThread();
  Check(frameAt(0.0) == middle && frameAt(0.5) < middle && frameAt(1.0) == 0,
        "rapid theme reversal continues from the displayed palette");
  BeginThemeTransition(UiTheme::Light);
  StopAnimationThread();
  Check(frameAt(1.0) == 255 && !Ui().themeTransition.active,
        "theme transition reaches its final palette");
  Check(!(GetWindowLongPtrW(settings, GWL_STYLE) & WS_VISIBLE),
        "theme completion preserves hidden settings visibility");
  RemoveAnimationState(root);
  Settings().theme = UiTheme::Dark;
  ApplyThemePalette();
  ApplyNativeControlTheme(theme);
  int firstTheme = comboProbe.theme;
  ApplyNativeControlTheme(theme);
  Check(comboProbe.theme == firstTheme,
        "identical native theme does not trigger another theme change");
  SendMessageW(theme, CB_SETCURSEL, 0, 0);
  TestSharedComboPopup(theme, true);
  SendMessageW(theme, WM_APP_COMMIT_COMBO_SELECTION, 0, 0);
  auto oldAnimation = Settings().animationLevel;
  Settings().animationLevel = AnimationLevel::Advanced;
  DWORD warmedGdi = 0, warmedUser = 0;
  for (int cycle = 0; cycle < 30; ++cycle) {
    ShowSettingsWindow();
    StopAnimationThread();
    Check(IsWindowVisible(settings) &&
              GetAnimationValue(settings, AnimChannel::Reveal, 1) < 1,
          "cached settings restarts opening motion");
    {
      std::lock_guard<std::mutex> lock(Animations().animMutex);
      auto &value = Animations().animations[settings][(int)AnimChannel::Reveal];
      value.current = .5;
      value.active = false;
    }
    HideSettingsWindow();
    StopAnimationThread();
    Check(Ui().settingsClosing && IsWindowVisible(settings),
          "settings closes through a cached-window animation");
    UINT_PTR previousClose = Ui().settingsCloseSerial;
    double reversingFrom = 0;
    {
      std::lock_guard<std::mutex> lock(Animations().animMutex);
      auto &value = Animations().animations[settings][(int)AnimChannel::Reveal];
      reversingFrom = value.current;
      value.active = false;
    }
    ShowSettingsWindow();
    StopAnimationThread();
    double newStart = 0;
    {
      std::lock_guard<std::mutex> lock(Animations().animMutex);
      newStart =
          Animations().animations[settings][(int)AnimChannel::Reveal].start;
    }
    Check(
        !Ui().settingsClosing && newStart == reversingFrom,
        "reopening reverses the captured closing frame without a restart jump");
    HideSettingsWindow();
    StopAnimationThread();
    SendMessageW(settings, WM_APP_ANIMATION_DONE, 0, (LPARAM)previousClose);
    Check(IsWindowVisible(settings) && Ui().settingsClosing,
          "stale close completion cannot hide a new animation");
    {
      std::lock_guard<std::mutex> lock(Animations().animMutex);
      auto &value = Animations().animations[settings][(int)AnimChannel::Reveal];
      value.current = 0;
      value.active = false;
    }
    SendMessageW(settings, WM_APP_ANIMATION_DONE, 0,
                 (LPARAM)Ui().settingsCloseSerial);
    if (IsWindowVisible(settings) || Ui().settingsClosing) {
      std::cerr << "close cycle=" << cycle
                << " visible=" << IsWindowVisible(settings)
                << " closing=" << Ui().settingsClosing
                << " serial=" << Ui().settingsCloseSerial << " reveal="
                << GetAnimationValue(settings, AnimChannel::Reveal, -1) << "\n";
      throw std::runtime_error(
          "close completion hides without destroying settings");
    }
    if (cycle == 0) {
      warmedGdi = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
      warmedUser = GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS);
    }
  }
  Check(GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS) <= warmedGdi + 4,
        "cached settings cycles do not accumulate GDI objects");
  Check(GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS) <= warmedUser + 2,
        "cached settings cycles do not accumulate USER objects");
  Settings().animationLevel = AnimationLevel::Off;
  RestartAnimation(settings, AnimChannel::Reveal, 1, 240);
  Check(GetAnimationValue(settings, AnimChannel::Reveal, 0) == 1,
        "animation off reaches final opening frame");
  ShowSettingsWindow();
  HideSettingsWindow();
  StopAnimationThread();
  if (IsWindowVisible(settings))
    std::cerr << "off close visible=" << IsWindowVisible(settings)
              << " closing=" << Ui().settingsClosing << " hidden="
              << (UINT_PTR)GetPropW(settings, L"Attendance.SettingsHidden")
              << "\n";
  Check(!IsWindowVisible(settings), "animation off closes immediately");
  SendMessageW(settings, WM_SETREDRAW, TRUE, 0);
  Check(!IsWindowVisible(settings),
        "late redraw enable preserves explicit hidden intent");
  Settings().animationLevel = oldAnimation;
  for (int language = 0; language < 20; ++language) {
    Settings().language = (UiLanguage)language;
    ApplyLanguageAtomically(settings);
    for (auto scale :
         {TextScale::Small, TextScale::Standard, TextScale::Large}) {
      Settings().textScale = scale;
      RecreateFonts();
      EnumChildWindows(settings, ApplyFontToChild, 0);
      for (auto density : {LayoutDensity::Compact, LayoutDensity::Comfortable,
                           LayoutDensity::Spacious}) {
        Settings().layoutDensity = density;
        for (int width : {520, 1040}) {
          SetWindowPos(settings, nullptr, -32000, -32000, width, 700,
                       SWP_NOZORDER | SWP_NOACTIVATE);
          ResizeSettingsLayout(settings);
          RECT area{};
          GetClientRect(settings, &area);
          for (int id : {IDC_SETTINGS_RESET, IDC_SETTINGS_CLOSE}) {
            RECT box{};
            GetWindowRect(SettingsControl(settings, id), &box);
            MapWindowPoints(nullptr, settings, (POINT *)&box, 2);
            Check(box.left >= 0 && box.right <= area.right &&
                      box.top >= Ui().settingsBodyBottom &&
                      box.bottom <= area.bottom,
                  "all-language footer geometry");
            Check(MeasuredTextWidth(SettingsControl(settings, id)) + 16 <=
                      box.right - box.left,
                  "translated actions fit without truncation");
          }
        }
      }
    }
  }
  StopOperationFeedback();
  RemoveWindowSubclass(theme, ProbeControl, 99);
  RemoveWindowSubclass(settings, ProbeControl, 99);
  DestroyWindow(settings);
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
  Ui() = previousUi;
  Settings().language = oldLanguage;
  Settings().theme = oldTheme;
  Settings().autosaveIntervalSeconds = oldInterval;
  Settings().textScale = oldScale;
  Settings().layoutDensity = oldDensity;
  std::cout << "UI redraw, hidden settings, buffered paint and unified layout "
               "checks passed\n";
}
