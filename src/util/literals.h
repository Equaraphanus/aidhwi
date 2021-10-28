#pragma once

#include <cstddef>
#include <cstdint>

// size_t literal helper
inline std::size_t operator"" _zu(unsigned long long x) {
    return x;
}
