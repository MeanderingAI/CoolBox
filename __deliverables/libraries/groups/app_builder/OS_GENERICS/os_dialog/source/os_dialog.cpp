#include "os_dialog.hpp"

#include <algorithm>
#include <filesystem>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commdlg.h>
#include <objbase.h>
#include <shlobj.h>
#endif

namespace {

using app_builder::os_generics::DialogAction;
using app_builder::os_generics::DialogRequest;
using app_builder::os_generics::DialogResult;

DialogResult fallback_show(const DialogRequest& request) {
    DialogResult result;
    const std::filesystem::path initial(request.initial_path);

    switch (request.action) {
        case DialogAction::OpenFile:
        case DialogAction::OpenFiles: {
            if (!request.initial_path.empty() && std::filesystem::exists(initial)) {
                result.accepted = true;
                result.selected_paths.push_back(request.initial_path);
                result.message = "Selected path from initial_path";
            } else {
                result.accepted = false;
                result.message = "No native dialog backend configured; provide a valid initial_path";
            }
            break;
        }
        case DialogAction::SaveFile: {
            if (!request.initial_path.empty()) {
                std::filesystem::path output_dir(request.initial_path);
                if (std::filesystem::is_regular_file(output_dir)) {
                    output_dir = output_dir.parent_path();
                }
                if (!output_dir.empty() && !std::filesystem::exists(output_dir)) {
                    result.accepted = false;
                    result.message = "Save directory does not exist";
                    break;
                }
                const std::filesystem::path output = output_dir / request.suggested_name;
                result.accepted = true;
                result.selected_paths.push_back(output.string());
                result.message = "Save target resolved";
            } else {
                result.accepted = true;
                result.selected_paths.push_back(request.suggested_name);
                result.message = "Save target resolved";
            }
            break;
        }
        case DialogAction::PickFolder: {
            if (!request.initial_path.empty() &&
                std::filesystem::exists(initial) &&
                std::filesystem::is_directory(initial)) {
                result.accepted = true;
                result.selected_paths.push_back(request.initial_path);
                result.message = "Folder selected from initial_path";
            } else {
                result.accepted = false;
                result.message = "No native dialog backend configured; provide an existing folder path";
            }
            break;
        }
    }

    return result;
}

#ifdef _WIN32

std::wstring utf8_to_wide(const std::string& input) {
    if (input.empty()) {
        return std::wstring();
    }
    const int size = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, nullptr, 0);
    if (size <= 0) {
        return std::wstring();
    }
    std::wstring output(static_cast<std::size_t>(size - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, output.data(), size);
    return output;
}

std::string wide_to_utf8(const std::wstring& input) {
    if (input.empty()) {
        return std::string();
    }
    const int size = WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
        return std::string();
    }
    std::string output(static_cast<std::size_t>(size - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, output.data(), size, nullptr, nullptr);
    return output;
}

std::wstring build_filter_string(const std::vector<app_builder::os_generics::DialogFilter>& filters) {
    std::wstring filter;
    if (filters.empty()) {
        filter += L"All Files";
        filter.push_back(L'\0');
        filter += L"*.*";
        filter.push_back(L'\0');
        filter.push_back(L'\0');
        return filter;
    }

    for (const auto& f : filters) {
        filter += utf8_to_wide(f.label.empty() ? "Files" : f.label);
        filter.push_back(L'\0');

        std::wstring pattern;
        if (f.patterns.empty()) {
            pattern = L"*.*";
        } else {
            for (std::size_t i = 0; i < f.patterns.size(); ++i) {
                if (i > 0U) {
                    pattern += L";";
                }
                pattern += utf8_to_wide(f.patterns[i]);
            }
        }

        filter += pattern;
        filter.push_back(L'\0');
    }

    filter.push_back(L'\0');
    return filter;
}

DialogResult show_native_win32(const DialogRequest& request) {
    DialogResult result;

    std::wstring title = utf8_to_wide(request.title);
    std::wstring initial_path = utf8_to_wide(request.initial_path);
    std::wstring suggested_name = utf8_to_wide(request.suggested_name);
    std::wstring filter = build_filter_string(request.filters);

    if (request.action == DialogAction::OpenFile || request.action == DialogAction::OpenFiles) {
        std::vector<wchar_t> buffer(65536, L'\0');

        OPENFILENAMEW ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFilter = filter.c_str();
        ofn.lpstrFile = buffer.data();
        ofn.nMaxFile = static_cast<DWORD>(buffer.size());
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
        if (request.action == DialogAction::OpenFiles) {
            ofn.Flags |= OFN_ALLOWMULTISELECT;
        }
        if (!title.empty()) {
            ofn.lpstrTitle = title.c_str();
        }
        if (!initial_path.empty()) {
            ofn.lpstrInitialDir = initial_path.c_str();
        }

        if (!GetOpenFileNameW(&ofn)) {
            result.accepted = false;
            result.message = "Dialog canceled";
            return result;
        }

        result.accepted = true;
        if (request.action == DialogAction::OpenFile) {
            result.selected_paths.push_back(wide_to_utf8(std::wstring(buffer.data())));
            result.message = "File selected";
            return result;
        }

        const std::wstring first = std::wstring(buffer.data());
        const wchar_t* cursor = buffer.data() + first.size() + 1;
        if (*cursor == L'\0') {
            result.selected_paths.push_back(wide_to_utf8(first));
            result.message = "File selected";
            return result;
        }

        while (*cursor != L'\0') {
            std::filesystem::path p(first);
            p /= cursor;
            result.selected_paths.push_back(wide_to_utf8(p.wstring()));
            cursor += std::wcslen(cursor) + 1;
        }
        result.message = "Files selected";
        return result;
    }

    if (request.action == DialogAction::SaveFile) {
        std::vector<wchar_t> buffer(65536, L'\0');
        if (!suggested_name.empty()) {
            const std::size_t copy_size = std::min(suggested_name.size(), buffer.size() - 1U);
            std::copy_n(suggested_name.data(), copy_size, buffer.data());
            buffer[copy_size] = L'\0';
        }

        OPENFILENAMEW ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFilter = filter.c_str();
        ofn.lpstrFile = buffer.data();
        ofn.nMaxFile = static_cast<DWORD>(buffer.size());
        ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
        if (!title.empty()) {
            ofn.lpstrTitle = title.c_str();
        }
        if (!initial_path.empty()) {
            ofn.lpstrInitialDir = initial_path.c_str();
        }

        if (!GetSaveFileNameW(&ofn)) {
            result.accepted = false;
            result.message = "Dialog canceled";
            return result;
        }

        result.accepted = true;
        result.selected_paths.push_back(wide_to_utf8(std::wstring(buffer.data())));
        result.message = "Save target selected";
        return result;
    }

    if (request.action == DialogAction::PickFolder) {
        BROWSEINFOW bi{};
        std::wstring title_copy = title.empty() ? L"Select folder" : title;
        bi.lpszTitle = title_copy.c_str();
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

        PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
        if (pidl == nullptr) {
            result.accepted = false;
            result.message = "Dialog canceled";
            return result;
        }

        wchar_t path[MAX_PATH] = {0};
        const BOOL ok = SHGetPathFromIDListW(pidl, path);
        CoTaskMemFree(pidl);

        if (!ok) {
            result.accepted = false;
            result.message = "Failed to resolve selected folder";
            return result;
        }

        result.accepted = true;
        result.selected_paths.push_back(wide_to_utf8(std::wstring(path)));
        result.message = "Folder selected";
        return result;
    }

    result.accepted = false;
    result.message = "Unsupported dialog action";
    return result;
}

#endif

} // namespace

namespace app_builder {
namespace os_generics {

DialogResult OsDialog::show(const DialogRequest& request) {
    if (!is_dialog_request_valid(request)) {
        return DialogResult{false, {}, "Invalid dialog request"};
    }

#ifdef _WIN32
    if (request.allow_native_ui) {
        return show_native_win32(request);
    }
#endif

    return fallback_show(request);
}

} // namespace os_generics
} // namespace app_builder
