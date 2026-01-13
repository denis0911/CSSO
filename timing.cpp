#include "timing.h"
#include <sstream>
#include <iomanip>

namespace Timing {
    // Static frequency cache for performance
    static int64_t g_qpcFrequency = 0;
    
    // Initialize QPC frequency on first use
    static int64_t GetQpcFrequency() {
        if (g_qpcFrequency == 0) {
            LARGE_INTEGER freq;
            QueryPerformanceFrequency(&freq);
            g_qpcFrequency = freq.QuadPart;
        }
        return g_qpcFrequency;
    }
    
    // Get current QPC timestamp
    int64_t NowQpc() {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return now.QuadPart;
    }
    
    // Calculate elapsed time in microseconds between two QPC timestamps
    int64_t ElapsedMicros(int64_t start, int64_t end) {
        int64_t freq = GetQpcFrequency();
        if (freq == 0) {
            return 0;
        }
        
        int64_t elapsed = end - start;
        
        // Convert QPC ticks to microseconds
        // microseconds = (elapsed * 1000000) / frequency
        // To avoid overflow, we check if we can multiply first
        const int64_t MICROSECONDS_PER_SECOND = 1000000LL;
        
        // Check if direct multiplication would overflow
        if (elapsed > (INT64_MAX / MICROSECONDS_PER_SECOND)) {
            // Do division first to avoid overflow
            return (elapsed / freq) * MICROSECONDS_PER_SECOND;
        } else {
            // Multiply first for better precision
            return (elapsed * MICROSECONDS_PER_SECOND) / freq;
        }
    }
    
    // Format microseconds as human-readable string
    std::wstring FormatMicros(int64_t us) {
        std::wstringstream ss;
        
        if (us < 1000) {
            // Less than 1 ms: show as microseconds
            ss << us << L" μs";
        } else if (us < 1000000) {
            // Less than 1 second: show as milliseconds
            double ms = us / 1000.0;
            ss << std::fixed << std::setprecision(3) << ms << L" ms";
        } else if (us < 60000000) {
            // Less than 1 minute: show as seconds
            double sec = us / 1000000.0;
            ss << std::fixed << std::setprecision(3) << sec << L" s";
        } else {
            // 1 minute or more: show as minutes and seconds
            int64_t minutes = us / 60000000;
            int64_t seconds = (us % 60000000) / 1000000;
            double remainder = ((us % 60000000) % 1000000) / 1000000.0;
            ss << minutes << L" min " << seconds;
            if (remainder > 0.001) {
                ss << std::fixed << std::setprecision(3) << remainder;
            }
            ss << L" s";
        }
        
        return ss.str();
    }
    
    // PerformanceTimer class implementation (existing code)
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