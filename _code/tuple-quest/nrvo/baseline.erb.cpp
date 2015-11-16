// Copyright Louis Dionne 2015
// Distributed under the Boost Software License, Version 1.0.

#include "element.hpp"


struct Baseline {
    <% (0..input_size).each do |n| %>
        x< <%= n %> > element_<%= n %>;
    <% end %>
};

auto f() {
    Baseline t;
    return t; // should NRVO
}

int main() {
    auto t = f();
}
