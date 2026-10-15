#include "core/text.h"
#include "core/constants.h"

namespace attendance {



std::wstring LowerText(std::wstring value){
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t ch) {
        return (wchar_t)towlower(ch);
    });
    return value;
}

bool ContainsText(const std::wstring& haystack, const std::wstring& needle){
    return std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(),
        [](wchar_t a, wchar_t b) { return towlower(a) == towlower(b); }) != haystack.end()
        || needle.empty();
}

std::wstring TrimWide(std::wstring value){
    auto isSpace = [](wchar_t ch) { return iswspace(ch) != 0; };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](wchar_t ch) { return !isSpace(ch); }));
    value.erase(std::find_if(value.rbegin(), value.rend(), [&](wchar_t ch) { return !isSpace(ch); }).base(), value.end());
    return value;
}

std::wstring UpperText(std::wstring value){
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t ch) {
        return (wchar_t)towupper(ch);
    });
    return value;
}

std::wstring NormalizeSingleLineText(const std::wstring& text){
    std::wstring normalized;
    normalized.reserve(text.size());
    bool pendingSeparator = false;
    for (wchar_t ch : text) {
        if (ch == L'\r' || ch == L'\n') {
            pendingSeparator = true;
            continue;
        }
        if (pendingSeparator && !normalized.empty()
            && !iswspace(normalized.back()) && !iswspace(ch)) {
            normalized.push_back(L' ');
        }
        pendingSeparator = false;
        normalized.push_back(ch);
    }
    return normalized;
}

} // namespace attendance
