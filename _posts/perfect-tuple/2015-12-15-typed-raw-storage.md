---
layout: post
title: "The quest for a perfect tuple: Typed raw storage"
comments: true
tags: tuple metaprogramming Hana
series: "The quest for a perfect tuple"
---

<!-- TODO:
Mention that Alisdair Meredith might have been aware of this
technique already, and also verify this is the case. -->

Recently, while looking for a way to optimize the heterogeneous `scan`
algorithms in the [Boost.Hana][] library, I stumbled over an idea that
made my day. Let me show you what it was.

{% include series.html %}

As you can imagine by the title of this post, my idea was a new technique for
implementing tuple. But before I introduce it, let me give some background
information. [Boost.Hana][] is a library providing heterogeneous containers
(tuples) and algorithms to manipulate them. For example, you might want to
iterate over each element of a tuple and call a function on that element.
Or you might want to remove all the elements that satisfy some predicate.
Or you might even want to sort the elements of a tuple according to some
predicate. In fact, one can really imagine doing almost any of the `std::`
algorithms on a tuple, since a tuple is just a sequence... Right?

In fact, there's one big catch: the elements of a tuple are not required to
have the same type. Hence, a function that wants to be called on every element
of a tuple needs to be callable on every type that's in the tuple. Similarly,
the predicate used to sort a tuple must be callable with any two types in the
tuple. Ok, but how is this relevant to this post? Well, it turns out that
heterogeneity brings many other restrictions, and heterogeneous algorithms
(algorithms that can handle objects with different types) must be implemented
quite differently from normal STL algorithms. This difference is precisely the
reason why the technique we're about to see is interesting. Consider the task
computing the partial sums of a tuple, and storing the result in another tuple:

{% highlight c++ %}
template <typename ...T, typename Init, typename F>
tuple<???> partial_sum(tuple<T...> const& ts, Init const& init, F const& f) {
    return tuple<???>{
        init,
        f(init, get<0>(ts)),
        f(f(init, get<0>(ts)), get<1>(ts)),
        f(f(f(init, get<0>(ts)), get<1>(ts)), get<2>(ts)),
        ...
    };
}
{% endhighlight %}

To see how this is effectively computing the partial sums, imagine that `f` is
`operator+`, `init` is 0 and the tuple contains numbers. In that case, it is
clear how this is equivalent to the `std::partial_sum` algorithm. But why do
we compute each element from the start, instead of reusing the previously
computed result? Well, since the elements of the tuple may have different
types, we can't simply use a loop and write something like

{% highlight c++ %}
template <typename ...T, typename Init, typename F>
tuple<???> partial_sum(tuple<T...> const& ts, Init init, F const& f) {
    tuple<???> result{};
    for (int i = 0; i < sizeof...(T); ++i) {
        get<i>(result) = init;
        init = f(init, get<i>(ts));
    }
    return result;
}
{% endhighlight %}

> Note that the above would also require the result tuple to be
> default-constructible.

To workaround this, what we'd really like to write is this:

{% highlight c++ %}
template <typename ...T, typename Init, typename F>
tuple<???> partial_sum(tuple<T...> const& ts, Init init, F const& f) {
    tuple<???> result{};
    get<0>(result) = init;
    get<1>(result) = f(get<0>(result), get<0>(ts));
    get<2>(result) = f(get<1>(result), get<1>(ts));
    get<3>(result) = f(get<2>(result), get<2>(ts));
    ...

    return result;
}
{% endhighlight %}

Of course, the above is problematic because we can't write it for a generic
number of tuple elements. Also, this suffers from the same weakness as above,
which is that the resulting tuple must be default-constructible. However, this
idea of constructing the `result` tuple "in-place" is the key advantage of the
technique I want to present in this post. Enough waiting, here's the technique:

{% highlight c++ %}
template <typename ...T>
struct tuple {
    // Somehow compute the size required by a tuple of Ts.
    // This must take into account alignment & al.
    static constexpr std::size_t total_size = /* magic */;

    // Store the elements in raw storage!
    typename std::aligned_storage<total_size>::type storage_;

    // Somehow return the address of the n-th element in the tuple.
    // This must also take into account alignment & al.
    constexpr void* raw_nth(std::size_t n) {
        return /* magic */;
    }

    explicit tuple(T const& ...t) {
        std::size_t i = 0;
        void* expand[] = {::new (this->raw_nth(i++)) T(t)...};
    }
};
{% endhighlight %}

How does this work? Basically, we put the tuple elements in `aligned_storage`,
and then initialize subparts of the storage as we require in the constructor.
But how might we access the n-th element of the tuple in a typed manner, i.e.
not a `void*`? Well, [we know how][pack-indexing] to get the n-th type of a
parameter pack and we can also get a `void*` to any element of the tuple, so
all that's left to do is to cast our `void*` to the proper type! And since
we're managing the raw storage ourselves, we know the cast will be legal:

{% highlight c++ %}
template <std::size_t i, typename ...T>
decltype(auto) get(tuple<T...> const& ts) {
    using Nth = typename nth_element<i, T...>::type;
    void const* nth = ts.raw_nth(i);
    return *static_cast<Nth const*>(nth);
}
{% endhighlight %}

Cool, but what's so special about this technique? Well, it turns out that the
the total size of the `aligned_storage` and the address of the n-th element
can both be computed in homogeneous `constexpr`-world, without costly template
instantiations. For example, computing the total size of the `aligned_storage`
would go like this:

{% highlight c++ %}
template <typename ...T>
constexpr std::size_t total_size() {
    std::size_t sizes[] = {sizeof(T)...};
    std::size_t alignments[] = {alignof(T)...};
    std::size_t total = 0;

    // Padd each member (but the last) so it is placed at an offset
    // which is a multiple of its alignment.
    for (std::size_t i = 0; i < sizeof...(T); ++i) {
        total += sizes[i];
        if (i != sizeof...(T) - 1)
            total += total % alignments[i + 1];
    }

    return total;
}
{% endhighlight %}

The index computation for `raw_nth` can be implemented similarly. But potential
compile-time improvements are not the only exciting thing about this technique.
Indeed, we could quite easily implement several optimizations like compressing
empty members, reordering the elements to optimize packing or even regroup
elements that are accessed most frequently to increase locality! And since
this is all just `constexpr` coding, it's much easier to do (efficiently)
than manipulating complex tuple representations like we saw in the other
techniques.

But this is not all. With this technique, we're stating very clearly the
operations that we're doing for the optimizer. Unlike with other techniques,
there is very little noise (so-called zero-cost abstractions) that could make
the code generation worst than expected. And it gets even better; this technique
allows us to write some heterogeneous algorithms in a very interesting way.
For example, consider our `partial_sum` example above. With this new tuple
implementation, the algorithm can be written in a very terse way. First,
let's add the following constructor to our tuple:

{% highlight c++ %}
struct uninitialized { };

template <typename ...T>
struct tuple {
    // ...

    explicit tuple(uninitialized const&) { }
};
{% endhighlight %}

This constructor creates a tuple, but without calling the constructor of any
element. What good is this? Well, it's very good since we'll leave the task of
initializing the storage of the tuple to our `partial_sum` algorithm. Take a
deep breath:

{% highlight c++ %}
template <typename ...T, typename State, typename F, std::size_t ...i>
auto partial_sum_impl(tuple<T...> const& ts, State const& state,
                      F const& f, std::index_sequence<0, i...>)
{
    // Assume nth_result<i> to be the type of the i-th element
    // of the result. It's fairly easy to compute, but it's left
    // out for simplicity.
    tuple<State, nth_result<i>...> result{uninitialized{}};

    void* expand[] = {
        ::new (result.raw_nth(0)) State(state),
        ::new (result.raw_nth(i)) nth_result<i>(
            f(get<i-1>(result), get<i-1>(ts))
        )...
    };

    return result;
}

template <typename ...T, typename State, typename F>
auto partial_sum(tuple<T...>const& ts, State const& state, F const& f)
{
    return partial_sum_impl(ts, state, f,
                std::make_index_sequence<sizeof...(T)+1>{});
}
{% endhighlight %}

The real meat of the algorithm is inside the body of `partial_sum_impl`. If you
expand the initializer list of the `expand` array, it looks somewhat like this:

{% highlight c++ %}
::new (result.raw_nth(0)) State(state)
::new (result.raw_nth(1)) nth_result<1>(f(get<0>(result), get<0>(ts)))
::new (result.raw_nth(2)) nth_result<2>(f(get<1>(result), get<1>(ts)))
...
{% endhighlight %}

We're initializing each element of the result tuple with the current element
of the input tuple, and the result accumulated so far (which is, incidentally,
the previous element of the result tuple). The twist is that we're using
placement-new to initialize each member of the tuple in-place, which means
that we're not making any copy whatsoever! Also notice how there are very
few template instantiations (apart from `f::operator()`, which can't be
avoided).

While I've been throwing quite a bit of flowers at this technique, notice
how this tuple can't be made `constexpr`, because placement-new can't appear
in constant expressions. This is a bit of a shame, but it's the language that
we have.

<!-- TODO: Finish this -->


## Conclusion

<!-- TODO: Write this -->


<!-- Links -->
[Boost.Hana]: https://github.com/boostorg/hana
[pack-indexing]: {% post_url 2015-11-29-efficient-parameter-pack-indexing %}