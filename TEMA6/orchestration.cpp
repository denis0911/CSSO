
#include "orchestration.h"
#include "sequential.h"
#include "parallel_static.h"
#include "parallel_dynamic.h"
#include "validation.h"
#include "fileio.h"
#include "timing.h"
#include "sysinfo.h"
#include <process.h>
#include <sstream>
#include <iomanip>
#include <float.h>

namespace Orchestration {
    void LogToUI(HWND hwnd, const std::wstring& message) {
        wchar_t* msg = new wchar_t[message.length() + 1];
        wcscpy_s(msg, message.length() + 1, message.c_str());
        PostMessageW(hwnd, WM_ORCHESTRATION_LOG, 0, reinterpret_cast<LPARAM>(msg));
    }

    uint32_t GetPhysicalCoreCount() {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);

        DWORD length = 0;
        GetLogicalProcessorInformation(nullptr, &length);

        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && length > 0) {
            std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buffer(
                length / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));

            if (GetLogicalProcessorInformation(buffer.data(), &length)) {
                uint32_t physicalCores = 0;
                for (const auto& info : buffer) {
                    if (info.Relationship == RelationProcessorCore) {
                        physicalCores++;
                    }
                }
                return physicalCores > 0 ? physicalCores : sysInfo.dwNumberOfProcessors;
            }
        }

        return sysInfo.dwNumberOfProcessors;
    }

    void UpdateMethodStats(MethodStats& stats, double time, bool validationPassed) {
        if (time < stats.minTime) stats.minTime = time;
        if (time > stats.maxTime) stats.maxTime = time;

        if (validationPassed) {
            stats.validationsPassed++;
        }
        else {
            stats.validationsFailed++;
        }
    }

    unsigned int __stdcall OrchestrationThread(void* param) {
        OrchestrationThreadData* data = static_cast<OrchestrationThreadData*>(param);
        HWND hwnd = data->targetWindow;
        TestConfig config = data->config;
        delete data;

        LogToUI(hwnd, L"========================================\r\nCOMPREHENSIVE TEST SUITE\r\n========================================\r\n\r\n");

        FileIO::MappedFile mf;
        std::wstring err;

        if (!FileIO::MapBinaryUInt32File(config.inputFilePath, mf, err)) {
            LogToUI(hwnd, L"ERROR: Failed to map input file: " + err + L"\r\n");
            PostMessageW(hwnd, WM_ORCHESTRATION_COMPLETE, 0, 0);
            return 1;
        }

        // RAII struct to ensure file is unmapped
        struct FileGuard {
            FileIO::MappedFile& mf;
            explicit FileGuard(FileIO::MappedFile& file) : mf(file) {}
            ~FileGuard() { FileIO::UnmapFile(mf); }
        } fileGuard(mf);

        std::wstringstream info;
        info << L"Input File: " << config.inputFilePath << L"\r\n"
            << L"File size: " << mf.count << L" values\r\n"
            << L"Physical cores (P): " << (config.maxWorkers / 2) << L"\r\n"
            << L"Testing worker counts: 1 to " << config.maxWorkers << L"\r\n\r\n";
        LogToUI(hwnd, info.str());

        TestSummary summary;
        summary.totalTests = 0;
        summary.totalFailures = 0;

        for (uint32_t T : config.tValues) {
            std::wstringstream tHeader;
            tHeader << L"========================================\r\nTesting with T = " << T
                << L"\r\n========================================\r\n\r\n";
            LogToUI(hwnd, tHeader.str());

            LogToUI(hwnd, L"Running Sequential...\r\n");
            Sequential::SequentialResult seqResult = Sequential::RunSequential(mf.data, mf.count, T);

            std::wstringstream seqLog;
            seqLog << L"  Time: " << Timing::FormatMicros(seqResult.time_us) << L"\r\n"
                << L"  Found: " << seqResult.count << L" values\r\n\r\n";
            LogToUI(hwnd, seqLog.str());

            UpdateMethodStats(summary.sequential, seqResult.time_us, true);

            for (uint32_t nWorkers = 1; nWorkers <= config.maxWorkers; nWorkers++) {
                std::wstringstream workerHeader;
                workerHeader << L"Testing with " << nWorkers << L" workers:\r\n----------------------------------\r\n";
                LogToUI(hwnd, workerHeader.str());

                LogToUI(hwnd, L"  Running Parallel Static...\r\n");
                ParallelStatic::ParallelStaticResult staticResult =
                    ParallelStatic::RunParallelStatic(mf.data, mf.count, T, nWorkers);

                std::wstringstream staticLog;
                staticLog << L"    Time: " << Timing::FormatMicros(staticResult.time_us)
                    << L" (Speedup: " << std::fixed << std::setprecision(2)
                    << (seqResult.time_us / staticResult.time_us) << L"x)\r\n"
                    << L"    Found: " << staticResult.totalCount << L" values\r\n";
                LogToUI(hwnd, staticLog.str());

                Validation::ValidationResult staticVal =
                    Validation::ValidateResults(seqResult.found, staticResult.unionSet);

                summary.totalTests++;
                UpdateMethodStats(summary.parallelStatic, staticResult.time_us, staticVal.passed);

                if (staticVal.passed) {
                    LogToUI(hwnd, L"    Validation: ✓ PASS\r\n\r\n");
                }
                else {
                    summary.totalFailures++;
                    std::wstringstream valErr;
                    valErr << L"    Validation: ✗ FAIL (missing: " << staticVal.missingCount
                        << L", extra: " << staticVal.extraCount << L")\r\n\r\n";
                    LogToUI(hwnd, valErr.str());
                }

                LogToUI(hwnd, L"  Running Parallel Dynamic...\r\n");
                ParallelDynamic::ParallelDynamicResult dynamicResult =
                    ParallelDynamic::RunParallelDynamic(mf.data, mf.count, T, nWorkers);

                std::wstringstream dynamicLog;
                dynamicLog << L"    Time: " << Timing::FormatMicros(dynamicResult.time_us)
                    << L" (Speedup: " << std::fixed << std::setprecision(2)
                    << (seqResult.time_us / dynamicResult.time_us) << L"x)\r\n"
                    << L"    Found: " << dynamicResult.totalCount << L" values\r\n";
                LogToUI(hwnd, dynamicLog.str());

                Validation::ValidationResult dynamicVal =
                    Validation::ValidateResults(seqResult.found, dynamicResult.unionSet);

                summary.totalTests++;
                UpdateMethodStats(summary.parallelDynamic, dynamicResult.time_us, dynamicVal.passed);

                if (dynamicVal.passed) {
                    LogToUI(hwnd, L"    Validation: ✓ PASS\r\n\r\n");
                }
                else {
                    summary.totalFailures++;
                    std::wstringstream valErr;
                    valErr << L"    Validation: ✗ FAIL (missing: " << dynamicVal.missingCount
                        << L", extra: " << dynamicVal.extraCount << L")\r\n\r\n";
                    LogToUI(hwnd, valErr.str());
                }

                Sleep(0);
            }

            LogToUI(hwnd, L"\r\n");
        }

        std::wstringstream summaryLog;
        summaryLog << L"========================================\r\nFINAL SUMMARY\r\n========================================\r\n\r\n"
            << L"Total Tests Run: " << summary.totalTests << L"\r\n"
            << L"Total Failures: " << summary.totalFailures << L"\r\n\r\n";

        if (summary.sequential.minTime != DBL_MAX) {
            summaryLog << L"Sequential:\r\n"
                << L"  Min time: " << Timing::FormatMicros(summary.sequential.minTime) << L"\r\n"
                << L"  Max time: " << Timing::FormatMicros(summary.sequential.maxTime) << L"\r\n\r\n";
        }

        summaryLog << L"Parallel Static:\r\n"
            << L"  Min time: " << Timing::FormatMicros(summary.parallelStatic.minTime) << L"\r\n"
            << L"  Max time: " << Timing::FormatMicros(summary.parallelStatic.maxTime) << L"\r\n"
            << L"  Validations passed: " << summary.parallelStatic.validationsPassed << L"\r\n"
            << L"  Validations failed: " << summary.parallelStatic.validationsFailed << L"\r\n\r\n"
            << L"Parallel Dynamic:\r\n"
            << L"  Min time: " << Timing::FormatMicros(summary.parallelDynamic.minTime) << L"\r\n"
            << L"  Max time: " << Timing::FormatMicros(summary.parallelDynamic.maxTime) << L"\r\n"
            << L"  Validations passed: " << summary.parallelDynamic.validationsPassed << L"\r\n"
            << L"  Validations failed: " << summary.parallelDynamic.validationsFailed << L"\r\n\r\n";

        summaryLog << (summary.totalFailures == 0 ? L"✓ ALL TESTS PASSED!\r\n" : L"✗ SOME TESTS FAILED - Review results above\r\n");
        summaryLog << L"\r\n========================================\r\n";
        LogToUI(hwnd, summaryLog.str());

        PostMessageW(hwnd, WM_ORCHESTRATION_COMPLETE, 0, 0);
        return 0;
    }

    void StartOrchestration(HWND targetWindow, const TestConfig& config) {
        OrchestrationThreadData* data = new OrchestrationThreadData();
        data->targetWindow = targetWindow;
        data->config = config;

        HANDLE hThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, OrchestrationThread, data, 0, nullptr));

        if (hThread) {
            CloseHandle(hThread);  // Thread continues running; we don't need the handle
        }
        else {
            delete data;
            MessageBoxW(targetWindow, L"Failed to create orchestration thread", L"Error", MB_OK | MB_ICONERROR);
        }
    }
}