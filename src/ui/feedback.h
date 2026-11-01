#pragma once
#include "core/types.h"

namespace attendance {
using OperationId = uint64_t;
enum class OperationPhase { Working, Success, Warning, Failure, Cancelled };
OperationId BeginOperation(HWND button, const std::wstring& label);
void FinishOperation(OperationId id, OperationPhase phase, const std::wstring& message);
bool OperationButtonBusy(HWND button);
bool DrawOperationButton(const DRAWITEMSTRUCT* draw);
OperationPhase OperationStatus(OperationId id);
void StopOperationFeedback();
} // namespace attendance
