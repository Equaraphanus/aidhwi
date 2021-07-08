#pragma once

#include <cstdint>

namespace Random {

// Generic pseudo-random number generator with 64-bit internal state.
// Based on http://xoroshiro.di.unimi.it/splitmix64.c
template <typename T = unsigned>
class Prng {
public:
    Prng(std::uint64_t seed) : m_state(seed) {}
    ~Prng() {}

    T Next() { return NextImpl(m_state); }

    T Peek() const {
        std::uint64_t next = m_state;
        return NextImpl(next);
    }

    template <typename I = int>
    inline I NextInt(I from, I to) {
        return from + static_cast<I>(Next() % static_cast<T>(to - from));
    }

    template <typename I = int>
    inline I PeekInt(I from, I to) const {
        return from + static_cast<I>(Peek() % static_cast<T>(to - from));
    }

    template <typename F = float>
    inline F NextFloat(F from = 0, F to = 1, T steps = 1024) {
        return from + (Next() % steps) * (to - from) / static_cast<F>(steps);
    }

    template <typename F = float>
    inline F PeekFloat(F from = 0, F to = 1, T steps = 1024) const {
        return from + (Peek() % steps) * (to - from) / static_cast<F>(steps);
    }

private:
    std::uint64_t m_state;

    static inline T NextImpl(std::uint64_t& state) {
        std::uint64_t result = (state += 0x9e3779b97f4a7c15);
        result = (result ^ (result >> 30)) * 0xbf58476d1ce4e5b9;
        result = (result ^ (result >> 27)) * 0x94d049bb133111eb;
        return static_cast<T>(result ^ (result >> 31));
    }
};

} // namespace Random
