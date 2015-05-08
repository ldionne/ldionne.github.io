---
layout: post
title: Move-independence and why you should care
comments: true
---

This post will try to answer a simple question. When is it legit to write the
following code?
{% highlight c++ %}
auto x = ...;
auto y = f(std::move(x));
auto z = g(std::move(x));
{% endhighlight %}

Notice how we call `std::move` on `x` twice. This is usually a bad idea,
because if `f` decides to move from `x`, we will be calling `g` on a
moved-from object, which is most likely not what we want. But before you
scream and say this is _always_ wrong, consider the following code:
{% highlight c++ %}
auto x = std::make_tuple("abc"s, "def"s);
auto y = std::get<0>(std::move(x));
auto z = std::get<1>(std::move(x));
{% endhighlight %}

Clearly, this code is _valid_, because `std::get` takes by reference (so it
does not move from `x`) and returns a reference to the `n`-th element of the
tuple. Precisely, the overload of `std::get` that's used is the one taking a
rvalue reference to the tuple, and returning a rvalue reference to the `n`-th
element. Since we're fetching two elements at different indices, (rvalue)
references to two different objects are returned and used to move-initialize
`y` and `z`. However, a valid question is whether this is _good style_.
Indeed, why not instead write
{% highlight c++ %}
auto x = std::make_tuple("abc"s, "def"s);
auto y = std::move(std::get<0>(x));
auto z = std::move(std::get<1>(x));
{% endhighlight %}

Notice how we are now moving the result of `std::get` instead of its argument.
The final result is the same as before, but now the overload of `std::get` for
lvalue references is used instead. This overload returns a lvalue reference to
the `n`-th element of the tuple, which we turn into a rvalue reference with
`std::move` and then use to move-initialize `y` and `z`, respectively. Agreed,
in this simple case, the second solution is probably better. However, it is
not always possible to do so. For example, consider the following code, which
happens quite often in generic programming and even more in metaprogramming:
{% highlight c++ %}

{% endhighlight %}

///////////////////////////////////////////////////////////////////////////////









Let's say I have a `std::pair`, but I don't know the exact type of its
elements:

    auto p = std::make_pair(...);

Let's also say that I can't use `p.first` and `p.second` to access its
elements, but that I must instead use a function named `get_first` and a
function named `get_second`:

    auto fst = get_first(p);
    auto snd = get_second(p);

`get_first` and `get_second` presumably just return `p.first` and `p.second`,
but let's assume for now that they could do anything they want in addition
to returning `p.first` and `p.second`.

Here's my question: How would you _optimally_ decompose `p` into its two
components, knowing that we are done with `p` and hence we can move from it?

    auto p = std::make_pair(...);
    auto&& fst = ?;
    auto&& snd = ?;

When I say optimally, I mean that `fst` and `snd` should be rvalue references
whenever possible, i.e. the above should be equivalent to

    auto p = std::make_pair(...);
    auto&& fst = std::move(p).first;
    auto&& snd = std::move(p).second;

If that is impossible to do with the current (lack of) restrictions on
`get_first` and `get_second`, then you can also suggest constraints that
we could put on them so that it becomes possible.


First attempt
-------------

    auto p = std::make_pair(...);
    auto&& fst = get_first(p);
    auto&& snd = get_second(std::move(p));

This is not optimal, because we're not retrieving `p.first` as an rvalue
reference when it sometimes could be.


Second attempt
--------------

    auto p = std::make_pair(...);
    auto&& fst = get_first(std::move(p));
    auto&& snd = get_second(std::move(p));

In the case where `get_first` and `get_second` are really just equivalent
to `p.first` and `p.second`, this is optimal because we'll get rvalue
references to both `p.first` and `p.second`. However, if `get_first` and
`get_second` are nasty and they do more than just returning the member,
this could give us a reference to a moved-from object:

    auto get_first = [](auto&& pair) -> decltype(auto) {
        // This might move-construct snd, making pair.second a moved-from object.
        auto snd = std::forward<decltype(pair)>(pair).second;
        return std::forward<decltype(pair)>(pair).first;
    };

    auto get_second = [](auto&& pair) -> decltype(auto) {
        return std::forward<decltype(pair)>(pair).second;
    };

    auto p = std::make_pair(...);
    auto&& fst = get_first(std::move(p));
    auto&& snd = get_second(std::move(p)); // reference to the moved-from p.second

Also, even if `get_first` and `get_second` were not nasty and just returned
`p.first` and `p.second`, this solution would still feel wrong because using
`std::move` twice on the same object feels wrong. Is that feeling justified?


Third attempt
-------------

    using First = decltype(get_first(std::move(p)));
    using Second = decltype(get_second(std::move(p)));
    auto&& fst = std::forward<First>(get_first(p));
    auto&& snd = std::forward<Second>(get_second(p));

TODO


A note on the question itself: Of course, this question is a metaphor for my
actual use case. Don't tell me "just use p.first and p.second", because that
would be irrelevant to my actual use case. You should instead view this
question as a general question on perfect forwarding through accessors.

