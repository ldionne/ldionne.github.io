// Copyright Louis Dionne 2015
// Distributed under the Boost Software License, Version 1.0.

#ifndef ATOMS_HPP
#define ATOMS_HPP

#include <cstddef>
#include <type_traits>


namespace atoms {
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
template <typename ...T>
struct tuple;

// An empty tuple has no members; this is the base case
template <>
struct tuple<> { };

// Otherwise, a tuple holds the first element and
// a tuple of the remaining elements
template <typename T, typename ...U>
struct tuple<T, U...> : compressed_element<sizeof...(U), T>, tuple<U...> {
    constexpr tuple() = default;

    constexpr explicit tuple(T const& t, U const& ...u)
        : compressed_element<sizeof...(U), T>(t), tuple<U...>(u...)
    { }
};
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
template <std::size_t n>
struct get_impl {
    template <typename T, typename ...U>
    static constexpr decltype(auto) apply(tuple<T, U...>& ts) {
        return get_impl<n-1>::apply(static_cast<tuple<U...>&>(ts));
    }
};

template <>
struct get_impl<0> {
    template <typename T, typename ...U>
    static constexpr T& apply(tuple<T, U...>& ts) {
        return static_cast<compressed_element<sizeof...(U), T>&>(ts).get();
    }
};

template <std::size_t n, typename ...T>
constexpr decltype(auto) get(tuple<T...>& ts) {
    static_assert(n < sizeof...(T),
        "trying to get an out-of-bounds element of the tuple");

    return get_impl<n>::apply(ts);
}
//////////////////////////////////////////////////////////////////////////////
}

#endif
