#pragma once
#include <string>
#include <vector>
#include <windows.h>

namespace FileIO {
    struct MappedFile {
        HANDLE hFile = INVALID_HANDLE_VALUE;
        HANDLE hMap = NULL;
        void* view = nullptr;
        const uint32_t* data = nullptr;
        size_t bytes = 0;
        size_t count = 0;
    };

    bool MapBinaryUInt32File(const std::wstring& path, MappedFile& out, std::wstring& err);
    void UnmapFile(MappedFile& mf);
    std::vector<uint64_t> ReadNumbersFromFile(const std::wstring& filepath);
    bool WriteResultsToFile(const std::wstring& filepath, const std::wstring& content);
    bool WriteResultsToFileWithAcl(const std::wstring& filepath, const std::wstring& content, std::wstring& err);
    std::wstring GetResultsFolder();
    bool EnsureResultsFolders(std::wstring& err);
    std::wstring GetStaticResultsPath();
    std::wstring GetDynamicResultsPath();

    // Create directory recursively
    bool CreateDirectoryRecursive(const std::wstring& path, std::wstring& err);

    // Write system info to fixed path
    bool WriteSystemInfoToFile(const std::wstring& content, std::wstring& err);

    bool CreateResultsFileWithAcl(const std::wstring& path, HANDLE& outFile, std::wstring& err);

    // NEW: finalize file (close handle) and apply ACL (if you prefer applying after)
    bool ApplyResultsAcl(const std::wstring& path, std::wstring& err);

    // NEW: append entire file contents (used to merge worker temp files)
    bool AppendFileToHandle(HANDLE hOut, const std::wstring& srcPath, std::wstring& err);

    // NEW: write wide string to file handle (UTF-16)
    bool WriteW(HANDLE h, const wchar_t* s, size_t wcharCount, std::wstring& err);

    // NEW: get temp path inside results folders
    std::wstring MakeTempPath(const std::wstring& folder, uint32_t T, uint32_t nWorkers, uint32_t workerId, const wchar_t* tag);
}