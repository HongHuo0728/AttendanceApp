#include "storage/backup_policy.h"
#include "ui/choice.h"
#include "app/recovery.h"
#include "core/text.h"
#include "ui/dialogs.h"
#include "localization/strings.h"
#include "storage/files.h"
#include "core/encoding.h"
#include "storage/workbook_codec.h"
#include "model/workbook.h"
#include "app/validation.h"
#include "settings/preferences.h"
#include "ui/file_dialogs.h"
#include "app/documents.h"
#include "model/state.h"
#include "settings/state.h"
#include "core/constants.h"

namespace attendance {

std::filesystem::path BackupDirectoryPath();
std::wstring TimestampForFileName();
std::vector<std::filesystem::path> ListBackupFiles();
std::filesystem::path RecentFileRecordPath();
std::wstring LoadRecentFilePath();
std::filesystem::path LatestBackupPath();
bool PreserveIgnoredAutosave();

std::filesystem::path BackupDirectoryPath(){
    auto marker = AppDataFilePath(L"backups\\marker.tmp");
    return marker.empty() ? std::filesystem::path{} : marker.parent_path();
}

std::wstring TimestampForFileName(){
    SYSTEMTIME st{};
    GetLocalTime(&st);
    wchar_t buffer[32]{};
    swprintf_s(buffer, L"%04d%02d%02d-%02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    static std::atomic_uint64_t sequence{0};
    return std::wstring(buffer)+L"-"+std::to_wstring(st.wMilliseconds)+L"-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(sequence++);
}

std::vector<std::filesystem::path> ListBackupFiles(){
    std::vector<std::filesystem::path> files;
    auto dir = BackupDirectoryPath();
    if (!FileExistsNoThrow(dir)) return files;
    std::error_code ec;
    std::filesystem::directory_iterator iterator(dir, ec);
    const std::filesystem::directory_iterator end;
    while (!ec && iterator != end) {
        std::error_code entryEc;
        if (iterator->is_regular_file(entryEc) && !entryEc && iterator->path().extension() == L".attd") {
            files.push_back(iterator->path());
        }
        iterator.increment(ec);
    }
    std::sort(files.begin(),files.end(),[](const auto& a,const auto& b){std::error_code ea,eb;auto ta=std::filesystem::last_write_time(a,ea),tb=std::filesystem::last_write_time(b,eb);return !ea&&!eb?ta>tb:a>b;});
    return files;
}

void ShowBackupManager(){
    auto files = ListBackupFiles();
    if(files.empty()){ShowMessage(Tr(L"No backup file was found.",L"未找到备份文件。"));return;}
    std::vector<std::wstring> names;for(const auto& file:files)names.push_back(file.filename().wstring());
    int index=ChooseItem(Tr(L"Backup manager",L"备份管理器"),names);if(index<0)return;
    if(!ConfirmDiscardUnsaved(Tr(L"Restore backup",L"恢复备份")))return;
    LoadAttendanceFile(files[index].wstring(),true,AttendanceLoadIntent::RestoreBackup);
}

std::filesystem::path RecentFileRecordPath(){
    return AppDataFilePath(L"recent.txt");
}

void SaveRecentFilePath(const std::wstring& path){
    auto recent = RecentFileRecordPath();
    if (recent.empty()) return;
    WriteBinaryFileAtomically(recent, WideToUtf8(path));
}

std::wstring LoadRecentFilePath(){
    auto recent = RecentFileRecordPath();
    if (!FileExistsNoThrow(recent)) return {};
    std::string value;
    std::wstring path;
    if (!ReadBinaryFileLimited(recent, value, MAX_RECENT_FILE_BYTES)
        || !DecodeUtf8Field(value, MAX_RECENT_FILE_BYTES, path)) {
        return {};
    }
    return path;
}

std::filesystem::path LatestBackupPath(){
    return AppDataFilePath(L"backup-latest.attd");
}

bool ClearAutosaveFile(){
    if (!PreserveIgnoredAutosave()) return false;
    auto path = AppDataFilePath(L"autosave.attd");
    if (path.empty() || !FileExistsNoThrow(path)) return true;
    std::error_code ec;
    bool removed = std::filesystem::remove(path, ec);
    return removed && !ec;
}

bool ArchiveExistingAutosaveForReplacement(){
    auto original = AppDataFilePath(L"autosave.attd");
    if (original.empty()) return true;
    if (!FileExistsNoThrow(original)) return true;
    auto directory = BackupDirectoryPath();
    if (directory.empty()) return false;
    auto protectedPath = directory /
        (L"autosave-protected-" + TimestampForFileName() + L"-"
            + std::to_wstring(GetTickCount64()) + L".attd");
    std::error_code ec;auto size=std::filesystem::file_size(original,ec);
    if(ec||!ReserveBackupSpace(directory,size,false))return false;
    return MoveFileExW(original.c_str(), protectedPath.c_str(), MOVEFILE_WRITE_THROUGH) != FALSE;
}

void BackupNow(){
    SyncActiveSheet();
    WorkbookValidationIssue issue = ValidateWorkbookModel(Document().sheets);
    if (issue != WorkbookValidationIssue::None) {
        ShowWorkbookValidationError(issue);
        return;
    }
    auto path = LatestBackupPath();
    if (path.empty()) return;
    std::string payload;
    if (!EncodeCurrentWorkbook(payload)) return;
    auto capacityDir=BackupDirectoryPath();
    if(capacityDir.empty()||!ReserveBackupSpace(capacityDir,payload.size(),true)){ShowMessage(Tr(L"Backup storage is full. Export or clean ordinary backups; the existing recovery file is preserved.",L"备份空间已满，请导出或清理普通备份；现有恢复文件已保留。"));return;}
    if (!WriteBinaryFileAtomically(path, payload)) {
        ShowMessage(Tr(L"Could not save the file.", L"\u65e0\u6cd5\u4fdd\u5b58\u6587\u4ef6\u3002"));
        return;
    }

    auto backupDir = BackupDirectoryPath();
    std::filesystem::path timestamped;
    if (!backupDir.empty()) {
        timestamped = backupDir / (L"backup-" + TimestampForFileName() + L".attd");
        if (!WriteBinaryFileAtomically(timestamped, payload)) timestamped.clear();
    }
    std::wstring message = Tr(L"Backup created:", L"\u5907\u4efd\u5df2\u521b\u5efa\uff1a") + L"\n" + path.wstring();
    if (!timestamped.empty()) message += L"\n" + timestamped.wstring();
    ShowMessage(message);
}

void RestoreLatestBackup(){
    auto path = LatestBackupPath();
    if (!FileExistsNoThrow(path)) {
        ShowMessage(Tr(L"No backup file was found.", L"\u672a\u627e\u5230\u5907\u4efd\u6587\u4ef6\u3002"));
        return;
    }
    if (!ConfirmDiscardUnsaved(Tr(L"Restore latest backup", L"\u6062\u590d\u6700\u65b0\u5907\u4efd"))) return;
    LoadAttendanceFile(path.wstring(), true, AttendanceLoadIntent::RestoreBackup);
}

void OpenRecentFile(){
    std::wstring path = LoadRecentFilePath();
    if (!FileExistsNoThrow(path)) {
        ShowMessage(Tr(L"No recent file was found.", L"\u672a\u627e\u5230\u6700\u8fd1\u6587\u4ef6\u3002"));
        return;
    }
    if (!ConfirmDiscardUnsaved(Tr(L"Open recent file", L"\u6253\u5f00\u6700\u8fd1\u6587\u4ef6"))) return;
    LoadAttendanceFile(path, true);
}

void SetDefaultSaveFolder(){
    std::wstring folder = ChooseFolderDialog();
    if (folder.empty()) return;
    Settings().defaultSaveDir = folder;
    if (!SaveSettingsWithFeedback()) return;
    ShowMessage(Tr(L"Default save folder updated:", L"\u9ed8\u8ba4\u4fdd\u5b58\u6587\u4ef6\u5939\u5df2\u66f4\u65b0\uff1a") + L"\n" + folder);
}

bool PreserveIgnoredAutosave(){
    if (Document().allowAutosaveOverwrite) return true;
    auto original = AppDataFilePath(L"autosave.attd");
    if (original.empty()) return false;
    if (!FileExistsNoThrow(original)) {
        Document().allowAutosaveOverwrite = true;
        return true;
    }
    auto directory = BackupDirectoryPath();
    if (directory.empty()) return false;
    auto protectedPath = directory /
        (L"autosave-protected-" + TimestampForFileName() + L"-"
            + std::to_wstring(GetTickCount64()) + L".attd");
    std::error_code ec;auto size=std::filesystem::file_size(original,ec);
    if(ec||!ReserveBackupSpace(directory,size,false)){ShowMessage(Tr(L"Backup storage is full. Export or clean ordinary backups; the existing recovery file is preserved.",L"备份空间已满，请导出或清理普通备份；现有恢复文件已保留。"));return false;}
    if (!CopyFileW(original.c_str(), protectedPath.c_str(), TRUE)) return false;
    Document().allowAutosaveOverwrite = true;
    return true;
}

AutosaveResult AutoSaveNow(){
    if (!Document().dirty || Document().autosavedRevision == Document().revision) return AutosaveResult::Skipped;
    if (!PreserveIgnoredAutosave()) {
        Document().autosaveFailurePending = true;
        return AutosaveResult::Failed;
    }
    SyncActiveSheet();
    auto path = AppDataFilePath(L"autosave.attd");
    std::string payload;
    if (path.empty() || !EncodeCurrentWorkbook(payload)
        || !WriteBinaryFileAtomically(path, payload)) {
        Document().autosaveFailurePending = true;
        return AutosaveResult::Failed;
    }
    Document().autosaveFailurePending = false;
    Document().autosavedRevision = Document().revision;
    return AutosaveResult::Saved;
}

void OpenAutosave(){
    auto path = AppDataFilePath(L"autosave.attd");
    if (!FileExistsNoThrow(path)) {
        ShowMessage(Tr(L"No autosave file was found.", L"\u672a\u627e\u5230\u81ea\u52a8\u4fdd\u5b58\u6587\u4ef6\u3002"));
        return;
    }
    if (!ConfirmDiscardUnsaved(Tr(L"Open autosave", L"\u6253\u5f00\u81ea\u52a8\u4fdd\u5b58"))) return;
    LoadAttendanceFile(path.wstring(), true, AttendanceLoadIntent::RestoreAutosave);
}

void PromptRestoreAutosave(){
    auto path = AppDataFilePath(L"autosave.attd");
    if (!FileExistsNoThrow(path)) return;
    Document().allowAutosaveOverwrite = false;
    if (!Settings().autosavePromptEnabled) return;
    std::wstring choice = L"1";
    std::wstring restoreTitle = Tr(L"Restore Autosave", L"\u6062\u590d\u81ea\u52a8\u4fdd\u5b58");
    std::wstring restoreMsg =
        Tr(L"An autosaved attendance file was found.", L"\u627e\u5230\u81ea\u52a8\u4fdd\u5b58\u7684\u70b9\u540d\u6587\u4ef6\u3002") + L"\n\n"
        L"1. " + Tr(L"Restore", L"\u6062\u590d") + L"\n"
        L"2. " + Tr(L"View file location", L"\u67e5\u770b\u6587\u4ef6\u4f4d\u7f6e") + L"\n"
        L"3. " + Tr(L"Ignore this time", L"\u672c\u6b21\u5ffd\u7565") + L"\n"
        L"4. " + Tr(L"Delete autosave", L"\u5220\u9664\u81ea\u52a8\u4fdd\u5b58") + L"\n\n"
        + Tr(L"Choose an option:", L"\u9009\u62e9\u64cd\u4f5c\uff1a");
    if (!PromptText(restoreTitle, restoreMsg, choice)) {
        Document().allowAutosaveOverwrite = false;
        return;
    }
    choice = TrimWide(choice);
    if (choice == L"1") {
        if (!LoadAttendanceFile(path.wstring(), false, AttendanceLoadIntent::RestoreAutosave)) {
            Document().allowAutosaveOverwrite = false;
        }
    } else if (choice == L"2") {
        ShowMessage(path.wstring(), Tr(L"Autosave file", L"\u81ea\u52a8\u4fdd\u5b58\u6587\u4ef6"));
        Document().allowAutosaveOverwrite = false;
    } else if (choice == L"4") {
        std::error_code ec;
        bool removed = std::filesystem::remove(path, ec);
        if (ec || !removed) {
            ShowMessage(Tr(L"Could not delete the autosave file.", L"无法删除自动保存文件。"));
            Document().allowAutosaveOverwrite = false;
        } else {
            Document().allowAutosaveOverwrite = true;
        }
    } else {
        Document().allowAutosaveOverwrite = false;
    }
}

} // namespace attendance
