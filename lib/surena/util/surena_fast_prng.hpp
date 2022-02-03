#pragma once

#include <cstdint>

#include "surena_fast_prng.hpp"

// using PCG32 minimal seeded via splitmix64
struct fast_prng {
    uint64_t state;
    uint64_t inc;
    void srand(uint64_t seed);
    uint32_t rand();

    fast_prng(uint64_t seed);
};
