#pragma once
#include "core/platform.h"

namespace attendance {

bool CanQueueReport();
bool QueueReport(HWND owner, std::function<bool()> work, std::function<void(bool)> completion);
bool QueuePreparedReport(HWND owner, const std::function<std::function<bool()>()>& prepare,
    std::function<void(bool)> completion);
void DispatchReportCompletions();
void StopReportJobs();

} // namespace attendance
