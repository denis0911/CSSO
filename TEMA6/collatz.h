#pragma once
#include <cstdint>

namespace Collatz {
    // Full stopping time (kept for unit tests if you want)
    uint64_t CollatzSteps(uint64_t n);

    // FAST requirement: check only if Collatz length >= T, early-exit
    bool CollatzAtLeastT(uint32_t n, uint32_t T);
}
