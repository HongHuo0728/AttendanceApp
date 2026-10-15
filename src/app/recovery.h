#pragma once

#include "core/types.h"

namespace attendance {

void ShowBackupManager();
void SaveRecentFilePath(const std::wstring& path);
bool ClearAutosaveFile();
bool ArchiveExistingAutosaveForReplacement();
void BackupNow();
void RestoreLatestBackup();
void OpenRecentFile();
void SetDefaultSaveFolder();
AutosaveResult AutoSaveNow();
void OpenAutosave();
void PromptRestoreAutosave();

} // namespace attendance
