#pragma once
#include <windows.h>
#include <string>

namespace Timing {
    LARGE_INTEGER NowQpc();
    double ElapsedMicros(LARGE_INTEGER start, LARGE_INTEGER end);
    std::wstring FormatMicros(double micros);

    class PerformanceTimer {
    private:
        LARGE_INTEGER frequency;
        LARGE_INTEGER startTime;
        LARGE_INTEGER endTime;

    public:
        PerformanceTimer();
        void Start();
        void Stop();
        double GetElapsedMilliseconds() const;
        std::wstring GetFormattedTime() const;
    };
}