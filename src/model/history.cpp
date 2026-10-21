#include "model/history.h"
#include "model/workbook.h"
#include "localization/strings.h"
#include "storage/workbook_codec.h"
#include "ui/dialogs.h"
#include "ui/table.h"
#include "model/state.h"
#include "core/constants.h"

namespace attendance {

void TrimSnapshotHistory(std::vector<std::string>& history);
void RestoreSnapshot(const std::string& snapshot);

void TrimSnapshotHistory(std::vector<std::string>& history){
    size_t keepFrom = history.size();
    size_t totalBytes = 0;
    size_t keptCount = 0;
    for (size_t i = history.size(); i > 0; --i) {
        const size_t bytes = history[i - 1].size();
        if (keptCount >= 50
            || bytes > MAX_HISTORY_SNAPSHOT_BYTES - totalBytes) {
            break;
        }
        keepFrom = i - 1;
        ++keptCount;
        totalBytes += bytes;
    }
    if (keepFrom > 0) history.erase(history.begin(), history.begin() + static_cast<std::ptrdiff_t>(keepFrom));
}

void PushUndo(){
    SyncActiveSheet();
    std::string snapshot = SerializeWorkbook();
    if (snapshot.size() > MAX_HISTORY_SNAPSHOT_BYTES) return;
    Document().undoStack.push_back(std::move(snapshot));
    TrimSnapshotHistory(Document().undoStack);
    Document().redoStack.clear();
}

void RestoreSnapshot(const std::string& snapshot){
    std::vector<AttendanceSheet> sheets;
    if (!DeserializeWorkbook(snapshot, sheets)) return;
    Document().sheets = std::move(sheets);
    Document().visibleRows.clear();
    Document().lessonFilter.clear();
    Document().activeSheet = std::clamp(Document().loadedActiveSheet, 0, (int)Document().sheets.size() - 1);
    MarkDirty();
    RefreshCourseCombo();
    RefreshList();
}

void UndoLast(){
    if (Document().undoStack.empty()) {
        ShowMessage(Tr(L"Nothing to undo.", L"\u6ca1\u6709\u53ef\u64a4\u9500\u7684\u64cd\u4f5c\u3002"));
        return;
    }
    SyncActiveSheet();
    Document().redoStack.push_back(SerializeWorkbook());
    TrimSnapshotHistory(Document().redoStack);
    std::string snapshot = Document().undoStack.back();
    Document().undoStack.pop_back();
    RestoreSnapshot(snapshot);
}

void RedoLast(){
    if (Document().redoStack.empty()) {
        ShowMessage(Tr(L"Nothing to redo.", L"\u6ca1\u6709\u53ef\u91cd\u505a\u7684\u64cd\u4f5c\u3002"));
        return;
    }
    SyncActiveSheet();
    Document().undoStack.push_back(SerializeWorkbook());
    TrimSnapshotHistory(Document().undoStack);
    std::string snapshot = Document().redoStack.back();
    Document().redoStack.pop_back();
    RestoreSnapshot(snapshot);
}

} // namespace attendance
