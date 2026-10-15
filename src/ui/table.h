#pragma once

#include "core/types.h"

namespace attendance {

LRESULT HandleListCustomDraw(LPARAM lParam);
void UpdateStats();
void RefreshList();
void RefreshCourseCombo();
void ApplyFilterText(const std::wstring& filter);
void InitListColumns();

} // namespace attendance
