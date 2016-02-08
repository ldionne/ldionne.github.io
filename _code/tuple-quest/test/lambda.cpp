// Copyright Louis Dionne 2015
// Distributed under the Boost Software License, Version 1.0.

#if defined(NDEBUG)
#   undef NDEBUG
#endif

#include "../lambda.hpp"
#include "../lambda_operations.hpp"

#include <cassert>
#include <sstream>
#include <string>


int main() {
    auto t = lambda::make_tuple(1, '2', 3.3);

    // get
    assert(lambda::get<0>(t) == 1);
    assert(lambda::get<1>(t) == '2');
    assert(lambda::get<2>(t) == 3.3);

    // unpack
    lambda::unpack(t, [](int i, char c, double d) {
        assert(i == 1);
        assert(c == '2');
        assert(d == 3.3);
    });

    // transform
    {
        auto s = lambda::transform(t, [](auto x) {
            std::ostringstream os;
            os << x;
            return os.str();
        });
        assert(lambda::get<0>(s) == "1");
        assert(lambda::get<1>(s) == "2");
        assert(lambda::get<2>(s) == "3.3");
    }

    // is_empty
    {
        assert(lambda::is_empty(lambda::make_tuple()));
        assert(!lambda::is_empty(t));
    }

    // concat
    {
        auto u = lambda::make_tuple(std::string{"abc"}, nullptr);
        auto r = lambda::concat(t, u);
        assert(lambda::get<0>(r) == 1);
        assert(lambda::get<1>(r) == '2');
        assert(lambda::get<2>(r) == 3.3);
        assert(lambda::get<3>(r) == std::string{"abc"});
        assert(lambda::get<4>(r) == nullptr);
    }

    // prepend
    {
        auto u = lambda::prepend(t, std::string{"abc"});
        assert(lambda::get<0>(u) == std::string{"abc"});
        assert(lambda::get<1>(u) == 1);
        assert(lambda::get<2>(u) == '2');
        assert(lambda::get<3>(u) == 3.3);
    }

    // append
    {
        auto u = lambda::append(t, std::string{"abc"});
        assert(lambda::get<0>(u) == 1);
        assert(lambda::get<1>(u) == '2');
        assert(lambda::get<2>(u) == 3.3);
        assert(lambda::get<3>(u) == std::string{"abc"});
    }
}
