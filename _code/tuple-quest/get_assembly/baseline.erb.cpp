// Copyright Louis Dionne 2015
// Distributed under the Boost Software License, Version 1.0.


struct Baseline {
<% (0..input_size).each do |n| %>
    int element_<%= n %>;
<% end %>
};


static void escape(void* p) {
    asm volatile("" : : "g"(p) : "memory");
}

int main() {
    Baseline t{<%= (0..input_size).to_a.join(', ') %>};

    <% (0..input_size).each do |n| %>
        escape(&t.element_<%= n %>);
    <% end %>
}
