#include "core/status.h"
#include "model/query.h"
#include "core/text.h"
#include "localization/strings.h"
#include "model/state.h"
#include "settings/state.h"
#include "core/constants.h"

namespace attendance {

std::wstring RecordDateKey(const AttendanceRecord& record);
bool DateKeyToFileTime(const std::wstring& date, FILETIME& out);
int64_t FileTimeTicks(const FILETIME& ft);
bool DateWithinRecentDays(const std::wstring& date, int days);
bool DateWithinCurrentMonth(const std::wstring& date);
std::vector<std::wstring> SplitWords(const std::wstring& text);

std::wstring RecordDateKey(const AttendanceRecord& record){
    return record.dateTime.size() >= 10 ? record.dateTime.substr(0, 10) : record.dateTime;
}

std::wstring TodayDateKey(){
    SYSTEMTIME st{};
    GetLocalTime(&st);
    wchar_t buffer[16]{};
    swprintf_s(buffer, L"%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
    return buffer;
}

bool DateKeyToFileTime(const std::wstring& date, FILETIME& out){
    if (date.size() != 10 || date[4] != L'-' || date[7] != L'-') return false;
    for (size_t i = 0; i < date.size(); ++i) {
        if (i == 4 || i == 7) continue;
        if (date[i] < L'0' || date[i] > L'9') return false;
    }
    SYSTEMTIME st{};
    try {
        st.wYear = (WORD)std::stoi(date.substr(0, 4));
        st.wMonth = (WORD)std::stoi(date.substr(5, 2));
        st.wDay = (WORD)std::stoi(date.substr(8, 2));
    } catch (...) {
        return false;
    }
    st.wHour = 12;
    return SystemTimeToFileTime(&st, &out) != FALSE;
}

int64_t FileTimeTicks(const FILETIME& ft){
    ULARGE_INTEGER value{};
    value.LowPart = ft.dwLowDateTime;
    value.HighPart = ft.dwHighDateTime;
    return (int64_t)value.QuadPart;
}

bool DateWithinRecentDays(const std::wstring& date, int days){
    FILETIME recordFt{};
    if (!DateKeyToFileTime(date, recordFt)) return false;
    SYSTEMTIME nowSt{};
    FILETIME nowFt{};
    GetLocalTime(&nowSt);
    nowSt.wHour = 12;
    nowSt.wMinute = 0;
    nowSt.wSecond = 0;
    nowSt.wMilliseconds = 0;
    SystemTimeToFileTime(&nowSt, &nowFt);
    constexpr int64_t dayTicks = 864000000000LL;
    int64_t delta = FileTimeTicks(nowFt) - FileTimeTicks(recordFt);
    return delta >= 0 && delta <= (int64_t)std::max(1, days) * dayTicks;
}

bool DateWithinCalendarWeek(const std::wstring& date, const std::wstring& today) {
 FILETIME record{},reference{};
 if(!DateKeyToFileTime(date,record)||!DateKeyToFileTime(today,reference))return false;
 SYSTEMTIME st{};FileTimeToSystemTime(&reference,&st);
 constexpr int64_t day=864000000000LL;
 const auto start=FileTimeTicks(reference)-((st.wDayOfWeek+6)%7)*day;
 return FileTimeTicks(record)>=start && FileTimeTicks(record)<start+7*day;
}

bool DateWithinCurrentMonth(const std::wstring& date){
    FILETIME ignored{};
    if (!DateKeyToFileTime(date, ignored)) return false;
    std::wstring today = TodayDateKey();
    return date.substr(0, 7) == today.substr(0, 7);
}

bool RecordMatchesStatsRange(const AttendanceRecord& record){
    if (Document().statsRange == StatsRange::All) return true;
    std::wstring date = RecordDateKey(record);
    if (Document().statsRange == StatsRange::ThisWeek) return DateWithinCalendarWeek(date, TodayDateKey());
    if (Document().statsRange == StatsRange::ThisMonth) return DateWithinCurrentMonth(date);
    return true;
}

std::vector<std::wstring> SplitWords(const std::wstring& text){
    std::wistringstream ss(text);
    std::vector<std::wstring> words;
    std::wstring word;
    while (ss >> word) words.push_back(word);
    return words;
}

bool RecordMatchesFilter(const AttendanceRecord& record){
    if (!Document().lessonFilter.empty() && record.dateTime != Document().lessonFilter) return false;
    if (Document().filterText.empty()) return true;
    const std::wstring displayStatus = StatusDisplayText(record.status);
    if (!Settings().advancedFilterEnabled) {
        return ContainsText(record.dateTime, Document().filterText)
            || ContainsText(record.name, Document().filterText)
            || ContainsText(record.status, Document().filterText)
            || ContainsText(displayStatus, Document().filterText)
            || ContainsText(record.other, Document().filterText);
    }

    struct ParsedFilter {
        std::wstring text, name, status, course, date, from, to;
        std::vector<std::wstring> freeTerms;
    };
    static ParsedFilter cached;
    if (cached.text != Document().filterText) {
    cached = {};
    cached.text = Document().filterText;

    for (const auto& token : SplitWords(Document().filterText)) {
        auto colon = token.find(L':');
        if (colon == std::wstring::npos) {
            cached.freeTerms.push_back(token);
            continue;
        }
        std::wstring key = token.substr(0, colon);
        std::wstring value = token.substr(colon + 1);
        if (key == L"name") cached.name = value;
        else if (key == L"status") cached.status = value;
        else if (key == L"course") cached.course = value;
        else if (key == L"date") cached.date = value;
        else if (key == L"from") cached.from = value;
        else if (key == L"to") cached.to = value;
        else cached.freeTerms.push_back(token);
    }
    }
    const auto& nameFilter = cached.name;
    const auto& statusFilter = cached.status;
    const auto& courseFilter = cached.course;
    const auto& exactDate = cached.date;
    const auto& fromDate = cached.from;
    const auto& toDate = cached.to;
    const auto& freeTerms = cached.freeTerms;

    if (std::find(freeTerms.begin(), freeTerms.end(), L"thisweek") != freeTerms.end()
        || std::find(freeTerms.begin(), freeTerms.end(), L"week") != freeTerms.end()) {
        if (!DateWithinCalendarWeek(RecordDateKey(record), TodayDateKey())) return false;
    }
    if (std::find(freeTerms.begin(), freeTerms.end(), L"thismonth") != freeTerms.end()
        || std::find(freeTerms.begin(), freeTerms.end(), L"month") != freeTerms.end()) {
        if (!DateWithinCurrentMonth(RecordDateKey(record))) return false;
    }

    std::wstring date = RecordDateKey(record);
    if (!exactDate.empty() && !ContainsText(LowerText(record.dateTime), exactDate)) return false;
    if (!fromDate.empty() && date < fromDate) return false;
    if (!toDate.empty() && date > toDate) return false;
    if (!nameFilter.empty() && !ContainsText(record.name, nameFilter)) return false;
    if (!statusFilter.empty()
        && !ContainsText(record.status, statusFilter)
        && !ContainsText(displayStatus, statusFilter)) return false;
    if (!courseFilter.empty()) {
        if (Document().activeSheet < 0 || Document().activeSheet >= (int)Document().sheets.size() || !ContainsText(Document().sheets[Document().activeSheet].name, courseFilter)) return false;
    }

    std::wstring joinedFree;
    for (const auto& term : freeTerms) {
        if (term == L"thisweek" || term == L"week" || term == L"thismonth" || term == L"month") continue;
        if (!joinedFree.empty()) joinedFree += L" ";
        joinedFree += term;
    }
    if (joinedFree.empty()) return true;
    return ContainsText(record.dateTime, joinedFree)
        || ContainsText(record.name, joinedFree)
        || ContainsText(record.status, joinedFree)
        || ContainsText(displayStatus, joinedFree)
        || ContainsText(record.other, joinedFree);
}

int VisibleToRecordIndex(int visibleIndex){
    if (visibleIndex < 0 || visibleIndex >= (int)Document().visibleRows.size()) return -1;
    int recordIndex = Document().visibleRows[visibleIndex];
    return recordIndex >= 0 && recordIndex < (int)Document().Records().size() ? recordIndex : -1;
}

int RecordToVisibleIndex(int recordIndex){
    const auto& rows = Document().visibleRows;
    auto found = std::lower_bound(rows.begin(), rows.end(), recordIndex);
    return found != rows.end() && *found == recordIndex
        ? static_cast<int>(found - rows.begin()) : -1;
}

std::wstring CurrentDateTimeText(){
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t buffer[64]{};
    swprintf_s(buffer, L"%04d-%02d-%02d %02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
    return buffer;
}

PptStats CountRecords(const std::vector<AttendanceRecord>& records){
    PptStats stats{};
    for (const auto& r : records) {
        if (!RecordMatchesStatsRange(r)) continue;
        AccumulateStatus(stats, r.status);
    }
    return stats;
}

std::vector<std::wstring> CurrentLessonKeys(){
    std::vector<std::wstring> lessons;
    std::unordered_set<std::wstring> seen;
    for (const auto& record : Document().Records()) {
        if (record.dateTime.empty()) continue;
        if (seen.insert(record.dateTime).second) {
            lessons.push_back(record.dateTime);
        }
    }
    std::sort(lessons.begin(), lessons.end(), std::greater<std::wstring>());
    return lessons;
}


} // namespace attendance
