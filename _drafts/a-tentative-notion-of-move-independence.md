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
move-initialize `y` and `z`. So there's no chance of using an object after
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
do not move from subobjects that are used in both functions. More formally,
let $$F$$ be the set of non-static data members of `f`'s parameter that are
accessed by `f`, and $$G$$ be that of `g`. Then, let $$F_\text{move}$$ be the
set of non-static data members of `f`'s parameter that `f` moves from, and let
$$G_\text{move}$$ be that of `g`. With this set up, `f` and `g` can be said to
be move-independent with respect to their (sole) parameter if

$$
    F_\text{move} \bigcap G = F \bigcap G_\text{move} = \emptyset
$$

The idea here is that as long as a function does not use a subobject that is
moved from by the other function, then both functions can be called on the
same xvalue, because they could be different xvalues as far as they are
concerned. Also note that the above definition does not account for the
order in which `f` and `g` are called. Indeed, the above definition could
be relaxed if we know that `g` is always called after `f`. In this case, `g`
may move from whatever it wants as long as it does not access something that's
been moved-from by `f`.

While a mathematician might be satisfied with the above definition, it is
obviously not applicable to most real-world applications. Indeed, getting
the sets of non-static data members used and moved-from by a function is
at best error-prone, and usually impossible. Hence, it would be interesting
to have a better definition for the working programmer. Here's my attempt.
Let `x` and `y` be two objects such that `x == y`. Then, `f` and `g` are
move-independent if

{% highlight c++ %}
f(std::move(x));
g(std::move(x));
{% endhighlight %}

is equivalent to

{% highlight c++ %}
f(std::move(x));
g(std::move(y));
{% endhighlight %}

Here, the idea is that using the same xvalue in the calls to `f` and `g`
should be equivalent to using different xvalues in both calls, given that
these two xvalues are actually equal. This definition is arguably less formal
than the first one, but at least I think it is easier to ask oneself whether
this equivalence holds for two functions than with the former definition.

But where does this definition bring us? Basically, with this almost-rigorous
notion set up, I can ask for the following property to be satisfied by the `get`
functions that I use to extract elements of a tuple. For any two distinct
indices `i` and `j`, I need the two functions

{% highlight c++ %}
[](auto&& x){ auto copy = std::get<i>(std::forward<decltype(x)>(x)); }

[](auto&& x){ auto copy = std::get<j>(std::forward<decltype(x)>(x)); }
{% endhighlight %}

to be move-independent. In other words, I'm asking for the following expressions
not to produce a moved-from object, which is only reasonable:

{% highlight c++ %}
auto x = std::make_tuple(...);
auto a = std::get<i>(std::move(x));
auto b = std::get<j>(std::move(x));
{% endhighlight %}

With this in place, I can go back to my generic library and continue writing
stuff such as

{% highlight c++ %}
template <typename Tuple, std::size_t ...i>
void foo(Tuple&& tuple, std::index_sequence<i...>) {
    use(f<i>(std::forward<Tuple>(tuple))...);
    //  ^ for some function f, for example std::get
}
{% endhighlight %}

without worrying that I might be moving from an object twice, with the caveat
that I must require the `f<i>`s to be move-independent.


Conclusion
----------
While it is most of the time a bad idea to call `std::move` on an object
multiple times, this article presents a use case in which it is wiser to
do so than to use an alternative solution, i.e. decomposing a tuple into
its subobjects. In addition, this article attempts to define a notion of
move-independence that makes it easier for generic library writers to express
the constraint that a set of functions must not pull the rug from under
each other's feet.

As the title suggests, this article is experimental in the sense that the
notion of move-independence presented here shouldn't be taken as an absolute
reference. Indeed, I feel like a better formalization might be possible, and
I hope that people will chime in on the issue of better formalizing the usage
of `std::move` and `std::forward`.

[1]: http://oliora.github.io/2016/02/12/where-to-put-std-move.html
[2]: https://www.reddit.com/r/cpp/comments/45w3fs/whats_the_difference_between_stdmoveobjectmember/?ref=share&ref_source=link
[3]: http://en.cppreference.com/w/cpp/language/value_category
