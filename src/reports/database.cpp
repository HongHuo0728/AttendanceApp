#include "ui/feedback.h"
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
    auto operation=BeginOperation(nullptr,L"Database");
    auto complete = [operation,path](bool exported) {
        FinishOperation(operation,exported?OperationPhase::Success:OperationPhase::Failure,
            exported?Tr(L"Completed successfully.",L"操作已完成。"):Tr(L"Operation failed. Existing files were preserved.",L"操作失败，已保留原文件。"));
    };
    if (!QueuePreparedReport(Ui().hwnd, [path] { return PrepareDatabaseExport(path); }, complete)) complete(false);
}

} // namespace attendance
