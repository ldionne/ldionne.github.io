// Copyright Louis Dionne 2015
// Distributed under the Boost Software License, Version 1.0.

#include "../lambda.hpp"
namespace ns = lambda;


static void escape(void* p) {
    asm volatile("" : : "g"(p) : "memory");
}

int main() {
    ns::tuple<
        <%= (0..input_size).map { |n| "int" }.join(', ') %>
    > t{
        <%= (0..input_size).to_a.join(', ') %>
    };

    <% (0..input_size).each do |n| %>
        escape(&ns::get<<%= n %>>(t));
    <% end %>
}
