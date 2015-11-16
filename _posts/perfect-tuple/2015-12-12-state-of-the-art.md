---
layout: post
title: "The quest for a perfect tuple: The state of the art"
comments: true
tags: tuple metaprogramming
series: "The quest for a perfect tuple"
charts: true
---

In this post, we will look at the current state of the art for implementing
tuples at the library level in C++. More specifically, we will look at two
different techniques for implementing tuples, and we will (try to) analyze
them through the criteria that we established in the previous post:

1. Compile-time efficiency
2. Runtime efficiency
3. Compression of empty objects
4. `constexpr`-friendliness

{% include series.html %}

## First implementation technique: recursive atoms

The first implementation technique that we'll explore is pretty straightforward.
To my knowledge, this technique was first used in the original implementation
of [`boost::tuple`][Boost.Tuple] by Jaakko Järvi, several years ago. Basically,
it consists in implementing a tuple as a linked list of atoms, each atom
holding a single element of the tuple and a list of the remaining elements,
recursively:

{% highlight c++ %}
template <typename ...T>
struct tuple;

// An empty tuple has no members; this is the base case
template <>
struct tuple<> { };

// Otherwise, a tuple holds the first element and
// a tuple of the remaining elements
template <typename T, typename ...U>
struct tuple<T, U...> {
    T element;
    tuple<U...> rest;

    constexpr explicit tuple(T const& t, U const& ...u)
        : element(t), rest(u...)
    { }
};
{% endhighlight %}

This is fairly simple. Then, to access the n-th element of such a tuple, we
can write the following:

{% highlight c++ %}
template <std::size_t n, typename T, typename ...U>
constexpr decltype(auto) get(tuple<T, U...> const& ts) {
    return get<n-1>(ts.rest);
}

template <typename T, typename ...U>
constexpr T const& get<0>(tuple<T, U...> const& ts) {
    return ts.element;
}
{% endhighlight %}

Actually, we can't write the above as-is because function templates can't be
partially specialized. It is very easy to work around this limitation by using
a `struct` instead, but let's focus on the important things.

With our naive implementation, empty objects are not compressed. However, it
is pretty easy to change this by using the well-known technique of
[empty base optimization][ebo]:

{% highlight c++ %}
template <typename T, typename ...U>
struct tuple<T, U...> : tuple<U...>, element_holder<T> { };
{% endhighlight %}

In the above, `element_holder` is an imaginary template that uses EBO to reduce
the storage required for empty members to 0. An in-depth explanation of how
this might look like is explained on [K-ballo's blog][ebo-impl]. Of course,
that would require adjusting the implementation of the constructor and the
`get` function, but those adjustments are almost trivial. Hence, we can say
that with a little bit more work, this implementation strategy for tuples
would be able to compress the storage of empty objects.

Also, since we never use `constexpr`-unfriendly operations, we can mark our
functions as `constexpr`. Hence, the above tuple is a literal type, i.e. one
that can be constructed at compile-time.


## Leveling up: multiple inheritance

The second implementation technique that we'll explore is much less obvious
than the first one. I was first made aware of it by a post on the [Boost.Dev][]
mailing list by [Agustín Bergé][talesofcpp], but he told me that Howard Hinnant
had been using the technique in libc++ before that. Basically, the technique
consists in inheriting multiply from some base classes that hold the actual
elements of the tuple:

{% highlight c++ %}
template <typename ...T>
struct tuple : holder<0, T0>, holder<1, T1>, ..., holder<n-1, Tn-1> {
    // ...
};
{% endhighlight %}

Of course, this isn't valid C++. However, the same effect can be achieved by
using `std::index_sequence` and a bit of creativity:

{% highlight c++ %}
template <std::size_t i, typename T>
struct holder { T element; };

template <typename Indices, typename ...T>
struct tuple_impl;

template <std::size_t ...i, typename ...T>
struct tuple_impl<std::index_sequence<i...>, T...> : holder<i, T>... {
    constexpr explicit tuple_impl(T const& ...t)
        : holder<i, T>{t}...
    { }
};

template <typename ...T>
struct tuple : tuple_impl<std::make_index_sequence<sizeof...(T)>, T...> {
    // ...
};
{% endhighlight %}

While original, the above should not be too surprising. However, the very
clever part is in the implementation of the `get` function for such a tuple:

{% highlight c++ %}
template <std::size_t n, typename T>
constexpr T const& get(holder<n, T> const& t) {
    return t.element;
}
{% endhighlight %}

Yes, that's all! When we call `get<n>` on a tuple, the only function that
matches is the `get` overloaded function. However, since we're specifying
`n` explicitly, the compiler has to match the tuple against `holder<n, T>`
for some type `T`. Since there is exactly one such `holder` base class of
the tuple, the tuple gets implicitly converted to the right `holder`, which
holds the element you were looking for! Clever, huh? For a more in-depth
explanation of how this works, see [this other post][efficient_packing] by
K-ballo.

With the naive implementation shown above, empty objects are obviously not
compressed. However, it is decently easy to implement this optimization with
EBO, by using a trick similar to what we outlined above. Hence, we can say
that this representation of a tuple supports compressing empty objects.

Also, we did not use any `constexpr`-unfriendly operation in our implementation,
so the functions can be marked as `constexpr` and this tuple is a literal type.
Let's now take a look at the compile-time and runtime performance of these
two tuple implementations.


## Compile-time efficiency

Now's the time to get serious, because compile-time efficiency is an important
point. We'll compare the two above implementations on several aspects. First,
we'll want to measure how well our tuple scales when creating a tuple with a
large number of elements. Then, we'll also want to check how well the `get`
function scales when accessing different indices in a large tuple. For the
first point, our benchmark code will look like this:

{% highlight c++ %}
template <int i>
struct x { };

int main() {
    tuple<x<0>, x<1>, ..., x<n>> t;
}
{% endhighlight %}

Then, we'll want to check how long this takes to compile for different values
of `n`, and for the two different implementations. Here's a chart showing the
results:

<div class="chart" style="width:100%; height:400px;">
<!-- TODO: tuple-quest/make_compile/chart.json -->
</div>

As we can see, the first implementation, which used recursive atoms, is much
worse than the flat implementation using multiple inheritance. Frankly, I'm
not sure why this is so. It could be due to the deeply nested instantiations,
or to the fact that the symbols are much longer with the first implementation.
Indeed, for the first implementation, we get something like:

{% highlight c++ %}
tuple<T1, T2, ..., Tn>
tuple<T2, ..., Tn>
...
tuple<Tn>
{% endhighlight %}

whereas for the second implementation, we only get

{% highlight c++ %}
tuple<T1, T2, ..., Tn>
tuple_impl<std::index_sequence<1, ..., n>, T1, T2, ..., Tn>
holder<1, T1>,
holder<2, T2>,
...
holder<n, Tn>
{% endhighlight %}

The trick here is that each `holder` is much, much shorter than a whole
`tuple<T1, ..., Tn>`. Let's now take a look at the compile-time behaviour of
the `get` function. To do this, we'll fix the size of a tuple to some value
`n`, and we'll access a single element at different indices `i`:

{% highlight c++ %}
template <int i>
struct x { };

int main() {
    tuple<x<0>, x<1>, ..., x<n>> t;
    get<i>(t);
}
{% endhighlight %}

Here is a benchmark for `n = 300`:

<div class="chart" style="width:100%; height:400px;">
<!-- TODO: tuple-quest/get_compile/chart.json -->
</div>

If not wildly better, we can still clearly see that the second implementation
is better than the first one. This is because it does not require instantiating
several `get` overloads each time an index is accessed.


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

<div class="chart" style="width:100%; height:400px;">
<!-- TODO: tuple-quest/get_assembly/chart.json -->
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

## Summary

In summary, we can see that the second implementation is vastly superior to
the first, naive one. Not only is it better in terms of compile-time, but it
also causes the compiler to generate superior code, which is quite important.

|                      | Compile-time | Runtime | Compression | `constexpr` |
| -------------------- | :----------: | :-----: | :---------: | :---------: |
| recursive atoms      | -            | -       |             | -           |
| multiple inheritance | -            | -       |             | -           |

<!-- TODO -->

<!-- Links -->
[Boost.Dev]: http://news.gmane.org/gmane.comp.lib.boost.devel
[Boost.Tuple]: http://www.boost.org/doc/libs/release/libs/tuple/doc/tuple_users_guide.html
[Chandler.micro]: https://youtu.be/nXaxk27zwlk?t=2527
[ebo-impl]: http://talesofcpp.fusionfenix.com/post-18/episode-ten-when-size-does-matter
[ebo]: http://en.cppreference.com/w/cpp/language/ebo
[efficient_packing]: http://talesofcpp.fusionfenix.com/post-22/true-story-efficient-packing#the-case-of-the-`tuple`
[NRVO]: https://en.wikipedia.org/wiki/Return_value_optimization
[talesofcpp]: http://talesofcpp.fusionfenix.com
