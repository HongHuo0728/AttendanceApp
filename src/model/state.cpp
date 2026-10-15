#include "model/state.h"

namespace attendance {

std::vector<AttendanceRecord>& DocumentState::Records() {
    if (sheets.empty()) {
        AttendanceSheet sheet;
        sheet.name = L"Default Course";
        sheets.push_back(std::move(sheet));
    }
    activeSheet = std::clamp(activeSheet, 0, static_cast<int>(sheets.size()) - 1);
    return sheets[activeSheet].records;
}

DocumentState& Document() {
    static DocumentState state;
    return state;
}

} // namespace attendance
