#define META

#ifdef META

    #include "meta/meta.hpp"

    #include <tuple>
    #include <type_traits>
    #include <cstddef>
    using namespace meta;


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

#elif defined(HANA)



#elif defined(HANA_CONCAT)


#endif

template <int> struct x { };

int main() {
    tuple_cat(
          tuple()
        , tuple(x<1>{})
        , tuple(x<1>{}, x<2>{})
        , tuple(x<1>{}, x<2>{}, x<3>{})
        , tuple(x<1>{}, x<2>{}, x<3>{}, x<4>{})
        , tuple(x<1>{}, x<2>{}, x<3>{}, x<4>{}, x<5>{})
        , tuple(2, 3, 4, 5, 6, 2, 3, 4, 5, 6, 2, 3, 4, 5, 6, 2, 3, 4, 5, 6, 2, 3, 4, 5, 6)
        , tuple(2, 3, 4, 5, 6, 2, 3, 4, 5, 6, 2, 3, 4, 5, 6, 2, 3, 4, 5, 3, 4, 5, 6)
        , tuple(2, 3, 4, 5, 6, 2, 3, 4, '5', 6, 2, 3, 4, 5, 6)
        , tuple(2, 3, 4, 5, 6, 2, 3, 4, 5, 6, 2, 3, 4, 5, 6, 2, 3, 4, 5)
        , tuple(2, 3, 4, 5, 6, 2, 3, 4, 5, 6, 2, 3, 4, 5, 6.2, 2, 3, 4, 5, 6, 4, 5, 6)
        , tuple(2, 3, 4, 5, 6, 2, 3, 4, 5, 6, 2, 3, 4, 5, 6, 2, 3, 4, 5, 6, 2, 3, 4, 5, 6)
        , tuple(2, 3, 4, 5, 6, 2, 3, 4, 5, 6, 2, 3, 4, 5, 6, 2, 3.3f, 4, 5, 6, 5, 6)
        , tuple(2, 3, 4, 5, 6, 2, 3, 4, 5, 6, 2, nullptr, 4, 5, 6, 2, 3, 4, 5, 6, 2, 3, 5, 6)
        , tuple(2, 3, 4, 5, 6, 2, 3, 4, 5, 6, 2, 3, 4, 5, "6", 2, 3, 4, 5, 6, 2, 3, 4, 5, 6)
    );
}
