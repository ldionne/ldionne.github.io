// Copyright Louis Dionne 2015
// Distributed under the Boost Software License, Version 1.0.

#include "../atoms.hpp"
#include "../baseline.hpp"
#include "../flat.hpp"
#include "../lambda.hpp"
#include "../raw.hpp"
namespace ns = <%= namespace %>;


template <int i>
struct x { };

int main() {
    // We create a tuple with each implementation to really just benchmark
    // the compile-time of using `get`.
    flat::tuple<    <%= (0..500).map { |n| "x<#{n}>" }.join(', ') %>> t_flat;
    atoms::tuple<   <%= (0..500).map { |n| "x<#{n}>" }.join(', ') %>> t_atoms;
    lambda::tuple<  <%= (0..500).map { |n| "x<#{n}>" }.join(', ') %>> t_lambda;
    raw::tuple<     <%= (0..500).map { |n| "x<#{n}>" }.join(', ') %>> t_raw;
    baseline::tuple<<%= (0..500).map { |n| "x<#{n}>" }.join(', ') %>> t_baseline;

    ns::get<<%= n+0 %>>(t_<%= namespace %>);
    ns::get<<%= n+1 %>>(t_<%= namespace %>);
    ns::get<<%= n+2 %>>(t_<%= namespace %>);
    ns::get<<%= n+3 %>>(t_<%= namespace %>);
    ns::get<<%= n+4 %>>(t_<%= namespace %>);
    ns::get<<%= n+5 %>>(t_<%= namespace %>);
    ns::get<<%= n+6 %>>(t_<%= namespace %>);
    ns::get<<%= n+7 %>>(t_<%= namespace %>);
    ns::get<<%= n+8 %>>(t_<%= namespace %>);
    ns::get<<%= n+9 %>>(t_<%= namespace %>);
}
