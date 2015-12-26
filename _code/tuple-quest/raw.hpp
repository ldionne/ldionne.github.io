// Copyright Louis Dionne 2015
// Distributed under the Boost Software License, Version 1.0.

#ifndef RAW_HPP
#define RAW_HPP

#include <array>
#include <cstddef>
#include <new>
#include <type_traits>

#include "nth_element.hpp"


namespace raw {
    template <typename ...T>
    struct tuple {
        static constexpr std::array<std::size_t, sizeof...(T)> sizes = {{sizeof(T)...}};
        static constexpr std::array<std::size_t, sizeof...(T)> alignments = {{alignof(T)...}};

        static constexpr std::array<std::size_t, sizeof...(T)> offsets_impl() {
            std::array<std::size_t, sizeof...(T)> result{};
            std::array<std::size_t, sizeof...(T)> const& result_ref = result;
            std::size_t offset = 0;
            for (std::size_t i = 0; i < sizeof...(T); ++i) {
                offset += sizes[i];
                if (i != sizeof...(T) - 1)
                    offset += offset % alignments[i + 1];
                const_cast<std::size_t&>(result_ref[i]) = offset;
            }
            return result;
        }

        static constexpr std::array<std::size_t, sizeof...(T)> offsets = offsets_impl();
        static constexpr std::size_t total_size = offsets[sizeof...(T)-1];
        typename std::aligned_storage<total_size>::type storage_;

        constexpr void const* raw_nth(std::size_t n) const {
            return ((char const*)&storage_) + offsets[n];
        }

        constexpr void* raw_nth(std::size_t n) {
            return ((char*)&storage_) + offsets[n];
        }

        /* constexpr */ tuple() {
            std::size_t i = 0;
            void* expand[] = {::new (this->raw_nth(i++)) T()...};
            (void)expand;
        }

        explicit /* constexpr */ tuple(T const& ...args) {
            std::size_t i = 0;
            void* expand[] = {::new (this->raw_nth(i++)) T(args)...};
            (void)expand;
        }

        ~tuple() {
            std::size_t i = 0;
            int expand[] = {(static_cast<T*>(this->raw_nth(i++))->~T(), int{})...};
            (void)expand;
        }
    };

    template <typename ...T>
    constexpr std::array<std::size_t, sizeof...(T)> tuple<T...>::offsets;

    template <>
    struct tuple<> {
        constexpr tuple() { }
    };

    template <std::size_t n, typename ...T>
    decltype(auto) get(tuple<T...> const& ts) {
        using Nth = nth_element<n, T...>;
        return *static_cast<Nth const*>(ts.raw_nth(n));
    }
} // end namespace raw

#endif
