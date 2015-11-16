// Copyright Louis Dionne 2015
// Distributed under the Boost Software License, Version 1.0.

#ifndef LAMBDA_HPP
#define LAMBDA_HPP

#include <cstddef>
#include <utility>

#include "../pack-indexing/overload.hpp"


namespace lambda {
template <typename ...T>
auto make_storage(T const& ...t) {
    return [=](auto const& f) -> decltype(auto) { return f(t...); };
}

template <typename ...T>
struct tuple {
    using Storage = decltype(make_storage(std::declval<T>()...));
    Storage storage_;

    tuple()
        : storage_(make_storage(T()...))
    { }

    explicit tuple(T const& ...t)
        : storage_(make_storage(t...))
    { }
};

template <std::size_t n, typename ...T>
decltype(auto) get(tuple<T...> const& ts) {
    using Nth = nth_element<n, T...>;

    return ts.storage_([](auto const& ...t) -> Nth const& {
        void const* addresses[] = {&t...};
        return *static_cast<Nth const*>(addresses[n]);
    });
}
}

#endif
