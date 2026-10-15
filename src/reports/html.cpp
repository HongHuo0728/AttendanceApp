#include "reports/html.h"
#include "localization/strings.h"
#include "storage/files.h"
#include "model/workbook.h"
#include "ui/dialogs.h"
#include "ui/file_dialogs.h"
#include "core/encoding.h"
#include "model/state.h"
#include "settings/state.h"
#include "ui/state.h"
#include "reports/jobs.h"
#include "reports/labels.h"

namespace attendance {
namespace {

struct HtmlReport {
    AttendanceSheet sheet;
    ReportTemplate reportTemplate = ReportTemplate::Complete;
    ReportLabels columns, statuses;
    std::array<std::wstring, 8> titles;
};

bool WriteHtmlReport(const std::wstring& path, const HtmlReport& report) {
    return WriteStreamFileAtomically(std::filesystem::path(path), [&](std::ofstream& file) {
        const auto& sheet = report.sheet;
        const auto& title = report.titles;
        file << "\xEF\xBB\xBF";
        file << "<!doctype html><meta charset='utf-8'><title>" << HtmlCell(title[0]) << "</title>";
        file << "<style>body{font-family:Segoe UI,Arial;margin:32px}table{border-collapse:collapse;width:100%}"
                "th,td{border:1px solid #999;padding:8px;text-align:left}th{background:#eee}"
                "@media print{button{display:none}}</style>";
        file << "<button onclick='window.print()'>" << HtmlCell(title[1]) << "</button>";
        file << "<h1>" << HtmlCell(sheet.name) << "</h1>";
        file << "<p><strong>" << HtmlCell(title[2]) << ":</strong> " << HtmlCell(title[3]) << "</p>";
        bool showCourseDetails = report.reportTemplate == ReportTemplate::Teacher || report.reportTemplate == ReportTemplate::Complete;
        bool showRosterSummary = report.reportTemplate != ReportTemplate::Simple;
        bool showOtherColumn = report.reportTemplate != ReportTemplate::Parent && report.reportTemplate != ReportTemplate::Simple;
        if (showCourseDetails && (!sheet.teacher.empty() || !sheet.location.empty() || !sheet.notes.empty())) {
            file << "<p>";
            if (!sheet.teacher.empty()) file << "<strong>" << HtmlCell(title[4]) << "</strong> " << HtmlCell(sheet.teacher) << "<br>";
            if (!sheet.location.empty()) file << "<strong>" << HtmlCell(title[5]) << "</strong> " << HtmlCell(sheet.location) << "<br>";
            if (!sheet.notes.empty()) file << "<strong>" << HtmlCell(title[6]) << "</strong> " << HtmlCell(sheet.notes);
            file << "</p>";
        }
        if (showRosterSummary && !sheet.students.empty()) {
            file << "<p><strong>" << HtmlCell(title[7]) << ":</strong> " << sheet.students.size() << "</p>";
        }
        file << "<table><tr><th>" << HtmlCell(report.columns[0]) << "</th><th>"
             << HtmlCell(report.columns[1]) << "</th><th>" << HtmlCell(report.columns[2]) << "</th>";
        if (showOtherColumn) file << "<th>" << HtmlCell(report.columns[3]) << "</th>";
        file << "</tr>";
        for (const auto& record : sheet.records) {
            file << "<tr><td>" << HtmlCell(record.dateTime) << "</td><td>" << HtmlCell(record.name)
                 << "</td><td>" << HtmlCell(ReportStatus(record.status, report.statuses)) << "</td>";
            if (showOtherColumn) file << "<td>" << HtmlCell(record.other) << "</td>";
            file << "</tr>";
        }
        file << "</table><script>setTimeout(()=>window.print(),300)</script>";
        return file.good();
    });
}

} // namespace

std::function<bool()> PrepareHtmlExport(const std::wstring& path) {
    SyncActiveSheet();
    HtmlReport report;
    report.sheet = Document().sheets[Document().activeSheet];
    report.reportTemplate = Settings().reportTemplate;
    report.columns = {Tr(L"Date/Time", L"\u65e5\u671f\u65f6\u95f4"), Tr(L"Name", L"\u59d3\u540d"),
        Tr(L"Status", L"\u72b6\u6001"), Tr(L"Other", L"\u5176\u4ed6")};
    report.statuses = {StatusDisplayText(L"Present"), StatusDisplayText(L"Absent"),
        StatusDisplayText(L"Late"), StatusDisplayText(L"Other")};
    report.titles = {Tr(L"Attendance", L"\u51fa\u52e4\u7387"),
        Tr(L"Print / Save as PDF", L"\u6253\u5370 / \u53e6\u5b58\u4e3a PDF"),
        Tr(L"Report template", L"\u62a5\u544a\u6a21\u677f"), ReportTemplateName(report.reportTemplate),
        Tr(L"Teacher/owner:", L"\u6559\u5e08/\u8d1f\u8d23\u4eba\uff1a"), Tr(L"Location:", L"\u5730\u70b9\uff1a"),
        Tr(L"Course notes:", L"\u8bfe\u7a0b\u5907\u6ce8\uff1a"), Tr(L"Manage students", L"\u7ba1\u7406\u5b66\u751f")};
    return [path, report = std::move(report)] { return WriteHtmlReport(path, report); };
}

void ExportPrintHtml() {
    std::wstring path = SaveHtmlFileDialog();
    if (path.empty()) return;
    auto complete = [path](bool exported) {
        if (exported) ShellExecuteW(Ui().hwnd, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        else ShowMessage(Tr(L"Could not export the print file.", L"\u65e0\u6cd5\u5bfc\u51fa\u6253\u5370\u6587\u4ef6\u3002"));
    };
    if (!QueuePreparedReport(Ui().hwnd, [path] { return PrepareHtmlExport(path); }, complete)) {
        complete(false);
    }
}

} // namespace attendance
