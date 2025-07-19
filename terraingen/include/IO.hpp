#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace terraingen {

// IO utilities for binary data (see implementation.md 4. IO.hpp)
std::vector<uint8_t> LoadBinary(const std::string& path);
bool SaveBinary(const std::string& path, const std::vector<uint8_t>& data);

} // namespace terraingen 