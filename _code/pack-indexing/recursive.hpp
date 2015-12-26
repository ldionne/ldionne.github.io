// Copyright Louis Dionne 2015
// Distributed under the Boost Software License, Version 1.0.

#ifndef NTH_ELEMENT_RECURSIVE_HPP
#define NTH_ELEMENT_RECURSIVE_HPP

template <std::size_t I, typename T, typename ...Ts>
struct nth_element_impl {
    using type = typename nth_element_impl<I-1, Ts...>::type;
};

template <typename T, typename ...Ts>
struct nth_element_impl<0, T, Ts...> {
    using type = T;
};

template <std::size_t I, typename ...Ts>
using nth_element = typename nth_element_impl<I, Ts...>::type;

#endif
