// Copyright Louis Dionne 2015
// Distributed under the Boost Software License, Version 1.0.

template <std::size_t n, typename = std::make_index_sequence<n>>
struct nth_element_impl;

template <std::size_t n, std::size_t ...ignore>
struct nth_element_impl<n, std::index_sequence<ignore...>> {
    template <typename Tn>
    static Tn f(decltype((void*)ignore)..., Tn*, ...);
};

template <typename T>
struct wrapper { using type = T; };

template <std::size_t n, typename ...T>
using nth_element = typename decltype(
    nth_element_impl<n>::f(static_cast<wrapper<T>*>(0)...)
)::type;
