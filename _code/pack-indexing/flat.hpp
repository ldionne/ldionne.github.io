// Copyright Louis Dionne 2015
// Distributed under the Boost Software License, Version 1.0.

template <std::size_t I, typename T>
struct indexed {
    using type = T;
};

template <typename Is, typename ...Ts>
struct indexer;

template <std::size_t ...Is, typename ...Ts>
struct indexer<std::index_sequence<Is...>, Ts...>
    : indexed<Is, Ts>...
{ };

template <std::size_t I, typename T>
static indexed<I, T> select(indexed<I, T>);

template <std::size_t I, typename ...Ts>
using nth_element = typename decltype(select<I>(
    indexer<std::index_sequence_for<Ts...>, Ts...>{}
))::type;
