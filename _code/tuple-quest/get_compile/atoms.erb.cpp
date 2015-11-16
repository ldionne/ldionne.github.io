// Copyright Louis Dionne 2015
// Distributed under the Boost Software License, Version 1.0.

#include "../atoms.hpp"
#include "../flat.hpp"
namespace ns = atoms;


template <int i>
struct x { };

int main() {
    // We create a tuple with each implementation to really just benchmark
    // the compile-time of using `get`.
    flat::tuple<
        <%= (0..tuple_size).map { |n| "x<#{n}>" }.join(', ') %>
    > t_flat;

    atoms::tuple<
        <%= (0..tuple_size).map { |n| "x<#{n}>" }.join(', ') %>
    > t_atoms;

    ns::get<<%= input_size %>>(t_atoms);
}
