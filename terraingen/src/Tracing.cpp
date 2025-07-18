#include "Tracing.hpp"
#include <chrono>
#include <iostream>
#include <string>

namespace terraingen {

struct TraceEvent {
    const char* name;
    std::chrono::high_resolution_clock::time_point start;
};

static thread_local std::vector<TraceEvent> s_stack;

TraceScope::TraceScope(const char* name) {
    s_stack.push_back({name, std::chrono::high_resolution_clock::now()});
    std::cout << "[TRACE] Begin " << name << std::endl;
}

TraceScope::~TraceScope() {
    auto ev = s_stack.back();
    s_stack.pop_back();
    auto end = std::chrono::high_resolution_clock::now();
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - ev.start).count();
    std::cout << "[TRACE] End   " << ev.name << " (" << us << " µs)" << std::endl;
}

} // namespace terraingen 