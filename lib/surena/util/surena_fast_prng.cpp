#include <cstdint>

#include "surena_fast_prng.hpp"

void fast_prng::srand(uint64_t seed)
{
    // splitmix pre-seed
    uint64_t z = (seed + UINT64_C(0x9E3779B97F4A7C15));
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    state =  z ^ (z >> 31);
    inc = 1;
}

uint32_t fast_prng::rand()
{
    uint64_t oldstate = state;
    // Advance internal state
    state = oldstate * 6364136223846793005ULL + (inc|1);
    // Calculate output function (XSH RR), uses old state for max ILP
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

fast_prng::fast_prng(uint64_t seed)
{
    this->srand(seed);
}
