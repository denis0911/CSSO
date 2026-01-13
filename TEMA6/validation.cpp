#include "validation.h"
#include <algorithm>
#include <sstream>

namespace Validation {
    ValidationResult ValidateResults(
        const std::vector<uint32_t>& sequentialFound,
        const std::vector<uint32_t>& parallelFoundUnion
    ) {
        ValidationResult result;
        result.passed = false;
        result.sequentialCount = sequentialFound.size();
        result.parallelCount = parallelFoundUnion.size();
        result.missingCount = 0;
        result.extraCount = 0;

        // Convert to sets for efficient comparison
        std::unordered_set<uint32_t> seqSet(sequentialFound.begin(), sequentialFound.end());
        std::unordered_set<uint32_t> parSet(parallelFoundUnion.begin(), parallelFoundUnion.end());

        // Find missing elements (in sequential but not in parallel)
        for (uint32_t val : seqSet) {
            if (parSet.find(val) == parSet.end()) {
                result.missingCount++;
                if (result.missingSamples.size() < 10) {
                    result.missingSamples.push_back(val);
                }
            }
        }

        // Find extra elements (in parallel but not in sequential)
        for (uint32_t val : parSet) {
            if (seqSet.find(val) == seqSet.end()) {
                result.extraCount++;
                if (result.extraSamples.size() < 10) {
                    result.extraSamples.push_back(val);
                }
            }
        }

        // Sort samples for consistent display
        std::sort(result.missingSamples.begin(), result.missingSamples.end());
        std::sort(result.extraSamples.begin(), result.extraSamples.end());

        // Validation passes if sets are equal
        result.passed = (result.missingCount == 0 && result.extraCount == 0);

        return result;
    }

    std::wstring FormatValidationResult(
        const ValidationResult& result,
        const std::wstring& testName
    ) {
        std::wstringstream ss;

        ss << L"Validation: " << testName << L"\r\n";
        ss << L"  Sequential count: " << result.sequentialCount << L"\r\n";
        ss << L"  Parallel count:   " << result.parallelCount << L"\r\n";

        if (result.passed) {
            ss << L"  Status: ✓ PASS - Results match perfectly!\r\n";
        }
        else {
            ss << L"  Status: ✗ FAIL - Results differ!\r\n";

            if (result.missingCount > 0) {
                ss << L"  Missing in parallel: " << result.missingCount << L" values\r\n";
                if (!result.missingSamples.empty()) {
                    ss << L"    Examples: ";
                    for (size_t i = 0; i < result.missingSamples.size(); i++) {
                        if (i > 0) ss << L", ";
                        ss << result.missingSamples[i];
                    }
                    if (result.missingCount > result.missingSamples.size()) {
                        ss << L", ... (+" << (result.missingCount - result.missingSamples.size()) << L" more)";
                    }
                    ss << L"\r\n";
                }
            }

            if (result.extraCount > 0) {
                ss << L"  Extra in parallel: " << result.extraCount << L" values\r\n";
                if (!result.extraSamples.empty()) {
                    ss << L"    Examples: ";
                    for (size_t i = 0; i < result.extraSamples.size(); i++) {
                        if (i > 0) ss << L", ";
                        ss << result.extraSamples[i];
                    }
                    if (result.extraCount > result.extraSamples.size()) {
                        ss << L", ... (+" << (result.extraCount - result.extraSamples.size()) << L" more)";
                    }
                    ss << L"\r\n";
                }
            }
        }

        return ss.str();
    }
}