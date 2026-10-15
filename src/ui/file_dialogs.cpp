#include "ui/file_dialogs.h"
#include "localization/strings.h"
#include "settings/state.h"
#include "ui/state.h"
#include "core/constants.h"

namespace attendance {

std::wstring ModernFileDialog(bool save, const std::wstring& defaultName, const std::wstring& defaultExtension,
                              const std::vector<std::pair<std::wstring, std::wstring>>& entries);

std::wstring ModernFileDialog(bool save, const std::wstring& defaultName, const std::wstring& defaultExtension,
                              const std::vector<std::pair<std::wstring, std::wstring>>& entries){
    IFileDialog* dialog = nullptr;
    const CLSID& classId = save ? CLSID_FileSaveDialog : CLSID_FileOpenDialog;
    HRESULT hr = CoCreateInstance(classId, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
    if (FAILED(hr) || !dialog) return {};

    DWORD options = 0;
    dialog->GetOptions(&options);
    options |= FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST;
    options |= save ? FOS_OVERWRITEPROMPT : FOS_FILEMUSTEXIST;
    dialog->SetOptions(options);

    std::vector<std::wstring> names;
    names.reserve(entries.size());
    for (const auto& entry : entries) names.push_back(entry.first + L" (" + entry.second + L")");
    std::vector<COMDLG_FILTERSPEC> filters;
    filters.reserve(entries.size());
    for (size_t i = 0; i < entries.size(); ++i) filters.push_back({names[i].c_str(), entries[i].second.c_str()});
    if (!filters.empty()) dialog->SetFileTypes(static_cast<UINT>(filters.size()), filters.data());
    if (!defaultExtension.empty()) dialog->SetDefaultExtension(defaultExtension.c_str());
    if (!defaultName.empty()) dialog->SetFileName(defaultName.c_str());

    if (save && !Settings().defaultSaveDir.empty()) {
        IShellItem* folder = nullptr;
        if (SUCCEEDED(SHCreateItemFromParsingName(Settings().defaultSaveDir.c_str(), nullptr, IID_PPV_ARGS(&folder)))) {
            dialog->SetFolder(folder);
            folder->Release();
        }
    }

    std::wstring path;
    if (SUCCEEDED(dialog->Show(Ui().hwnd))) {
        IShellItem* result = nullptr;
        if (SUCCEEDED(dialog->GetResult(&result)) && result) {
            PWSTR value = nullptr;
            if (SUCCEEDED(result->GetDisplayName(SIGDN_FILESYSPATH, &value)) && value) {
                path = value;
                CoTaskMemFree(value);
            }
            result->Release();
        }
    }
    dialog->Release();
    return path;
}

std::wstring SaveFileDialog(){
    return ModernFileDialog(true, L"attendance.attd", L"attd", {
        {Tr(L"Attendance Files", L"\u70b9\u540d\u6587\u4ef6"), L"*.attd"},
        {Tr(L"All Files", L"\u6240\u6709\u6587\u4ef6"), L"*.*"}
    });
}

std::wstring SaveCsvFileDialog(){
    return ModernFileDialog(true, L"attendance.csv", L"csv", {
        {Tr(L"CSV Files", L"CSV \u6587\u4ef6"), L"*.csv"},
        {Tr(L"All Files", L"\u6240\u6709\u6587\u4ef6"), L"*.*"}
    });
}

std::wstring OpenFileDialog(){
    return ModernFileDialog(false, L"", L"attd", {
        {Tr(L"Attendance Files", L"\u70b9\u540d\u6587\u4ef6"), L"*.attd"},
        {Tr(L"All Files", L"\u6240\u6709\u6587\u4ef6"), L"*.*"}
    });
}

std::wstring OpenCsvFileDialog(){
    return ModernFileDialog(false, L"", L"csv", {
        {Tr(L"CSV Files", L"CSV \u6587\u4ef6"), L"*.csv"},
        {Tr(L"Text Files", L"\u6587\u672c\u6587\u4ef6"), L"*.txt"},
        {Tr(L"All Files", L"\u6240\u6709\u6587\u4ef6"), L"*.*"}
    });
}

std::wstring SaveHtmlFileDialog(){
    return ModernFileDialog(true, L"attendance-print.html", L"html", {
        {Tr(L"HTML Files", L"HTML \u6587\u4ef6"), L"*.html"},
        {Tr(L"All Files", L"\u6240\u6709\u6587\u4ef6"), L"*.*"}
    });
}

std::wstring SavePptxFileDialog(){
    return ModernFileDialog(true, L"attendance-report.pptx", L"pptx", {
        {Tr(L"PowerPoint Files", L"PowerPoint \u6587\u4ef6"), L"*.pptx"},
        {Tr(L"All Files", L"\u6240\u6709\u6587\u4ef6"), L"*.*"}
    });
}

std::wstring ChooseFolderDialog(){
    IFileDialog* dialog = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog))) || !dialog) return {};
    DWORD options = 0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
    std::wstring title = Tr(L"Set default save folder", L"\u8bbe\u7f6e\u9ed8\u8ba4\u4fdd\u5b58\u6587\u4ef6\u5939");
    dialog->SetTitle(title.c_str());
    std::wstring path;
    if (SUCCEEDED(dialog->Show(Ui().hwnd))) {
        IShellItem* result = nullptr;
        if (SUCCEEDED(dialog->GetResult(&result)) && result) {
            PWSTR value = nullptr;
            if (SUCCEEDED(result->GetDisplayName(SIGDN_FILESYSPATH, &value)) && value) {
                path = value;
                CoTaskMemFree(value);
            }
            result->Release();
        }
    }
    dialog->Release();
    return path;
}

} // namespace attendance
