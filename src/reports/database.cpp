#include "reports/database.h"
#include "localization/strings.h"
#include "storage/files.h"
#include "model/workbook.h"
#include "ui/dialogs.h"
#include "core/csv.h"
#include "model/state.h"
#include "reports/jobs.h"
#include "ui/state.h"

namespace attendance {

std::function<bool()> PrepareDatabaseExport(const std::filesystem::path& path) {
    SyncActiveSheet();
    return [path, sheets = Document().sheets] {
        return WriteStreamFileAtomically(path, [&](std::ofstream& file) {
            file << "\xEF\xBB\xBF";
            file << "course\tteacher\tlocation\tstudent_count\tdate_time\tname\tstatus\tother\n";
            for (const auto& sheet : sheets) {
                const std::string prefix = TsvCell(sheet.name) + '\t' + TsvCell(sheet.teacher) + '\t'
                    + TsvCell(sheet.location) + '\t' + std::to_string(sheet.students.size()) + '\t';
                for (const auto& record : sheet.records) {
                    file << prefix << TsvCell(record.dateTime) << '\t' << TsvCell(record.name) << '\t'
                         << TsvCell(record.status) << '\t' << TsvCell(record.other) << '\n';
                    if (!file) return false;
                }
            }
            return file.good();
        });
    };
}

void ExportDatabaseMirror() {
    auto path = AppDataFilePath(L"attendance.attddb");
    auto complete = [path](bool exported) {
        if (!exported) {
            ShowMessage(Tr(L"Could not write the database file.", L"\u65e0\u6cd5\u5199\u5165\u6570\u636e\u5e93\u6587\u4ef6\u3002"));
            return;
        }
        ShowMessage(Tr(L"Database mirror exported to:", L"\u6570\u636e\u5e93\u955c\u50cf\u5df2\u5bfc\u51fa\u5230\uff1a") + L"\n" + path.wstring());
    };
    if (!QueuePreparedReport(Ui().hwnd, [path] { return PrepareDatabaseExport(path); }, complete)) complete(false);
}

} // namespace attendance
