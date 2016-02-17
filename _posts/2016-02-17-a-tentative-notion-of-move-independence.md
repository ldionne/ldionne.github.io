---
layout: post
title: A tentative notion of move-independence
comments: true
---

This post will try to answer a simple question: when is it legit to call
`std::move` twice on the same object? While some people would say "never",
I actually think the answer is more subtle. To capture these subtleties,
this article will introduce the concept of move-independence, which I
believe to be a useful notion to keep in mind when writing generic libraries.

A [recent article][1] posted on [Reddit][2] outlined the difference between
`std::move(object.member)` and `std::move(object).member`. The article looked
at potential pitfalls of using `std::move(object.member)` that would cause it
to undesirably move from a shared object, and concluded by recommending the
usage of `std::move(object).member`. I, for one, would tend to agree with this
advice. However, some people on Reddit criticized `std::move(object).member`
by saying that it encourages moving twice from `object`, and that it should
thus be avoided. This brings us to the question at hand: when is it okay to
call `std::move` multiple times on the same object?

> Note that the exact same question applies to `std::forward`, since this one
> is essentially a "use `std::move` when you can".

Well, first, if we don't do anything with the result of calling `std::move`,
then it is obviously safe to call it multiple times on the same object:

{% highlight c++ %}
auto x = ...;
std::move(x); // didn't do anything with it
std::move(x); // obviously safe to call std::move on x again
{% endhighlight %}

With this triviality out of the way, it hopefully becomes clear that we need
to study the behaviour of `std::move` _in relation_ to other operations we
wish to perform on the resulting [xvalue][3]. To put ourselves in the most
general setting, we'll consider two arbitrary operations performed on the
same xvalue. For convenience, we'll encode these operations as two functions
`f` and `g`, but there's no loss of generality because these functions could
do anything at all:

{% highlight c++ %}
auto x = ...;
f(std::move(x));
g(std::move(x));
{% endhighlight %}

Given the above, where `f` and `g` could do _anything_, our goal is now to
determine what properties must be satisfied by `f` and `g` for this code to
be safe. Of course, without any additional information about the behaviour of
`f` and `g`, the above code is usually a bad idea because if `f` decides to
move from `x`, we will be calling `g` on a moved-from object, which is most
likely not what we want. But before you scream and say this is _always_ wrong,
consider the following code:

{% highlight c++ %}
auto x = std::make_tuple("abc"s, "def"s);
auto abc = std::get<0>(std::move(x));
auto def = std::get<1>(std::move(x));
{% endhighlight %}

Clearly, this code is valid, because `std::get` takes its argument by reference
(so it does not move from `x`), and returns a reference to the `n`-th element
of the tuple. Precisely, the overload of `std::get` that's used is the one
taking a rvalue reference to the tuple, and returning a rvalue reference to
the `n`-th element. Since we're fetching two elements at different indices,
(rvalue) references to two different objects are returned and used to
move-initialize `abc` and `def`. So there's no chance of using an object after
it has been moved from, __in this case__. Now, you might ask whether this
is good style, and suggest that we use the following instead:

{% highlight c++ %}
auto x = std::make_tuple("abc"s, "def"s);
auto abc = std::move(std::get<0>(x));
auto def = std::move(std::get<1>(x));
{% endhighlight %}

However, this code has the exact same problem as the code in the motivating
article: if `x` is refactored to hold references to shared data instead,
we'll be moving from this shared data if we don't remove the calls to
`std::move`. That's a serious problem, because I write a lot of generic
code that looks like

{% highlight c++ %}
template <typename ...T, std::size_t ...i>
void foo(std::tuple<T...>&& tuple, std::index_sequence<i...>) {
  use(std::get<i>(std::move(tuple))...);
}
{% endhighlight %}

If I were to change it to `use(std::move(std::get<i>(tuple))...);`, and
if the `use` function moved from its arguments, I could end up hiding a
move-from-a-shared-object deep inside a generic library, and set my users
up for hours of fun debugging a crash in their application. And that, IMO,
is definitely not good style.

OK, so when is it safe to call two functions on the same xvalue? Intuitively,
it is when those two functions do not _interfere_ with each other, i.e. they
do not access subobjects that are used in both functions. More formally, let
$$F$$ be the set of non-static data members of `f`'s parameter that are
accessed by `f`, and let $$G$$ be that of `g`. Then, `f` and `g` can be
said to be move-independent with respect to their (sole) parameter if

$$
    F \bigcap G = \emptyset
$$

> Note that the notion could be extended to multi-valued functions, by
> considering the sets $$F_i$$ and $$G_i$$ of non-static data members of
> the `i`-th parameter accessed by `f` and `g`, respectively.

The idea here is that as long as a function does not use a subobject that
is used by the other function, then both functions can be called on the same
xvalue, and we can dispose of the result as we wish (i.e. move from it)
without risking to end up using a moved-from object. However, note that this
definition is too strict to my liking, because it makes the following two
functions non move-independent, while it would make sense to think that they
are:

{% highlight c++ %}
template <typename Tuple>
decltype(auto) f(Tuple&& t) {
  // just access the third element
  std::get<2>(std::forward<Tuple>(t));
  return std::get<0>(std::forward<Tuple>(t));
}

template <typename Tuple>
decltype(auto) g(Tuple&& t) {
  // again, just access the third element
  std::get<2>(std::forward<Tuple>(t));
  return std::get<1>(std::forward<Tuple>(t));
}
{% endhighlight %}

Also, while a mathematician might be satisfied with the above definition, it
is obviously not applicable to most real-world use cases. Indeed, getting
the set of non-static data members used by a function is at best error-prone,
and usually impossible or impractical. Hence, it would be interesting to
have a better definition for the working C++ programmer, one that could be used
to verify whether two functions are move-independent without looking at their
definition. Unfortunately, I was not able to get to such a definition without
constraining myself to the very specific case of accessor functions (like
`std::get`).

But even without a user-friendly definition, where does this bring us?
Well basically, I can ask for the functions I use to extract elements of
a tuple to be move-independent. In other words, for any two distinct indices
`i` and `j`, I need `f<i>` and `f<j>` to be move-independent, where `f` is
the function I wish to call on my tuple. This way, I can go back to my generic
library and continue writing stuff such as

{% highlight c++ %}
template <typename Tuple, std::size_t ...i>
void foo(Tuple&& tuple, std::index_sequence<i...>) {
  use(f<i>(std::forward<Tuple>(tuple))...);
  //  ^ for some function f, for example std::get
}
{% endhighlight %}

without worrying that I might be moving from an object twice. The most common
case, by far, is where `f == std::get` or some equivalent of it. However, it's
easy to come up with a use case where the functions would be something else,
such as slicing a tuple at two disjoint ranges of indices. For example, using
the [Boost.Hana][4] library,

{% highlight c++ %}
auto tuple = hana::make_tuple("abc"s, "def"s, "ghi"s, "jkl"s);
auto abc_def = hana::slice_c<0, 2>(std::move(tuple));
auto ghi_jkl = hana::slice_c<2, 4>(std::move(tuple));
{% endhighlight %}

This will create two tuples containing the first two and the last two elements
of the original `tuple`, moving them instead of doing a copy. This is safe to
perform, since we're slicing the tuple at two non-overlapping ranges. Plus,
it wouldn't be equivalent to write `std::move(hana::slice_c<0, 2>(tuple))`,
for the copy would have already been made inside of `hana::slice_c`, the
library being unable to rely on the fact that `tuple` can be moved from.


Conclusion
----------
While it is most of the time a bad idea to call `std::move` on an object
multiple times, this article presents a use case in which it makes a lot
of sense to do so, i.e. decomposing a tuple into its subobjects. In addition,
this article attempts to define a notion of move-independence that makes it
easier for generic library writers to express the constraint that a set of
functions must not pull the rug from under each other's feet when called on
a common xvalue.

As the title suggests, this article is experimental in the sense that the
notion of move-independence presented here shouldn't be taken as an absolute
reference. Indeed, I feel like a better formalization might be possible, and
I hope that people will chime in on the issue of better formalizing the usage
of `std::move` and `std::forward`.

[1]: http://oliora.github.io/2016/02/12/where-to-put-std-move.html
[2]: https://www.reddit.com/r/cpp/comments/45w3fs/whats_the_difference_between_stdmoveobjectmember/?ref=share&ref_source=link
[3]: http://en.cppreference.com/w/cpp/language/value_category
[4]: https://github.com/boostorg/hana
