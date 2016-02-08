// Copyright Louis Dionne 2015
// Distributed under the Boost Software License, Version 1.0.

#ifndef LAMBDA_OPERATIONS_HPP
#define LAMBDA_OPERATIONS_HPP

#include "lambda.hpp"

#include <cstddef>
#include <type_traits>


namespace lambda {
    template <typename ...T>
    tuple<std::decay_t<T>...> make_tuple(T&& ...t) {
        return tuple<std::decay_t<T>...>{std::forward<T>(t)...};
    }

    // Foldable
    template <typename Xs, typename F>
    decltype(auto) unpack(Xs&& xs, F&& f) {
        return static_cast<Xs&&>(xs).storage_(static_cast<F&&>(f));
    }

    // Functor
    template <typename Xs, typename F>
    auto transform(Xs&& xs, F const& f) {
        return static_cast<Xs&&>(xs).storage_([&f](auto&& ...xs) {
            return lambda::make_tuple(f(static_cast<decltype(xs)&&>(xs))...);
        });
    }

    // Iterable
    template <typename Xs>
    bool is_empty(Xs const& xs) {
        return xs.storage_([](auto const& ...xs) {
            return sizeof...(xs) == 0;
        });
    }

    // MonadPlus
    template <typename Xs, typename Ys>
    auto concat(Xs&& xs, Ys&& ys) {
        return static_cast<Xs&&>(xs).storage_([&](auto&& ...xs_) {
            return static_cast<Ys&&>(ys).storage_([&](auto&& ...ys_) {
                return lambda::make_tuple(
                    static_cast<decltype(xs_)&&>(xs_)...,
                    static_cast<decltype(ys_)&&>(ys_)...
                );
            });
        });
    }

    template <typename Xs, typename X>
    auto prepend(Xs&& xs, X&& x) {
        return static_cast<Xs&&>(xs).storage_([&](auto&& ...xs) {
            return lambda::make_tuple(
                static_cast<X&&>(x),
                static_cast<decltype(xs)&&>(xs)...
            );
        });
    }

    template <typename Xs, typename X>
    auto append(Xs&& xs, X&& x) {
        return static_cast<Xs&&>(xs).storage_([&](auto&& ...xs)  {
            return lambda::make_tuple(
                static_cast<decltype(xs)&&>(xs)...,
                static_cast<X&&>(x)
            );
        });
    }
} // end namespace lambda

#endif
