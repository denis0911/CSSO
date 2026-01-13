#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <windows.h>

namespace Orchestration {
    struct TestConfig {
        std::wstring inputFilePath;
        std::vector<uint32_t> tValues;
        uint32_t maxWorkers;  // 2*P
    };

    struct MethodStats {
        double minTime;
        double maxTime;
        uint32_t validationsFailed;
        uint32_t validationsPassed;

        MethodStats() : minTime(DBL_MAX), maxTime(0.0),
            validationsFailed(0), validationsPassed(0) {
        }
    };

    struct TestSummary {
        MethodStats sequential;
        MethodStats parallelStatic;
        MethodStats parallelDynamic;
        uint32_t totalTests;
        uint32_t totalFailures;
    };

    // Thread parameter structure
    struct OrchestrationThreadData {
        HWND targetWindow;
        TestConfig config;
    };

    // Custom message for UI updates
#define WM_ORCHESTRATION_LOG (WM_USER + 100)
#define WM_ORCHESTRATION_COMPLETE (WM_USER + 101)

// Start orchestration in background thread
    void StartOrchestration(HWND targetWindow, const TestConfig& config);

    // Get physical core count
    uint32_t GetPhysicalCoreCount();
}