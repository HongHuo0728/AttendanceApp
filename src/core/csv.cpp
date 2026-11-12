#include "core/csv.h"
#include "core/encoding.h"
#include "core/constants.h"
#include "core/text.h"

namespace attendance {



std::string CsvCell(const std::wstring& value){
    std::string text = WideToUtf8(value);
    auto first = std::find_if_not(value.begin(), value.end(),
        [](wchar_t ch) { return iswspace(ch) != 0; });
    bool formula = first != value.end() &&
        std::wstring_view(L"=+-@\uFF1D\uFF0B\uFF0D\uFF20").find(*first) != std::wstring_view::npos;
    bool controlPrefix = !value.empty() &&
        (value.front() == L'\t' || value.front() == L'\r' || value.front() == L'\n');
    // CSV is a spreadsheet-facing report. Keep workbook/database data exact.
    if (formula || controlPrefix) text.insert(text.begin(), '\'');
    bool quote = formula || controlPrefix || text.find_first_of(",;\"\r\n") != std::string::npos;
    std::string out;
    if (quote) out.push_back('"');
    for (char ch : text) {
        if (ch == '"') out += "\"\"";
        else out.push_back(ch);
    }
    if (quote) out.push_back('"');
    return out;
}

std::string TsvCell(const std::wstring& value){
    return Escape(WideToUtf8(value));
}

std::vector<std::string> ParseCsvLine(const std::string& line){
    std::vector<std::string> cells;
    std::string cell;
    bool inQuotes = false, closedQuote = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char ch = line[i];
        if (ch == '"') {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                cell.push_back('"');
                ++i;
            } else if (inQuotes) {
                inQuotes = false;
                closedQuote = true;
            } else if (!closedQuote && TrimUtf8(cell).empty()) {
                cell.clear();
                inQuotes = true;
            } else return {};
        } else if (ch == ',' && !inQuotes) {
            cells.push_back(cell);
            cell.clear();
            closedQuote = false;
        } else {
            if (closedQuote) {
                if (ch == ' ' || ch == '\t' || ch == '\r') continue;
                return {};
            }
            cell.push_back(ch);
        }
    }
    if (inQuotes) return {};
    cells.push_back(cell);
    return cells;
}

bool ParseRosterCsv(const std::string& data, std::vector<std::wstring>& names) {
    names.clear();
    if (data.size() > MAX_ROSTER_FILE_BYTES) return false;
    std::istringstream input(data);
    std::string line;
    std::vector<std::wstring> parsed;
    std::unordered_set<std::wstring> seen;
    bool firstLine = true, firstRecord = true;
    while (std::getline(input, line)) {
        if (firstLine && line.rfind("\xEF\xBB\xBF", 0) == 0) line.erase(0, 3);
        firstLine = false;
        if (line.size() > MAX_RECORD_FIELD_BYTES * 4 + 3) return false;
        if (TrimUtf8(line).empty()) continue;
        auto cells = ParseCsvLine(line);
        if (cells.empty()) return false;
        std::wstring name;
        for (const auto& cell : cells) {
            auto text = TrimUtf8(cell);
            if (text.empty()) continue;
            if (!DecodeUtf8Field(text, MAX_RECORD_FIELD_BYTES, name)) return false;
            name = TrimWide(name);
            break;
        }
        if (name.empty()) continue;
        bool header = firstRecord && LooksLikeNameHeader(name);
        firstRecord = false;
        if (header || !seen.insert(name).second) continue;
        if (parsed.size() >= MAX_STUDENTS_PER_SHEET) return false;
        parsed.push_back(std::move(name));
    }
    names = std::move(parsed);
    return true;
}

std::string TrimUtf8(std::string value){
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t' || value.front() == '\r')) value.erase(value.begin());
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r')) value.pop_back();
    return value;
}

bool LooksLikeNameHeader(const std::wstring& value){
    std::wstring lower = value;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](wchar_t ch) { return (wchar_t)towlower(ch); });
    return lower == L"name" || lower == L"student" || lower == L"student name" || lower == L"\u59d3\u540d" || lower == L"\u5b66\u751f" || lower == L"\u5b66\u751f\u59d3\u540d";
}

} // namespace attendance
