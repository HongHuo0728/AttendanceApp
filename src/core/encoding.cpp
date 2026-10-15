#include "core/encoding.h"
#include "core/constants.h"

namespace attendance {

std::string Unescape(std::string_view input);

std::string WideToUtf8(const std::wstring& input){
    if (input.empty()) return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, input.c_str(), (int)input.size(), nullptr, 0, nullptr, nullptr);
    std::string output(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, input.c_str(), (int)input.size(), output.data(), size, nullptr, nullptr);
    return output;
}

std::wstring Utf8ToWide(std::string_view input){
    if (input.empty()) return {};
    int size = MultiByteToWideChar(CP_UTF8, 0, input.data(), (int)input.size(), nullptr, 0);
    std::wstring output(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, input.data(), (int)input.size(), output.data(), size);
    return output;
}

std::string Escape(const std::string& input){
    std::string out;
    out.reserve(input.size());
    for (char ch : input) {
        if (ch == '\\') out += "\\\\";
        else if (ch == '\t') out += "\\t";
        else if (ch == '\n') out += "\\n";
        else if (ch == '\r') out += "\\r";
        else out += ch;
    }
    return out;
}

std::string Unescape(std::string_view input){
    std::string out;
    out.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\\' && i + 1 < input.size()) {
            char next = input[++i];
            if (next == 't') out += '\t';
            else if (next == 'n') out += '\n';
            else if (next == 'r') out += '\r';
            else out += next;
        } else {
            out += input[i];
        }
    }
    return out;
}

bool DecodeUtf8Field(std::string_view decoded, size_t maximumBytes, std::wstring& output){
    output.clear();
    if (decoded.size() > maximumBytes) return false;
    if (decoded.empty()) return true;
    if (decoded.size() > static_cast<size_t>(INT_MAX)) return false;
    int size = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, decoded.data(), static_cast<int>(decoded.size()), nullptr, 0);
    if (size <= 0) return false;
    output.resize(static_cast<size_t>(size));
    return MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, decoded.data(), static_cast<int>(decoded.size()), output.data(), size) == size;
}

bool DecodeEscapedUtf8Field(std::string_view encoded, size_t maximumBytes, std::wstring& output){
    if (maximumBytes > SIZE_MAX / 2 || encoded.size() > maximumBytes * 2) return false;
    if (encoded.find('\\') == std::string_view::npos) return DecodeUtf8Field(encoded, maximumBytes, output);
    return DecodeUtf8Field(Unescape(encoded), maximumBytes, output);
}

bool SplitRecordFields(std::string_view line, std::array<std::string_view, 4>& parts){
    size_t field = 0, start = 0;
    bool escaped = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char ch = line[i];
        if (escaped) {
            escaped = false;
        } else if (ch == '\\') {
            escaped = true;
        } else if (ch == '\t') {
            if (field >= 3) return false;
            parts[field++] = line.substr(start, i - start);
            start = i + 1;
        }
    }
    if (field != 3) return false;
    parts[3] = line.substr(start);
    return true;
}

std::string HtmlCell(const std::wstring& value){
    std::string text = WideToUtf8(value);
    std::string out;
    for (char ch : text) {
        if (ch == '&') out += "&amp;";
        else if (ch == '<') out += "&lt;";
        else if (ch == '>') out += "&gt;";
        else if (ch == '"') out += "&quot;";
        else out.push_back(ch);
    }
    return out;
}

std::string XmlCell(const std::wstring& value){
    return HtmlCell(value);
}

} // namespace attendance
