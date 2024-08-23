#pragma once

#include <chrono>

struct Timer {
    std::chrono::high_resolution_clock::time_point start;
    inline Timer() : start(Now()) {}
    inline float Elapsed() const
    {
        auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(Now() - start);
        return static_cast<float>(duration_us.count()) / 1000.0f;
    }
    static inline std::chrono::high_resolution_clock::time_point Now() { return std::chrono::high_resolution_clock::now(); }
    inline void Reset() { start = Now(); }
};