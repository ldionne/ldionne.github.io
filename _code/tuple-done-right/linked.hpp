/*
@copyright Louis Dionne 2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

template <bool>
struct when;


template <unsigned int n, typename = when<true>>
struct tuple_impl;

template <>
struct tuple_impl<0> {
    template <typename ...>
    struct result { };
};

template <>
struct tuple_impl<1> {
    template <typename Xs>
    static constexpr decltype(auto) get(Xs&& xs)
    { return static_cast<Xs&&>(xs).t1; }

    template <typename T1>
    struct result {
        T1 t1;
    };
};

template <>
struct tuple_impl<2> {
    template <typename Xs>
    static constexpr decltype(auto) get(Xs&& xs)
    { return static_cast<Xs&&>(xs).t2; }

    template <typename T1, typename T2>
    struct result {
        T1 t1; T2 t2;
    };
};

template <>
struct tuple_impl<3> {
    template <typename Xs>
    static constexpr decltype(auto) get(Xs&& xs)
    { return static_cast<Xs&&>(xs).t3; }

    template <typename T1, typename T2, typename T3>
    struct result {
        T1 t1; T2 t2; T3 t3;
    };
};

template <>
struct tuple_impl<4> {
    template <typename Xs>
    static constexpr decltype(auto) get(Xs&& xs)
    { return static_cast<Xs&&>(xs).t4; }

    template <typename T1, typename T2, typename T3, typename T4>
    struct result {
        T1 t1; T2 t2; T3 t3; T4 t4;
    };
};

template <>
struct tuple_impl<5> {
    template <typename Xs>
    static constexpr decltype(auto) get(Xs&& xs)
    { return static_cast<Xs&&>(xs).t5; }

    template <typename T1, typename T2, typename T3, typename T4, typename T5>
    struct result {
        T1 t1; T2 t2; T3 t3; T4 t4; T5 t5;
    };
};

template <>
struct tuple_impl<6> {
    template <typename Xs>
    static constexpr decltype(auto) get(Xs&& xs)
    { return static_cast<Xs&&>(xs).t6; }

    template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
    struct result {
        T1 t1; T2 t2; T3 t3; T4 t4; T5 t5; T6 t6;
    };
};

template <unsigned int n>
struct tuple_impl<n, when<(n >= 7) && (n < 14)>> {
#error how to implement this

    template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename ...Tn>
    struct result : tuple_impl<sizeof...(Tn)>::template result<Tn...> {
        using base = typename tuple_impl<sizeof...(Tn)>::template result<Tn...>;
        constexpr base& rest() & { return *this; }
        constexpr base const& rest() const& { return *this; }
        constexpr base&& rest() && { return *this; }

        T1 t1; T2 t2; T3 t3; T4 t4; T5 t5; T6 t6; T7 t7;
    };
};

template <unsigned int n>
struct tuple_impl<n, when<(n >= 14) && (n < 28)>> {
    template <
        typename T1, typename T2, typename T3,  typename T4,  typename T5,  typename T6,  typename T7,
        typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14,
        typename ...Tn
    >
    struct result : tuple_impl<sizeof...(Tn)>::template result<Tn...> {
        using base = typename tuple_impl<sizeof...(Tn)>::template result<Tn...>;
        constexpr base& rest() & { return *this; }
        constexpr base const& rest() const& { return *this; }
        constexpr base&& rest() && { return *this; }

        T1  t1;  T2  t2;  T3  t3;  T4  t4;  T5  t5;  T6  t6;  T7  t7;
        T8  t8;  T9  t9;  T10 t10; T11 t11; T12 t12; T13 t13; T14 t14;
    };
};

template <unsigned int n>
struct tuple_impl<n, when<(n >= 28) && (n < 56)>> {
    template <
        typename T1,  typename T2,  typename T3,  typename T4,  typename T5,  typename T6,  typename T7,
        typename T8,  typename T9,  typename T10, typename T11, typename T12, typename T13, typename T14,
        typename T15, typename T16, typename T17, typename T18, typename T19, typename T20, typename T21,
        typename T22, typename T23, typename T24, typename T25, typename T26, typename T27, typename T28,
        typename ...Tn
    >
    struct result : tuple_impl<sizeof...(Tn)>::template result<Tn...> {
        using base = typename tuple_impl<sizeof...(Tn)>::template result<Tn...>;
        constexpr base& rest() & { return *this; }
        constexpr base const& rest() const& { return *this; }
        constexpr base&& rest() && { return *this; }

        T1  t1;  T2  t2;  T3  t3;  T4  t4;  T5  t5;  T6  t6;  T7  t7;
        T8  t8;  T9  t9;  T10 t10; T11 t11; T12 t12; T13 t13; T14 t14;
        T15 t15; T16 t16; T17 t17; T18 t18; T19 t19; T20 t20; T21 t21;
        T22 t22; T23 t23; T24 t24; T25 t25; T26 t26; T27 t27; T28 t28;
    };
};

template <unsigned int n>
struct tuple_impl<n, when<(n >= 56)>> {
    template <
        typename T1,  typename T2,  typename T3,  typename T4,  typename T5,  typename T6,  typename T7,
        typename T8,  typename T9,  typename T10, typename T11, typename T12, typename T13, typename T14,
        typename T15, typename T16, typename T17, typename T18, typename T19, typename T20, typename T21,
        typename T22, typename T23, typename T24, typename T25, typename T26, typename T27, typename T28,
        typename T29, typename T30, typename T31, typename T32, typename T33, typename T34, typename T35,
        typename T36, typename T37, typename T38, typename T39, typename T40, typename T41, typename T42,
        typename T43, typename T44, typename T45, typename T46, typename T47, typename T48, typename T49,
        typename T50, typename T51, typename T52, typename T53, typename T54, typename T55, typename T56,
        typename ...Tn
    >
    struct result : tuple_impl<sizeof...(Tn)>::template result<Tn...> {
        using base = typename tuple_impl<sizeof...(Tn)>::template result<Tn...>;
        constexpr base& rest() & { return *this; }
        constexpr base const& rest() const& { return *this; }
        constexpr base&& rest() && { return *this; }

        T1  t1;  T2  t2;  T3  t3;  T4  t4;  T5  t5;  T6  t6;  T7  t7;
        T8  t8;  T9  t9;  T10 t10; T11 t11; T12 t12; T13 t13; T14 t14;
        T15 t15; T16 t16; T17 t17; T18 t18; T19 t19; T20 t20; T21 t21;
        T22 t22; T23 t23; T24 t24; T25 t25; T26 t26; T27 t27; T28 t28;
        T29 t29; T30 t30; T31 t31; T32 t32; T33 t33; T34 t34; T35 t35;
        T36 t36; T37 t37; T38 t38; T39 t39; T40 t40; T41 t41; T42 t42;
        T43 t43; T44 t44; T45 t45; T46 t46; T47 t47; T48 t48; T49 t49;
        T50 t50; T51 t51; T52 t52; T53 t53; T54 t54; T55 t55; T56 t56;

    };
};

template <typename ...T>
struct tuple
    : tuple_impl<sizeof...(T)>::template result<T...>
{ };

template <unsigned int n, typename Xs>
constexpr decltype(auto) get(Xs&& xs)
{ return tuple_impl<n + 1>::get(static_cast<Xs&&>(xs)); }
