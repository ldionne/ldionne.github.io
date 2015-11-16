// Copyright Louis Dionne 2015
// Distributed under the Boost Software License, Version 1.0.

#ifndef FLAT_HPP
#define FLAT_HPP

#include <cstddef>
#include <type_traits>
#include <utility>


namespace flat {
//////////////////////////////////////////////////////////////////////////////
template <std::size_t i, typename T, bool = std::is_class<T>::value && !std::is_final<T>::value>
struct compressed_element;

template <std::size_t i, typename T>
struct compressed_element<i, T, true> : T {
    compressed_element() = default;
    explicit constexpr compressed_element(T const& t) : T(t) { }
    T& get() & { return *this; }
};

template <std::size_t i, typename T>
struct compressed_element<i, T, false> {
    compressed_element() = default;
    explicit constexpr compressed_element(T const& t) : value_(t) { }
    T& get() & { return value_; }
    T value_;
};
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
template <typename Indices, typename ...T>
struct tuple_impl;

template <std::size_t ...i, typename ...T>
struct tuple_impl<std::index_sequence<i...>, T...> : compressed_element<i, T>... {
    constexpr tuple_impl() = default;

    constexpr explicit tuple_impl(T const& ...t)
        : compressed_element<i, T>(t)...
    { }
};

template <typename ...T>
struct tuple : tuple_impl<std::make_index_sequence<sizeof...(T)>, T...> {
    using tuple_impl<std::make_index_sequence<sizeof...(T)>, T...>::tuple_impl;
};
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
template <std::size_t n, typename T>
constexpr T& get(compressed_element<n, T>& t) {
    return t.get();
}
//////////////////////////////////////////////////////////////////////////////
}

#endif
