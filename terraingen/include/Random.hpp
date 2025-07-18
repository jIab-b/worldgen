#pragma once

#include <cstdint>

namespace terraingen {

// State for PCG64 random number generator (see implementation.md 4. Random.hpp)
struct PCG64State {
    uint64_t state;
    uint64_t inc;
};

// Initialize PCG64 state with seed and sequence number
PCG64State InitPCG64(uint64_t seed, uint64_t seq = 1);

// Advance state and return next random number
uint64_t PCG64Next(PCG64State& s);

// Jump the RNG state forward
void PCG64Jump(PCG64State& s);

// Hash 3D coordinates into a deterministic 64-bit value
uint64_t HashCoords(int64_t x, int64_t y, int64_t z, uint64_t seed);

} // namespace terraingen 