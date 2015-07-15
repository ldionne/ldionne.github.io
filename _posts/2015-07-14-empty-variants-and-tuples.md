---
layout: post
title: A mathematical intuition for empty variants and tuples
comments: true
tags: variant tuple "product type" "sum type"
---

The goal of this post is to introduce sum types and product types from a
very high level perspective, and to try and derive an intuition for what
the meaning of `tuple<>` and `variant<>` should be. The mathematics in
this post are purposefully kept a bit vague, because being more formal
would make the post heavier.

> This post was lifted from an email I was going to send in answer to
> [this message][1] on the Boost.Devel mailing list. The email was getting
> a bit long and I thought it was a good candidate for a short blog post,
> so here we are. Check out the initial thread if you're interested in
> the `std::variant` proposal.

Without getting into category theoretical stuff, we know that a tuple is
what's called a _product type_, i.e. the type `tuple<A, B>` is essentially
equivalent to the cartesian product of the types `A` and `B`. Similarly, a
variant is what's called a _sum type_, i.e. the type `variant<A, B>` is
essentially equivalent to a disjoint union of `A` and `B`, which is a set
containing all elements of `A` and all elements of `B`, with duplicates.

Tuples and variants with arities other than 2 are simply syntactic sugar.
Indeed, we have that

$$
    \mathtt{tuple<A, B, C>} \simeq \mathtt{tuple<A, tuple<B, C>>} \\
    \mathtt{variant<A, B, C>} \simeq \mathtt{variant<A, variant<B, C>>}
$$

where $$\simeq$$ means "is morally equivalent to". This relation of being
"morally equivalent to" can be formalised with the notion of [isomorphism][2]
in category theory. We won't define isomorphisms in this post to keep things
light, but for the purpose of this post you can think of two types as being
"morally equivalent" if they can be converted to one another _without any loss
of information_.

Returning to the above equivalences, we can see that arities higher than 2 are
really just provided for convenience. Indeed, you could express a n-ary tuple
or variant by applying the above equations recursively, and you would always
end up with nested binary tuples or variants. Hence, for the remainder of this
post, we will only consider tuples and variants with arities lower than or
equal to 2. Continuing on this road, we can see that some types have a special
interaction with these product types and sum types. Indeed, consider the
following type, often called the "unit type":

{% highlight c++ %}
struct Unit { };
{% endhighlight %}

This is a singleton type, which contains only one possible value. Not very
interesting, except that when you use it in a tuple, it makes the tuple
"morally equivalent" to the other type in the tuple:

$$
    \mathtt{tuple<Unit, T>} \simeq \mathtt{T}
$$

Indeed, that is because the `Unit` type does not bring any new information
to the tuple, so having a `tuple<Unit, T>` and having a `T` carries the same
amount of information. If you think about a tuple as a product, you can kinda
imagine the `Unit` type as being `1`:

$$
    \mathtt{tuple<Unit, T>} \simeq \mathtt{T} \\
    1 \times x = x
$$

Now, suppose you had the following type, often called the "bottom type":

{% highlight c++ %}
struct Bottom {
    Bottom() = delete;
    Bottom(Bottom const&) = delete;
};
{% endhighlight %}

This unfriendly type is such that you can never create an object of that type.
Whereas the `Unit` type was like a set containing a single element, this type
is like an empty set, because there is no object of that type. Now, that object
interacts with variants in a similar way as `Unit` did with tuples:

$$
    \mathtt{variant<Bottom, T>} \simeq \mathtt{T}
$$

Indeed, having a `variant<Bottom, T>` is like having "either a `Bottom` or a
`T`". However, you know you can never have a `Bottom`, because an object of
this type can't exist. The only possibility left is that you have a `T`, which
makes `variant<Bottom, T>` "morally equivalent" to `T`. If you think of a
variant as a sum, you can kinda imagine the `Bottom` type as being `0`:

$$
    \mathtt{variant<Bottom, T>} \simeq \mathtt{T} \\
    0 + T = T
$$

Back to the initial question about lower arity tuples and variants. First, I
think no one will be surprised if I say that unary tuples and variants should
satisfy

$$
    \mathtt{tuple<T>} \simeq \mathtt{T} \\
    \mathtt{variant<T>} \simeq \mathtt{T}
$$

This is indeed not very surprising if you think about the meaning of a tuple
and a variant; clearly, a tuple or a variant with a single type in it should
be "morally equivalent" to the type itself. Now, for the empty tuples and
variants, I think the most mathematically correct definition would be

$$
    \mathtt{tuple<>} \simeq \mathtt{Unit} \\
    \mathtt{variant<>} \simeq \mathtt{Bottom}
$$

Hence, you could create an object of type `tuple<>`, but you can't create an
object of type `variant<>`; trying to do so should be a compile-time error.
This intuition is backed by the usual behavior of products and sums on empty
collections as defined in mathematics. Indeed, we normally define

$$
    \prod_{x \, \in \, \emptyset} x = 1 \\
    \sum_{x \, \in \, \emptyset} x = 0
$$

This is convenient in mathematics, and I also find it quite intuitive.
Naively, I think the design of `variant<>` and `tuple<>` should follow this.

### Edits
15 Jul 2015: Add more information about isomorphisms and reformulate the intro

[1]: http://thread.gmane.org/gmane.comp.lib.boost.devel/261503/focus=261881
[2]: https://en.wikipedia.org/wiki/Isomorphism
