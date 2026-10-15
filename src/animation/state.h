#pragma once

#include "core/types.h"

namespace attendance {

struct AnimationsState {
    std::atomic_uint64_t lastParticleSetupUs{0};
    std::atomic_uint64_t maxParticleFrameUs{0};
    std::mutex animMutex;
    std::map<HWND, std::map<int, AnimValue>> animations;
    std::set<HWND> pendingAnimationTicks;
    std::atomic_bool animationShutdown{false};
    std::thread animationThread;
    HANDLE animationWakeEvent = nullptr;
    std::condition_variable animationWakeCondition;
    std::mutex animationWakeMutex;
    std::atomic_uint64_t animationWakeGeneration{0};
    std::map<HWND, ButtonEffectState> buttonEffects;
    std::mt19937 effectRng{std::random_device{}()};
};

AnimationsState& Animations();

} // namespace attendance
