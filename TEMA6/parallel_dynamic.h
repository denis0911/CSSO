#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace ParallelDynamic {
    struct WorkerResult {
        uint32_t workerId;        // Worker index (0-based)
        size_t count;             // Number of values found by this worker
        std::vector<uint32_t> found;  // Values that meet the criteria
    };

    struct ParallelDynamicResult {
        double time_us;           // Computation time in microseconds
        size_t totalCount;        // Total count across all workers
        std::vector<WorkerResult> workerResults;  // Per-worker results
        std::vector<uint32_t> unionSet;  // Combined unique values
    };

    ParallelDynamicResult RunParallelDynamic(
        const uint32_t* v,
        size_t n,
        uint32_t T,
        uint32_t nWorkers
    );
}