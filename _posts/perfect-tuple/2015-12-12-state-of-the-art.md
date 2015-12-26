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
we'll want to measure the sheer cost of instantiating tuples implemented in
both ways. More specifically, we'll want to measure how well an implementation
scales in terms of the size of the tuple created, and how well it scales in
terms of the number of tuples instantiated (given a fixed tuple size). For
the first aspect, our benchmark code will look like this:

{% highlight c++ %}
template <int i>
struct x { };

int main() {
    tuple<x1<0>, x1<1>, ..., x1<n>> t1;
    tuple<x2<0>, x2<1>, ..., x2<n>> t2;
    ...
    tuple<x10<0>, x10<1>, ..., x10<n>> t10;
}
{% endhighlight %}

The methodology is simply to measure the compile-time of such a file for various
values of `n`. The reason why we instantiate 10 different tuples every time is
that it reduces the relative incertitude on the measurement. We're almost ready
to look at the result, but before we do, let's pick a baseline to compare all
implementations against:

{% highlight c++ %}
template <typename ...T>
struct tuple {
    static constexpr std::size_t sizes[] = {sizeof(T)...};
};
{% endhighlight %}

This baseline is obviously not a valid tuple, but measuring it will still
allow us to factor out some noise such as compiler startup time. Because
of the `sizes` array, this will also allow us to factor out the cost of
instantiating the elements of the tuple themselves. Here's a chart
showing the results:

<!-- ninja -C _code/build tuple-quest.make_compile1 -->
<div class="chart" style="width:100%; height:400px;">
TODO
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
`tuple<T1, ..., Tn>`. OK; measuring the instantiation cost of large tuples
is nice, but we'd also like to know how these techniques scales with respect
to the number of different tuples created. To do this, we'll benchmark the
following code:

{% highlight c++ %}
template <int i>
struct x { };

int main() {
    tuple<x<1>, x<2>, ..., x<10>> t1;
    tuple<x<2>, x<3>, ..., x<11>> t2;
    ...
    tuple<x<n>, x<n+1>, ..., x<n+10>> tn;
}
{% endhighlight %}

Here, we're arbitrarily setting what seems to be a sensible tuple size (`10`),
and then measuring the time required to instantiate `n` tuples of that size.
To make sure we're actually instantiating different tuple types, we shift the
types of the elements of each tuple by one. Here's the result:

<!-- ninja -C _code/build tuple-quest.make_compile2 -->
<div class="chart" style="width:100%; height:400px;">
TODO
</div>

<!-- TODO: COMMENT -->

Now, creating tuples is fun, but accessing elements in a tuple is more fun.
Hence, we'll also want to measure how well our implementations scale when
accessing elements with the `get` function. Much like the previous benchmarks,
there are mainly two interesting access patterns we can benchmark. The first
one is to fix a tuple size, and to access varying numbers of elements inside
the tuple. This pattern can be measured with the following benchmark:

{% highlight c++ %}
template <int i>
struct x { };

int main() {
    tuple<x<0>, x<1>, ..., x<1000>> t;
    get<0 * (1000/n)>(t);
    get<1 * (1000/n)>(t);
    ...
    get<n * (1000/n)>(t);
}
{% endhighlight %}

Here, we arbitrarily set the size of the tuple to `1000`, which is large enough,
and then access `n` elements inside the tuple. Note that the `n` elements that
we access are placed at constant intervals, and they are almost uniformly
distributed in the $[0, 1000]$ interval. We do this to avoid accessing the
first (or last) `n` elements, which would favor an implementation where
accessing leading (or trailing) elements is faster. Also, while not shown
here, we actually create a tuple of each implementation and only access
elements of the implementation we're benchmarking. This is to factor out
the initial cost of creating the tuple, which varies between implementations.
Before we show the results, let's show the baseline that we'll use:

{% highlight c++ %}
template <std::size_t i, typename Tuple>
constexpr int get(Tuple const&) {
    return 0;
}
{% endhighlight %}

Again, this is obviously not a correct implementation of `get`, but it will at
least allow us to factor out the cost of instantiating the `get` function. Here
is the result:

<!-- ninja -C _code/build tuple-quest.get_compile1 -->
<div class="chart" style="width:100%; height:400px;">
TODO
</div>

<!-- TODO: COMMENT -->

The second access pattern that we might want to measure is to fix a number of
elements to access inside the tuple, and to let the indices of these elements
grow larger and larger. This access pattern can be measured with the following
benchmark:

{% highlight c++ %}
template <int i>
struct x { };

int main() {
    tuple<x<0>, x<1>, ..., x<1000>> t;
    get<n>(t);
    get<n+1>(t);
    get<n+2>(t);
    get<n+3>(t);
    get<n+4>(t);
    get<n+5>(t);
    get<n+6>(t);
    get<n+7>(t);
    get<n+8>(t);
    get<n+9>(t);
}
{% endhighlight %}

Here, we decide to access exactly `10` elements in the tuple. We then let the
index of these elements vary. The result is:

<!-- ninja -C _code/build tuple-quest.get_compile2 -->
<div class="chart" style="width:100%; height:400px;">
TODO
</div>

<!-- TODO: COMMENT

If not wildly better, we can still clearly see that the second implementation
is better than the first one. This is because it does not require instantiating
several `get` overloads each time an index is accessed.
 -->


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
