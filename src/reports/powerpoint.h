#pragma once

#include "core/types.h"

namespace attendance {

int PptStatusBarWidth(int count,int maximum);
void ExportPptx();
std::function<bool()> PreparePptxExport(const std::wstring& path);
bool ExportPptxFile(const std::wstring& path);

} // namespace attendance
