// Copyright Louis Dionne 2015
// Distributed under the Boost Software License, Version 1.0.

#ifndef BASELINE_HPP
#define BASELINE_HPP

#include <cstddef>


namespace baseline {
    template <typename ...T>
    struct tuple { };

    template <std::size_t n, typename Tuple>
    constexpr int get(Tuple const& t) {
        return 0;
    }
}

#endif
