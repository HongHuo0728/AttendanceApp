#pragma once
#include "core/types.h"
namespace attendance {
enum class NoteMode { Preserve, Replace, Append };
std::vector<int> CurrentScopeRows();
bool ChangeRecords(std::vector<int> rows, const std::wstring &status,
                   const std::wstring &note, NoteMode mode);
bool MakeUpRecord(const std::wstring &lesson, const std::wstring &name,
                  const std::wstring &status, const std::wstring &reason);
bool RenameLesson(const std::wstring &source, const std::wstring &target);
bool CopyLesson(const std::wstring &source, const std::wstring &target);
bool CreateCourseFromTemplate(size_t source, const std::wstring &name);
} // namespace attendance
