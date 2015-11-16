// Copyright Louis Dionne 2015
// Distributed under the Boost Software License, Version 1.0.

#ifndef ELEMENT_HPP
#define ELEMENT_HPP

#include <iostream>

template <int i>
struct x {
    x() { }
    x(x const&) { std::cout << "no-rvo" << std::endl; }
};

#endif
