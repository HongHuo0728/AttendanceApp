#include "model/query.h"
#include "core/status.h"
#include "reports/csv.h"
#include "localization/strings.h"
#include "storage/files.h"
#include "ui/dialogs.h"
#include "ui/file_dialogs.h"
#include "core/csv.h"
#include "model/state.h"
#include "reports/jobs.h"
#include "reports/labels.h"
#include "ui/state.h"

namespace attendance {

std::function<bool()> PrepareCsvExport(const std::wstring& path) {
    const ReportLabels headers = {Tr(L"Date/Time", L"\u65e5\u671f\u65f6\u95f4"),
        Tr(L"Name", L"\u59d3\u540d"), Tr(L"Status", L"\u72b6\u6001"), Tr(L"Other", L"\u5176\u4ed6")};
    const ReportLabels statuses = {StatusDisplayText(L"Present"), StatusDisplayText(L"Absent"),
        StatusDisplayText(L"Late"), StatusDisplayText(L"Other"), StatusDisplayText(L"Leave"), StatusDisplayText(L"Pending")};
    auto records=Document().Records();
    records.erase(std::remove_if(records.begin(),records.end(),[](const auto& r){return !RecordMatchesStatsRange(r);}),records.end());
    return [path, headers, statuses, records = std::move(records)] {
        return WriteStreamFileAtomically(std::filesystem::path(path), [&](std::ofstream& file) {
            file << "\xEF\xBB\xBF";
            file << CsvCell(headers[0]) << ',' << CsvCell(headers[1]) << ','
                 << CsvCell(headers[2]) << ',' << CsvCell(headers[3]) << '\n';
            for (const auto& record : records) {
                file << CsvCell(record.dateTime) << ',' << CsvCell(record.name) << ','
                     << CsvCell(ReportStatus(record.status, statuses)) << ',' << CsvCell(record.other) << '\n';
                if (!file) return false;
            }
            return file.good();
        });
    };
}

void ExportCsv() {
    if (Document().Records().empty()) {
        ShowMessage(Tr(L"There are no records to export.", L"\u6ca1\u6709\u53ef\u5bfc\u51fa\u7684\u8bb0\u5f55\u3002"));
        return;
    }
    std::wstring path = SaveCsvFileDialog();
    if (path.empty()) return;
    auto complete = [](bool exported) {
        ShowMessage(exported
            ? Tr(L"CSV exported successfully.", L"CSV \u5bfc\u51fa\u6210\u529f\u3002")
            : Tr(L"Could not export the CSV file.", L"\u65e0\u6cd5\u5bfc\u51fa CSV \u6587\u4ef6\u3002"));
    };
    if (!QueuePreparedReport(Ui().hwnd, [path] { return PrepareCsvExport(path); }, complete)) complete(false);
}

} // namespace attendance
