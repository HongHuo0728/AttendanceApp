#pragma once

#include "core/types.h"

namespace attendance {

void ShowStatisticsSummary();
void CreateTodayLessonFromRoster();
void SwitchLessonFilter();
void ShowStudentProfile();
void ShowAdvancedFilterHelp();
void ShowStatsRangeMenu(HWND button);
void ShowRiskStudents();
void ReviewLessonCompletion();

} // namespace attendance
