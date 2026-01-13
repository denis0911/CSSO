#include "collatz.h"
#include <limits>

namespace Collatz {
    uint32_t CollatzSteps(uint32_t x) {
        // Handle edge cases
        if (x == 0) return 0;
        if (x == 1) return 0;
        
        uint64_t n = x;
        uint32_t steps = 0;
        
        // Safety limit to prevent infinite loops or extreme cases
        // If n grows beyond ULLONG_MAX/4, we're in danger of overflow
        const uint64_t SAFETY_LIMIT = ULLONG_MAX / 4;
        
        while (n != 1) {
            // Check for overflow protection
            if (n > SAFETY_LIMIT) {
                // Return sentinel value indicating potential overflow
                return UINT32_MAX;
            }
            
            if (n % 2 == 0) {
                // Even: divide by 2
                n = n / 2;
            } else {
                // Odd: 3n + 1
                // Check if 3*n+1 would overflow
                if (n > (ULLONG_MAX - 1) / 3) {
                    return UINT32_MAX;
                }
                n = 3 * n + 1;
            }
            
            steps++;
            
            // Additional safety: prevent infinite loops
            // The Collatz conjecture suggests this should always terminate,
            // but we add a safety limit
            if (steps >= UINT32_MAX - 1) {
                return UINT32_MAX;
            }
        }
        
        return steps;
    }
    
    // Legacy function - kept for compatibility
    uint64_t ComputeSequenceLength(uint64_t n) {
        if (n == 0) return 0;
        
        uint64_t length = 0;
        const uint64_t SAFETY_LIMIT = ULLONG_MAX / 4;
        
        while (n != 1) {
            if (n > SAFETY_LIMIT) {
                return UINT64_MAX;
            }
            
            if (n % 2 == 0) {
                n = n / 2;
            } else {
                if (n > (ULLONG_MAX - 1) / 3) {
                    return UINT64_MAX;
                }
                n = 3 * n + 1;
            }
            length++;
            
            if (length >= UINT64_MAX - 1) {
                return UINT64_MAX;
            }
        }
        return length;
    }
    
    std::vector<uint64_t> ProcessNumbers(const std::vector<uint64_t>& numbers, int iterations) {
        std::vector<uint64_t> results;
        results.reserve(numbers.size());
        
        for (const auto& num : numbers) {
            // Only run for uint32_t range
            if (num > UINT32_MAX) {
                results.push_back(UINT64_MAX);
                continue;
            }
            
            uint64_t lastResult = 0;
            for (int i = 0; i < iterations; i++) {
                uint32_t steps = CollatzSteps(static_cast<uint32_t>(num));
                lastResult = steps;
            }
            results.push_back(lastResult);
        }
        
        return results;
    }
}