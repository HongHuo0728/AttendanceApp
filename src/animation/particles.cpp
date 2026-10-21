#include "animation/particles.h"
#include "animation/scheduler.h"
#include "ui/theme.h"
#include "settings/state.h"
#include "animation/state.h"
#include "ui/state.h"
#include "core/constants.h"

namespace attendance {

double RandomRange(double minValue, double maxValue);
int RandomInt(int minValue, int maxValue);

bool IsCoreParticleButton(int id){
    switch (id) {
    case IDC_PRESENT:
    case IDC_ABSENT:
    case IDC_LATE:
    case IDC_OTHER_STATUS:
    case IDC_ADD_UPDATE:
    case IDC_ALL_PRESENT:
    case IDC_SAVE:
    case IDC_EXPORT_CSV:
    case IDC_IMPORT:
    case IDC_NEW:
    case IDC_CLEAR_FILTER:
    case IDC_COURSE_OPTIONS:
    case IDC_TOOLS:
    case IDC_SETTINGS:
    case IDC_QUICK_PRESENT:
    case IDC_QUICK_ABSENT:
    case IDC_QUICK_LATE:
    case IDC_QUICK_OTHER:
    case IDC_QUICK_FINISH:
        return true;
    default:
        return false;
    }
}

double RandomRange(double minValue, double maxValue){
    std::uniform_real_distribution<double> dist(minValue, maxValue);
    return dist(Animations().effectRng);
}

int RandomInt(int minValue, int maxValue){
    std::uniform_int_distribution<int> dist(minValue, maxValue);
    return dist(Animations().effectRng);
}

void TriggerButtonFeedback(HWND hwnd, POINT origin){
    const uint64_t setupStartUs = PerformanceNowUs();
    if (!hwnd) return;
    wchar_t className[32]{};
    GetClassNameW(hwnd, className, 32);
    if (lstrcmpiW(className, L"Button") != 0) return;
    if ((GetWindowLongPtrW(hwnd, GWL_STYLE) & BS_OWNERDRAW) == 0) return;

    int id = GetDlgCtrlID(hwnd);
    if (id == IDC_SETTINGS_PARTICLES || id == IDC_SETTINGS_RISK_ALERTS
        || id == IDC_SETTINGS_AUTOSAVE_PROMPT || id == IDC_SETTINGS_COMMAND_PALETTE
        || id == IDC_SETTINGS_ADVANCED_FILTER
        || id == IDC_SETTINGS_SHOW_STATS || id == IDC_SETTINGS_SHOW_HINT
        || id == IDC_SETTINGS_TABLE_GRID) return;
    bool particlesEnabled = Settings().particlesEnabled && IsCoreParticleButton(id);
    bool rippleEnabled = Settings().animationLevel != AnimationLevel::Off;
    if (!rippleEnabled && !particlesEnabled) {
        auto existing = Animations().buttonEffects.find(hwnd);
        if (existing != Animations().buttonEffects.end()) {
            existing->second.active = false;
            existing->second.particles.clear();
        }
        return;
    }

    ButtonEffectState& state = Animations().buttonEffects[hwnd];
    state.particles.clear();
    state.origin = origin;
    state.startMs = AnimationNowMs();
    state.rippleEnabled = rippleEnabled;
    state.durationMs = rippleEnabled ? 400u : 0u;
    state.active = true;

    if (particlesEnabled) {
        int minCount = 30;
        int maxCount = 50;
        if (Settings().particleLevel == ParticleLevel::Low) {
            minCount = 12;
            maxCount = 22;
        } else if (Settings().particleLevel == ParticleLevel::High) {
            minCount = 50;
            maxCount = 72;
        }
        int count = RandomInt(minCount, maxCount);
        if (state.particles.capacity() < 72) state.particles.reserve(72);
        for (int i = 0; i < count; ++i) {
            double angle = RandomRange(0.0, 6.28318530717958647692);
            double speed = RandomRange(95.0, 250.0);
            ButtonParticle particle;
            particle.x = (double)origin.x;
            particle.y = (double)origin.y;
            particle.vx = std::cos(angle) * speed;
            particle.vy = std::sin(angle) * speed - RandomRange(40.0, 110.0);
            particle.radius = RandomRange(1.0, 3.0);
            particle.lifetimeMs = (uint32_t)RandomInt(800, 1200);
            particle.star = RandomInt(0, 5) == 0;
            particle.color = RandomInt(0, 12) == 0
                ? Ui().COLOR_ACCENT
                : (IsRenderedPaletteDark() ? RGB(255, 255, 255) : RGB(0, 0, 0));
            state.particles.push_back(particle);
            state.durationMs = std::max(state.durationMs, particle.lifetimeMs);
        }
    }

    RestartAnimation(hwnd, AnimChannel::Effect, 1.0, state.durationMs + 50);
    InvalidateRect(hwnd, nullptr, FALSE);
    Animations().lastParticleSetupUs.store(PerformanceNowUs() - setupStartUs, std::memory_order_relaxed);
}

void DrawButtonEffects(const DRAWITEMSTRUCT* draw, const RECT& buttonRc, COLORREF baseFill){
    const uint64_t frameStartUs = PerformanceNowUs();
    auto it = Animations().buttonEffects.find(draw->hwndItem);
    if (it == Animations().buttonEffects.end() || !it->second.active) return;

    uint64_t now = AnimationNowMs();
    ButtonEffectState& state = it->second;
    uint64_t elapsedMs = now > state.startMs ? now - state.startMs : 0;
    if (elapsedMs > state.durationMs + 40) {
        state.active = false;
        state.particles.clear();
        return;
    }

    HRGN clip = CreateRoundRectRgn(buttonRc.left, buttonRc.top, buttonRc.right + 1, buttonRc.bottom + 1, 6, 6);
    int savedDc = SaveDC(draw->hDC);
    SelectClipRgn(draw->hDC, clip);

    double rippleProgress = std::clamp(elapsedMs / 400.0, 0.0, 1.0);
    if (state.rippleEnabled && rippleProgress < 1.0) {
        double eased = EaseOut(rippleProgress);
        int radius = (int)std::lround(80.0 * eased);
        double strength = 0.30 * (1.0 - rippleProgress);
        COLORREF rippleTarget = IsRenderedPaletteDark() ? RGB(255, 255, 255) : RGB(0, 0, 0);
        COLORREF rippleColor = BlendColor(baseFill, rippleTarget, strength);
        HPEN ripplePen = CreatePen(PS_SOLID, 2, rippleColor);
        HGDIOBJ oldPen = SelectObject(draw->hDC, ripplePen);
        HGDIOBJ oldBrush = SelectObject(draw->hDC, GetStockObject(HOLLOW_BRUSH));
        Ellipse(draw->hDC, state.origin.x - radius, state.origin.y - radius,
            state.origin.x + radius, state.origin.y + radius);
        SelectObject(draw->hDC, oldBrush);
        SelectObject(draw->hDC, oldPen);
        DeleteObject(ripplePen);
    }

    HGDIOBJ oldBrush = SelectObject(draw->hDC, GetStockObject(DC_BRUSH));
    HGDIOBJ oldPen = SelectObject(draw->hDC, GetStockObject(DC_PEN));
    for (const auto& particle : state.particles) {
        double progress = std::clamp(elapsedMs / (double)particle.lifetimeMs, 0.0, 1.0);
        if (progress >= 1.0) continue;
        double seconds = elapsedMs / 1000.0;
        double gravity = 260.0 * 0.3;
        int x = (int)std::lround(particle.x + particle.vx * seconds);
        int y = (int)std::lround(particle.y + particle.vy * seconds + 0.5 * gravity * seconds * seconds);
        double size = particle.radius * (1.0 + 0.5 * progress);
        int r = std::max(1, (int)std::lround(size));
        double opacity = std::pow(1.0 - progress, 1.35);
        COLORREF color = BlendColor(particle.color, Ui().COLOR_MUTED, progress);
        color = BlendColor(baseFill, color, opacity);

        SetDCBrushColor(draw->hDC, color);
        SetDCPenColor(draw->hDC, color);
        if (particle.star) {
            POINT pts[8] = {
                {x, y - r * 2}, {x + r / 2, y - r / 2}, {x + r * 2, y},
                {x + r / 2, y + r / 2}, {x, y + r * 2}, {x - r / 2, y + r / 2},
                {x - r * 2, y}, {x - r / 2, y - r / 2}
            };
            Polygon(draw->hDC, pts, 8);
        } else {
            Ellipse(draw->hDC, x - r, y - r, x + r, y + r);
        }
    }
    SelectObject(draw->hDC, oldPen);
    SelectObject(draw->hDC, oldBrush);

    RestoreDC(draw->hDC, savedDc);
    DeleteObject(clip);
    RecordMaximum(Animations().maxParticleFrameUs, PerformanceNowUs() - frameStartUs);
}

} // namespace attendance
