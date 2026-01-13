#include "security.h"
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

namespace Security {
    bool ValidateFilePath(const wchar_t* path) {
        if (!path || wcslen(path) == 0) {
            return false;
        }
        
        // Check if path exists
        return PathFileExistsW(path) != FALSE;
    }
    
    bool CreateSecureDirectory(const wchar_t* path) {
        return CreateDirectoryW(path, NULL) != FALSE || GetLastError() == ERROR_ALREADY_EXISTS;
    }
}