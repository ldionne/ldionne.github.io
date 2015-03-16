---
layout: post
title: Is constexpr a Functor?
comments: true
---

Trying to make the Constant data type a Functor. It won't work because we can't
take a runtime function and constexpr-map it over a constexpr structure.

{% highlight c++ linenos %}
//////////////////////////////////////////////////////////////////////////
// Model of Functor for Constants with a Functor
//////////////////////////////////////////////////////////////////////////
template <template <typename ...> class C, typename T>
struct models<Functor(C<T>), when<models<Constant(C<T>)>{} && models<Functor(T)>{}>>
    : detail::std::true_type
{ };

template <template <typename ...> class C, typename T>
struct transform_impl<C<T>, when<models<Constant(C<T>)>{} && models<Functor(T)>{}>> {
    template <typename X, typename F>
    static constexpr decltype(auto) apply(X, F f) {
        struct tmp {
            static constexpr decltype(auto) get() {
                return hana::transform(hana::value2<X>(), f); // <-- HERE
            }
        };
        return to<C<T>>(tmp{});
    }
};
{% endhighlight %}

Then, one might eventually think that Constant could be an Applicative, because
if you gave me a function inside a Constant, i.e. a constexpr function, then
I could map it over another constexpr value and give you back a Constant. So
the signature would be
{% highlight haskell %}
    ap :: Constant (a -> b) -> Constant a -> Constant b
{% endhighlight %}

which matches exactly the signature for `ap`:
{% highlight haskell %}
    <*> :: f (a -> b) -> f a -> f b
{% endhighlight %}

But the catch is that; we can't take a non-`constexpr` function and make it
`constexpr`, so we would never be able to implement `lift`!
{% highlight c++ %}
template <typename F>
constexpr auto lift(F f) {
    return ???;
}
{% endhighlight %}

Returning anything that depends on `f` would make the function non-`constexpr`
whenever `f` is not constexpr:
{% highlight c++ %}
void f(int) { }
// impossible unless lift ignores f
constexpr auto constexpr_f = lift(f);
{% endhighlight %}
