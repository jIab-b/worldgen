#include "Random.hpp"
#include <cstdint>

namespace terraingen {

// Multiplier and increment from PCG paper (LCG 64-bit)
static constexpr uint64_t PCG64_MULT = 6364136223846793005ull;
static constexpr uint64_t PCG64_INC  = 1442695040888963407ull; // odd required

// Rotate right helper
static inline uint64_t rotr64(uint64_t value, unsigned int rot) {
    return (value >> rot) | (value << ((64 - rot) & 63));
}

PCG64State InitPCG64(uint64_t seed, uint64_t seq) {
    // Ensure increment is odd
    PCG64State s{0u, (seq << 1u) | 1u};
    // Advance once with seed, then add seed
    s.state = 0u;
    PCG64Next(s);
    s.state += seed;
    PCG64Next(s);
    return s;
}

uint64_t PCG64Next(PCG64State& s) {
    uint64_t oldstate = s.state;
    // Advance internal state
    s.state = oldstate * PCG64_MULT + (s.inc | 1ull);

    // Output function (XSL RR 64/32 from PCG)
    uint64_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint64_t rot = oldstate >> 59u;
    uint32_t hi32 = static_cast<uint32_t>((xorshifted >> rot) | (xorshifted << ((-rot) & 31u)));

    // Generate second 32-bit half to build 64-bit output
    oldstate = s.state;
    s.state = oldstate * PCG64_MULT + (s.inc | 1ull);
    xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    rot = oldstate >> 59u;
    uint32_t lo32 = static_cast<uint32_t>((xorshifted >> rot) | (xorshifted << ((-rot) & 31u)));

    return (static_cast<uint64_t>(hi32) << 32) | lo32;
}

void PCG64Jump(PCG64State& s) {
    // Jump ahead 2^64 steps using exponentiation by squaring (simple version)
    uint64_t cur_mult = PCG64_MULT;
    uint64_t cur_inc  = s.inc;
    uint64_t acc_mult = 1ull;
    uint64_t acc_inc  = 0ull;

    uint64_t delta = 0x8000000000000000ull; // 2^63 (arbitrary large step)
    while (delta > 0) {
        if (delta & 1ull) {
            acc_mult *= cur_mult;
            acc_inc   = acc_inc * cur_mult + cur_inc;
        }
        cur_inc *= (cur_mult + 1ull);
        cur_mult *= cur_mult;
        delta >>= 1u;
    }
    s.state = acc_mult * s.state + acc_inc;
}

// splitmix64 hash helper
static inline uint64_t splitmix64(uint64_t x) {
    x += 0x9e3779b97f4a7c15ull;
    x = (x ^ (x >> 30u)) * 0xbf58476d1ce4e5b9ull;
    x = (x ^ (x >> 27u)) * 0x94d049bb133111ebull;
    return x ^ (x >> 31u);
}

uint64_t HashCoords(int64_t x, int64_t y, int64_t z, uint64_t seed) {
    uint64_t h = splitmix64(static_cast<uint64_t>(x) ^ seed);
    h = splitmix64(h ^ static_cast<uint64_t>(y));
    h = splitmix64(h ^ static_cast<uint64_t>(z));
    return h;
}

} // namespace terraingen 