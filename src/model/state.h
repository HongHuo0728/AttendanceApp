#pragma once

#include "core/types.h"

namespace attendance {

struct DocumentState {
    std::vector<AttendanceSheet> sheets;
    std::vector<int> visibleRows;
    int activeSheet = 0;
    int loadedActiveSheet = 0;
    std::vector<std::string> undoStack;
    std::vector<std::string> redoStack;
    bool allowAutosaveOverwrite = true;
    bool autosaveFailurePending = false;
    bool dirty = false;
    std::wstring filterText;
    std::wstring lessonFilter;
    std::wstring currentFile;
    StatsRange statsRange = StatsRange::All;
    uint64_t revision = 0;
    uint64_t autosavedRevision = UINT64_MAX;

    std::vector<AttendanceRecord>& Records();
};

DocumentState& Document();

} // namespace attendance
