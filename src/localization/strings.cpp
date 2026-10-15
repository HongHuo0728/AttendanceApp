#include "localization/catalog_data.h"
#include "localization/strings.h"
#include "core/encoding.h"
#include "settings/state.h"
#include "core/constants.h"
#include "zh_hk_strings.h"

namespace attendance {

bool TranslateFeatureText(const std::wstring& key, std::wstring& out);
bool TranslateAdditionalLanguage(const std::wstring& key, std::wstring& out);

const wchar_t* LanguageName(UiLanguage language){
    switch (language) {
    case UiLanguage::ChineseSimplified: return L"\u7b80\u4f53\u4e2d\u6587";
    case UiLanguage::Maltese: return L"Malti";
    case UiLanguage::Japanese: return L"\u65e5\u672c\u8a9e";
    case UiLanguage::French: return L"Fran\u00e7ais";
    case UiLanguage::German: return L"Deutsch";
    case UiLanguage::Russian: return L"\u0420\u0443\u0441\u0441\u043a\u0438\u0439";
    case UiLanguage::ChineseTraditional: return L"\u7e41\u9ad4\u4e2d\u6587\uff08\u53f0\u7063\uff09";
    case UiLanguage::Spanish: return L"Espa\u00f1ol";
    case UiLanguage::Italian: return L"Italiano";
    case UiLanguage::Mongolian: return L"\u041c\u043e\u043d\u0433\u043e\u043b";
    case UiLanguage::Esperanto: return L"Esperanto";
    case UiLanguage::ClassicalChinese: return L"\u6587\u8a00\u6587";
    case UiLanguage::Thai: return L"\u0e44\u0e17\u0e22";
    case UiLanguage::Filipino: return L"Filipino";
    case UiLanguage::Turkish: return L"T\u00fcrk\u00e7e";
    case UiLanguage::Lithuanian: return L"Lietuvi\u0173";
    case UiLanguage::Norwegian: return L"Norsk";
    case UiLanguage::Vietnamese: return L"Ti\u1ebfng Vi\u1ec7t";
    case UiLanguage::ChineseTraditionalHongKong: return L"\u7e41\u9ad4\u4e2d\u6587\uff08\u9999\u6e2f\u7cb5\u8a9e\uff09";
    default: return L"English";
    }
}

bool TranslateFeatureText(const std::wstring& key, std::wstring& out){
    const std::string lookupKey = WideToUtf8(key);
    if (Settings().language == UiLanguage::English) return false;

    

    const auto& entries = catalog::Feature;

    for (const auto& entry : entries) {
        if (lookupKey != entry.key.get()) continue;
        switch (Settings().language) {
        case UiLanguage::ChineseSimplified: out = entry.zh.get(); break;
        case UiLanguage::Maltese: out = Utf8ToWide(entry.mt.get()); break;
        case UiLanguage::Japanese: out = entry.ja.get(); break;
        case UiLanguage::French: out = Utf8ToWide(entry.fr.get()); break;
        case UiLanguage::German: out = Utf8ToWide(entry.de.get()); break;
        case UiLanguage::Russian: out = entry.ru.get(); break;
        case UiLanguage::ChineseTraditional: out = entry.zht.get(); break;
        case UiLanguage::Spanish: out = Utf8ToWide(entry.es.get()); break;
        case UiLanguage::Italian: out = Utf8ToWide(entry.it.get()); break;
        case UiLanguage::Mongolian: out = entry.mn.get(); break;
        case UiLanguage::Esperanto: out = Utf8ToWide(entry.eo.get()); break;
        case UiLanguage::ClassicalChinese: out = entry.lzh.get(); break;
        case UiLanguage::Thai: out = entry.th.get(); break;
        case UiLanguage::Filipino: out = Utf8ToWide(entry.fil.get()); break;
        case UiLanguage::Turkish: out = Utf8ToWide(entry.tr.get()); break;
        case UiLanguage::Lithuanian: out = Utf8ToWide(entry.lt.get()); break;
        case UiLanguage::Norwegian: out = Utf8ToWide(entry.no.get()); break;
        case UiLanguage::Vietnamese: out = Utf8ToWide(entry.vi.get()); break;
        case UiLanguage::ChineseTraditionalHongKong: out = entry.zhhk.get(); break;
        default: return false;
        }
        return true;
    }
    return false;
}

bool TranslateAdditionalLanguage(const std::wstring& key, std::wstring& out){
    const std::string lookupKey = WideToUtf8(key);
    if (Settings().language == UiLanguage::Norwegian || Settings().language == UiLanguage::Vietnamese) {
        
        const auto& entries = catalog::Legacy;
        for (const auto& entry : entries) {
            if (lookupKey == entry.key.get()) {
                if (Settings().language == UiLanguage::Norwegian) out = Utf8ToWide(entry.no.get());
                else out = Utf8ToWide(entry.vi.get());
                return true;
            }
        }
        return false;
    }

    if (Settings().language < UiLanguage::Italian) return false;

    if (Settings().language == UiLanguage::ClassicalChinese) {
        
        const auto& classical = catalog::Classical;

        for (const auto& entry : classical) {
            if (lookupKey == entry.key.get()) {
                out = entry.text.get();
                return true;
            }
        }
    }

    

    const auto& entries = catalog::Additional;

    for (const auto& entry : entries) {
        if (lookupKey == entry.key.get()) {
            switch (Settings().language) {
            case UiLanguage::Italian: out = Utf8ToWide(entry.it.get()); return true;
            case UiLanguage::Mongolian: out = entry.mn.get(); return true;
            case UiLanguage::Esperanto: out = Utf8ToWide(entry.eo.get()); return true;
            case UiLanguage::ClassicalChinese: out = entry.lzh.get(); return true;
            case UiLanguage::Thai: out = entry.th.get(); return true;
            case UiLanguage::Filipino: out = Utf8ToWide(entry.fil.get()); return true;
            case UiLanguage::Turkish: out = Utf8ToWide(entry.tr.get()); return true;
            case UiLanguage::Lithuanian: out = Utf8ToWide(entry.lt.get()); return true;
            default: return false;
            }
        }
    }
    return false;
}

std::wstring Tr(const wchar_t* english, const wchar_t* chinese){
    std::wstring key = english;
    if (Settings().language == UiLanguage::English) return key;
    const std::string lookupKey = WideToUtf8(english);
    std::wstring additional;
    if (Settings().language == UiLanguage::ChineseTraditionalHongKong) {
        if (attendance::localization::TranslateHongKong(key, additional)) return additional;
        std::wstring diagnostic = L"AttendanceApp missing zh-HK translation: " + key + L"\n";
        OutputDebugStringW(diagnostic.c_str());
        return key;
    }
    if (TranslateFeatureText(key, additional)) return additional;
    if (TranslateAdditionalLanguage(key, additional)) return additional;

    auto zh = [&](const wchar_t* s) -> std::wstring { return s; };
    auto mt = [&](const char* s) -> std::wstring { return Utf8ToWide(s); };
    auto ja = [&](const wchar_t* s) -> std::wstring { return s; };
    auto fr = [&](const char* s) -> std::wstring { return Utf8ToWide(s); };
    auto de = [&](const char* s) -> std::wstring { return Utf8ToWide(s); };
    auto ru = [&](const wchar_t* s) -> std::wstring { return s; };
    auto zht = [&](const wchar_t* s) -> std::wstring { return s; };
    auto es = [&](const char* s) -> std::wstring { return Utf8ToWide(s); };

    

    const auto& entries = catalog::Main;
    for (const auto& entry : entries) {
        if (lookupKey == entry.key.get()) {
            switch (Settings().language) {
            case UiLanguage::ChineseSimplified: return zh(entry.zh.get());
            case UiLanguage::Maltese: return mt(entry.mt.get());
            case UiLanguage::Japanese: return ja(entry.ja.get());
            case UiLanguage::French: return fr(entry.fr.get());
            case UiLanguage::German: return de(entry.de.get());
            case UiLanguage::Russian: return ru(entry.ru.get());
            case UiLanguage::ChineseTraditional: return zht(entry.zht.get());
            case UiLanguage::Spanish: return es(entry.es.get());
            default: return key;
            }
        }
    }
    // Every user-visible call site supplies a Chinese fallback. Keep non-English
    // modes readable while a legacy string is being promoted into a language pack.
    return chinese && *chinese ? std::wstring(chinese) : key;
}

std::wstring ParticleEffectsLabel(){
    switch (Settings().language) {
    case UiLanguage::ChineseSimplified: return L"\u542f\u7528\u7c92\u5b50\u7279\u6548";
    case UiLanguage::Maltese: return L"Ixg\u0127el effetti tal-parti\u010belli";
    case UiLanguage::Japanese: return L"\u30d1\u30fc\u30c6\u30a3\u30af\u30eb\u52b9\u679c\u3092\u6709\u52b9\u5316";
    case UiLanguage::French: return L"Activer les effets de particules";
    case UiLanguage::German: return L"Partikeleffekte aktivieren";
    case UiLanguage::Russian: return L"\u0412\u043a\u043b\u044e\u0447\u0438\u0442\u044c \u044d\u0444\u0444\u0435\u043a\u0442\u044b \u0447\u0430\u0441\u0442\u0438\u0446";
    case UiLanguage::ChineseTraditional: return L"\u555f\u7528\u7c92\u5b50\u7279\u6548";
    case UiLanguage::Spanish: return L"Activar efectos de part\u00edculas";
    case UiLanguage::Italian: return L"Attiva effetti particellari";
    case UiLanguage::Mongolian: return L"\u0411\u04e9\u04e9\u043c\u0441\u0438\u0439\u043d \u044d\u0444\u0444\u0435\u043a\u0442\u0438\u0439\u0433 \u0438\u0434\u044d\u0432\u0445\u0436\u04af\u04af\u043b\u044d\u0445";
    case UiLanguage::Esperanto: return L"\u015calti partiklo-efektojn";
    case UiLanguage::ClassicalChinese: return L"\u555f\u7c92\u5b50\u4e4b\u6548";
    case UiLanguage::Thai: return L"\u0e40\u0e1b\u0e34\u0e14\u0e40\u0e2d\u0e1f\u0e40\u0e1f\u0e01\u0e15\u0e4c\u0e2d\u0e19\u0e38\u0e20\u0e32\u0e04";
    case UiLanguage::Filipino: return L"I-enable ang particle effects";
    case UiLanguage::Turkish: return L"Par\u00e7ac\u0131k efektlerini etkinle\u015ftir";
    case UiLanguage::Lithuanian: return L"\u012ejungti daleli\u0173 efektus";
    case UiLanguage::Norwegian: return L"Sl\u00e5 p\u00e5 partikkeleffekter";
    case UiLanguage::Vietnamese: return L"B\u1eadt hi\u1ec7u \u1ee9ng h\u1ea1t";
    case UiLanguage::ChineseTraditionalHongKong: return Tr(L"Enable particle effects", L"");
    default: return L"Enable particle effects";
    }
}

std::wstring SmoothDropdownScrollingLabel(){
    switch (Settings().language) {
    case UiLanguage::ChineseSimplified: return L"\u542f\u7528\u4e0b\u62c9\u83dc\u5355\u5e73\u6ed1\u6eda\u52a8";
    case UiLanguage::Maltese: return L"Ippermetti skrolljar lixx fil-listi";
    case UiLanguage::Japanese: return L"\u30c9\u30ed\u30c3\u30d7\u30c0\u30a6\u30f3\u306e\u30b9\u30e0\u30fc\u30ba\u30b9\u30af\u30ed\u30fc\u30eb\u3092\u6709\u52b9\u306b\u3059\u308b";
    case UiLanguage::French: return L"Activer le d\u00e9filement fluide des listes";
    case UiLanguage::German: return L"Sanftes Scrollen in Dropdownlisten aktivieren";
    case UiLanguage::Russian: return L"\u0412\u043a\u043b\u044e\u0447\u0438\u0442\u044c \u043f\u043b\u0430\u0432\u043d\u0443\u044e \u043f\u0440\u043e\u043a\u0440\u0443\u0442\u043a\u0443 \u0441\u043f\u0438\u0441\u043a\u043e\u0432";
    case UiLanguage::ChineseTraditional: return L"\u555f\u7528\u4e0b\u62c9\u9078\u55ae\u5e73\u6ed1\u6372\u52d5";
    case UiLanguage::Spanish: return L"Activar desplazamiento suave en listas";
    case UiLanguage::Italian: return L"Attiva scorrimento fluido nei menu";
    case UiLanguage::Mongolian: return L"\u0423\u043d\u0436\u0434\u0430\u0433 \u0436\u0430\u0433\u0441\u0430\u0430\u043b\u0442\u044b\u043d \u0436\u0438\u0433\u0434 \u0433\u04af\u0439\u043b\u0433\u044d\u043b\u0442\u0438\u0439\u0433 \u0438\u0434\u044d\u0432\u0445\u0436\u04af\u04af\u043b\u044d\u0445";
    case UiLanguage::Esperanto: return L"\u015calti glatan ruladon de falmenuoj";
    case UiLanguage::ClassicalChinese: return L"\u555f\u4e0b\u62c9\u55ae\u4e4b\u5e73\u6ed1\u6372\u52d5";
    case UiLanguage::Thai: return L"\u0e40\u0e1b\u0e34\u0e14\u0e43\u0e0a\u0e49\u0e01\u0e32\u0e23\u0e40\u0e25\u0e37\u0e48\u0e2d\u0e19\u0e23\u0e32\u0e22\u0e01\u0e32\u0e23\u0e41\u0e1a\u0e1a\u0e25\u0e37\u0e48\u0e19\u0e44\u0e2b\u0e25";
    case UiLanguage::Filipino: return L"I-enable ang maayos na pag-scroll ng dropdown";
    case UiLanguage::Turkish: return L"A\u00e7\u0131l\u0131r listelerde yumu\u015fak kayd\u0131rmay\u0131 etkinle\u015ftir";
    case UiLanguage::Lithuanian: return L"\u012ejungti skland\u0173 i\u0161skleid\u017eiam\u0173j\u0173 s\u0105ra\u0161\u0173 slinkim\u0105";
    case UiLanguage::Norwegian: return L"Aktiver jevn rulling i nedtrekkslister";
    case UiLanguage::Vietnamese: return L"B\u1eadt cu\u1ed9n m\u01b0\u1ee3t trong danh s\u00e1ch th\u1ea3 xu\u1ed1ng";
    case UiLanguage::ChineseTraditionalHongKong: return Tr(L"Enable smooth dropdown scrolling", L"");
    default: return L"Enable smooth dropdown scrolling";
    }
}

std::wstring ScrollFrameRateLabel(){
    switch (Settings().language) {
    case UiLanguage::ChineseSimplified: return L"\u4e0b\u62c9\u6eda\u52a8\u5e27\u7387\uff0860-360\uff09";
    case UiLanguage::Maltese: return L"Rata tal-frejms tal-iskrolljar (60-360)";
    case UiLanguage::Japanese: return L"\u30c9\u30ed\u30c3\u30d7\u30c0\u30a6\u30f3\u306e\u30b9\u30af\u30ed\u30fc\u30ebFPS\uff0860-360\uff09";
    case UiLanguage::French: return L"Fr\u00e9quence du d\u00e9filement (60-360)";
    case UiLanguage::German: return L"Bildrate beim Dropdown-Scrollen (60-360)";
    case UiLanguage::Russian: return L"\u0427\u0430\u0441\u0442\u043e\u0442\u0430 \u043f\u0440\u043e\u043a\u0440\u0443\u0442\u043a\u0438 \u0441\u043f\u0438\u0441\u043a\u043e\u0432 (60-360)";
    case UiLanguage::ChineseTraditional: return L"\u4e0b\u62c9\u6372\u52d5\u5e40\u7387\uff0860-360\uff09";
    case UiLanguage::Spanish: return L"FPS del desplazamiento (60-360)";
    case UiLanguage::Italian: return L"FPS dello scorrimento (60-360)";
    case UiLanguage::Mongolian: return L"\u0416\u0430\u0433\u0441\u0430\u0430\u043b\u0442\u044b\u043d \u0433\u04af\u0439\u043b\u0433\u044d\u0445 FPS (60-360)";
    case UiLanguage::Esperanto: return L"Bildrapido de falmenua rulado (60-360)";
    case UiLanguage::ClassicalChinese: return L"\u4e0b\u62c9\u6372\u52d5\u5e40\u7387\uff0860-360\uff09";
    case UiLanguage::Thai: return L"FPS \u0e01\u0e32\u0e23\u0e40\u0e25\u0e37\u0e48\u0e2d\u0e19\u0e23\u0e32\u0e22\u0e01\u0e32\u0e23 (60-360)";
    case UiLanguage::Filipino: return L"FPS ng dropdown scroll (60-360)";
    case UiLanguage::Turkish: return L"A\u00e7\u0131l\u0131r liste kayd\u0131rma FPS'i (60-360)";
    case UiLanguage::Lithuanian: return L"I\u0161skleid\u017eiamojo slinkimo FPS (60-360)";
    case UiLanguage::Norwegian: return L"Bildefrekvens for rullegardin (60-360)";
    case UiLanguage::Vietnamese: return L"FPS cu\u1ed9n danh s\u00e1ch th\u1ea3 xu\u1ed1ng (60-360)";
    case UiLanguage::ChineseTraditionalHongKong: return Tr(L"Dropdown scroll frame rate (60-360)", L"");
    default: return L"Dropdown scroll frame rate (60-360)";
    }
}

std::wstring AnimationLevelName(AnimationLevel level){
    switch (level) {
    case AnimationLevel::Off: return Tr(L"Off", L"\u5173\u95ed");
    case AnimationLevel::Standard: return Tr(L"Standard", L"\u6807\u51c6");
    default: return Tr(L"Advanced", L"\u9ad8\u7ea7");
    }
}

std::wstring ParticleLevelName(ParticleLevel level){
    switch (level) {
    case ParticleLevel::Low: return Tr(L"Low", L"\u4f4e");
    case ParticleLevel::High: return Tr(L"High", L"\u9ad8");
    default: return Tr(L"Medium", L"\u4e2d");
    }
}

std::wstring ReportTemplateName(ReportTemplate templateType){
    switch (templateType) {
    case ReportTemplate::Simple: return Tr(L"Simple", L"\u7b80\u6d01\u7248");
    case ReportTemplate::Teacher: return Tr(L"Teacher", L"\u6559\u5e08\u7248");
    case ReportTemplate::Parent: return Tr(L"Parent", L"\u5bb6\u957f\u7248");
    default: return Tr(L"Complete", L"\u5b8c\u6574\u7248");
    }
}

std::wstring StatsRangeName(StatsRange range){
    switch (range) {
    case StatsRange::ThisWeek: return Tr(L"This week", L"\u672c\u5468");
    case StatsRange::ThisMonth: return Tr(L"This month", L"\u672c\u6708");
    default: return Tr(L"All time", L"\u5168\u90e8");
    }
}

std::string LanguageToString(UiLanguage language){
    switch (language) {
    case UiLanguage::ChineseSimplified: return "zh-CN";
    case UiLanguage::Maltese: return "mt";
    case UiLanguage::Japanese: return "ja";
    case UiLanguage::French: return "fr";
    case UiLanguage::German: return "de";
    case UiLanguage::Russian: return "ru";
    case UiLanguage::ChineseTraditional: return "zh-TW";
    case UiLanguage::Spanish: return "es";
    case UiLanguage::Italian: return "it";
    case UiLanguage::Mongolian: return "mn";
    case UiLanguage::Esperanto: return "eo";
    case UiLanguage::ClassicalChinese: return "lzh";
    case UiLanguage::Thai: return "th";
    case UiLanguage::Filipino: return "fil";
    case UiLanguage::Turkish: return "tr";
    case UiLanguage::Lithuanian: return "lt";
    case UiLanguage::Norwegian: return "no";
    case UiLanguage::Vietnamese: return "vi";
    case UiLanguage::ChineseTraditionalHongKong: return "zh-HK";
    default: return "en";
    }
}

UiLanguage LanguageFromString(const std::string& value){
    if (value == "zh-CN") return UiLanguage::ChineseSimplified;
    if (value == "mt") return UiLanguage::Maltese;
    if (value == "ja") return UiLanguage::Japanese;
    if (value == "fr") return UiLanguage::French;
    if (value == "de") return UiLanguage::German;
    if (value == "ru") return UiLanguage::Russian;
    if (value == "zh-TW") return UiLanguage::ChineseTraditional;
    if (value == "es") return UiLanguage::Spanish;
    if (value == "it") return UiLanguage::Italian;
    if (value == "mn") return UiLanguage::Mongolian;
    if (value == "eo") return UiLanguage::Esperanto;
    if (value == "lzh") return UiLanguage::ClassicalChinese;
    if (value == "th") return UiLanguage::Thai;
    if (value == "fil") return UiLanguage::Filipino;
    if (value == "tr") return UiLanguage::Turkish;
    if (value == "lt") return UiLanguage::Lithuanian;
    if (value == "no") return UiLanguage::Norwegian;
    if (value == "vi") return UiLanguage::Vietnamese;
    if (value == "zh-HK") return UiLanguage::ChineseTraditionalHongKong;
    return UiLanguage::English;
}

std::wstring StatusDisplayText(const std::wstring& status){
    static UiLanguage cachedLanguage = static_cast<UiLanguage>(-1);
    static std::array<std::wstring, 4> labels;
    if (cachedLanguage != Settings().language) {
        labels = {Tr(L"Present", L"\u51fa\u5e2d"), Tr(L"Absent", L"\u7f3a\u5e2d"),
            Tr(L"Late", L"\u8fdf\u5230"), Tr(L"Other", L"\u5176\u4ed6")};
        cachedLanguage = Settings().language;
    }
    if (status == L"Present") return labels[0];
    if (status == L"Absent") return labels[1];
    if (status == L"Late") return labels[2];
    if (status == L"Other") return labels[3];
    return status;
}

} // namespace attendance
