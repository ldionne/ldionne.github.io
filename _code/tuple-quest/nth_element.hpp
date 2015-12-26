// Copyright Louis Dionne 2015
// Distributed under the Boost Software License, Version 1.0.

#ifndef NTH_ELEMENT_HPP
#define NTH_ELEMENT_HPP

#include <cstddef>


template <std::size_t N, typename ...T>
using nth_element = __nth_element<N, T...>;

#endif
