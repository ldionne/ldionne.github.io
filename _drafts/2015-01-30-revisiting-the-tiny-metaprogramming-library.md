---
layout: post
title: Revisiting the Tiny Metaprogramming Library
comments: true
---

This post is an answer to both Eric Niebler's [Tiny Metaprogramming
Library][1] post in last November, which presented a small library for template
metaprogramming, and to [this thread][2] on the Boost.Devel mailing list. Having
worked on the [MPL11][3] and now [Boost.Hana][4], I am pretty familiar with C++
metaprogramming, the challenges it presents and also its compile-time performance
implications. Hence, I thought I would give a small comparison of different ways
to implement the functionality in Eric's post using his `meta` library, the
Boost.MPL, the MPL11 and finally Hana.

Eric's goal was to implement the `std::tuple_cat` function from the standard
library, whose signature is:

{% highlight c++ %}
template <typename ...Tuples>
constexpr std::tuple<CTypes...> tuple_cat(Tuples&& ...tuples);
{% endhighlight %}

The general strategy used by Eric is well explained in his post, but here's
the gist of it. First, we create a tuple containing references to each argument
tuple. This tuple of tuples can actually be seen as a 2 dimensional array
containing heterogeneous objects:

{% highlight c++ %}
[
    [x00, x01, x02, x03, ..., x0?], // first argument tuple
    [x10, x11, x12, x13, ..., x1?], // second argument tuple
    ...
    [xk0, xk1, xk2, xk3, ..., xk?]  // k-th argument tuple
]
{% endhighlight %}

However, note that each argument tuple may have a different number of elements.
This view as a 2-dimensional array is the key for the next part. What we do next
is compute two sequences of indices, one which holds the row number of each
element in the two-dimensional array, and the other one which holds its column
number. So if you call your 2-dimensional array `a` and your indices `i` and `j`,
respectively, then `a[i][j]` contains the element `xij` in the array above. Then,
if you have these sequences in the form of parameter packs, and if you know the
type of the tuple you must return (let's call it `Res` for now, more on this
below), it becomes very easy to implement `std::tuple_cat`:

{% highlight c++ linenos %}
template <typename Res, typename Tuples, std::size_t ...i,
                                         std::size_t ...j>
Res tuple_cat_impl(Tuples&& tuples, std::index_sequence<i...>,
                                    std::index_sequence<j...>)
{
    return Res{
        std::get<j>(std::get<i>(std::forward<Tuples>(tuples)))...
    };
}
{% endhighlight %}

The `Res` type can be computed in the following way, which is not compile-time
efficient if you try to concatenate a truckload of tuples, but it should not
matter for most practical purposes:

{% highlight c++ linenos %}
template <typename ...Tuples>
struct cat_result;

template <typename ...Tuples>
using cat_result_t = typename cat_result<Tuples...>::type;

template <>
struct cat_result<> {
    using type = std::tuple<>;
};

template <typename Tuple>
struct cat_result<Tuple> {
    using type = Tuple;
};

template <typename ...T, typename ...U, typename ...Tuples>
struct cat_result<std::tuple<T...>, std::tuple<U...>, Tuples...>
    : cat_result<std::tuple<T..., U...>, Tuples...>
{ };
{% endhighlight %}

The hard part is computing the indices at compile-time, and that is what
we will be focusing on for the rest of this post.


## The `meta` library
The final solution Eric ends up with is this classic metaprogramming solution:

{% highlight c++ linenos %}
namespace detail
{
    template<typename Ret, typename...Is, typename ...Ks,
        typename Tuples>
    Ret tuple_cat_(typelist<Is...>, typelist<Ks...>,
        Tuples tpls)
    {
        return Ret{std::get<Ks::value>(
            std::get<Is::value>(tpls))...};
    }
}

template<typename...Tuples,
    typename Res =
        typelist_apply_t<
            meta_quote<std::tuple>,
            typelist_cat_t<typelist<as_typelist_t<Tuples>...> > > >
Res tuple_cat(Tuples &&... tpls)
{
    static constexpr std::size_t N = sizeof...(Tuples);
    // E.g. [0,0,0,2,2,2,3,3]
    using inner =
        typelist_cat_t<
            typelist_transform_t<
                typelist<as_typelist_t<Tuples>...>,
                typelist_transform_t<
                    as_typelist_t<make_index_sequence<N> >,
                    meta_quote<meta_always> >,
                meta_quote<typelist_transform_t> > >;
    // E.g. [0,1,2,0,1,2,0,1]
    using outer =
        typelist_cat_t<
            typelist_transform_t<
                typelist<as_typelist_t<Tuples>...>,
                meta_compose<
                    meta_quote<as_typelist_t>,
                    meta_quote_i<std::size_t, make_index_sequence>,
                    meta_quote<typelist_size_t> > > >;
    return detail::tuple_cat_<Res>(
        inner{},
        outer{},
        std::forward_as_tuple(std::forward<Tuples>(tpls)...));
}
{% endhighlight %}

I won't explain how the code works, since the original article does that just
fine. The code is much better than what we could have done in C++03, but there
is still a lot of syntactic noise. Consider `meta_quote` and `meta_quote_i`;
they exist solely because templates are not first-class citizens of the
metaprogramming world (types are, but not templates), so we sometimes have to
lift templates to metafunction classes (which are types) before we can use them.

## The `Boost.MPL` library
Let's now try to implement the same thing using the Boost.MPL library, which
has been the de-facto metaprogramming standard for years. First, we'll have
to figure out the return type of `std::tuple_cat`. That's fairly easy:

{% highlight c++ linenos %}
template <typename Tuple>
struct as_vector
    : as_vector<std::remove_cv_t<std::remove_reference_t<Tuple>>>
{ };

template <typename Tuple>
struct as_vector<std::tuple<T...>> {
    using type = mpl::vector<T...>;
};

template <typename ...Tuples>
struct tuple_cat_result
    : mpl::fold<
        mpl::vector<typename as_vector<Tuples>::type...>,
        mpl::vector<>,
        mpl::insert_range<mpl::_1, mpl::end<mpl::_1>, mpl::_2>
    >
{ };
{% endhighlight %}

TODO: Finish this up


## The MPL11 library
TODO: Finish this up


## The Hana library





## What about performance?
TODO: measure the compile-time (and runtime?) performance. then measure the
      performance of computing the indices only


## Hana (the actual implementation)
... show the actual constexpr-based code for Tuple


## Conclusions
...



[1]: http://ericniebler.com/2014/11/13/tiny-metaprogramming-library/
[2]: http://thread.gmane.org/gmane.comp.lib.boost.devel/257680
[3]: https://github.com/ldionne/mpl11
[4]: https://github.com/ldionne/hana