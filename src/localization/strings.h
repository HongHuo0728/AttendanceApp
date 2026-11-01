#pragma once

#include "core/types.h"

namespace attendance {

// Thread-local report language; workers never change the user's active language.
class TranslationScope {
public:
    explicit TranslationScope(UiLanguage language);
    ~TranslationScope();
    TranslationScope(const TranslationScope&)=delete;
    TranslationScope& operator=(const TranslationScope&)=delete;
private:
    int previous_;
};
UiLanguage CurrentTranslationLanguage();
const wchar_t* LanguageName(UiLanguage language);
std::wstring Tr(const wchar_t* english, const wchar_t* chinese);
std::wstring ParticleEffectsLabel();
std::wstring SmoothDropdownScrollingLabel();
std::wstring ScrollFrameRateLabel();
std::wstring AnimationLevelName(AnimationLevel level);
std::wstring ParticleLevelName(ParticleLevel level);
std::wstring ReportTemplateName(ReportTemplate templateType);
std::wstring StatsRangeName(StatsRange range);
std::string LanguageToString(UiLanguage language);
UiLanguage LanguageFromString(const std::string& value);
std::wstring StatusDisplayText(const std::wstring& status);

} // namespace attendance
