#include "timing.h"
#include <windows.h>
#include <sstream>
#include <iomanip>
#include <cstdint>

namespace Timing {
    static int64_t g_qpcFrequency = 0;

    static int64_t GetQpcFrequency() {
        if (g_qpcFrequency == 0) {
            LARGE_INTEGER freq;
            QueryPerformanceFrequency(&freq);
            g_qpcFrequency = freq.QuadPart;
        }
        return g_qpcFrequency;
    }

    LARGE_INTEGER NowQpc() {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return now;
    }

    double ElapsedMicros(LARGE_INTEGER start, LARGE_INTEGER end) {
        int64_t freq = GetQpcFrequency();
        if (freq == 0) return 0.0;

        int64_t elapsed = end.QuadPart - start.QuadPart;
        const int64_t MICROSECONDS_PER_SECOND = 1000000LL;

        if (elapsed > (INT64_MAX / MICROSECONDS_PER_SECOND)) {
            return static_cast<double>(elapsed / freq) * MICROSECONDS_PER_SECOND;
        }
        else {
            return static_cast<double>(elapsed * MICROSECONDS_PER_SECOND) / freq;
        }
    }

    std::wstring FormatMicros(double us) {
        std::wstringstream ss;

        if (us < 1000.0) {
            ss << std::fixed << std::setprecision(1) << us << L" μs";
        }
        else if (us < 1000000.0) {
            double ms = us / 1000.0;
            ss << std::fixed << std::setprecision(3) << ms << L" ms";
        }
        else if (us < 60000000.0) {
            double sec = us / 1000000.0;
            ss << std::fixed << std::setprecision(3) << sec << L" s";
        }
        else {
            int64_t minutes = static_cast<int64_t>(us / 60000000.0);
            int64_t seconds = static_cast<int64_t>((us - minutes * 60000000.0) / 1000000.0);
            ss << minutes << L" min " << seconds << L" s";
        }

        return ss.str();
    }

    PerformanceTimer::PerformanceTimer() {
        QueryPerformanceFrequency(&frequency);
        startTime.QuadPart = 0;
        endTime.QuadPart = 0;
    }

    void PerformanceTimer::Start() {
        QueryPerformanceCounter(&startTime);
    }

    void PerformanceTimer::Stop() {
        QueryPerformanceCounter(&endTime);
    }

    double PerformanceTimer::GetElapsedMilliseconds() const {
        if (frequency.QuadPart == 0) return 0.0;

        double elapsed = static_cast<double>(endTime.QuadPart - startTime.QuadPart);
        return (elapsed * 1000.0) / static_cast<double>(frequency.QuadPart);
    }

    std::wstring PerformanceTimer::GetFormattedTime() const {
        double ms = GetElapsedMilliseconds();
        std::wstringstream ss;
        ss << std::fixed << std::setprecision(3) << ms << L" ms";
        return ss.str();
    }
}