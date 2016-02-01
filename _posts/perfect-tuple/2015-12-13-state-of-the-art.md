---
layout: post
title: "The quest for a perfect tuple: The state of the art"
comments: true
tags: tuple metaprogramming
series: "The quest for a perfect tuple"
---

In this post, we will look at the current state of the art for implementing
tuples at the library level in C++. More specifically, we will look at two
different techniques for implementing tuples, one using recursive templates
and the other one using multiple inheritance. Both of these techniques are
well known and documented, and they are presented here for completeness.

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
this might look like is explained [here][ebo-impl]. Of course, that would
require adjusting the implementation of the constructor and the `get` function,
but those adjustments are almost trivial. Hence, we can say that with a little
bit more work, this implementation strategy for tuples would be able to compress
the storage of empty objects.

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

While convoluted, the above should not be too surprising. However, the very
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
Agustín.

With the naive implementation shown above, empty objects are obviously not
compressed. However, it is decently easy to implement this optimization with
EBO, by using a trick similar to what we outlined above. Hence, we can say
that this representation of a tuple supports compressing empty objects. Also,
we did not use any `constexpr`-unfriendly operation in our implementation,
so the functions can be marked as `constexpr` and this tuple can be made a
literal type.

## Conclusion

In this post, we have seen two techniques for implementing tuples at the
library level. These techniques are currently used in real-world standard
library implementations. libc++ uses the second technique, and last time I
checked libstdc++ used the first technique. As we'll see later, the second
technique turns out to be vastly superior, and libstdc++ should consider
moving to it.

Stay tuned for the next post, where I will introduce a technique using
C++14 generic lambdas to implement tuples.


<!-- Links -->
[Boost.Dev]: http://news.gmane.org/gmane.comp.lib.boost.devel
[Boost.Tuple]: http://www.boost.org/doc/libs/release/libs/tuple/doc/tuple_users_guide.html
[ebo-impl]: http://talesofcpp.fusionfenix.com/post-18/episode-ten-when-size-does-matter
[ebo]: http://en.cppreference.com/w/cpp/language/ebo
[efficient_packing]: http://talesofcpp.fusionfenix.com/post-22/true-story-efficient-packing#the-case-of-the-`tuple`
[talesofcpp]: http://talesofcpp.fusionfenix.com
