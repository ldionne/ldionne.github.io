// Copyright Louis Dionne 2015
// Distributed under the Boost Software License, Version 1.0.

#include "../atoms.hpp"
#include "../baseline.hpp"
#include "../flat.hpp"
#include "../lambda.hpp"
namespace ns = <%= namespace %>;


template <int i>
struct x { };

int main() {
    // We create a tuple with each implementation to really just benchmark
    // the compile-time of using `get`.
    flat::tuple<    <%= (0..1000).map { |n| "x<#{n}>" }.join(', ') %>> t_flat;
    atoms::tuple<   <%= (0..1000).map { |n| "x<#{n}>" }.join(', ') %>> t_atoms;
    lambda::tuple<  <%= (0..1000).map { |n| "x<#{n}>" }.join(', ') %>> t_lambda;
    baseline::tuple<<%= (0..1000).map { |n| "x<#{n}>" }.join(', ') %>> t_baseline;

    <% (0..n).each do |i| %>
        ns::get<<%= i * (1000/n) %>>(t_<%= namespace %>);
    <% end %>
}
