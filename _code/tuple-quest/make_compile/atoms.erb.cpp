// Copyright Louis Dionne 2015
// Distributed under the Boost Software License, Version 1.0.

#include "../atoms.hpp"
#include "../flat.hpp"
namespace ns = atoms;


template <int>
struct x { };

int main() {
    ns::tuple<
        <%= (0..input_size).map { |n| "x<#{n}>" }.join(', ') %>
    > t;
}
