#include "fileio.h"
#include <windows.h>
#include "security.h"
#include <vector>
#include <sstream>

using namespace std;

extern std::wstring GetLastErrorMessage(DWORD err);

namespace FileIO {
    static const std::wstring kBaseFolder = L"E:\\Facultate\\CSSO\\FinalWeek";
    static const std::wstring kResultsFolder = kBaseFolder + L"\\rezultate";
    static const std::wstring kInfoFilePath = kBaseFolder + L"\\info.txt";

    bool MapBinaryUInt32File(const std::wstring& path, MappedFile& out, std::wstring& err) {
        out = MappedFile();
        err.clear();

        out.hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (out.hFile == INVALID_HANDLE_VALUE) {
            err = L"Failed to open file: " + GetLastErrorMessage(GetLastError());
            return false;
        }

        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(out.hFile, &fileSize)) {
            err = L"Failed to get file size: " + GetLastErrorMessage(GetLastError());
            CloseHandle(out.hFile);
            out.hFile = INVALID_HANDLE_VALUE;
            return false;
        }

        if (fileSize.QuadPart == 0) {
            err = L"File is empty";
            CloseHandle(out.hFile);
            out.hFile = INVALID_HANDLE_VALUE;
            return false;
        }

        if (fileSize.QuadPart > SIZE_MAX) {
            err = L"File is too large";
            CloseHandle(out.hFile);
            out.hFile = INVALID_HANDLE_VALUE;
            return false;
        }

        out.bytes = static_cast<size_t>(fileSize.QuadPart);

        if (out.bytes % sizeof(uint32_t) != 0) {
            std::wstringstream ss;
            ss << L"File size (" << out.bytes << L" bytes) is not a multiple of 4";
            err = ss.str();
            CloseHandle(out.hFile);
            out.hFile = INVALID_HANDLE_VALUE;
            return false;
        }

        out.hMap = CreateFileMappingW(out.hFile, NULL, PAGE_READONLY, 0, 0, NULL);
        if (out.hMap == NULL) {
            err = L"Failed to create file mapping: " + GetLastErrorMessage(GetLastError());
            CloseHandle(out.hFile);
            out.hFile = INVALID_HANDLE_VALUE;
            return false;
        }

        out.view = MapViewOfFile(out.hMap, FILE_MAP_READ, 0, 0, out.bytes);
        if (out.view == nullptr) {
            err = L"Failed to map view: " + GetLastErrorMessage(GetLastError());
            CloseHandle(out.hMap);
            CloseHandle(out.hFile);
            out.hMap = NULL;
            out.hFile = INVALID_HANDLE_VALUE;
            return false;
        }

        out.data = static_cast<const uint32_t*>(out.view);
        out.count = out.bytes / sizeof(uint32_t);
        return true;
    }

    void UnmapFile(MappedFile& mf) {
        if (mf.view != nullptr) {
            UnmapViewOfFile(mf.view);
            mf.view = nullptr;
            mf.data = nullptr;
        }
        if (mf.hMap != NULL) {
            CloseHandle(mf.hMap);
            mf.hMap = NULL;
        }
        if (mf.hFile != INVALID_HANDLE_VALUE) {
            CloseHandle(mf.hFile);
            mf.hFile = INVALID_HANDLE_VALUE;
        }
        mf.bytes = 0;
        mf.count = 0;
    }

    std::vector<uint64_t> ReadNumbersFromFile(const std::wstring& filepath) {
        std::vector<uint64_t> numbers;
        MappedFile mf;
        std::wstring err;
        if (MapBinaryUInt32File(filepath, mf, err)) {
            numbers.reserve(mf.count);
            for (size_t i = 0; i < mf.count; i++) {
                numbers.push_back(static_cast<uint64_t>(mf.data[i]));
            }
            UnmapFile(mf);
        }
        return numbers;
    }

    bool WriteResultsToFile(const std::wstring& filepath, const std::wstring& content) {
        HANDLE hFile = CreateFileW(filepath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) return false;

        int utf8Size = WideCharToMultiByte(CP_UTF8, 0, content.c_str(), -1, NULL, 0, NULL, NULL);
        if (utf8Size == 0) {
            CloseHandle(hFile);
            return false;
        }

        std::vector<char> utf8Buffer(utf8Size);
        WideCharToMultiByte(CP_UTF8, 0, content.c_str(), -1, utf8Buffer.data(), utf8Size, NULL, NULL);

        DWORD bytesWritten;
        BOOL result = WriteFile(hFile, utf8Buffer.data(), utf8Size - 1, &bytesWritten, NULL);
        CloseHandle(hFile);
        return result != FALSE;
    }

    std::wstring GetResultsFolder() {
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(NULL, path, MAX_PATH);
        std::wstring exePath(path);
        size_t pos = exePath.find_last_of(L"\\/");
        return exePath.substr(0, pos) + L"\\Results";
    }

    bool CreateDirectoryRecursive(const std::wstring& path, std::wstring& err) {
        err.clear();

        // Check if already exists
        DWORD attrs = GetFileAttributesW(path.c_str());
        if (attrs != INVALID_FILE_ATTRIBUTES) {
            if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
                return true; // Already exists
            }
            else {
                err = L"Path exists but is not a directory: " + path;
                return false;
            }
        }

        // Find parent directory
        size_t lastSlash = path.find_last_of(L"\\/");
        if (lastSlash != std::wstring::npos && lastSlash > 0) {
            std::wstring parent = path.substr(0, lastSlash);

            // Skip drive letter (e.g., "C:")
            if (parent.length() > 2 || (parent.length() == 2 && parent[1] != L':')) {
                if (!CreateDirectoryRecursive(parent, err)) {
                    return false;
                }
            }
        }

        // Create this directory
        if (!CreateDirectoryW(path.c_str(), NULL)) {
            DWORD error = GetLastError();
            if (error != ERROR_ALREADY_EXISTS) {
                err = L"Failed to create directory '" + path + L"': " + GetLastErrorMessage(error);
                return false;
            }
        }

        return true;
    }

    bool WriteSystemInfoToFile(const std::wstring& content, std::wstring& err) {
        err.clear();

        // Fixed path
        const std::wstring fixedPath = kInfoFilePath;


        // Extract directory path
        size_t lastSlash = fixedPath.find_last_of(L"\\/");
        if (lastSlash == std::wstring::npos) {
            err = L"Invalid file path";
            return false;
        }

        std::wstring dirPath = fixedPath.substr(0, lastSlash);

        // Create directories recursively
        if (!CreateDirectoryRecursive(dirPath, err)) {
            return false;
        }

        // Create and write to file using CreateFile + WriteFile
        HANDLE hFile = CreateFileW(
            fixedPath.c_str(),
            GENERIC_WRITE,
            0,  // No sharing
            NULL,
            CREATE_ALWAYS,  // Overwrite if exists
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        if (hFile == INVALID_HANDLE_VALUE) {
            err = L"Failed to create file: " + GetLastErrorMessage(GetLastError());
            return false;
        }

        // Convert to UTF-8 for consistent encoding
        int utf8Size = WideCharToMultiByte(CP_UTF8, 0, content.c_str(), -1, NULL, 0, NULL, NULL);
        if (utf8Size == 0) {
            err = L"Failed to convert to UTF-8: " + GetLastErrorMessage(GetLastError());
            CloseHandle(hFile);
            return false;
        }

        std::vector<char> utf8Buffer(utf8Size);
        WideCharToMultiByte(CP_UTF8, 0, content.c_str(), -1, utf8Buffer.data(), utf8Size, NULL, NULL);

        // Write UTF-8 BOM for better compatibility
        const unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
        DWORD bytesWritten;
        if (!WriteFile(hFile, bom, sizeof(bom), &bytesWritten, NULL)) {
            err = L"Failed to write BOM: " + GetLastErrorMessage(GetLastError());
            CloseHandle(hFile);
            return false;
        }

        // Write content (exclude null terminator)
        if (!WriteFile(hFile, utf8Buffer.data(), utf8Size - 1, &bytesWritten, NULL)) {
            err = L"Failed to write content: " + GetLastErrorMessage(GetLastError());
            CloseHandle(hFile);
            return false;
        }

        CloseHandle(hFile);
        return true;
    }

    std::wstring GetStaticResultsPath() {
        return kResultsFolder + L"\\static";

    }

    std::wstring GetDynamicResultsPath() {
        return kResultsFolder + L"\\dinamic";

    }

    bool EnsureResultsFolders(std::wstring& err) {
        err.clear();

        // Base results folder
       const std::wstring baseResultsPath = kResultsFolder;


        // Create base results folder
        if (!CreateDirectoryRecursive(baseResultsPath, err)) {
            return false;
        }

        // Create static subfolder
        std::wstring staticPath = GetStaticResultsPath();
        if (!CreateDirectoryW(staticPath.c_str(), NULL)) {
            DWORD error = GetLastError();
            if (error != ERROR_ALREADY_EXISTS) {
                err = L"Failed to create static folder: " + GetLastErrorMessage(error);
                return false;
            }
        }

        // Create dinamic subfolder
        std::wstring dynamicPath = GetDynamicResultsPath();
        if (!CreateDirectoryW(dynamicPath.c_str(), NULL)) {
            DWORD error = GetLastError();
            if (error != ERROR_ALREADY_EXISTS) {
                err = L"Failed to create dinamic folder: " + GetLastErrorMessage(error);
                return false;
            }
        }

        return true;
    }

    bool WriteResultsToFileWithAcl(const std::wstring& filepath, const std::wstring& content, std::wstring& err) {
        err.clear();

        // First, write the file normally
        HANDLE hFile = CreateFileW(
            filepath.c_str(),
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        if (hFile == INVALID_HANDLE_VALUE) {
            err = L"Failed to create file: " + GetLastErrorMessage(GetLastError());
            return false;
        }

        // Convert to UTF-8
        int utf8Size = WideCharToMultiByte(CP_UTF8, 0, content.c_str(), -1, NULL, 0, NULL, NULL);
        if (utf8Size == 0) {
            err = L"Failed to convert to UTF-8: " + GetLastErrorMessage(GetLastError());
            CloseHandle(hFile);
            return false;
        }

        std::vector<char> utf8Buffer(utf8Size);
        WideCharToMultiByte(CP_UTF8, 0, content.c_str(), -1, utf8Buffer.data(), utf8Size, NULL, NULL);

        // Write UTF-8 BOM
        const unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
        DWORD bytesWritten;
        if (!WriteFile(hFile, bom, sizeof(bom), &bytesWritten, NULL)) {
            err = L"Failed to write BOM: " + GetLastErrorMessage(GetLastError());
            CloseHandle(hFile);
            return false;
        }

        // Write content
        if (!WriteFile(hFile, utf8Buffer.data(), utf8Size - 1, &bytesWritten, NULL)) {
            err = L"Failed to write content: " + GetLastErrorMessage(GetLastError());
            CloseHandle(hFile);
            return false;
        }

        CloseHandle(hFile);

        // Now apply ACL
        PSID currentUserSid = Security::GetCurrentUserSidBinary();
        PSID everyoneSid = Security::GetEveryoneSidBinary();

        if (!currentUserSid || !everyoneSid) {
            if (currentUserSid) Security::FreeSidBinary(currentUserSid);
            if (everyoneSid) Security::FreeSidBinary(everyoneSid);
            err = L"Failed to get SIDs for ACL";
            return false;
        }

        bool aclResult = Security::ApplyResultsFileAcl(filepath, currentUserSid, everyoneSid, err);

        // Clean up SIDs
        Security::FreeSidBinary(currentUserSid);
        Security::FreeSidBinary(everyoneSid);

        return aclResult;
    }
   
    bool ApplyResultsAcl(const std::wstring& path, std::wstring& err) {
        return Security::ApplyResultsFileAcl(path, err);
    }

    bool CreateResultsFileWithAcl(const std::wstring& path, HANDLE& outFile, std::wstring& err) {
        err.clear();
        outFile = CreateFileW(
            path.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );

        if (outFile == INVALID_HANDLE_VALUE) {
            err = L"CreateFileW failed: " + std::to_wstring(GetLastError());
            return false;
        }

        // apply ACL immediately (safe, file exists)
        std::wstring aclErr;
        if (!Security::ApplyResultsFileAcl(path, aclErr)) {
            // Close and fail: requirement wants ACL correct
            CloseHandle(outFile);
            outFile = INVALID_HANDLE_VALUE;
            err = L"ApplyResultsFileAcl failed: " + aclErr;
            return false;
        }

        return true;
    }

    bool WriteW(HANDLE h, const wchar_t* s, size_t wcharCount, std::wstring& err) {
        err.clear();
        DWORD bw = 0;
        const DWORD bytes = (DWORD)(wcharCount * sizeof(wchar_t));
        if (!WriteFile(h, s, bytes, &bw, nullptr) || bw != bytes) {
            err = L"WriteFile failed: " + std::to_wstring(GetLastError());
            return false;
        }
        return true;
    }

    bool AppendFileToHandle(HANDLE hOut, const std::wstring& srcPath, std::wstring& err) {
        err.clear();

        HANDLE hIn = CreateFileW(srcPath.c_str(), GENERIC_READ, FILE_SHARE_READ,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hIn == INVALID_HANDLE_VALUE) {
            err = L"Open temp failed: " + std::to_wstring(GetLastError());
            return false;
        }

        const DWORD BUF_BYTES = 1 << 20; // 1MB
        std::vector<char> buf(BUF_BYTES);

        for (;;) {
            DWORD br = 0;
            if (!ReadFile(hIn, buf.data(), BUF_BYTES, &br, nullptr)) {
                err = L"ReadFile failed: " + std::to_wstring(GetLastError());
                CloseHandle(hIn);
                return false;
            }
            if (br == 0) break;

            DWORD bw = 0;
            if (!WriteFile(hOut, buf.data(), br, &bw, nullptr) || bw != br) {
                err = L"WriteFile merge failed: " + std::to_wstring(GetLastError());
                CloseHandle(hIn);
                return false;
            }
        }

        CloseHandle(hIn);
        return true;
    }

    std::wstring MakeTempPath(const std::wstring& folder, uint32_t T, uint32_t nWorkers, uint32_t workerId, const wchar_t* tag) {
        DWORD pid = GetCurrentProcessId();
        std::wstring p = folder;
        p += L"\\tmp_";
        p += std::to_wstring(pid);
        p += L"_";
        p += tag;
        p += L"_T";
        p += std::to_wstring(T);
        p += L"_W";
        p += std::to_wstring(nWorkers);
        p += L"_";
        p += std::to_wstring(workerId);
        p += L".tmp";
        return p;
    }
}