#include "collatz.h"
#include <cstdint>

namespace Collatz {

    uint64_t CollatzSteps(uint64_t n) {
        if (n <= 1) return 0;
        uint64_t steps = 0;
        while (n != 1) {
            if ((n & 1ULL) == 0ULL) n >>= 1;
            else n = 3ULL * n + 1ULL;
            steps++;
        }
        return steps;
    }

    bool CollatzAtLeastT(uint32_t n32, uint32_t T) {
        if (T == 0) return true;
        if (n32 <= 1) return false;

        uint64_t n = n32;
        uint32_t steps = 0;

        while (n != 1) {
            if (steps >= T) return true; // EARLY EXIT

            if ((n & 1ULL) == 0ULL) n >>= 1;
            else n = 3ULL * n + 1ULL;

            steps++;
        }
        return steps >= T;
    }
}
