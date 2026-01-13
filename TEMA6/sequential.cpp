#include "sequential.h"
#include "collatz.h"
#include "timing.h"
#include "fileio.h"
#include <windows.h>
#include <sstream>
#include <iomanip>

namespace Sequential {

    // Writes required format: "<count>:<list>" into final file.
    // Uses placeholder count and overwrites later.
    static bool WriteSequentialStreaming(
        const std::wstring& path,
        const uint32_t* v,
        size_t n,
        uint32_t T,
        uint64_t& outCount,
        std::wstring& err
    ) {
        outCount = 0;
        err.clear();

        HANDLE hFile = INVALID_HANDLE_VALUE;
        if (!FileIO::CreateResultsFileWithAcl(path, hFile, err)) return false;

        // placeholder: 10 digits + ':'
        const wchar_t* prefix = L"0000000000:";
        if (!FileIO::WriteW(hFile, prefix, wcslen(prefix), err)) { CloseHandle(hFile); return false; }

        bool first = true;
        wchar_t buf[32];

        for (size_t i = 0; i < n; i++) {
            uint32_t x = v[i];
            if (!Collatz::CollatzAtLeastT(x, T)) continue;

            int len = first ? swprintf_s(buf, L"%u", x) : swprintf_s(buf, L",%u", x);
            first = false;

            if (len > 0) {
                DWORD bw = 0;
                if (!WriteFile(hFile, buf, (DWORD)(len * sizeof(wchar_t)), &bw, nullptr)) {
                    err = L"WriteFile failed: " + std::to_wstring(GetLastError());
                    CloseHandle(hFile);
                    return false;
                }
            }
            outCount++;
        }

        // overwrite first 10 digits with real count
        LARGE_INTEGER li; li.QuadPart = 0;
        if (!SetFilePointerEx(hFile, li, nullptr, FILE_BEGIN)) {
            err = L"SetFilePointerEx failed: " + std::to_wstring(GetLastError());
            CloseHandle(hFile);
            return false;
        }

        wchar_t countBuf[16];
        swprintf_s(countBuf, L"%010llu", (unsigned long long)outCount);

        DWORD bw2 = 0;
        if (!WriteFile(hFile, countBuf, (DWORD)(10 * sizeof(wchar_t)), &bw2, nullptr)) {
            err = L"WriteFile(count) failed: " + std::to_wstring(GetLastError());
            CloseHandle(hFile);
            return false;
        }

        CloseHandle(hFile);
        return true;
    }

    SequentialResult RunSequential(const uint32_t* v, size_t n, uint32_t T) {
        SequentialResult result{};
        result.count = 0;
        result.time_us = 0.0;
        result.found.clear(); // you can leave it unused

        if (!v || n == 0) return result;

        LARGE_INTEGER start = Timing::NowQpc();

        // Write to temp first, then rename to include measured time in filename
        const std::wstring folder = L"E:\\Facultate\\CSSO\\FinalWeek\\rezultate";
        std::wstring tmp = folder + L"\\__tmp_seq";

        uint64_t count = 0;
        std::wstring err;
        bool ok = WriteSequentialStreaming(tmp, v, n, T, count, err);

        LARGE_INTEGER end = Timing::NowQpc();
        result.time_us = Timing::ElapsedMicros(start, end);
        result.count = (size_t)count;

        // REQUIRED filename (no .txt):
        // C:\Facultate\CSSO\FinalWeek\rezultate\<T>_<timp>_secvential
        std::wstringstream finalName;
        finalName << folder << L"\\"
            << T << L"_"
            << std::fixed << std::setprecision(0) << result.time_us
            << L"_secvential";
        std::wstring finalPath = finalName.str();

        MoveFileExW(tmp.c_str(), finalPath.c_str(), MOVEFILE_REPLACE_EXISTING);

        if (!ok) {
            OutputDebugStringW((L"Sequential write failed: " + err).c_str());
        }

        return result;
    }
}
