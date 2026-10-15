#include "settings/fonts.h"
#include "settings/state.h"
#include "settings/font_state.h"
#include "ui/state.h"
#include "core/constants.h"

namespace attendance {

int CALLBACK EnumFontFamilyProc(const LOGFONTW* logFont, const TEXTMETRICW*, DWORD, LPARAM lParam);
std::vector<std::wstring> EnumerateInstalledFontFamilies();
void EnsureFontEnumerationThread();

HFONT CreateUiFont(int height, int weight){
    return CreateFontW(height, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, Settings().fontFamily.c_str());
}

int ComboItemHeight(){
    if (Settings().textScale == TextScale::Small) return 28;
    if (Settings().textScale == TextScale::Large) return 36;
    return 30;
}

int CALLBACK EnumFontFamilyProc(const LOGFONTW* logFont, const TEXTMETRICW*, DWORD, LPARAM lParam){
    auto* fonts = reinterpret_cast<std::set<std::wstring>*>(lParam);
    if (logFont && logFont->lfFaceName[0]) {
        fonts->insert(logFont->lfFaceName);
    }
    return 1;
}

std::vector<std::wstring> EnumerateInstalledFontFamilies(){
    std::set<std::wstring> fonts;
    HDC hdc = GetDC(nullptr);
    if (hdc) {
        LOGFONTW lf{};
        lf.lfCharSet = DEFAULT_CHARSET;
        EnumFontFamiliesExW(hdc, &lf, EnumFontFamilyProc, reinterpret_cast<LPARAM>(&fonts), 0);
        ReleaseDC(nullptr, hdc);
    }
    if (fonts.empty()) {
        fonts.insert(L"Segoe UI");
        fonts.insert(L"Microsoft YaHei UI");
    }
    return {fonts.begin(), fonts.end()};
}

void EnsureFontEnumerationThread(){
    if (Fonts().fontEnumerationThread.joinable()) return;
    Fonts().fontEnumerationThread = std::thread([] {
        while (true) {
            uint64_t generation = 0;
            {
                std::unique_lock<std::mutex> lock(Fonts().fontEnumerationMutex);
                Fonts().fontEnumerationCondition.wait(lock, [] {
                    return Fonts().fontEnumerationShutdown
                        || Fonts().fontPublishedGeneration < Fonts().fontRequestedGeneration;
                });
                if (Fonts().fontEnumerationShutdown) return;
                generation = Fonts().fontRequestedGeneration;
            }

            auto fonts = EnumerateInstalledFontFamilies();
            {
                std::lock_guard<std::mutex> lock(Fonts().fontEnumerationMutex);
                if (Fonts().fontEnumerationShutdown) return;
                if (generation != Fonts().fontRequestedGeneration) continue;
                Fonts().availableFonts = std::move(fonts);
                Fonts().fontCacheReady = true;
                Fonts().fontPublishedGeneration = generation;
            }
            HWND target = Ui().hwnd;
            if (target && IsWindow(target)) PostMessageW(target, WM_APP_FONT_CACHE_READY, 0, 0);
        }
    });
}

void RequestFontEnumeration(){
    EnsureFontEnumerationThread();
    {
        std::lock_guard<std::mutex> lock(Fonts().fontEnumerationMutex);
        ++Fonts().fontRequestedGeneration;
        Fonts().fontCacheReady = false;
    }
    Fonts().fontEnumerationCondition.notify_one();
}

void StopFontEnumerationThread(){
    {
        std::lock_guard<std::mutex> lock(Fonts().fontEnumerationMutex);
        Fonts().fontEnumerationShutdown = true;
    }
    Fonts().fontEnumerationCondition.notify_all();
    if (Fonts().fontEnumerationThread.joinable()) Fonts().fontEnumerationThread.join();
}

} // namespace attendance
