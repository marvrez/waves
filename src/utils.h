#pragma once

#include "logger.h"

// https://github.com/Reedbeta/nrr_enumerate
template <typename T>
constexpr auto enumerate(T&& iterable)
{
    struct iterator {
        size_t i;
        decltype(std::begin(iterable)) iter;
        bool operator!= (const iterator & other) const { return iter != other.iter; }
        void operator++ () { ++i; ++iter; }
        auto operator* () const { return std::tie(i, *iter); }
    };
    struct iterable_wrapper {
        T iterable;
        auto begin() { return iterator{ 0, std::begin(iterable) }; }
        auto end() { return iterator{ 0, std::end(iterable) }; }
    };
    return iterable_wrapper{ std::forward<T>(iterable) };
}

static inline std::string ReadFile(const std::string_view& path)
{
    std::ifstream file = std::ifstream(path.data());
    std::string line, text;
    if (!file.is_open()) {
        std::string file_path = std::string(std::filesystem::current_path()) + '/' + path.data();
        file = std::ifstream(file_path.data());
        assert(file.is_open());
    }
    if (!file.is_open()) LOG_WARN("Could not open file: {}", path.data());
    while (std::getline(file, line)) text += line + "\n";
    return text;
}

static inline std::string ReadSourceFile(const std::string_view& filename)
{
    std::ifstream file(filename.data());
    if(!file.good()) return "";
    // Found usable source file
    std::stringstream source_buffer;
    source_buffer << file.rdbuf();
    return source_buffer.str();
}

template<typename T>
constexpr T DivRoundUp(T a, T b) { return (a + b - (T)1) / b; }
template<typename T>
constexpr T GetAlignedSize(T size, T alignment) { return DivRoundUp(size, alignment) * alignment; }