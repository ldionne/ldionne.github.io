// Copyright Louis Dionne 2015
// Distributed under the Boost Software License, Version 1.0.

#include "../atoms.hpp"
#include "../baseline.hpp"
#include "../flat.hpp"
#include "../lambda.hpp"
namespace ns = <%= namespace %>;


template <int>
struct x { };

int main() {
    // instantiate the x<n>'s
    <% (0..input_size+10).each do |n| %>
        x<  <%= n %>  > x_<%= n %>;
    <% end %>

    <% 10.times do |i| %>
        ns::tuple<
            <%= (0..input_size).map { |n| "x<#{n + i}>" }.join(', ') %>
        > t_<%= i %>;
    <% end %>
}
