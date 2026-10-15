#include "animation/scheduler.h"
#include "settings/state.h"
#include "animation/state.h"
#include "core/constants.h"

namespace attendance {

double EaseInOut(double value);
void EnsureAnimationThread();
uint32_t EffectiveAnimationDuration(AnimChannel channel, uint32_t durationMs);

uint64_t AnimationNowMs(){
    using namespace std::chrono;
    return (uint64_t)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

uint64_t PerformanceNowUs(){
    static LARGE_INTEGER frequency = [] {
        LARGE_INTEGER value{};
        QueryPerformanceFrequency(&value);
        return value;
    }();
    LARGE_INTEGER counter{};
    QueryPerformanceCounter(&counter);
    if (frequency.QuadPart <= 0) return 0;
    const uint64_t wholeSeconds = static_cast<uint64_t>(counter.QuadPart / frequency.QuadPart);
    const uint64_t remainder = static_cast<uint64_t>(counter.QuadPart % frequency.QuadPart);
    return wholeSeconds * 1000000ull
        + remainder * 1000000ull / static_cast<uint64_t>(frequency.QuadPart);
}

void RecordMaximum(std::atomic_uint64_t& maximum, uint64_t value){
    uint64_t current = maximum.load(std::memory_order_relaxed);
    while (current < value
        && !maximum.compare_exchange_weak(current, value, std::memory_order_relaxed)) {
    }
}

double EaseInOut(double value){
    value = std::clamp(value, 0.0, 1.0);
    return value < 0.5
        ? 4.0 * value * value * value
        : 1.0 - std::pow(-2.0 * value + 2.0, 3.0) / 2.0;
}

double EaseOut(double value){
    value = std::clamp(value, 0.0, 1.0);
    return 1.0 - std::pow(1.0 - value, 3.0);
}

double CubicBezierEase(double value, double x1, double y1, double x2, double y2){
    value = std::clamp(value, 0.0, 1.0);
    auto sample = [](double t, double p1, double p2) {
        double inverse = 1.0 - t;
        return 3.0 * inverse * inverse * t * p1
            + 3.0 * inverse * t * t * p2
            + t * t * t;
    };
    double low = 0.0;
    double high = 1.0;
    double t = value;
    for (int i = 0; i < 12; ++i) {
        t = (low + high) * 0.5;
        if (sample(t, x1, x2) < value) low = t;
        else high = t;
    }
    return sample(t, y1, y2);
}

double ElasticOut(double value){
    value = std::clamp(value, 0.0, 1.0);
    if (value <= 0.0 || value >= 1.0) return value;
    constexpr double c4 = (2.0 * 3.14159265358979323846) / 3.0;
    return std::pow(2.0, -10.0 * value) * std::sin((value * 10.0 - 0.75) * c4) + 1.0;
}

COLORREF BlendColor(COLORREF from, COLORREF to, double amount){
    amount = std::clamp(amount, 0.0, 1.0);
    auto mix = [&](int a, int b) {
        return (int)std::lround(a + (b - a) * amount);
    };
    return RGB(
        mix(GetRValue(from), GetRValue(to)),
        mix(GetGValue(from), GetGValue(to)),
        mix(GetBValue(from), GetBValue(to)));
}

void SignalAnimationScheduler(){
    Animations().animationWakeGeneration.fetch_add(1);
    if (Animations().animationWakeEvent) SetEvent(Animations().animationWakeEvent);
    Animations().animationWakeCondition.notify_all();
}

void EnsureAnimationThread(){
    if (Animations().animationThread.joinable()) return;
    Animations().animationShutdown = false;
    Animations().animationWakeEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    Animations().animationThread = std::thread([] {
        HANDLE timer = CreateWaitableTimerExW(
            nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
        if (!timer) timer = CreateWaitableTimerW(nullptr, FALSE, nullptr);

        using Clock = std::chrono::steady_clock;
        Clock::time_point nextDeadline{};
        Clock::time_point nextStandardDeadline{};
        int scheduledFps = 0;
        std::vector<HWND> tickWindows;
        std::vector<HWND> doneWindows;
        tickWindows.reserve(32);
        doneWindows.reserve(8);
        auto waitForFallbackWake = [&](Clock::time_point deadline) {
            uint64_t generation = Animations().animationWakeGeneration.load();
            std::unique_lock<std::mutex> lock(Animations().animationWakeMutex);
            if (deadline == Clock::time_point::max()) {
                Animations().animationWakeCondition.wait(lock, [&] {
                    return Animations().animationShutdown.load()
                        || Animations().animationWakeGeneration.load() != generation;
                });
            } else {
                Animations().animationWakeCondition.wait_until(lock, deadline, [&] {
                    return Animations().animationShutdown.load()
                        || Animations().animationWakeGeneration.load() != generation;
                });
            }
            return Animations().animationShutdown.load()
                || Animations().animationWakeGeneration.load() != generation;
        };

        while (!Animations().animationShutdown.load()) {
            bool hasActiveAnimation = false;
            bool hasSmoothScroll = false;
            {
                std::lock_guard<std::mutex> lock(Animations().animMutex);
                for (const auto& hwndEntry : Animations().animations) {
                    for (const auto& channelEntry : hwndEntry.second) {
                        if (!channelEntry.second.active) continue;
                        hasActiveAnimation = true;
                        if (channelEntry.first == (int)AnimChannel::SmoothScroll) {
                            hasSmoothScroll = true;
                        }
                    }
                }
            }

            if (!hasActiveAnimation) {
                scheduledFps = 0;
                nextDeadline = {};
                nextStandardDeadline = {};
                if (Animations().animationWakeEvent) {
                    WaitForSingleObject(Animations().animationWakeEvent, INFINITE);
                } else {
                    waitForFallbackWake(Clock::time_point::max());
                }
                continue;
            }

            int targetFps = hasSmoothScroll
                ? std::clamp(Settings().scrollAnimationFps.load(), 60, 360)
                : 60;
            auto frameDuration = std::chrono::duration_cast<Clock::duration>(
                std::chrono::duration<double>(1.0 / targetFps));
            auto nowClock = Clock::now();
            if (scheduledFps != targetFps || nextDeadline.time_since_epoch().count() == 0) {
                nextDeadline = nowClock + frameDuration;
                scheduledFps = targetFps;
            }

            bool wakeRequested = false;
            if (timer) {
                auto waitDuration = nextDeadline - Clock::now();
                LONGLONG ticks100ns = std::max<LONGLONG>(
                    1, std::chrono::duration_cast<std::chrono::nanoseconds>(waitDuration).count() / 100);
                LARGE_INTEGER due{};
                due.QuadPart = -ticks100ns;
                if (SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE)) {
                    if (Animations().animationWakeEvent) {
                        HANDLE handles[] = {Animations().animationWakeEvent, timer};
                        wakeRequested = WaitForMultipleObjects(2, handles, FALSE, INFINITE)
                            == WAIT_OBJECT_0;
                    } else {
                        WaitForSingleObject(timer, INFINITE);
                    }
                } else {
                    wakeRequested = waitForFallbackWake(nextDeadline);
                }
            } else {
                wakeRequested = waitForFallbackWake(nextDeadline);
            }

            if (Animations().animationShutdown.load()) break;
            if (wakeRequested) {
                if (Clock::now() < nextDeadline) continue;
            }

            tickWindows.clear();
            doneWindows.clear();
            uint64_t now = AnimationNowMs();
            auto frameClock = Clock::now();
            const auto standardFrameDuration = std::chrono::duration_cast<Clock::duration>(
                std::chrono::duration<double>(1.0 / 60.0));
            bool updateStandardChannels = !hasSmoothScroll
                || nextStandardDeadline.time_since_epoch().count() == 0
                || frameClock >= nextStandardDeadline;
            if (updateStandardChannels) {
                if (nextStandardDeadline.time_since_epoch().count() == 0) {
                    nextStandardDeadline = frameClock + standardFrameDuration;
                } else {
                    do {
                        nextStandardDeadline += standardFrameDuration;
                    } while (nextStandardDeadline <= frameClock);
                }
            }
            {
                std::lock_guard<std::mutex> lock(Animations().animMutex);
                for (auto& hwndEntry : Animations().animations) {
                    bool changed = false;
                    for (auto& channelEntry : hwndEntry.second) {
                        AnimValue& anim = channelEntry.second;
                        if (!anim.active) continue;
                        bool smoothChannel =
                            channelEntry.first == (int)AnimChannel::SmoothScroll;
                        if (!smoothChannel && !updateStandardChannels) continue;
                        double t = anim.durationMs == 0
                            ? 1.0
                            : (double)(now >= anim.startMs ? now - anim.startMs : 0) / (double)anim.durationMs;
                        if (t >= 1.0) {
                            anim.current = anim.target;
                            anim.active = false;
                            changed = true;
                            if (anim.closeOnDone) doneWindows.push_back(hwndEntry.first);
                        } else {
                            double eased = channelEntry.first == (int)AnimChannel::ThemeTransition
                                ? CubicBezierEase(t, 0.4, 0.0, 0.2, 1.0)
                                : channelEntry.first == (int)AnimChannel::SmoothScroll
                                ? CubicBezierEase(t, 0.22, 1.0, 0.36, 1.0)
                                : (channelEntry.first == (int)AnimChannel::Reveal
                                    || channelEntry.first == (int)AnimChannel::Close)
                                    ? EaseOut(t)
                                    : EaseInOut(t);
                            anim.current = anim.start + (anim.target - anim.start) * eased;
                            changed = true;
                        }
                    }
                    if (changed && Animations().pendingAnimationTicks.insert(hwndEntry.first).second) {
                        tickWindows.push_back(hwndEntry.first);
                    }
                }
            }

            for (HWND hwnd : tickWindows) {
                if (!PostMessageW(hwnd, WM_APP_ANIMATION_TICK, 0, 0)) {
                    std::lock_guard<std::mutex> lock(Animations().animMutex);
                    Animations().pendingAnimationTicks.erase(hwnd);
                }
            }
            for (HWND hwnd : doneWindows) {
                PostMessageW(hwnd, WM_APP_ANIMATION_DONE, 0, 0);
            }

            nowClock = Clock::now();
            do {
                nextDeadline += frameDuration;
            } while (nextDeadline <= nowClock);
        }

        if (timer) CloseHandle(timer);
    });
}

void StopAnimationThread(){
    Animations().animationShutdown = true;
    SignalAnimationScheduler();
    if (Animations().animationThread.joinable()) Animations().animationThread.join();
    if (Animations().animationWakeEvent) {
        CloseHandle(Animations().animationWakeEvent);
        Animations().animationWakeEvent = nullptr;
    }
}

uint32_t EffectiveAnimationDuration(AnimChannel channel, uint32_t durationMs){
    // Particle frames must follow their physical lifetime, independently of
    // the interface animation preset. Shortening this channel freezes effects.
    if (channel == AnimChannel::Effect
        || channel == AnimChannel::ThemeTransition
        || channel == AnimChannel::SmoothScroll) {
        return durationMs;
    }
    if (Settings().animationLevel == AnimationLevel::Off) return 0;
    if (Settings().animationLevel == AnimationLevel::Standard) return std::min<uint32_t>(durationMs, 160);
    return durationMs;
}

void StartAnimation(HWND hwnd, AnimChannel channel, double target, uint32_t durationMs, bool closeOnDone){
    if (!hwnd) return;
    durationMs = EffectiveAnimationDuration(channel, durationMs);
    EnsureAnimationThread();
    bool schedulerChanged = false;
    {
        std::lock_guard<std::mutex> lock(Animations().animMutex);
        AnimValue& anim = Animations().animations[hwnd][(int)channel];
        target = std::clamp(target, 0.0, 1.0);
        if (std::abs(anim.target - target) <= 0.001
            && (anim.active || std::abs(anim.current - target) <= 0.001)) {
            return;
        }
        anim.start = anim.current;
        anim.target = target;
        anim.startMs = AnimationNowMs();
        anim.durationMs = durationMs;
        anim.active = durationMs > 0 && std::abs(anim.target - anim.start) > 0.001;
        anim.closeOnDone = closeOnDone;
        if (!anim.active) anim.current = anim.target;
        schedulerChanged = anim.active;
    }
    if (schedulerChanged) SignalAnimationScheduler();
}

void RestartAnimation(HWND hwnd, AnimChannel channel, double target, uint32_t durationMs){
    if (!hwnd) return;
    durationMs = EffectiveAnimationDuration(channel, durationMs);
    EnsureAnimationThread();
    {
        std::lock_guard<std::mutex> lock(Animations().animMutex);
        AnimValue& anim = Animations().animations[hwnd][(int)channel];
        anim.current = 0.0;
        anim.start = 0.0;
        anim.target = std::clamp(target, 0.0, 1.0);
        anim.startMs = AnimationNowMs();
        anim.durationMs = durationMs;
        anim.active = durationMs > 0;
        anim.closeOnDone = false;
    }
    if (durationMs > 0) SignalAnimationScheduler();
}

double GetAnimationValue(HWND hwnd, AnimChannel channel, double fallback){
    if (!hwnd) return fallback;
    std::lock_guard<std::mutex> lock(Animations().animMutex);
    auto hwndIt = Animations().animations.find(hwnd);
    if (hwndIt == Animations().animations.end()) return fallback;
    auto channelIt = hwndIt->second.find((int)channel);
    if (channelIt == hwndIt->second.end()) return fallback;
    return channelIt->second.current;
}

void RemoveAnimationState(HWND hwnd){
    if (!hwnd) return;
    {
        std::lock_guard<std::mutex> lock(Animations().animMutex);
        Animations().animations.erase(hwnd);
        Animations().pendingAnimationTicks.erase(hwnd);
    }
    SignalAnimationScheduler();
}

void AcknowledgeAnimationTick(HWND hwnd){
    if (!hwnd) return;
    std::lock_guard<std::mutex> lock(Animations().animMutex);
    Animations().pendingAnimationTicks.erase(hwnd);
}

void CancelAnimationChannel(AnimChannel channel){
    {
        std::lock_guard<std::mutex> lock(Animations().animMutex);
        for (auto& windowEntry : Animations().animations) {
            auto channelIt = windowEntry.second.find((int)channel);
            if (channelIt == windowEntry.second.end()) continue;
            channelIt->second.current = channelIt->second.target;
            channelIt->second.active = false;
        }
    }
    SignalAnimationScheduler();
}

void StartWindowReveal(HWND hwnd, uint32_t durationMs){
    if (!hwnd) return;
    RestartAnimation(hwnd, AnimChannel::Reveal, 1.0, durationMs);
}

void BeginAnimatedClose(HWND hwnd, uint32_t durationMs){
    (void)durationMs;
    if (!hwnd) return;
    DestroyWindow(hwnd);
}

} // namespace attendance
