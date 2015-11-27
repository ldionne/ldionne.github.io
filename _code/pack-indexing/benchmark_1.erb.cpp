// Copyright Louis Dionne 2015
// Distributed under the Boost Software License, Version 1.0.

#include <cstddef>
#include <tuple>
#include <utility>

#include "<%= header %>"

<% (0..input_size-1).each do |n| %>
    using T_<%= n %> = nth_element<
        <%= n %>,
        <%= (1..input_size).map { |i| "int" }.join(', ') %>
    >;
<% end %>
