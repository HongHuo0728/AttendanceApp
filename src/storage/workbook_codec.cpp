#include "core/status.h"
#include "storage/workbook_codec.h"
#include "core/encoding.h"
#include "model/workbook.h"
#include "model/state.h"
#include "core/constants.h"

namespace attendance {

bool ParseSizeLineStrict(const std::string& line, size_t& value);
bool ParseNonNegativeIntLineStrict(const std::string& line, int& value);
bool StreamHasOnlyTrailingWhitespace(std::istream& stream);
bool DeserializeRecords(const std::string& plainText, std::vector<AttendanceRecord>& output);
uint32_t Rotl(uint32_t value, int bits);
std::string XorCrypt(std::string data);
std::string EncodeAttd(std::string plainText);

class ReadOnlyBuffer : public std::streambuf {
public:
    explicit ReadOnlyBuffer(const std::string& input) {
        auto* begin = const_cast<char*>(input.data());
        setg(begin, begin, begin + input.size());
    }
};

bool WorkbookFieldIsValid(const std::wstring& value, size_t maximumBytes){
    if (value.empty()) return true;
    if (value.size() > maximumBytes || value.size() > INT_MAX) return false;
    const int bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
        value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    return bytes > 0 && static_cast<size_t>(bytes) <= maximumBytes;
}

WorkbookValidationIssue ValidateWorkbookModel(const std::vector<AttendanceSheet>& sheets){
    if (sheets.size() > MAX_SHEET_COUNT) return WorkbookValidationIssue::TooManySheets;
    size_t totalStudents = 0;
    size_t totalRecords = 0;
    size_t serializedBytes = 40;
    auto validField = [&](const std::wstring& value, size_t limit) {
        if (!WorkbookFieldIsValid(value, limit)) return false;
        const int bytes = value.empty() ? 0 : WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
            value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
        size_t escapedBytes = static_cast<size_t>(bytes) + 1;
        for (wchar_t ch : value) if (ch == L'\\' || ch == L'\t' || ch == L'\r' || ch == L'\n') ++escapedBytes;
        if (escapedBytes > MAX_ATTD_DECODED_BYTES - serializedBytes) return false;
        serializedBytes += escapedBytes;
        return true;
    };
    for (const auto& sheet : sheets) {
        if (serializedBytes > MAX_ATTD_DECODED_BYTES - 32) return WorkbookValidationIssue::FieldTooLarge;
        serializedBytes += 32;
        if (sheet.students.size() > MAX_STUDENTS_PER_SHEET
            || sheet.students.size() > MAX_TOTAL_STUDENTS - totalStudents) {
            return WorkbookValidationIssue::TooManyStudents;
        }
        if (sheet.records.size() > MAX_RECORDS_PER_SHEET
            || sheet.records.size() > MAX_TOTAL_RECORDS - totalRecords) {
            return WorkbookValidationIssue::TooManyRecords;
        }
        totalStudents += sheet.students.size();
        totalRecords += sheet.records.size();
        if (!validField(sheet.name, MAX_METADATA_FIELD_BYTES)
            || !validField(sheet.teacher, MAX_METADATA_FIELD_BYTES)
            || !validField(sheet.location, MAX_METADATA_FIELD_BYTES)
            || !validField(sheet.notes, MAX_METADATA_FIELD_BYTES)) {
            return WorkbookValidationIssue::FieldTooLarge;
        }
        for (const auto& student : sheet.students) {
            if (!validField(student, MAX_RECORD_FIELD_BYTES)) {
                return WorkbookValidationIssue::FieldTooLarge;
            }
        }
        for (const auto& record : sheet.records) {
            if (!validField(record.dateTime, MAX_RECORD_FIELD_BYTES)
                || !validField(record.name, MAX_RECORD_FIELD_BYTES)
                || !validField(record.status, MAX_RECORD_FIELD_BYTES)
                || !validField(record.other, MAX_RECORD_FIELD_BYTES)) {
                return WorkbookValidationIssue::FieldTooLarge;
            }
        }
    }
    return WorkbookValidationIssue::None;
}

bool ParseSizeLineStrict(const std::string& line, size_t& value){
    if (line.empty()) return false;
    uint64_t parsed = 0;
    const char* begin = line.data();
    const char* end = begin + line.size();
    auto result = std::from_chars(begin, end, parsed, 10);
    if (result.ec != std::errc{} || result.ptr != end
        || parsed > static_cast<uint64_t>(SIZE_MAX)) {
        return false;
    }
    value = static_cast<size_t>(parsed);
    return true;
}

bool ParseNonNegativeIntLineStrict(const std::string& line, int& value){
    size_t parsed = 0;
    if (!ParseSizeLineStrict(line, parsed) || parsed > static_cast<size_t>(INT_MAX)) {
        return false;
    }
    value = static_cast<int>(parsed);
    return true;
}

bool StreamHasOnlyTrailingWhitespace(std::istream& stream){
    char ch = 0;
    while (stream.get(ch)) {
        if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n'
            && ch != '\f' && ch != '\v') {
            return false;
        }
    }
    return true;
}

bool DeserializeRecords(const std::string& plainText, std::vector<AttendanceRecord>& output){
    ReadOnlyBuffer buffer(plainText);
    std::istream ss(&buffer);
    std::string header;
    std::getline(ss, header);
    if (header != "ATTENDANCE_V1") return false;

    std::string countLine;
    if (!std::getline(ss, countLine)) return false;
    size_t count = 0;
    if (!ParseSizeLineStrict(countLine, count)) return false;
    if (count > MAX_RECORDS_PER_SHEET || count > plainText.size() / 4) return false;

    std::vector<AttendanceRecord> records;
    records.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        std::string line;
        if (!std::getline(ss, line)) return false;
        if (line.size() > MAX_RECORD_FIELD_BYTES * 8 + 3) return false;
        std::array<std::string_view, 4> parts{};
        if (!SplitRecordFields(line, parts)) return false;
        AttendanceRecord record;
        if (!DecodeEscapedUtf8Field(parts[0], MAX_RECORD_FIELD_BYTES, record.dateTime)
            || !DecodeEscapedUtf8Field(parts[1], MAX_RECORD_FIELD_BYTES, record.name)
            || !DecodeEscapedUtf8Field(parts[2], MAX_RECORD_FIELD_BYTES, record.status)
            || !DecodeEscapedUtf8Field(parts[3], MAX_RECORD_FIELD_BYTES, record.other)) {
            return false;
        }
        records.push_back(std::move(record));
    }

    if (!StreamHasOnlyTrailingWhitespace(ss)) return false;

    output = std::move(records);
    return true;
}

std::string SerializeWorkbookData(const std::vector<AttendanceSheet>& sheets,int active,int version){
    std::string text;
    size_t estimate = 64;
    for (const auto& sheet : sheets) estimate += sheet.records.size() * 64 + sheet.students.size() * 24;
    text.reserve(std::min(estimate, MAX_ATTD_DECODED_BYTES));
    text += version==5?"ATTENDANCE_V5\n":"ATTENDANCE_V6\n";
    text += std::to_string(active) + '\n';
    text += std::to_string(sheets.size()) + '\n';
    auto append = [&](const std::wstring& value, char separator = '\n') {
        text += Escape(WideToUtf8(value));
        text += separator;
    };
    for (const auto& sheet : sheets) {
        append(sheet.name);
        append(sheet.teacher);
        append(sheet.location);
        append(sheet.notes);
        text += std::to_string(sheet.students.size()) + '\n';
        for (const auto& student : sheet.students) {
            append(student);
        }
        text += std::to_string(sheet.records.size()) + '\n';
        for (const auto& record : sheet.records) {
            append(record.dateTime, '\t');
            append(record.name, '\t');
            append(record.status, '\t');
            append(record.other);
        }
    }
    return text;
}

std::string SerializeWorkbook(){SyncActiveSheet();return SerializeWorkbookData(Document().sheets,Document().activeSheet);}

bool DeserializeWorkbook(const std::string& plainText, std::vector<AttendanceSheet>& output){
    if (plainText.size() > MAX_ATTD_DECODED_BYTES) return false;
    ReadOnlyBuffer buffer(plainText);
    std::istream ss(&buffer);
    std::string header;
    std::getline(ss, header);
    int loadedActiveSheet = 0;
    if (header == "ATTENDANCE_V1") {
        std::vector<AttendanceRecord> records;
        if (!DeserializeRecords(plainText, records)) return false;
        output = {MakeSheet(L"Default Course", std::move(records))};
        Document().loadedActiveSheet = 0;
        return true;
    }
    if (header != "ATTENDANCE_V2" && header != "ATTENDANCE_V3" && header != "ATTENDANCE_V4" && header != "ATTENDANCE_V5" && header != "ATTENDANCE_V6") return false;

    std::string countLine;
    if (header == "ATTENDANCE_V3" || header == "ATTENDANCE_V4" || header == "ATTENDANCE_V5" || header == "ATTENDANCE_V6") {
        std::string activeLine;
        if (!std::getline(ss, activeLine)
            || !ParseNonNegativeIntLineStrict(activeLine, loadedActiveSheet)) return false;
    }
    if (!std::getline(ss, countLine)) return false;
    size_t sheetCount = 0;
    if (!ParseSizeLineStrict(countLine, sheetCount)) return false;
    if (sheetCount > MAX_SHEET_COUNT || sheetCount > plainText.size() / 3) return false;

    std::vector<AttendanceSheet> sheets;
    sheets.reserve(sheetCount);
    size_t totalStudents = 0;
    size_t totalRecords = 0;
    for (size_t s = 0; s < sheetCount; ++s) {
        std::string nameLine;
        std::string recordCountLine;
        if (!std::getline(ss, nameLine)) return false;

        AttendanceSheet sheet;
        if (!DecodeEscapedUtf8Field(nameLine, MAX_METADATA_FIELD_BYTES, sheet.name)) return false;
        if (sheet.name.empty()) sheet.name = L"Course";
        if (header == "ATTENDANCE_V4" || header == "ATTENDANCE_V5" || header == "ATTENDANCE_V6") {
            std::string teacherLine;
            std::string locationLine;
            std::string notesLine;
            std::string studentCountLine;
            if (!std::getline(ss, teacherLine)) return false;
            if (!std::getline(ss, locationLine)) return false;
            if (!std::getline(ss, notesLine)) return false;
            if (!std::getline(ss, studentCountLine)) return false;
            if (!DecodeEscapedUtf8Field(teacherLine, MAX_METADATA_FIELD_BYTES, sheet.teacher)
                || !DecodeEscapedUtf8Field(locationLine, MAX_METADATA_FIELD_BYTES, sheet.location)
                || !DecodeEscapedUtf8Field(notesLine, MAX_METADATA_FIELD_BYTES, sheet.notes)) {
                return false;
            }
            size_t studentCount = 0;
            if (!ParseSizeLineStrict(studentCountLine, studentCount)) return false;
            if (studentCount > plainText.size() || studentCount > MAX_STUDENTS_PER_SHEET
                || studentCount > MAX_TOTAL_STUDENTS - totalStudents) return false;
            totalStudents += studentCount;
            sheet.students.reserve(studentCount);
            for (size_t i = 0; i < studentCount; ++i) {
                std::string studentLine;
                if (!std::getline(ss, studentLine)) return false;
                std::wstring student;
                if (!DecodeEscapedUtf8Field(studentLine, MAX_RECORD_FIELD_BYTES, student)) return false;
                if (!student.empty()) sheet.students.push_back(student);
            }
        }
        if (!std::getline(ss, recordCountLine)) return false;
        size_t recordCount = 0;
        if (!ParseSizeLineStrict(recordCountLine, recordCount)) return false;
        if (recordCount > plainText.size() / 4 || recordCount > MAX_RECORDS_PER_SHEET || recordCount > MAX_TOTAL_RECORDS - totalRecords) return false;
        totalRecords += recordCount;
        sheet.records.reserve(recordCount);
        for (size_t i = 0; i < recordCount; ++i) {
            std::string line;
            if (!std::getline(ss, line)) return false;
            if (line.size() > MAX_RECORD_FIELD_BYTES * 8 + 3) return false;
            std::array<std::string_view, 4> parts{};
            if (!SplitRecordFields(line, parts)) return false;
            AttendanceRecord record;
            if (!DecodeEscapedUtf8Field(parts[0], MAX_RECORD_FIELD_BYTES, record.dateTime)
                || !DecodeEscapedUtf8Field(parts[1], MAX_RECORD_FIELD_BYTES, record.name)
                || !DecodeEscapedUtf8Field(parts[2], MAX_RECORD_FIELD_BYTES, record.status)
                || !DecodeEscapedUtf8Field(parts[3], MAX_RECORD_FIELD_BYTES, record.other)) {
                return false;
            }
            sheet.records.push_back(std::move(record));
        }
        sheets.push_back(std::move(sheet));
    }

    if (!StreamHasOnlyTrailingWhitespace(ss)) return false;
    if (sheets.empty()) sheets.push_back(MakeSheet(L"Default Course"));
    if (loadedActiveSheet < 0 || loadedActiveSheet >= static_cast<int>(sheets.size())) return false;
    output = std::move(sheets);
    Document().loadedActiveSheet = loadedActiveSheet;
    return true;
}

uint32_t Rotl(uint32_t value, int bits){
    return (value << bits) | (value >> (32 - bits));
}

std::string XorCrypt(std::string data){
    uint32_t state = 0xA77D2026u;
    const std::string key = "AttendanceProject.attd.CustomKey";
    std::string& out = data;
    for (size_t i = 0; i < out.size(); ++i) {
        state ^= (uint8_t)key[i % key.size()] + (uint32_t)i * 2654435761u;
        state = Rotl(state, 7) * 1664525u + 1013904223u;
        out[i] = (char)((uint8_t)out[i] ^ (uint8_t)(state >> 16));
    }
    return data;
}

std::string Base64Encode(const std::string& input){
    static const char* table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string output;
    if (input.size() > (SIZE_MAX - 2) / 3) return {};
    output.reserve(((input.size() + 2) / 3) * 4);
    for (size_t i = 0; i < input.size(); i += 3) {
        uint32_t block = static_cast<uint8_t>(input[i]) << 16;
        const size_t remaining = input.size() - i;
        if (remaining > 1) block |= static_cast<uint8_t>(input[i + 1]) << 8;
        if (remaining > 2) block |= static_cast<uint8_t>(input[i + 2]);
        output.push_back(table[(block >> 18) & 0x3F]);
        output.push_back(table[(block >> 12) & 0x3F]);
        output.push_back(remaining > 1 ? table[(block >> 6) & 0x3F] : '=');
        output.push_back(remaining > 2 ? table[block & 0x3F] : '=');
    }
    return output;
}

bool Base64Decode(std::string_view input, std::string& output){
    static const auto map = [] {
        std::array<int, 256> result{};
        result.fill(-1);
        const char* table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        for (int i = 0; i < 64; ++i) result[static_cast<uint8_t>(table[i])] = i;
        return result;
    }();

    output.clear();
    if (input.empty()) return true;
    if (input.size() > MAX_ATTD_FILE_BYTES) return false;
    output.reserve(std::min(MAX_ATTD_DECODED_BYTES, input.size() * 3 / 4));
    uint32_t val = 0;
    int valb = -8;
    size_t symbolCount = 0;
    size_t dataSymbolCount = 0;
    size_t paddingCount = 0;
    bool paddingStarted = false;
    for (uint8_t ch : input) {
        if (ch == '\r' || ch == '\n' || ch == ' ' || ch == '\t') continue;
        ++symbolCount;
        if (ch == '=') {
            paddingStarted = true;
            if (++paddingCount > 2) return false;
            continue;
        }
        if (paddingStarted) return false;
        if (map[ch] == -1) {
            return false;
        }
        ++dataSymbolCount;
        val = (val << 6) | static_cast<uint32_t>(map[ch]);
        valb += 6;
        if (valb >= 0) {
            if (output.size() >= MAX_ATTD_DECODED_BYTES) return false;
            output.push_back((char)((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    if (symbolCount == 0 || symbolCount % 4 != 0 || dataSymbolCount % 4 == 1) return false;
    if ((paddingCount == 0 && dataSymbolCount % 4 != 0)
        || (paddingCount == 1 && dataSymbolCount % 4 != 3)
        || (paddingCount == 2 && dataSymbolCount % 4 != 2)) {
        output.clear();
        return false;
    }
    return true;
}

std::string EncodeAttd(std::string plainText){
    return "ATTD1:" + Base64Encode(XorCrypt(std::move(plainText)));
}

bool DecodeAttd(const std::string& fileText, std::string& plainText){
    const std::string prefix = "ATTD1:";
    if (fileText.rfind(prefix, 0) != 0) return false;
    std::string encrypted;
    if (!Base64Decode(std::string_view(fileText).substr(prefix.size()), encrypted)) return false;
    if (encrypted.size() > MAX_ATTD_DECODED_BYTES) return false;
    plainText = XorCrypt(std::move(encrypted));
    return true;
}

bool EncodeCurrentWorkbook(std::string& encoded){
    SyncActiveSheet();
    if (ValidateWorkbookModel(Document().sheets) != WorkbookValidationIssue::None) return false;
    std::string plainText = SerializeWorkbook();
    if (plainText.size() > MAX_ATTD_DECODED_BYTES) return false;
    encoded = EncodeAttd(std::move(plainText));
    return encoded.size() <= MAX_ATTD_FILE_BYTES;
}


bool EncodeWorkbookData(const std::vector<AttendanceSheet>& sheets,int active,std::string& output,int version){
 output.clear();if(sheets.empty()||active<0||static_cast<size_t>(active)>=sheets.size()||(version!=5&&version!=6)||ValidateWorkbookModel(sheets)!=WorkbookValidationIssue::None)return false;
 auto plain=SerializeWorkbookData(sheets,active,version);if(plain.size()>MAX_ATTD_DECODED_BYTES)return false;
 output=EncodeAttd(std::move(plain));return output.size()<=MAX_ATTD_FILE_BYTES;
}
bool EncodeV5Compatible(std::string& output){
 auto sheets=Document().sheets;
 for(auto& sheet:sheets)for(auto& r:sheet.records)if(r.status==L"Leave"||r.status==L"Pending"){
  r.other=L"[2.1.0 status: "+r.status+L"] "+r.other;r.status=L"Other";
 }
 return EncodeWorkbookData(sheets,Document().activeSheet,output,5);
}
} // namespace attendance
