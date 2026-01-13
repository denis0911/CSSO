#pragma once
#include <vector>
#include <cstdint>

namespace Collatz {
    // Single number Collatz computation
    uint32_t CollatzSteps(uint32_t x);
    
    // Legacy functions
    uint64_t ComputeSequenceLength(uint64_t n);
    std::vector<uint64_t> ProcessNumbers(const std::vector<uint64_t>& numbers, int iterations);
}