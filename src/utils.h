#pragma once

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