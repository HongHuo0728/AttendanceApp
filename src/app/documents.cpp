#include <memory>
#include "reports/jobs.h"
#include "ui/feedback.h"
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



namespace {
std::vector<HWND> importDisabledControls;
void SetImportInputState(bool loading){
    Document().loading=loading;
    if(loading && Ui().hwnd){
        EnumChildWindows(Ui().hwnd,[](HWND child,LPARAM)->BOOL{
            int id=GetDlgCtrlID(child);wchar_t type[32]{};GetClassNameW(child,type,32);
            if(id!=IDC_SETTINGS && id!=IDC_LIST && lstrcmpiW(type,L"Static")!=0 && IsWindowEnabled(child)){
                importDisabledControls.push_back(child);EnableWindow(child,FALSE);
            }
            return TRUE;
        },0);
    }else{
        for(auto child:importDisabledControls)if(IsWindow(child))EnableWindow(child,TRUE);
        importDisabledControls.clear();
    }
}
}

bool FinishSavedSnapshot(const std::wstring& path,uint64_t generation,uint64_t revision){
    if(Document().generation!=generation)return false;
    Document().currentFile=path;SaveRecentFilePath(path);
    if(Document().revision!=revision)return true;
    bool autosaveCleared=ClearAutosaveFile();
    Document().dirty=false;
    if(autosaveCleared)Document().allowAutosaveOverwrite=true;
    return autosaveCleared;
}
void SaveAttendance(){
    if(Document().saving||Document().loading)return;
    std::wstring path=SaveFileDialog();if(path.empty())return;
    SyncActiveSheet();
    WorkbookValidationIssue issue=ValidateWorkbookModel(Document().sheets);
    if(issue!=WorkbookValidationIssue::None){ShowWorkbookValidationError(issue);return;}
    auto sheets=std::make_shared<const std::vector<AttendanceSheet>>(Document().sheets);
    int active=Document().activeSheet;
    uint64_t generation=Document().generation,revision=Document().revision;
    auto operation=BeginOperation(GetDlgItem(Ui().hwnd,IDC_SAVE),Tr(L"Save .attd",L"保存 .attd"));
    Document().saving=true;
    auto complete=[operation,path,generation,revision](bool saved){
        Document().saving=false;
        if(!saved){FinishOperation(operation,OperationPhase::Failure,Tr(L"Could not save the file.",L"无法保存文件。"));return;}
        bool clean=FinishSavedSnapshot(path,generation,revision);
        FinishOperation(operation,clean?OperationPhase::Success:OperationPhase::Warning,
            clean?Tr(L"Saved successfully.",L"保存成功。"):Tr(L"File saved; recovery cleanup needs attention.",L"文件已保存，请检查恢复文件清理。"));
    };
    if(!QueueReport(Ui().hwnd,[path,sheets,active]{
        std::string payload;
        return EncodeWorkbookData(*sheets,active,payload) && EnsureMigrationBackup(path) && WriteBinaryFileAtomically(path,payload);
    },complete))complete(false);
}

bool CommitLoadedWorkbook(const std::wstring& path,std::vector<AttendanceSheet> imported,int active,
    bool showSuccess,AttendanceLoadIntent intent,bool protectedAlready);

bool LoadAttendanceFile(const std::wstring& path, bool showSuccess, AttendanceLoadIntent intent){
    if(Document().saving||Document().loading)return false;
    std::string fileText;
    if (!ReadBinaryFileLimited(std::filesystem::path(path), fileText, MAX_ATTD_FILE_BYTES)) {
        ShowMessage(Tr(L"Could not open the file.", L"\u65e0\u6cd5\u6253\u5f00\u6587\u4ef6\u3002"));
        return false;
    }

    std::string plainText;
    std::vector<AttendanceSheet> imported;int active=0;
    if (!DecodeAttd(fileText, plainText) || !DeserializeWorkbookData(plainText, imported, active)) {
        ShowMessage(Tr(L"This .attd file could not be decoded.", L"\u65e0\u6cd5\u89e3\u7801\u8be5 .attd \u6587\u4ef6\u3002"));
        return false;
    }

    return CommitLoadedWorkbook(path,std::move(imported),active,showSuccess,intent,false);
}

bool CommitLoadedWorkbook(const std::wstring& path,std::vector<AttendanceSheet> imported,int active,
    bool showSuccess,AttendanceLoadIntent intent,bool protectedAlready){
    if ((intent == AttendanceLoadIntent::OpenFile || intent == AttendanceLoadIntent::CommandLine)
        && !protectedAlready && !ArchiveExistingAutosaveForReplacement()) {
        ShowMessage(Tr(
            L"Could not protect the existing autosave. The current data was not replaced.",
            L"无法保护现有自动保存文件，当前数据未被替换。"));
        return false;
    }

    ++Document().generation;
    Document().loadedActiveSheet=active;
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
    (void)showSuccess;
    return true;
}

bool BeginLoadAttendanceFile(const std::wstring& path,AttendanceLoadIntent intent){
    if(Document().loading||Document().saving)return false;
    struct Decoded {std::vector<AttendanceSheet> sheets;int active=0;};
    auto decoded=std::make_shared<Decoded>();uint64_t generation=Document().generation,revision=Document().revision;
    auto operation=BeginOperation(GetDlgItem(Ui().hwnd,IDC_IMPORT),Tr(L"Import .attd",L"导入 .attd"));
    SetImportInputState(true);
    auto complete=[decoded,path,intent,generation,revision,operation](bool success){
        SetImportInputState(false);
        if(Document().generation!=generation||Document().revision!=revision)success=false;
        if(success)success=CommitLoadedWorkbook(path,std::move(decoded->sheets),decoded->active,false,intent,true);
        FinishOperation(operation,success?OperationPhase::Success:OperationPhase::Failure,
            success?Tr(L"Imported successfully.",L"导入成功。"):Tr(L"Import failed. Current attendance data was kept.",L"导入失败，已保留当前考勤数据。"));
    };
    if(!QueueReport(Ui().hwnd,[decoded,path,intent]{
        std::string file,plain;
        if(!ReadBinaryFileLimited(path,file,MAX_ATTD_FILE_BYTES)||!DecodeAttd(file,plain)||
            !DeserializeWorkbookData(plain,decoded->sheets,decoded->active))return false;
        return (intent!=AttendanceLoadIntent::OpenFile && intent!=AttendanceLoadIntent::CommandLine)||ArchiveExistingAutosaveForReplacement();
    },complete)){complete(false);return false;}
    return true;
}

void ImportAttendance(){
    if(Document().loading||Document().saving)return;
    if(Ui().quickRollCallWindow&&IsWindow(Ui().quickRollCallWindow)){
        SetForegroundWindow(Ui().quickRollCallWindow);return;
    }
    std::wstring path = OpenFileDialog();
    if (path.empty()) return;
    if (!ConfirmDiscardUnsaved(Tr(L"Import .attd", L"\u5bfc\u5165 .attd"))) return;
    BeginLoadAttendanceFile(path,AttendanceLoadIntent::OpenFile);
}


bool EnsureMigrationBackup(const std::filesystem::path& path){
 if(!FileExistsNoThrow(path))return true;
 std::string bytes,plain;std::vector<AttendanceSheet> parsed;
 if(!ReadBinaryFileLimited(path,bytes,MAX_ATTD_FILE_BYTES)||!DecodeAttd(bytes,plain))return false;
 if(plain.rfind("ATTENDANCE_V6\n",0)==0)return true;
 int active=0;if(!DeserializeWorkbookData(plain,parsed,active))return false;
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
