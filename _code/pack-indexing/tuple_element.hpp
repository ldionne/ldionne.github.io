// Copyright Louis Dionne 2015
// Distributed under the Boost Software License, Version 1.0.

template <std::size_t I, typename ...Ts>
using nth_element = typename std::tuple_element<I, std::tuple<Ts...>>::type;
