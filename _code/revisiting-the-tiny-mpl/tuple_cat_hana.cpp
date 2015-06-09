// Copyright Louis Dionne 2015
// Distributed under the Boost Software License, Version 1.0.

#include <boost/hana.hpp>

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
namespace hana = boost::hana;
using namespace boost::hana::literals;


template <typename Tuples, std::size_t ...i, std::size_t ...j,
  typename Res = std::tuple<
    std::tuple_element_t<j, std::tuple_element_t<i,
      std::remove_reference_t<Tuples>>>...
  >
>
constexpr Res tuple_cat_impl(Tuples&& tuples,
                             std::index_sequence<i...>,
                             std::index_sequence<j...>)
{
  return Res{
      std::get<j>(std::get<i>(std::forward<Tuples>(tuples)))...
  };
}


template <typename ...Tuples>
constexpr auto tuple_cat(Tuples&& ...tuples) {
  constexpr std::size_t N = sizeof...(Tuples);
  hana::tuple<Tuples&&...> xs{std::forward<Tuples>(tuples)...};

  constexpr auto indices = hana::to<hana::tuple_tag>(hana::range_c<int, 0, N>);
  auto inner = hana::fill(tuples, indices)...;
  auto outer = hana::make_range(0_c, hana::length(tuples))...;

  return tuple_cat_impl(
      std::move(xs),
      hana::to<hana::ext::std::integer_sequence_tag<std::size_t>>(inner),
      hana::to<hana::ext::std::integer_sequence_tag<std::size_t>>(outer)
  );
}

int main() { }


// constexpr std::size_t N = sizeof...(Tuples);
// hana::_tuple<Tuples&&...> xs{std::forward<Tuples>(tuples)...};
// auto inner = hana::flatten(hana::zip.with(hana::fill, xs, hana::to<hana::Tuple>(hana::range_c<int, 0, N>)));
// auto outer = flatten(tuple(to<Tuple>(range(0_c, length(tuples)))...));
// return tuple_cat_impl<Res>(
//     std::move(xs),
//     to<ext::std::IntegerSequence<std::size_t>>(inner),
//     to<ext::std::IntegerSequence<std::size_t>>(outer)
// );
