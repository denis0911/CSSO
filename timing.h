#pragma once
#include <windows.h>
#include <string>
#include <cstdint>

namespace Timing {
    class PerformanceTimer {
    public:
        PerformanceTimer();
        void Start();
        void Stop();
        double GetElapsedMilliseconds() const;
        std::wstring GetFormattedTime() const;
        
    private:
        LARGE_INTEGER frequency;
        LARGE_INTEGER startTime;
        LARGE_INTEGER endTime;
    };
    
    // High-precision QPC-based timing functions
    int64_t NowQpc();
    int64_t ElapsedMicros(int64_t start, int64_t end);
    std::wstring FormatMicros(int64_t us);
}