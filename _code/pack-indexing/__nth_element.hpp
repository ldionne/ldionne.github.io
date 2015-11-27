// Copyright Louis Dionne 2015
// Distributed under the Boost Software License, Version 1.0.

template <std::size_t N, typename ...T>
using nth_element = __nth_element<std::size_t, N, T...>;
