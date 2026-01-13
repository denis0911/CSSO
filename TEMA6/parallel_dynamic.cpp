#include "parallel_dynamic.h"
#include "collatz.h"
#include "timing.h"
#include "fileio.h"
#include <windows.h>
#include <process.h>
#include <sstream>
#include <iomanip>
#include <vector>

namespace ParallelDynamic {

    struct Task {
        size_t startIndex;
        size_t endIndex;   // [startIndex, endIndex)
        bool shutdown;
    };

    struct WorkerSync {
        HANDLE requestEvent;   // worker -> coordinator
        HANDLE assignedEvent;  // coordinator -> worker
        Task taskSlot;
    };

    struct CoordinatorState {
        const uint32_t* data;
        size_t n;
        uint32_t T;
        uint32_t nWorkers;
        size_t nextIndex;

        CRITICAL_SECTION cs;
        std::vector<WorkerSync>* sync;

        CoordinatorState() : data(nullptr), n(0), T(0), nWorkers(0), nextIndex(0), sync(nullptr) {
            InitializeCriticalSection(&cs);
        }
        ~CoordinatorState() { DeleteCriticalSection(&cs); }
    };

    struct WorkerData {
        uint32_t workerId;
        CoordinatorState* state;

        uint64_t count = 0;
        std::wstring tempPath;
    };

    static size_t CalculateChunkSize(size_t remaining, uint32_t nWorkers) {
        if (remaining == 0) return 0;
        size_t chunk = remaining / (2ull * nWorkers);
        if (chunk < 1) chunk = 1;
        return chunk;
    }

    static unsigned int __stdcall WorkerThreadProc(void* param) {
        WorkerData* wd = static_cast<WorkerData*>(param);
        CoordinatorState* st = wd->state;

        HANDLE hTmp = CreateFileW(wd->tempPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
            nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
        if (hTmp == INVALID_HANDLE_VALUE) return 0;

        bool first = true;
        wchar_t buf[32];

        WorkerSync& ws = (*st->sync)[wd->workerId];

        for (;;) {
            SetEvent(ws.requestEvent);

            DWORD w = WaitForSingleObject(ws.assignedEvent, INFINITE);
            if (w != WAIT_OBJECT_0) break;

            Task t = ws.taskSlot;
            ResetEvent(ws.assignedEvent);

            if (t.shutdown) break;

            for (size_t i = t.startIndex; i < t.endIndex; i++) {
                uint32_t x = st->data[i];
                if (!Collatz::CollatzAtLeastT(x, st->T)) continue;

                int len = first ? swprintf_s(buf, L"%u", x) : swprintf_s(buf, L",%u", x);
                first = false;

                DWORD bw = 0;
                if (!WriteFile(hTmp, buf, (DWORD)(len * sizeof(wchar_t)), &bw, nullptr)) {
                    // disk fail: stop trying
                    break;
                }
                wd->count++;
            }
        }

        CloseHandle(hTmp);
        return 0;
    }

    struct CoordinatorThreadData {
        CoordinatorState* st;
        std::vector<HANDLE> requestEvents;
    };

    static unsigned int __stdcall CoordinatorThreadProc(void* param) {
        CoordinatorThreadData* cd = static_cast<CoordinatorThreadData*>(param);
        CoordinatorState* st = cd->st;

        uint32_t shutdownIssued = 0;

        for (;;) {
            if (shutdownIssued >= st->nWorkers) break;

            DWORD idx = WaitForMultipleObjects(st->nWorkers, cd->requestEvents.data(), FALSE, INFINITE);
            if (idx < WAIT_OBJECT_0 || idx >= WAIT_OBJECT_0 + st->nWorkers) break;

            uint32_t workerId = (uint32_t)(idx - WAIT_OBJECT_0);
            ResetEvent((*st->sync)[workerId].requestEvent);

            Task task{};
            task.shutdown = false;

            EnterCriticalSection(&st->cs);

            size_t remaining = (st->nextIndex < st->n) ? (st->n - st->nextIndex) : 0;
            if (remaining == 0) {
                task.shutdown = true;
                shutdownIssued++;
            }
            else {
                size_t chunk = CalculateChunkSize(remaining, st->nWorkers);
                task.startIndex = st->nextIndex;
                size_t end = st->nextIndex + chunk;
                if (end > st->n) end = st->n;
                task.endIndex = end;
                st->nextIndex = end;
            }

            (*st->sync)[workerId].taskSlot = task;
            SetEvent((*st->sync)[workerId].assignedEvent);

            LeaveCriticalSection(&st->cs);
        }

        return 0;
    }

    ParallelDynamicResult RunParallelDynamic(const uint32_t* v, size_t n, uint32_t T, uint32_t nWorkers) {
        ParallelDynamicResult result{};
        result.time_us = 0.0;
        result.totalCount = 0;

        if (!v || n == 0 || nWorkers == 0) return result;
        if (nWorkers > n) nWorkers = (uint32_t)n;

        CoordinatorState st;
        st.data = v;
        st.n = n;
        st.T = T;
        st.nWorkers = nWorkers;
        st.nextIndex = 0;

        std::vector<WorkerSync> sync(nWorkers);
        for (uint32_t i = 0; i < nWorkers; i++) {
            sync[i].requestEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
            sync[i].assignedEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
            sync[i].taskSlot = { 0,0,false };
        }
        st.sync = &sync;

        std::vector<WorkerData> wd(nWorkers);
        std::vector<HANDLE> workerHandles(nWorkers);

        for (uint32_t i = 0; i < nWorkers; i++) {
            wd[i].workerId = i;
            wd[i].state = &st;
            wd[i].tempPath = FileIO::MakeTempPath(FileIO::GetDynamicResultsPath(), T, nWorkers, i, L"dyn");
        }

        CoordinatorThreadData cd;
        cd.st = &st;
        cd.requestEvents.reserve(nWorkers);
        for (uint32_t i = 0; i < nWorkers; i++) cd.requestEvents.push_back(sync[i].requestEvent);

        LARGE_INTEGER start = Timing::NowQpc();

        for (uint32_t i = 0; i < nWorkers; i++) {
            workerHandles[i] = (HANDLE)_beginthreadex(nullptr, 0, WorkerThreadProc, &wd[i], 0, nullptr);
            if (!workerHandles[i]) return result;
        }

        HANDLE coordHandle = (HANDLE)_beginthreadex(nullptr, 0, CoordinatorThreadProc, &cd, 0, nullptr);
        if (!coordHandle) return result;

        WaitForSingleObject(coordHandle, INFINITE);
        CloseHandle(coordHandle);

        WaitForMultipleObjects(nWorkers, workerHandles.data(), TRUE, INFINITE);

        LARGE_INTEGER end = Timing::NowQpc();
        result.time_us = Timing::ElapsedMicros(start, end);

        for (uint32_t i = 0; i < nWorkers; i++) CloseHandle(workerHandles[i]);
        for (uint32_t i = 0; i < nWorkers; i++) { CloseHandle(sync[i].requestEvent); CloseHandle(sync[i].assignedEvent); }

        // output file required:
        // ...\rezultate\dinamic\<T>_<nWorker>_<timp>.txt
        std::wstringstream name;
        name << FileIO::GetDynamicResultsPath() << L"\\"
            << T << L"_"
            << nWorkers << L"_"
            << std::fixed << std::setprecision(0) << result.time_us
            << L".txt";
        std::wstring path = name.str();

        std::wstring err;
        HANDLE hOut = INVALID_HANDLE_VALUE;
        if (!FileIO::CreateResultsFileWithAcl(path, hOut, err)) {
            OutputDebugStringW((L"Create dynamic output failed: " + err).c_str());
            for (uint32_t i = 0; i < nWorkers; i++) DeleteFileW(wd[i].tempPath.c_str());
            return result;
        }

        for (uint32_t i = 0; i < nWorkers; i++) {
            result.totalCount += (size_t)wd[i].count;

            wchar_t header[64];
            int hlen = swprintf_s(header, L"%u_%llu:", i, (unsigned long long)wd[i].count);
            FileIO::WriteW(hOut, header, (size_t)hlen, err);

            FileIO::AppendFileToHandle(hOut, wd[i].tempPath, err);
            FileIO::WriteW(hOut, L"\r\n", 2, err);

            DeleteFileW(wd[i].tempPath.c_str());
        }

        CloseHandle(hOut);

        result.workerResults.clear();
        result.unionSet.clear();
        return result;
    }
}
