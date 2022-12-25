#pragma once

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <chrono>

#include <execinfo.h>
#include <unistd.h>

#include <fmt/core.h>
#include <fmt/chrono.h>
#include <fmt/os.h>
#include <fmt/color.h>

// API
#define LOG_DEBUG(message,...) do { \
_LOG_CORE ('D', ((message)), ##__VA_ARGS__); \
} while (0)

#define LOG_INFO(message,...) do { \
_LOG_CORE ('I', ((message)), ##__VA_ARGS__); \
} while (0)

#define LOG_WARN(message,...) do { \
_LOG_CORE ('W', ((message)), ##__VA_ARGS__); \
} while (0)

#define LOG_ERROR(message,...) do { \
_LOG_CORE ('E', ((message)), ##__VA_ARGS__ ); \
_BACKTRACE(); \
} while (0)

#define BT_SIZE  16
static inline void _BACKTRACE()
{
    int num_calls;
    void* bt_buffer[BT_SIZE];

    num_calls = backtrace(bt_buffer, BT_SIZE);

    backtrace_symbols_fd(bt_buffer, num_calls, STDERR_FILENO);
}
#undef BT_SIZE

template <typename... Args>
static inline void _LOG_CORE(char level, std::string_view message, Args... args)
{
    // Print current time
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    fmt::print("[{:%H:%M:%S}] ", now);

    // Sets color to message
    auto color = (level=='I') ? fmt::fg(fmt::color::green)
               : (level=='W') ? fmt::fg(fmt::color::yellow)
               : (level=='E') ? fmt::fg(fmt::color::crimson) | fmt::emphasis::bold
               : (level=='D') ? fmt::fg(fmt::color::dark_magenta)
               :                fmt::fg(fmt::color::white);

    //actual message formatting
    fmt::print(color, "{:9}",
               (level=='I') ? "INFO"
             : (level=='W') ? "WARNING"
             : (level=='E') ? "ERROR"
             : (level=='D') ? "DEBUG"
             :                "");
    fmt::print(fmt::fg(fmt::color::white), message, args...);
    fmt::print("\n");
}