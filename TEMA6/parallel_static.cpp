#include "parallel_static.h"
#include "collatz.h"
#include "timing.h"
#include "fileio.h"
#include <windows.h>
#include <process.h>
#include <sstream>
#include <iomanip>
#include <vector>

namespace ParallelStatic {

    struct ThreadData {
        uint32_t workerId;
        const uint32_t* data;
        size_t startIndex;
        size_t endIndex;
        uint32_t threshold;

        uint64_t count = 0;
        std::wstring tempPath;
    };

    static unsigned int __stdcall WorkerThread(void* param) {
        ThreadData* td = static_cast<ThreadData*>(param);

        HANDLE hTmp = CreateFileW(td->tempPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
            nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
        if (hTmp == INVALID_HANDLE_VALUE) return 0;

        bool first = true;
        wchar_t buf[32];

        for (size_t i = td->startIndex; i < td->endIndex; i++) {
            uint32_t x = td->data[i];

            if (!Collatz::CollatzAtLeastT(x, td->threshold)) continue;

            int len = first ? swprintf_s(buf, L"%u", x) : swprintf_s(buf, L",%u", x);
            first = false;

            DWORD bw = 0;
            if (!WriteFile(hTmp, buf, (DWORD)(len * sizeof(wchar_t)), &bw, nullptr)) {
                // ignore, but stop writing further if disk fails
                break;
            }
            td->count++;
        }

        CloseHandle(hTmp);
        return 0;
    }

    ParallelStaticResult RunParallelStatic(const uint32_t* v, size_t n, uint32_t T, uint32_t nWorkers) {
        ParallelStaticResult result{};
        result.time_us = 0.0;
        result.totalCount = 0;

        if (!v || n == 0 || nWorkers == 0) return result;
        if (nWorkers > n) nWorkers = (uint32_t)n;

        std::vector<ThreadData> td(nWorkers);
        std::vector<HANDLE> th(nWorkers);

        // static distribution (as required)
        size_t base = n / nWorkers;
        size_t rem = n % nWorkers;

        size_t cur = 0;
        for (uint32_t i = 0; i < nWorkers; i++) {
            td[i].workerId = i;
            td[i].data = v;
            td[i].threshold = T;
            td[i].startIndex = cur;

            size_t chunk = base + (i < rem ? 1 : 0);
            td[i].endIndex = cur + chunk;
            cur = td[i].endIndex;

            td[i].tempPath = FileIO::MakeTempPath(FileIO::GetStaticResultsPath(), T, nWorkers, i, L"static");
        }

        LARGE_INTEGER start = Timing::NowQpc();

        for (uint32_t i = 0; i < nWorkers; i++) {
            th[i] = (HANDLE)_beginthreadex(nullptr, 0, WorkerThread, &td[i], 0, nullptr);
            if (!th[i]) {
                for (uint32_t j = 0; j < i; j++) { WaitForSingleObject(th[j], INFINITE); CloseHandle(th[j]); }
                return result;
            }
        }

        WaitForMultipleObjects(nWorkers, th.data(), TRUE, INFINITE);

        LARGE_INTEGER end = Timing::NowQpc();
        result.time_us = Timing::ElapsedMicros(start, end);

        for (uint32_t i = 0; i < nWorkers; i++) CloseHandle(th[i]);

        // output file required:
        // ...\rezultate\static\<T>_<nWorker>_<timp>.txt
        std::wstringstream name;
        name << FileIO::GetStaticResultsPath() << L"\\"
            << T << L"_"
            << nWorkers << L"_"
            << std::fixed << std::setprecision(0) << result.time_us
            << L".txt";
        std::wstring path = name.str();

        std::wstring err;
        HANDLE hOut = INVALID_HANDLE_VALUE;
        if (!FileIO::CreateResultsFileWithAcl(path, hOut, err)) {
            OutputDebugStringW((L"Create static output failed: " + err).c_str());
            // cleanup temp
            for (uint32_t i = 0; i < nWorkers; i++) DeleteFileW(td[i].tempPath.c_str());
            return result;
        }

        for (uint32_t i = 0; i < nWorkers; i++) {
            result.totalCount += (size_t)td[i].count;

            wchar_t header[64];
            int hlen = swprintf_s(header, L"%u_%llu:", i, (unsigned long long)td[i].count);
            FileIO::WriteW(hOut, header, (size_t)hlen, err);

            // append list
            FileIO::AppendFileToHandle(hOut, td[i].tempPath, err);

            // newline
            FileIO::WriteW(hOut, L"\r\n", 2, err);

            DeleteFileW(td[i].tempPath.c_str());
        }

        CloseHandle(hOut);

        // NOTE: result.workerResults / unionSet are no longer meaningful for huge outputs.
        // Keep them empty; validation will use external compare (see validation changes section).
        result.workerResults.clear();
        result.unionSet.clear();

        return result;
    }
}
