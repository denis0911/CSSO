#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace Sequential {
    struct SequentialResult {
        double time_us;           // Computation time in microseconds
        size_t count;             // Number of values found
        std::vector<uint32_t> found;  // Values that meet the criteria
    };

    SequentialResult RunSequential(const uint32_t* v, size_t n, uint32_t T);
}