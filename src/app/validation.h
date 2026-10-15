#pragma once

#include "core/types.h"

namespace attendance {

void ShowWorkbookValidationError(WorkbookValidationIssue issue);
bool ValidateInputField(const std::wstring& value, size_t maximumBytes);
bool CanAppendSheets(size_t count);
bool CanAppendStudents(size_t count);
bool CanAppendRecords(size_t count);

} // namespace attendance
