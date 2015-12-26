// Copyright Louis Dionne 2015
// Distributed under the Boost Software License, Version 1.0.

#include "../atoms.hpp"
#include "../baseline.hpp"
#include "../flat.hpp"
#include "../lambda.hpp"
#include "../raw.hpp"
namespace ns = <%= namespace %>;


template <int>
struct x { };

int main() {
    <% (1..input_size).each do |i| %>
        ns::tuple<
            <%= (1..10).map { |n| "x<#{n + i}>" }.join(', ') %>
        > t_<%= i %>;
    <% end %>
}
