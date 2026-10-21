#include "core/text.h"
#include "ui/state.h"
#include "app/documents.h"
#include "ui/dialogs.h"
#include "localization/strings.h"
#include "storage/files.h"
#include "storage/workbook_codec.h"
#include "model/workbook.h"
#include "app/validation.h"
#include "ui/table.h"
#include "ui/file_dialogs.h"
#include "app/recovery.h"
#include "model/state.h"
#include "core/constants.h"

namespace attendance {



void SaveAttendance(){
    std::wstring path = SaveFileDialog();
    if (path.empty()) return;

    SyncActiveSheet();
    WorkbookValidationIssue issue = ValidateWorkbookModel(Document().sheets);
    if (issue != WorkbookValidationIssue::None) {
        ShowWorkbookValidationError(issue);
        return;
    }
    std::string payload;
    if (!EncodeCurrentWorkbook(payload)
        || !EnsureMigrationBackup(std::filesystem::path(path))
        || !WriteBinaryFileAtomically(std::filesystem::path(path), payload)) {
        ShowMessage(Tr(L"Could not save the file.", L"\u65e0\u6cd5\u4fdd\u5b58\u6587\u4ef6\u3002"));
        return;
    }
    bool autosaveCleared = ClearAutosaveFile();
    Document().dirty = false;
    Document().currentFile=path;
    if (autosaveCleared) Document().allowAutosaveOverwrite = true;
    SaveRecentFilePath(path);
    std::wstring message = Tr(L"Saved successfully.", L"\u4fdd\u5b58\u6210\u529f\u3002");
    if (!autosaveCleared) {
        message += L"\n" + Tr(
            L"Could not delete the autosave file.", L"\u65e0\u6cd5\u5220\u9664\u81ea\u52a8\u4fdd\u5b58\u6587\u4ef6\u3002");
    }
    ShowMessage(message);
}

bool LoadAttendanceFile(const std::wstring& path, bool showSuccess, AttendanceLoadIntent intent){
    std::string fileText;
    if (!ReadBinaryFileLimited(std::filesystem::path(path), fileText, MAX_ATTD_FILE_BYTES)) {
        ShowMessage(Tr(L"Could not open the file.", L"\u65e0\u6cd5\u6253\u5f00\u6587\u4ef6\u3002"));
        return false;
    }

    std::string plainText;
    std::vector<AttendanceSheet> imported;
    if (!DecodeAttd(fileText, plainText) || !DeserializeWorkbook(plainText, imported)) {
        ShowMessage(Tr(L"This .attd file could not be decoded.", L"\u65e0\u6cd5\u89e3\u7801\u8be5 .attd \u6587\u4ef6\u3002"));
        return false;
    }

    if ((intent == AttendanceLoadIntent::OpenFile || intent == AttendanceLoadIntent::CommandLine)
        && !ArchiveExistingAutosaveForReplacement()) {
        ShowMessage(Tr(
            L"Could not protect the existing autosave. The current data was not replaced.",
            L"无法保护现有自动保存文件，当前数据未被替换。"));
        return false;
    }

    Document().sheets = std::move(imported);
    Document().currentFile=(intent==AttendanceLoadIntent::OpenFile||intent==AttendanceLoadIntent::CommandLine)?path:L"";
    Document().visibleRows.clear();
    Document().lessonFilter.clear();
    Document().activeSheet = std::clamp(Document().loadedActiveSheet, 0, (int)Document().sheets.size() - 1);
    Document().undoStack.clear();
    Document().redoStack.clear();
    const bool restored = intent == AttendanceLoadIntent::RestoreAutosave
        || intent == AttendanceLoadIntent::RestoreBackup;
    Document().dirty = restored;
    ++Document().revision;
    Document().autosavedRevision = UINT64_MAX;
    if (intent == AttendanceLoadIntent::RestoreBackup) {
        Document().allowAutosaveOverwrite = !FileExistsNoThrow(AppDataFilePath(L"autosave.attd"));
    } else {
        Document().allowAutosaveOverwrite = true;
    }
    if (intent == AttendanceLoadIntent::OpenFile || intent == AttendanceLoadIntent::CommandLine) {
        SaveRecentFilePath(path);
    }
    RefreshCourseCombo();
    RefreshList();
    if (showSuccess) ShowMessage(Tr(L"Imported successfully.", L"\u5bfc\u5165\u6210\u529f\u3002"));
    return true;
}

void ImportAttendance(){
    std::wstring path = OpenFileDialog();
    if (path.empty()) return;
    if (!ConfirmDiscardUnsaved(Tr(L"Import .attd", L"\u5bfc\u5165 .attd"))) return;
    LoadAttendanceFile(path, true);
}


bool EnsureMigrationBackup(const std::filesystem::path& path){
 if(!FileExistsNoThrow(path))return true;
 std::string bytes,plain;std::vector<AttendanceSheet> parsed;
 if(!ReadBinaryFileLimited(path,bytes,MAX_ATTD_FILE_BYTES)||!DecodeAttd(bytes,plain))return false;
 if(plain.rfind("ATTENDANCE_V6\n",0)==0)return true;
 if(!DeserializeWorkbook(plain,parsed))return false;
 auto backup=std::filesystem::path(path.wstring()+L".pre-v6.bak");
 std::error_code ec;if(std::filesystem::is_regular_file(backup,ec)&&!ec)return true;
 return CopyFileW(path.c_str(),backup.c_str(),TRUE)!=FALSE;
}
void ExportV5Compatible(){
 if(ThemedMessageBox(Ui().hwnd,Tr(L"V5 changes Leave and Pending to Other with notes. Older versions calculate rates differently. Export a separate copy?",L"V5 将请假和未点名转换为带说明的其他状态。旧版本统计方式不同。是否导出单独副本？"),Tr(L"Export V5 compatible file",L"导出 V5 兼容文件"),true)!=IDYES)return;
 auto path=SaveFileDialog();if(path.empty())return;
 std::error_code ec;bool same=!Document().currentFile.empty()&&std::filesystem::equivalent(std::filesystem::path(path),std::filesystem::path(Document().currentFile),ec);
 if(!same&&!Document().currentFile.empty())same=LowerText(std::filesystem::absolute(path).lexically_normal().wstring())==LowerText(std::filesystem::absolute(Document().currentFile).lexically_normal().wstring());
 std::string bytes,plain;
 if(same||(ReadBinaryFileLimited(path,bytes,MAX_ATTD_FILE_BYTES)&&DecodeAttd(bytes,plain)&&plain.rfind("ATTENDANCE_V6\n",0)==0)){
  ShowMessage(Tr(L"Choose a separate file; the active workbook and V6 files cannot be overwritten by V5 export.",L"请选择其他文件；V5 导出不能覆盖当前工作簿或 V6 文件。"));return;
 }
 std::string payload;bool success=EncodeV5Compatible(payload)&&WriteBinaryFileAtomically(path,payload);
 ShowMessage(success?Tr(L"Saved successfully.",L"保存成功。"):Tr(L"Could not save the file.",L"无法保存文件。"));
}
} // namespace attendance
