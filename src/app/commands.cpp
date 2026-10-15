#include "app/commands.h"
#include "core/text.h"
#include "settings/shortcuts.h"
#include "ui/dialogs.h"
#include "localization/strings.h"
#include "settings/preferences.h"
#include "ui/popup_menu.h"
#include "reports/csv.h"
#include "model/history.h"
#include "reports/powerpoint.h"
#include "reports/html.h"
#include "app/courses.h"
#include "app/insights.h"
#include "app/recovery.h"
#include "ui/roll_call.h"
#include "reports/database.h"
#include "ui/statistics.h"
#include "app/documents.h"
#include "ui/settings_window.h"
#include "model/state.h"
#include "settings/state.h"
#include "core/constants.h"

namespace attendance {

void ShowReportTemplateMenu(HWND button);
void ShowShortcutCenter();
void ShowShortcuts();

void ShowReportTemplateMenu(HWND button){
    int command = ShowThemedPopupMenu(button, {
        {1, ReportTemplateName(ReportTemplate::Simple), false},
        {2, ReportTemplateName(ReportTemplate::Teacher), false},
        {3, ReportTemplateName(ReportTemplate::Parent), false},
        {4, ReportTemplateName(ReportTemplate::Complete), false}
    });
    if (command >= 1 && command <= 4) {
        Settings().reportTemplate = (ReportTemplate)(command - 1);
        SaveSettingsWithFeedback();
    }
}

void ShowShortcutCenter(){
    std::wstring config = ShortcutConfigText();
    std::wstring prompt = Tr(
        L"Edit shortcuts as action=key; use Ctrl+letter or F keys:",
        L"\u6309 action=key \u7f16\u8f91\u5feb\u6377\u952e\uff0c\u652f\u6301 Ctrl+\u5b57\u6bcd \u6216 F \u952e\uff1a"
    );
    if (!PromptText(Tr(L"Shortcut center", L"\u5feb\u6377\u952e\u4e2d\u5fc3"), prompt, config)) return;
    if (ApplyShortcutConfigText(config)) {
        SaveSettingsWithFeedback();
        ShowMessage(Tr(L"Shortcuts updated.", L"\u5feb\u6377\u952e\u5df2\u66f4\u65b0\u3002"), Tr(L"Shortcut center", L"\u5feb\u6377\u952e\u4e2d\u5fc3"));
    } else {
        ShowMessage(Tr(L"No shortcut changes were applied.", L"\u672a\u5e94\u7528\u5feb\u6377\u952e\u66f4\u6539\u3002"), Tr(L"Shortcut center", L"\u5feb\u6377\u952e\u4e2d\u5fc3"));
    }
}

void RunCommandPalette(){
    if (!Settings().commandPaletteEnabled) return;
    std::wstring command;
    if (!PromptText(Tr(L"Command palette", L"\u547d\u4ee4\u9762\u677f"), Tr(L"Type a command: save, import, export, backup, stats, risk, filter, lesson, rollcall, review, profile, settings", L"\u8f93\u5165\u547d\u4ee4\uff1asave\u3001import\u3001export\u3001backup\u3001stats\u3001risk\u3001filter\u3001lesson\u3001rollcall\u3001\u70b9\u540d\u3001review\u3001profile\u3001settings"), command)) return;
    command = LowerText(TrimWide(command));
    if (command.find(L"save") != std::wstring::npos || command.find(L"\u4fdd\u5b58") != std::wstring::npos || command.find(L"儲存") != std::wstring::npos) SaveAttendance();
    else if (command.find(L"import") != std::wstring::npos || command.find(L"\u5bfc\u5165") != std::wstring::npos || command.find(L"匯入") != std::wstring::npos) ImportAttendance();
    else if (command.find(L"export") != std::wstring::npos || command.find(L"\u5bfc\u51fa") != std::wstring::npos || command.find(L"匯出") != std::wstring::npos) ExportCsv();
    else if (command.find(L"backup") != std::wstring::npos || command.find(L"\u5907\u4efd") != std::wstring::npos || command.find(L"備份") != std::wstring::npos) BackupNow();
    else if (command.find(L"stats") != std::wstring::npos || command.find(L"\u7edf\u8ba1") != std::wstring::npos || command.find(L"統計") != std::wstring::npos) ShowStatisticsSummary();
    else if (command.find(L"risk") != std::wstring::npos || command.find(L"\u98ce\u9669") != std::wstring::npos || command.find(L"風險") != std::wstring::npos) ShowRiskStudents();
    else if (command.find(L"filter") != std::wstring::npos || command.find(L"\u7b5b\u9009") != std::wstring::npos || command.find(L"篩選") != std::wstring::npos) ShowAdvancedFilterHelp();
    else if (command.find(L"rollcall") != std::wstring::npos || command.find(L"\u70b9\u540d") != std::wstring::npos || command.find(L"點名") != std::wstring::npos) ShowQuickRollCall();
    else if (command.find(L"review") != std::wstring::npos || command.find(L"\u68c0\u67e5") != std::wstring::npos || command.find(L"檢查") != std::wstring::npos) ReviewLessonCompletion();
    else if (command.find(L"lesson") != std::wstring::npos || command.find(L"\u8bfe\u6b21") != std::wstring::npos || command.find(L"課堂") != std::wstring::npos) CreateTodayLessonFromRoster();
    else if (command.find(L"profile") != std::wstring::npos || command.find(L"\u6863\u6848") != std::wstring::npos || command.find(L"檔案") != std::wstring::npos) ShowStudentProfile();
    else if (command.find(L"settings") != std::wstring::npos || command.find(L"\u8bbe\u7f6e") != std::wstring::npos || command.find(L"設定") != std::wstring::npos) ShowSettingsWindow();
}

void ShowShortcuts(){
    std::wstring text =
        Settings().shortcutFullscreen + L": " + Tr(L"Fullscreen", L"\u5168\u5c4f") + L"\n" +
        Settings().shortcutSave + L": " + Tr(L"Save .attd", L"\u4fdd\u5b58 .attd") + L"\n" +
        Settings().shortcutImport + L": " + Tr(L"Import .attd", L"\u5bfc\u5165 .attd") + L"\n" +
        Settings().shortcutUndo + L": " + Tr(L"Undo", L"\u64a4\u9500") + L"\n" +
        Settings().shortcutRedo + L": " + Tr(L"Redo", L"\u91cd\u505a") + L"\n" +
        Settings().shortcutCommand + L": " + Tr(L"Command palette", L"\u547d\u4ee4\u9762\u677f") + L"\n" +
        Tr(L"Double-click row: Edit selected record", L"\u53cc\u51fb\u884c\uff1a\u7f16\u8f91\u9009\u4e2d\u8bb0\u5f55") + L"\n" +
        Tr(L"Ctrl/Shift click: Multi-select rows", L"Ctrl/Shift \u70b9\u51fb\uff1a\u591a\u9009\u884c");
    ShowMessage(text, Tr(L"Keyboard Shortcuts", L"\u5feb\u6377\u952e"));
}

void ShowToolsMenu(HWND button){
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
    std::wstring statsRange = Tr(L"Statistics range", L"\u7edf\u8ba1\u8303\u56f4") + L": " + StatsRangeName(Document().statsRange);
    std::wstring riskStudents = Tr(L"Risk students", L"\u98ce\u9669\u5b66\u751f");
    std::wstring backupManager = Tr(L"Backup manager", L"\u5907\u4efd\u7ba1\u7406\u4e2d\u5fc3");
    std::wstring reportTemplate = Tr(L"Report template", L"\u62a5\u544a\u6a21\u677f") + L": " + ReportTemplateName(Settings().reportTemplate);
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

} // namespace attendance
