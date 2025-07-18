#pragma once

#include <cstdint>
#include <vector>

namespace terraingen {

// RAII tracer: logs entry and exit events into a ring buffer (see implementation.md 4. Tracing.hpp)
class TraceScope {
public:
    TraceScope(const char* name);
    ~TraceScope();
};

// Log a key/value pair within a trace scope
inline void TraceValue(const char* key, int32_t value) {
    // stub: implementation writes to ring buffer
}

} // namespace terraingen 