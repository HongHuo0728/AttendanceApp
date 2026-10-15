#include "tests/parser_regression.h"
#include "core/text.h"
#include "settings/preferences.h"
#include "core/encoding.h"
#include "storage/workbook_codec.h"
#include "model/workbook.h"
#include "core/constants.h"

namespace attendance {



bool RunParserRegressionChecks(){
    if (NormalizeSingleLineText(L"Alice\r\nBob\nCharlie") != L"Alice Bob Charlie"
        || NormalizeSingleLineText(L"Search term\r\n") != L"Search term"
        || ClampSettingInt("1junk", 2, 0, 3) != 2
        || ClampSettingInt("999", 2, 0, 3) != 3
        || ParseBoolSetting("true", false)
        || !ParseBoolSetting("invalid", true)) {
        return false;
    }
    const std::vector<std::string> base64Samples = {
        std::string(), std::string("f"), std::string("fo"), std::string("foo"),
        std::string("\0\x01\x7f\xff", 4)
    };
    for (const auto& sample : base64Samples) {
        std::string decoded;
        if (!Base64Decode(Base64Encode(sample), decoded) || decoded != sample) return false;
    }
    std::vector<AttendanceSheet> oversizedSheets(MAX_SHEET_COUNT + 1, MakeSheet(L"Course"));
    if (ValidateWorkbookModel(oversizedSheets) != WorkbookValidationIssue::TooManySheets) return false;
    std::wstring escapedRoundTrip(40000, L'\\');
    std::wstring decodedEscaped;
    std::string encodedEscaped = Escape(WideToUtf8(escapedRoundTrip));
    if (!WorkbookFieldIsValid(escapedRoundTrip, MAX_RECORD_FIELD_BYTES)
        || !DecodeEscapedUtf8Field(encodedEscaped, MAX_RECORD_FIELD_BYTES, decodedEscaped)
        || decodedEscaped != escapedRoundTrip) return false;
    const std::string validWorkbook =
        "ATTENDANCE_V5\n0\n1\nCourse\nTeacher\nRoom\nNotes\n1\nStudent\n"
        "1\n2026-08-19 09:00\tStudent\tPresent\t\n";
    std::vector<AttendanceSheet> parsed;
    if (!DeserializeWorkbook(validWorkbook, parsed)
        || parsed.size() != 1 || parsed[0].records.size() != 1) {
        return false;
    }

    const std::vector<std::string> invalidWorkbooks = {
        "ATTENDANCE_V5\n-1\n1\nCourse\nTeacher\nRoom\nNotes\n0\n0\n",
        "ATTENDANCE_V5\n0\n1junk\n",
        "ATTENDANCE_V5\n0\n1\nCourse\nTeacher\nRoom\nNotes\n1junk\n",
        "ATTENDANCE_V5\n0\n1\nCourse\nTeacher\nRoom\nNotes\n0\n1junk\n",
        validWorkbook + "unexpected-data",
        "ATTENDANCE_V1\n1junk\n"
    };
    for (const std::string& invalid : invalidWorkbooks) {
        std::vector<AttendanceSheet> unchanged = {MakeSheet(L"Sentinel")};
        if (DeserializeWorkbook(invalid, unchanged)
            || unchanged.size() != 1 || unchanged[0].name != L"Sentinel") {
            return false;
        }
    }

    parsed.clear();
    return DeserializeWorkbook(validWorkbook + " \t\r\n", parsed)
        && parsed.size() == 1 && parsed[0].records.size() == 1;
}

} // namespace attendance
