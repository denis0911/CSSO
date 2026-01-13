#pragma once
#include <windows.h>

namespace Security {
    bool ValidateFilePath(const wchar_t* path);
    bool CreateSecureDirectory(const wchar_t* path);
}