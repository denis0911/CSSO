#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <unordered_set>

namespace Validation {
    struct ValidationResult {
        bool passed;
        size_t sequentialCount;
        size_t parallelCount;
        size_t missingCount;      // In sequential but not in parallel
        size_t extraCount;        // In parallel but not in sequential
        std::vector<uint32_t> missingSamples;   // Up to 10 examples
        std::vector<uint32_t> extraSamples;     // Up to 10 examples
    };

    ValidationResult ValidateResults(
        const std::vector<uint32_t>& sequentialFound,
        const std::vector<uint32_t>& parallelFoundUnion
    );

    std::wstring FormatValidationResult(
        const ValidationResult& result,
        const std::wstring& testName
    );
}