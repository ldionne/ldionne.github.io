// Copyright Louis Dionne 2015
// Distributed under the Boost Software License, Version 1.0.

#include "../atoms.hpp"
#include "../flat.hpp"
#include "../lambda.hpp"
namespace ns = lambda;


template <int>
struct x { };

int main() {
    ns::tuple<
        <%= (0..input_size).map { |n| "x<#{n}>" }.join(', ') %>
    > t;
}
