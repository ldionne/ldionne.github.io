---
layout: post
title: Laziness as a Comonad
comments: true
---

This (first!) post will present my current understanding of how laziness can
be represented as a Comonad. The interest of seeing laziness as a Comonad
appeared while working on a C++ metaprogramming library called [Hana][1] as
a possible generalization of the `eval_if` construct from the [MPL][2] library.
More generally, anyone trying to implement a domain-specific language with
a notion of branching inside a strict language (as opposed to non-strict like
Haskell) will have to design a system to emulate laziness, in order to
evaluate only the branch that was picked by the condition. This post is
an attempt to describe such a system using the notion of Comonad.

To simplify this post, I'll assume the reader to be familiar with Comonads.
If that is not the case, there are several resources available online which
do a better job explaining Comonad than I would. For example, see [here][3],
[here][4] and [here][5].

It all started when I tried to define a structure that could be used to hold
lazy computations in C++. This structure would hold the function and the
arguments to call the function with, but it would only perform the call when
explicitly `eval`uated. It basically went like this:

{% highlight c++ %}
template <typename F, typename ...Args>
struct _lazy {
    F function;
    hana::_tuple<Args...> args;
};

template <typename F, typename ...Args>
auto eval(_lazy<F, Args...> expr) {
    return hana::unpack(expr.args, expr.function);
}

template <typename F>
auto lazy(F f) {
    return [=](auto ...args) -> _lazy<F, decltype(args)...> {
        return {f, {args...}};
    };
}
{% endhighlight %}

`_lazy` is straightforward; it's just a dumb holder for the function and
its arguments. Likewise, `eval` just takes a `_lazy` computation and calls
the function with its arguments. To do that, I use Hana's `unpack` method,
which satisfies

{% highlight c++ %}
unpack(make_tuple(x1, ..., xN), f) == f(x1, ..., xN)
{% endhighlight %}

Finally, there's the `lazy` function. It just creates a `_lazy` computation,
but it does it in a way that gives us a nice syntax. Let's say you have a
function `f`. Then, `lazy(f)` is a lambda that expects the arguments and
returns a lazy computation. Hence, `lazy(f)(args...)` is a lazy computation
of `f(args...)`. Achieving this (IMO) nice syntax is the reason why I'm
defining `lazy` as above instead of the (functionally equivalent)

{% highlight c++ %}
template <typename F, typename ...Args>
auto lazy(F f, Args ...args) {
    return _lazy<F, Args...>{f, {args...}};
}
{% endhighlight %}


### Generalized types: a bit of terminology

Before we go deeper, let me quickly introduce what I'll call generalized types
for the rest of this post. Basically, the idea is that when we see the type
`_lazy<F, Args...>`, we're actually thinking about a container that holds a
lazy value of type `std::result_of_t<F(Args...)>`. As a matter of fact, we
could actually implement `_lazy` as follows:

{% highlight c++ %}
template <typename T>
struct _lazy {
    std::function<T()> function;

    template <typename F, typename ...Args>
    explicit _lazy(F f, Args ...args)
        : function{[=]{ return f(args...); }}
    { }
};
{% endhighlight %}

We could then implement `eval` as

{% highlight c++ %}
template <typename T>
auto eval(_lazy<T> expr) {
    return expr.function();
}
{% endhighlight %}

However, providing a `_lazy<T>` type forces us to use type erasure via
`std::function`, which is less efficient than simply storing it like we
did in our first implementation of `_lazy`. Also, although this is not the
case here, it would make it impossible to use `_lazy` for metaprogramming
purposes since all the type information of the `F` and `Args...` template
arguments in the first implementation is erased in the second one. Providing
a `_lazy<T>` type also restricts the implementation more, since `_lazy<T>`
can only depend on the type of the computation's result, not on the type of
`F` and/or `Args...`. Finally, you have to _know_ the type of the computation
before creating and computing it, which is a huge problem when you're trying
to do type-level computations, since that type `T` is usually what you're
trying to compute! Of course, there are also advantages to using type erasure,
and sometimes there's no other option, but for our current use case it's just
not what we want.

To work around this, we could define a `Lazy(T)` concept that would represent
a lazy computation returning something of type `T`. The concept could be
roughly specified by saying that for any model `c` of `Lazy(T)`, the `eval(c)`
expression must be well-formed and it must represent an object of type `T` (or
implicitly convertible to `T` if you're feeling fancy). However, since there's
really only one model of that concept, I prefer saying that `Lazy(T)` is a
generalized type instead of a concept.

> __Note__: Obviously, there is more than a single model of `Lazy(T)`, since
> I just wrote two different models, one with type erasure and one without it.
> However, what I mean by "only one model" is that they are all equivalent, in
> some way which I'm still trying to define. I think it what I mean is unique
> up to unique isomorphism, but I don't grok the details well enough to be
> sure yet.

Anyway, the important point is that I'll refer to `_lazy<F, Args...>`
has having a _generalized type_ of `Lazy(T)` to mean that `eval`uating
such a lazy computation yields something of type `T`, happily disregarding
the fact that the _actual_ type of that computation is something else
entirely (`_lazy<F, Args...>`). In other words, I'll consider the _actual_
type of the lazy computation to be a simple implementation detail. In a
similar vein, I'll say that the pseudo-signature of `lazy` is:

$$
    \mathrm{lazy} : (T \to U) \to (T \to \mathrm{Lazy}(U))
$$

which means that `lazy` is a function which takes a function from `T` to `U`,
and returns a function which takes a `T` and returns a `Lazy(U)`. Note that I
voluntarily ignored the multi-argument case for simplicity.


### Laziness as a Functor

Ok; so far, we can create lazy computations and evaluate them. This can be
useful on its own, but we would like to develop a richer set of tools for
manipulating lazy computations. Being guided by the goal of making Lazy a
Comonad, let's recall the definition of the Comonad type class from the
Haskell [Control.Comonad package][6]:

{% highlight haskell %}
class Functor w => Comonad w where
    ...
{% endhighlight %}

We see that a Comonad should also be a Functor, so we'll start by trying to
make Lazy a Functor. To do this, we must define a function named `transform`
which has the following pseudo-signature (using generalized types instead of
types):

$$
    \mathrm{transform} : \mathrm{Lazy}(T) \times (T \to U) \to \mathrm{Lazy}(U)
$$

> __Note__:
> We're using `transform` instead of `fmap` because it's used in Hana and it's
> more familiar to C++ programmers.

Here, `Lazy(T)` simply represents a lazy computation which returns an
object of type `T`, and `T -> U` is a function taking a `T` and giving
back a `U`. Hence, `transform` should take a lazy computation of type
`T` and a function transforming `T`s into `U`s, and give back a new lazy
computation of type `U`. An obvious way of making this happen is to simply
return a new lazy computation doing the composition of the two functions:

{% highlight c++ %}
template <typename Computation, typename F>
auto transform(Computation computation, F f) {
    auto eval_f = [=](auto x) { return f(eval(x)); };
    return lazy(eval_f)(computation);
}
{% endhighlight %}

First, we create a new function `eval_f` which takes a lazy computation as
an argument and calls the `f` function with the evaluated result. Then, we
lazily apply that `eval_f` function to our lazy computation. The result
is a lazy computation which, when `eval`uated, will first evaluate the lazy
`computation` and then call `f` with the result. Also, the types match; if
`computation` is a lazy computation of type `T` (i.e. a `Lazy(T)`), and if
`f` is a function of type `T -> U`, then `transform(computation, f)` is
a lazy computation of type `U` (i.e. a `Lazy(U)`). We now have to make
sure the laws of Functor are satisfied, i.e.

{% highlight c++ %}
transform(computation, id) == computation

transform(transform(computation, f), g)
    == transform(computation, compose(g, f))
{% endhighlight %}

where `compose(g, f)` is simply function composition, i.e.

{% highlight c++ %}
compose(g, f) == [](auto x) {
    return g(f(x));
};
{% endhighlight %}

As a side note, let's observe that our definition of `transform` above is
actually equivalent to

{% highlight c++ %}
template <typename Computation, typename F>
auto transform(Computation computation, F f) {
    return lazy(compose(f, eval))(computation);
}
{% endhighlight %}

with the definition of `compose` we just gave. Now, both laws are
straightforward to prove, but before we prove them we must define what
it means for two computations to be equal. Intuitively, I would think of
two lazy computations as being equal if their evaluated result is equal,
and so we'll use that definition of equality. In other words, for any two
lazy computations `a` and `b`,
{% highlight c++ %}
a == b    if and only if    eval(a) == eval(b)
{% endhighlight %}

> __Note__: Making sure this is an equivalence relation is easy and left
> as an exercise.


### Proving the Functor laws
> __Note__: This part is technical and may be skipped. It's there because
> proofs are important, but there's no real insight to be gained here.

To prove the first law, we just expand `transform` from its definition and
then use basic properties of `compose`.

{% highlight c++ %}
transform(comp, id)
    // from the definition of transform
    == lazy(compose(id, eval))(comp)

    // because compose(id, anything) == anything
    == lazy(eval)(comp)

    // because eval(lazy(eval)(comp)) == eval(comp)
    // by definition of eval
    == comp
{% endhighlight %}

As for the second law, on the left hand side we have

{% highlight c++ %}
transform(transform(comp, f), g)
    // definition of transform applied to transform(comp, f)
    == transform(lazy(compose(f, eval))(comp), g)

    // definition of transform applied to transform(..., g)
    == lazy(compose(g, eval))(lazy(compose(f, eval))(comp))
{% endhighlight %}

Let's now evaluate this last expression:

{% highlight c++ %}
eval(lazy(compose(g, eval))(lazy(compose(f, eval))(comp)))
    // by definition of eval
    == compose(g, eval)(lazy(compose(f, eval))(comp))

    // by definition of compose
    == g(eval(lazy(compose(f, eval))(comp)))

    // by definition of eval, again
    == g(compose(f, eval)(comp))

    // by definition of compose, again
    == g(f(eval(comp)))
{% endhighlight %}

On the right hand side, by definition of `transform`, we have

{% highlight c++ %}
transform(comp, compose(g, f))
    == lazy(compose(compose(g, f), eval))(comp)
{% endhighlight %}

Evaluating this gives:

{% highlight c++ %}
eval(lazy(compose(compose(g, f), eval))(comp))
    // by definition of eval
    == compose(compose(g, f), eval)(comp)

    // by definition of compose applied twice
    == g(f(eval(comp)))
{% endhighlight %}

Which tells us that the second law holds too. Phew!


### Laziness as a Comonad
Now that we have a Functor, let's look at the full definition of the Comonad
type class and see what we can do with it:

{% highlight haskell %}
class Functor w => Comonad w where
    extract :: w a -> a
    duplicate :: w a -> w (w a)
    extend :: (w a -> b) -> w a -> w b
{% endhighlight %}

Since our goal is to make `Lazy` a Comonad, the obvious first step is to
substitute `Lazy` in that definition and see what happens:

{% highlight haskell %}
extract :: Lazy a -> a
duplicate :: Lazy a -> Lazy (Lazy a)
extend :: (Lazy a -> b) -> Lazy a -> Lazy b
{% endhighlight %}

Looking at `extract`, it has exactly the same type as `eval`. `duplicate`
does not seem to be very useful, but we could definitely write a function
with that signature. Finally, `extend` looks like it should just turn a
computation accepting a lazy argument into a lazy computation, which we
can also implement quite easily. We can also observe that the type of
`extend` is actually pretty close to that of `lazy`:

$$
    \mathrm{lazy} : (T \to U) \to (T \to Lazy(U))
$$

$$
    \mathrm{extend} : (Lazy(T) \to U) \to (T \to Lazy(U))
$$

> __Note__: In Haskell, `->` is right associative, so `A -> B -> C` is
> actually equivalent to `A -> (B -> C)`, which is why `extend` in the
> above type class is equivalent to the `extend` just above.

Let's go. First, `extract` is just `eval`:

{% highlight c++ %}
template <typename Computation>
auto extract(Computation computation) {
    return eval(computation);
}
{% endhighlight %}

Second, `duplicate`:

{% highlight c++ %}
template <typename Computation>
auto duplicate(Computation computation) {
    return lazy([=]{ return computation; })();
}
{% endhighlight %}

For the explanation, let's say `computation` is a lazy computation of type
`T`, i.e. `computation` has a generalized type of `Lazy(T)`. What happens here
is that we create a nullary function returning the lazy computation, and we
then apply it lazily. `eval`uating that gives us back our `computation`, which
as a generalized type of `Lazy(T)`, and hence `lazy([=]{ return computation; })()`
has a generalized type of `Lazy(Lazy(T))`, since you must `eval` it twice to
get back the initial `T`.

Thirdly, let's implement `extend`. Strictly speaking, this would not be
required because `extend` can be implemented in terms of `duplicate` and
`transform`, but we'll still do it for completeness:

{% highlight c++ %}
// F :: Lazy(T) -> U
// Computation :: Lazy(T)
// We want to return a Lazy(U)
template <typename F, typename Computation>
auto extend(F f, Computation computation) {
    return lazy([=]{ return f(computation); })();
}
{% endhighlight %}

We're given a lazy computation of generalized type `Lazy(T)` and a function
of type `Lazy(T) -> U`. What we do is create a function of type `() -> U`
with the `[=]{ return f(computation); }` closure. We then apply this lazily,
which effectively gives us a `Lazy(U)`.


### Proving the Comonad laws
> __Note__: Again, this part is mostly technical. Feel free to skip or to
> skim through.

So far so good; we have definitions for an hypothetical Comonad representing
laziness. The last thing we must check is whether the properties of a Comonad
are respected by `Lazy`. There are two equivalent sets of laws that must be
satisfied in order to be a Comonad, so we can pick either of those. Since I
find it easier, I'll pick the one with `duplicate` and `transform`. Translated
from Haskell to C++, the laws look like:

{% highlight c++ %}
extract(duplicate(comp)) == comp
transform(duplicate(comp), extract) == comp
duplicate(duplicate(comp)) == transform(duplicate(comp), duplicate)
{% endhighlight %}

Let's prove the first law:

{% highlight c++ %}
extract(duplicate(comp))
    // by definition of duplicate
    == extract(lazy([=]{ return comp; })())

    // by definition of extract as eval
    == [=]{ return comp; }()
    == comp
{% endhighlight %}

This one was easy. Let's prove the second law:

{% highlight c++ %}
transform(duplicate(comp), extract)
    // by definition of transform
    == lazy(compose(extract, eval))(duplicate(comp))

    // because extract == eval
    == lazy(compose(eval, eval))(duplicate(comp))

    // by definition of duplicate
    == lazy(compose(eval, eval))(lazy([=]{ return comp; })())
{% endhighlight %}

Evaluating this last expression gives

{% highlight c++ %}
eval(lazy(compose(eval, eval))(lazy([=]{ return comp; })()))
    // by definition of eval
    == compose(eval, eval)(lazy([=]{ return comp; })())

    // by definition of compose
    == eval(eval(lazy([=]{ return comp; })()))

    // by definition of eval, again
    == eval([=]{ return comp; }())
    == eval(comp)
{% endhighlight %}

Hence,

{% highlight c++ %}
eval(transform(duplicate(comp), extract)) == eval(comp)
{% endhighlight %}

which proves the second law because `a == b` if and only if `eval(a) == eval(b)`.
This was a bit more involved. Let's go for the third and last law. On the
left hand side, we have

{% highlight c++ %}
duplicate(duplicate(comp))
{% endhighlight %}

Evaluating this, we find

{% highlight c++ %}
eval(duplicate(duplicate(comp)))
    // because eval == extract and the first law
    == duplicate(comp)
{% endhighlight %}

We now evaluate the right hand side:

{% highlight c++ %}
eval(transform(duplicate(comp), duplicate))
    // by definition of transform
    == eval(lazy(compose(duplicate, eval))(duplicate(comp)))

    // by definition of eval
    == compose(duplicate, eval)(duplicate(comp))

    // by definition of compose
    == duplicate(eval(duplicate(comp)))

    // because eval == extract and the first law
    == duplicate(comp)
{% endhighlight %}

Hence, both expressions are equal and the third law holds. Phew!


### So Lazy is a Comonad. What now?

My initial goal was to generalize the `eval_if` construct from the MPL so it
could work with arbitrary Comonads. Here's how we could now specify the type
of an hypothetical `eval_if` working not only on types (as in the MPL). Given
an arbitrary Comonad `W`,

$$
    \mathrm{eval\_if} : \mathrm{bool} \times W(T) \times W(T) \to T
$$

For the special case of `Lazy`, this gives

$$
    \mathrm{eval\_if} : \mathrm{bool} \times \mathrm{Lazy}(T) \times \mathrm{Lazy}(T) \to T
$$

which effectively matches our expectations. Since infinite streams can be
represented as Comonads, we could, for example, peek the first element of
one of two streams based on a boolean condition using `eval_if`. Whether
this is useful is debatable and I'm not sure yet; honestly I have not
found an interesting use case besides `Lazy` so far, but at least we
did not lose anything. However, if we go a bit further down the rabbit
hole, we can also see that `Lazy` is an Applicative Functor and a Monad.
This observation is super useful because it tells us how to compose `Lazy`
computations. The Functor/Applicative/Monad part is already implemented in
Hana (see [here][7]), so I won't explain it further here. I'm not sure yet
whether Hana will be supporting arbitrary Comonads as branches to a
conditional expression, but one thing is certain: the conditionals
will have to play well with the composition of `Lazy` computations.


[1]: http://github.com/ldionne/hana
[2]: http://www.boost.org/doc/libs/release/libs/mpl/doc/index.html
[3]: http://stackoverflow.com/questions/8428554/what-is-the-comonad-typeclass-in-haskell
[4]: http://comonad.com/haskell/Comonads_1.pdf
[5]: http://www.haskellforall.com/2013/02/you-could-have-invented-comonads.html
[6]: https://hackage.haskell.org/package/comonad-4.2.4/docs/Control-Comonad.html
[7]: https://github.com/ldionne/hana/blob/a8586ec1812e14e43dfd6867209412aa1d254e1a/include/boost/hana/lazy.hpp#L102