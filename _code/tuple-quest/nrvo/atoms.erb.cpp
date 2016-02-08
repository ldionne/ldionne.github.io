// Copyright Louis Dionne 2015
// Distributed under the Boost Software License, Version 1.0.

#include "../atoms.hpp"
#include "element.hpp"
namespace ns = atoms;


auto f() {
    ns::tuple<<%= (0..input_size).map { |n| "x<#{n}>" }.join(', ') %>> t;
    return t; // should NRVO
}

int main() {
    auto t = f();
}