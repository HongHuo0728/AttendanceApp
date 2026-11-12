#pragma once

#include "core/types.h"

namespace attendance {

void ShowWorkbookValidationError(WorkbookValidationIssue issue);
bool ValidateInputField(const std::wstring& value, size_t maximumBytes);
bool ValidateWorkbookInput(const std::vector<AttendanceSheet>& sheets);
bool ValidateCourseNameInput(const std::wstring& name, int except = -1);
bool CanAppendSheets(size_t count);
bool CanAppendStudents(size_t count);
bool CanAppendRecords(size_t count);

} // namespace attendance
