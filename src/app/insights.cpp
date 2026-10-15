#include "app/insights.h"
#include "model/workbook.h"
#include "ui/common.h"
#include "core/text.h"
#include "model/query.h"
#include "ui/dialogs.h"
#include "localization/strings.h"
#include "app/validation.h"
#include "ui/table.h"
#include "ui/popup_menu.h"
#include "model/history.h"
#include "model/state.h"
#include "settings/state.h"
#include "ui/state.h"
#include "core/constants.h"

namespace attendance {



void ShowStatisticsSummary(){
    SyncActiveSheet();
    int present = 0;
    int absent = 0;
    int late = 0;
    int other = 0;
    CountStatuses(present, absent, late, other);
    int totalInRange = present + absent + late + other;
    std::unordered_map<std::wstring, int> issueCounts;
    for (const auto& record : Document().Records()) {
        if (!RecordMatchesStatsRange(record)) continue;
        if (record.status != L"Absent" && record.status != L"Late") continue;
        ++issueCounts[record.name];
    }
    std::vector<std::pair<std::wstring, int>> studentIssues(issueCounts.begin(), issueCounts.end());
    std::sort(studentIssues.begin(), studentIssues.end(), [](const auto& a, const auto& b) { return a.second > b.second; });

    std::wstringstream ss;
    ss << Tr(L"Statistics summary", L"\u7edf\u8ba1\u6458\u8981") << L"\n\n";
    ss << Tr(L"Courses", L"\u8bfe\u7a0b") << L": " << Document().sheets[Document().activeSheet].name << L"\n";
    ss << Tr(L"Range", L"\u8303\u56f4") << L": " << StatsRangeName(Document().statsRange) << L"\n";
    ss << Tr(L"Total", L"\u603b\u6570") << L": " << totalInRange
       << L"    " << Tr(L"Present", L"\u51fa\u5e2d") << L": " << present << L"\n";
    ss << Tr(L"Absent", L"\u7f3a\u5e2d") << L": " << absent << L"\n";
    ss << Tr(L"Late", L"\u8fdf\u5230") << L": " << late << L"\n";
    ss << Tr(L"Other", L"\u5176\u4ed6") << L": " << other << L"\n";
    if (!studentIssues.empty()) {
        ss << L"\n" << Tr(L"Top absent/late students", L"\u7f3a\u5e2d/\u8fdf\u5230\u6700\u591a\u5b66\u751f") << L":\n";
        for (int i = 0; i < (int)std::min<size_t>(5, studentIssues.size()); ++i) {
            ss << L"- " << studentIssues[i].first << L": " << studentIssues[i].second << L"\n";
        }
    }
    ShowMessage(ss.str(), Tr(L"Statistics summary", L"\u7edf\u8ba1\u6458\u8981"));
}

void CreateTodayLessonFromRoster(){
    SyncActiveSheet();
    const auto& students = Document().sheets[Document().activeSheet].students;
    if (students.empty()) {
        ShowMessage(Tr(L"No students in the roster.", L"\u540d\u5355\u4e2d\u6ca1\u6709\u5b66\u751f\u3002"));
        return;
    }
    std::wstring lesson = TodayDateKey() + L" " + Tr(L"Lesson", L"\u8bfe\u6b21");
    if (!PromptText(Tr(L"Create today's lesson", L"\u521b\u5efa\u4eca\u65e5\u8bfe\u6b21"), Tr(L"Lesson name:", L"\u8bfe\u6b21\u540d\u79f0\uff1a"), lesson)) return;
    lesson = TrimWide(lesson);
    if (lesson.empty()) return;
    if (!ValidateInputField(lesson, MAX_RECORD_FIELD_BYTES)) return;

    std::vector<AttendanceRecord> additions;
    std::unordered_set<std::wstring> recordedStudents;
    for (const auto& record : Document().Records()) {
        if (record.dateTime == lesson) recordedStudents.insert(record.name);
    }
    for (const auto& student : students) {
        if (recordedStudents.insert(student).second) {
            additions.push_back({lesson, student, L"Absent", L""});
        }
    }
    if (!additions.empty()) {
        if (!CanAppendRecords(additions.size())) return;
        PushUndo();
        MarkDirty();
        Document().Records().insert(Document().Records().end(), additions.begin(), additions.end());
        RefreshList();
    }
    SetText(Ui().dateEdit, lesson);
    ApplyFilterText(L"date:" + LowerText(lesson));
    std::wstringstream ss;
    ss << Tr(L"Lesson records created.", L"\u8bfe\u6b21\u8bb0\u5f55\u5df2\u521b\u5efa\u3002")
       << L" (" << additions.size() << L")";
    ShowMessage(ss.str(), Tr(L"Create today's lesson", L"\u521b\u5efa\u4eca\u65e5\u8bfe\u6b21"));
}

void SwitchLessonFilter(){
    auto lessons = CurrentLessonKeys();
    if (lessons.empty()) {
        ShowMessage(Tr(L"No lessons were found.", L"\u672a\u627e\u5230\u8bfe\u6b21\u3002"));
        return;
    }
    std::wstringstream list;
    for (int i = 0; i < (int)std::min<size_t>(lessons.size(), 20); ++i) {
        list << (i + 1) << L". " << lessons[i] << L"\n";
    }
    std::wstring choice;
    if (!PromptText(Tr(L"Switch lesson", L"\u5207\u6362\u8bfe\u6b21"), list.str() + L"\n" + Tr(L"Enter lesson number:", L"\u8f93\u5165\u8bfe\u6b21\u7f16\u53f7\uff1a"), choice)) return;
    int index = 0;
    try { index = std::stoi(choice) - 1; } catch (...) { return; }
    if (index < 0 || index >= (int)lessons.size()) return;
    SetText(Ui().dateEdit, lessons[index]);
    ApplyFilterText(L"date:" + LowerText(lessons[index]));
}

void ShowStudentProfile(){
    SyncActiveSheet();
    std::wstring name = GetText(Ui().nameEdit);
    int selected = VisibleToRecordIndex(ListView_GetNextItem(Ui().list, -1, LVNI_SELECTED));
    if (selected >= 0) name = Document().Records()[selected].name;
    if (!PromptText(Tr(L"Student profile", L"\u5b66\u751f\u6863\u6848"), Tr(L"Student name:", L"\u5b66\u751f\u59d3\u540d\uff1a"), name)) return;
    name = TrimWide(name);
    if (name.empty()) return;

    int total = 0, present = 0, absent = 0, late = 0, other = 0;
    std::vector<std::wstring> history;
    for (const auto& sheet : Document().sheets) {
        for (const auto& record : sheet.records) {
            if (!ContainsText(record.name, LowerText(name))) continue;
            ++total;
            if (record.status == L"Present") ++present;
            else if (record.status == L"Absent") ++absent;
            else if (record.status == L"Late") ++late;
            else ++other;
            if (history.size() < 12) {
                history.push_back(sheet.name + L" | " + record.dateTime + L" | " + Tr(record.status.c_str(), record.status.c_str()));
            }
        }
    }
    double rate = total ? present * 100.0 / total : 0.0;
    std::wstringstream ss;
    ss << Tr(L"Student profile", L"\u5b66\u751f\u6863\u6848") << L": " << name << L"\n\n"
       << Tr(L"Total", L"\u603b\u6570") << L": " << total << L"\n"
       << Tr(L"Present", L"\u51fa\u5e2d") << L": " << present << L"\n"
       << Tr(L"Absent", L"\u7f3a\u5e2d") << L": " << absent << L"\n"
       << Tr(L"Late", L"\u8fdf\u5230") << L": " << late << L"\n"
       << Tr(L"Other", L"\u5176\u4ed6") << L": " << other << L"\n"
       << Tr(L"Attendance", L"\u51fa\u52e4\u7387") << L": " << std::fixed << std::setprecision(1) << rate << L"%\n";
    if (!history.empty()) {
        ss << L"\n" << Tr(L"Recent history", L"\u6700\u8fd1\u8bb0\u5f55") << L":\n";
        for (const auto& line : history) ss << L"- " << line << L"\n";
    }
    ShowMessage(ss.str(), Tr(L"Student profile", L"\u5b66\u751f\u6863\u6848"));
}

void ShowAdvancedFilterHelp(){
    std::wstring filter = GetText(Ui().filterEdit);
    if (filter.empty()) filter = L"name: status: from:2026-07-01 to:2026-07-31";
    std::wstring help =
        Tr(L"Advanced filter examples:", L"\u9ad8\u7ea7\u7b5b\u9009\u793a\u4f8b\uff1a") + L"\n"
        L"name:Alice\nstatus:Absent\nfrom:2026-07-01 to:2026-07-31\nthisweek\nthismonth\ndate:2026-07-09\n\n"
        + Tr(L"Edit filter:", L"\u7f16\u8f91\u7b5b\u9009\uff1a");
    if (PromptText(Tr(L"Advanced filter", L"\u9ad8\u7ea7\u7b5b\u9009"), help, filter)) {
        ApplyFilterText(filter);
    }
}

void ShowStatsRangeMenu(HWND button){
    int command = ShowThemedPopupMenu(button, {
        {1, StatsRangeName(StatsRange::All), false},
        {2, StatsRangeName(StatsRange::ThisWeek), false},
        {3, StatsRangeName(StatsRange::ThisMonth), false}
    });
    if (command == 1) Document().statsRange = StatsRange::All;
    else if (command == 2) Document().statsRange = StatsRange::ThisWeek;
    else if (command == 3) Document().statsRange = StatsRange::ThisMonth;
    else return;
    RefreshList();
}

void ShowRiskStudents(){
    if (!Settings().riskAlertsEnabled) {
        ShowMessage(Tr(L"Risk student reminders are disabled in Settings.", L"\u98ce\u9669\u5b66\u751f\u63d0\u9192\u5df2\u5728\u8bbe\u7f6e\u4e2d\u5173\u95ed\u3002"));
        return;
    }
    std::map<std::wstring, std::pair<int, int>> stats;
    for (const auto& sheet : Document().sheets) {
        for (const auto& record : sheet.records) {
            auto& entry = stats[record.name];
            ++entry.first;
            if (record.status == L"Absent" || record.status == L"Late") ++entry.second;
        }
    }
    std::vector<std::pair<std::wstring, std::pair<int, int>>> risky;
    for (const auto& item : stats) {
        double issueRate = item.second.first ? item.second.second * 100.0 / item.second.first : 0.0;
        if (item.second.second >= 3 || issueRate >= 20.0) risky.push_back(item);
    }
    std::sort(risky.begin(), risky.end(), [](const auto& a, const auto& b) {
        return a.second.second == b.second.second ? a.first < b.first : a.second.second > b.second.second;
    });
    std::wstringstream ss;
    ss << Tr(L"Risk students", L"\u98ce\u9669\u5b66\u751f") << L"\n\n";
    if (risky.empty()) {
        ss << Tr(L"No risk students found.", L"\u672a\u53d1\u73b0\u98ce\u9669\u5b66\u751f\u3002");
    } else {
        for (int i = 0; i < (int)std::min<size_t>(risky.size(), 20); ++i) {
            double issueRate = risky[i].second.first ? risky[i].second.second * 100.0 / risky[i].second.first : 0.0;
            ss << L"- " << risky[i].first << L": " << risky[i].second.second << L"/" << risky[i].second.first
               << L" (" << std::fixed << std::setprecision(1) << issueRate << L"%)\n";
        }
    }
    ShowMessage(ss.str(), Tr(L"Risk students", L"\u98ce\u9669\u5b66\u751f"));
}

void ReviewLessonCompletion(){
    SyncActiveSheet();
    const std::wstring lesson = TrimWide(GetText(Ui().dateEdit));
    if (lesson.empty()) {
        ShowMessage(Tr(L"Enter a lesson name or date first.", L"请先输入课次名称或日期。"));
        return;
    }

    const auto& sheet = Document().sheets[Document().activeSheet];
    std::map<std::wstring, int> recordCounts;
    int recordsInLesson = 0;
    for (const auto& record : Document().Records()) {
        if (record.dateTime != lesson) continue;
        ++recordsInLesson;
        ++recordCounts[record.name];
    }

    std::vector<std::wstring> roster = sheet.students;
    if (roster.empty()) {
        for (const auto& item : recordCounts) roster.push_back(item.first);
    }
    std::vector<std::wstring> missing;
    for (const auto& student : roster) {
        if (recordCounts.find(student) == recordCounts.end()) missing.push_back(student);
    }
    std::vector<std::wstring> duplicates;
    for (const auto& item : recordCounts) {
        if (item.second > 1) duplicates.push_back(item.first);
    }

    std::wstringstream report;
    report << Tr(L"Lesson completion check", L"课次完成检查") << L"\n\n"
           << Tr(L"Lesson", L"课次") << L": " << lesson << L"\n"
           << Tr(L"Roster", L"名单人数") << L": " << roster.size() << L"\n"
           << Tr(L"Records", L"记录数") << L": " << recordsInLesson << L"\n";
    if (roster.empty()) {
        report << L"\n" << Tr(L"No roster or lesson records are available to check.", L"当前没有名单或课次记录可供检查。") << L"\n";
    } else if (missing.empty()) {
        report << L"\n" << Tr(L"Every roster student has a record.", L"名单中的每位学生都已有记录。") << L"\n";
    } else {
        report << L"\n" << Tr(L"Missing records", L"缺少记录") << L" (" << missing.size() << L"): ";
        for (size_t i = 0; i < missing.size(); ++i) {
            if (i) report << L", ";
            report << missing[i];
        }
        report << L"\n";
    }
    if (!duplicates.empty()) {
        report << L"\n" << Tr(L"Duplicate records", L"重复记录") << L" (" << duplicates.size() << L"): ";
        for (size_t i = 0; i < duplicates.size(); ++i) {
            if (i) report << L", ";
            report << duplicates[i];
        }
    }
    ShowMessage(report.str(), Tr(L"Lesson completion check", L"课次完成检查"));
}

} // namespace attendance
