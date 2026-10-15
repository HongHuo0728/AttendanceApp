#pragma once
#include "core/types.h"

namespace attendance {

using ReportLabels = std::array<std::wstring, 4>;

inline const std::wstring& ReportStatus(const std::wstring& status, const ReportLabels& labels) {
    if (status == L"Present") return labels[0];
    if (status == L"Absent") return labels[1];
    if (status == L"Late") return labels[2];
    if (status == L"Other") return labels[3];
    return status;
}

} // namespace attendance
