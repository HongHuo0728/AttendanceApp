#pragma once

#include "core/types.h"

namespace attendance {

struct FontsState {
    std::vector<std::wstring> availableFonts;
    bool fontComboRefreshPending = false;
    bool fontEnumerationShutdown = false;
    bool fontCacheReady = false;
    uint64_t fontRequestedGeneration = 0;
    uint64_t fontPublishedGeneration = 0;
    uint64_t fontAppliedGeneration = UINT64_MAX;
    std::mutex fontEnumerationMutex;
    std::condition_variable fontEnumerationCondition;
    std::thread fontEnumerationThread;
};

FontsState& Fonts();

} // namespace attendance
