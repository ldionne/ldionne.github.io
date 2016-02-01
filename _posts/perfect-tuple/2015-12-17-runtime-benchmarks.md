---
layout: post
title: "The quest for a perfect tuple: Runtime benchmarks"
comments: true
tags: tuple metaprogramming
series: "The quest for a perfect tuple"
charts: true
---

TODO: Présenter la méthodologie des benchmarks runtime et les résultats

{% include series.html %}


## Runtime efficiency

In theory, both implementations only rely on the compiler being able to see
through base classes, `static_cast`s and inline functions. Hence, they should
cause the compiler to generate equivalent code, right? That's what we'll see.
To check this, we'll look at the assembly generated when accessing the elements
of a tuple at different indices:

{% highlight c++ %}
int main() {
    tuple<int, ..., int> t{0, 1, ..., n};
    get<0>(t);
    get<1>(t);
    ...
    get<n>(t);
}
{% endhighlight %}

At the same time, we'll compare it with the assembly generated for equivalent
code using a `struct` instead of a `tuple`. We'll assume that the `struct` is
optimized as much as possible by the compiler (otherwise that's arguably a
compiler bug), and use this as our baseline:

{% highlight c++ %}
struct Baseline {
    int element_0;
    int element_1;
    ...
    int element_n;
};

int main() {
    Baseline t{0, 1, ..., n};
    t.element_0;
    t.element_1;
    ...
    t.element_n;
}
{% endhighlight %}

To make sure the optimizer does not get rid of the code altogether, we'll use
the `escape()` trick described by Chandler Carruth in [this talk][Chandler.micro]
about microbenchmarks. Then, since we don't want to actually read a bunch of
repetitive assembly, we'll only compare the number of lines in the assembly
produced by each of the above:

<!-- TODO: tuple-quest/get_assembly/chart.json -->
<div class="chart" style="width:100%; height:400px;">
TODO
</div>

<!-- TODO: comment -->

### The NRVO test

A very important optimization in C++ is [NRVO][]. Basically, NRVO is when the
compiler constructs a local variable returned by value in the caller instead
of creating it in the caller and then copying it into the callee. Since this
is an important optimization, and since I've already seen some disturbing cases
where NRVO wasn't applied but it should have been, let's look at how our tuple
implementations play with this optimization. To achieve this, we'll create a
tuple with many elements, and we'll return it by value from a function.
Normally, the tuple should be constructed in-place and never copied. Here's
the code:

{% highlight c++ %}
template <int i>
struct x {
    x() { std::cout << "x()" << std::endl; }
    x(x const&) { std::cout << "x(x const&)" << std::endl; }
};

auto f() {
    tuple<x<0>, x<1>, ..., x<n>> t;
    return t; // should NRVO
}

int main() {
    auto t = f();
}
{% endhighlight %}

Let's now run this code with different values of `n`, and see if NVRO is
applied:

<!-- TODO: tuple-quest/nrvo/table.md -->

<!-- TODO: comment -->

### Length of symbols

<!-- TODO: Design a benchmark to check for code bloat due to symbol length. -->


<!-- Links -->
[NRVO]: https://en.wikipedia.org/wiki/Return_value_optimization
[Chandler.micro]: https://youtu.be/nXaxk27zwlk?t=2527
