#pragma once

#include "core/types.h"

namespace attendance {

bool WorkbookFieldIsValid(const std::wstring& value, size_t maximumBytes);
WorkbookValidationIssue ValidateWorkbookModel(const std::vector<AttendanceSheet>& sheets);
std::string SerializeWorkbook();
std::string SerializeWorkbookData(const std::vector<AttendanceSheet>& sheets,int active,int version=6);
bool EncodeWorkbookData(const std::vector<AttendanceSheet>& sheets,int active,std::string& output,int version=6);
bool EncodeV5Compatible(std::string& output);
bool DeserializeWorkbook(const std::string& plainText, std::vector<AttendanceSheet>& output);
std::string Base64Encode(const std::string& input);
bool Base64Decode(std::string_view input, std::string& output);
bool DecodeAttd(const std::string& fileText, std::string& plainText);
bool EncodeCurrentWorkbook(std::string& encoded);

} // namespace attendance
