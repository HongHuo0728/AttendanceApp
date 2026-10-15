#pragma once

#include "core/types.h"

namespace attendance {

void AddOrUpdateRecord(const std::wstring& status);
void LoadRecordIntoEditor(int index);
void UpdateSelectedRecord();
void MarkAllPresent();
void ShowDeleteMenu(HWND button);
void EraseRows(std::vector<int> rows);

} // namespace attendance
