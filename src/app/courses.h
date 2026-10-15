#pragma once

#include "core/types.h"

namespace attendance {

void SwitchCourse(int index);
void ShowCourseMenu(HWND button);
void ImportRosterCsv();
void AddStudentToRoster();
void RemoveStudentFromRoster();
void CreateRecordsFromRoster();
void EditCourseDetails();

} // namespace attendance
