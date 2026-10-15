#pragma once

#include "core/types.h"

namespace attendance {

uint64_t AnimationNowMs();
uint64_t PerformanceNowUs();
void RecordMaximum(std::atomic_uint64_t& maximum, uint64_t value);
double EaseOut(double value);
double CubicBezierEase(double value, double x1, double y1, double x2, double y2);
double ElasticOut(double value);
COLORREF BlendColor(COLORREF from, COLORREF to, double amount);
void SignalAnimationScheduler();
void StopAnimationThread();
void StartAnimation(HWND hwnd, AnimChannel channel, double target, uint32_t durationMs = 160, bool closeOnDone = false);
void RestartAnimation(HWND hwnd, AnimChannel channel, double target, uint32_t durationMs = 220);
double GetAnimationValue(HWND hwnd, AnimChannel channel, double fallback = 0.0);
void RemoveAnimationState(HWND hwnd);
void AcknowledgeAnimationTick(HWND hwnd);
void CancelAnimationChannel(AnimChannel channel);
void StartWindowReveal(HWND hwnd, uint32_t durationMs = 180);
void BeginAnimatedClose(HWND hwnd, uint32_t durationMs = 140);

} // namespace attendance
