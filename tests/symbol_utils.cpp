/* Copyright 2009-2017 Francesco Biscani (bluescarni@gmail.com)

This file is part of the Piranha library.

The Piranha library is free software; you can redistribute it and/or modify
it under the terms of either:

  * the GNU Lesser General Public License as published by the Free
    Software Foundation; either version 3 of the License, or (at your
    option) any later version.

or

  * the GNU General Public License as published by the Free Software
    Foundation; either version 3 of the License, or (at your option) any
    later version.

or both in parallel, as here.

The Piranha library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received copies of the GNU General Public License and the
GNU Lesser General Public License along with the Piranha library.  If not,
see https://www.gnu.org/licenses/. */

#include <piranha/symbol_utils.hpp>

#define BOOST_TEST_MODULE symbol_utils_test
#include <boost/test/included/unit_test.hpp>

#include <boost/algorithm/string/predicate.hpp>
#include <initializer_list>
#include <stdexcept>
#include <tuple>
#include <vector>

#include <piranha/init.hpp>

using namespace piranha;

BOOST_AUTO_TEST_CASE(symbol_utils_merge_symbol_fsets_test)
{
    init();
    // The empty test.
    auto ret = merge_symbol_fsets(symbol_fset{}, symbol_fset{});
    BOOST_CHECK(std::get<0>(ret).empty());
    BOOST_CHECK(std::get<1>(ret).empty());
    BOOST_CHECK(std::get<2>(ret).empty());
    // Non-empty vs empty.
    ret = merge_symbol_fsets(symbol_fset{"a", "b", "c"}, symbol_fset{});
    BOOST_CHECK((std::get<0>(ret) == symbol_fset{"a", "b", "c"}));
    BOOST_CHECK(std::get<1>(ret).empty());
    BOOST_CHECK((std::get<2>(ret) == symbol_idx_fmap<symbol_fset>{{0, {"a", "b", "c"}}}));
    // Non-empty vs non-empty.
    ret = merge_symbol_fsets(symbol_fset{"a", "b", "c"}, symbol_fset{"a", "b", "c"});
    BOOST_CHECK((std::get<0>(ret) == symbol_fset{"a", "b", "c"}));
    BOOST_CHECK(std::get<1>(ret).empty());
    BOOST_CHECK((std::get<2>(ret).empty()));
    // Empty vs non-empty.
    ret = merge_symbol_fsets(symbol_fset{}, symbol_fset{"a", "b", "c"});
    BOOST_CHECK((std::get<0>(ret) == symbol_fset{"a", "b", "c"}));
    BOOST_CHECK((std::get<1>(ret) == symbol_idx_fmap<symbol_fset>{{0, {"a", "b", "c"}}}));
    BOOST_CHECK(std::get<2>(ret).empty());
    // Subsets left.
    ret = merge_symbol_fsets(symbol_fset{"a", "c"}, symbol_fset{"a", "b", "c"});
    BOOST_CHECK((std::get<0>(ret) == symbol_fset{"a", "b", "c"}));
    BOOST_CHECK((std::get<1>(ret) == symbol_idx_fmap<symbol_fset>{{1, {"b"}}}));
    BOOST_CHECK(std::get<2>(ret).empty());
    ret = merge_symbol_fsets(symbol_fset{"a", "b"}, symbol_fset{"a", "b", "c"});
    BOOST_CHECK((std::get<0>(ret) == symbol_fset{"a", "b", "c"}));
    BOOST_CHECK((std::get<1>(ret) == symbol_idx_fmap<symbol_fset>{{2, {"c"}}}));
    BOOST_CHECK(std::get<2>(ret).empty());
    ret = merge_symbol_fsets(symbol_fset{"b", "c"}, symbol_fset{"a", "b", "c"});
    BOOST_CHECK((std::get<0>(ret) == symbol_fset{"a", "b", "c"}));
    BOOST_CHECK((std::get<1>(ret) == symbol_idx_fmap<symbol_fset>{{0, {"a"}}}));
    BOOST_CHECK(std::get<2>(ret).empty());
    // Subsets right.
    ret = merge_symbol_fsets(symbol_fset{"a", "b", "c"}, symbol_fset{"a", "c"});
    BOOST_CHECK((std::get<0>(ret) == symbol_fset{"a", "b", "c"}));
    BOOST_CHECK(std::get<1>(ret).empty());
    BOOST_CHECK((std::get<2>(ret) == symbol_idx_fmap<symbol_fset>{{1, {"b"}}}));
    ret = merge_symbol_fsets(symbol_fset{"a", "b", "c"}, symbol_fset{"a", "b"});
    BOOST_CHECK((std::get<0>(ret) == symbol_fset{"a", "b", "c"}));
    BOOST_CHECK(std::get<1>(ret).empty());
    BOOST_CHECK((std::get<2>(ret) == symbol_idx_fmap<symbol_fset>{{2, {"c"}}}));
    ret = merge_symbol_fsets(symbol_fset{"a", "b", "c"}, symbol_fset{"b", "c"});
    BOOST_CHECK((std::get<0>(ret) == symbol_fset{"a", "b", "c"}));
    BOOST_CHECK(std::get<1>(ret).empty());
    BOOST_CHECK((std::get<2>(ret) == symbol_idx_fmap<symbol_fset>{{0, {"a"}}}));
    // Disjoint.
    ret = merge_symbol_fsets(symbol_fset{"a", "b", "c"}, symbol_fset{"d", "e", "f"});
    BOOST_CHECK((std::get<0>(ret) == symbol_fset{"a", "b", "c", "d", "e", "f"}));
    BOOST_CHECK((std::get<1>(ret) == symbol_idx_fmap<symbol_fset>{{3, {"d", "e", "f"}}}));
    BOOST_CHECK((std::get<2>(ret) == symbol_idx_fmap<symbol_fset>{{0, {"a", "b", "c"}}}));
    ret = merge_symbol_fsets(symbol_fset{"d", "e", "f"}, symbol_fset{"a", "b", "c"});
    BOOST_CHECK((std::get<0>(ret) == symbol_fset{"a", "b", "c", "d", "e", "f"}));
    BOOST_CHECK((std::get<1>(ret) == symbol_idx_fmap<symbol_fset>{{0, {"a", "b", "c"}}}));
    BOOST_CHECK((std::get<2>(ret) == symbol_idx_fmap<symbol_fset>{{3, {"d", "e", "f"}}}));
    // Misc.
    ret = merge_symbol_fsets(symbol_fset{"b", "c", "e"}, symbol_fset{"a", "c", "d", "f", "g"});
    BOOST_CHECK((std::get<0>(ret) == symbol_fset{"a", "b", "c", "d", "e", "f", "g"}));
    BOOST_CHECK((std::get<1>(ret) == symbol_idx_fmap<symbol_fset>{{0, {"a"}}, {2, {"d"}}, {3, {"f", "g"}}}));
    BOOST_CHECK((std::get<2>(ret) == symbol_idx_fmap<symbol_fset>{{1, {"b"}}, {3, {"e"}}}));
    ret = merge_symbol_fsets(symbol_fset{"b", "n", "t", "z"}, symbol_fset{"a", "c", "d", "f", "g", "m", "o", "x"});
    BOOST_CHECK((std::get<0>(ret) == symbol_fset{"a", "b", "c", "d", "f", "g", "m", "n", "o", "t", "x", "z"}));
    BOOST_CHECK((std::get<1>(ret)
                 == symbol_idx_fmap<symbol_fset>{{0, {"a"}}, {1, {"c", "d", "f", "g", "m"}}, {2, {"o"}}, {3, {"x"}}}));
    BOOST_CHECK((std::get<2>(ret) == symbol_idx_fmap<symbol_fset>{{1, {"b"}}, {6, {"n"}}, {7, {"t"}}, {8, {"z"}}}));
    ret = merge_symbol_fsets(symbol_fset{"b", "n", "t"}, symbol_fset{"a", "c", "d", "f", "g", "m", "o", "x"});
    BOOST_CHECK((std::get<0>(ret) == symbol_fset{"a", "b", "c", "d", "f", "g", "m", "n", "o", "t", "x"}));
    BOOST_CHECK((std::get<1>(ret)
                 == symbol_idx_fmap<symbol_fset>{{0, {"a"}}, {1, {"c", "d", "f", "g", "m"}}, {2, {"o"}}, {3, {"x"}}}));
    BOOST_CHECK((std::get<2>(ret) == symbol_idx_fmap<symbol_fset>{{1, {"b"}}, {6, {"n"}}, {7, {"t"}}}));
}

BOOST_AUTO_TEST_CASE(symbol_utils_index_of_test)
{
    BOOST_CHECK_EQUAL(index_of(symbol_fset{}, "x"), 0u);
    BOOST_CHECK_EQUAL(index_of(symbol_fset{"x", "y"}, "x"), 0u);
    BOOST_CHECK_EQUAL(index_of(symbol_fset{"x", "y", "z"}, "y"), 1u);
    BOOST_CHECK_EQUAL(index_of(symbol_fset{"x", "y", "z"}, "z"), 2u);
    BOOST_CHECK_EQUAL(index_of(symbol_fset{"x", "y", "z"}, "a"), 3u);
}

BOOST_AUTO_TEST_CASE(symbol_utils_trim_symbol_set_test)
{
    BOOST_CHECK(trim_symbol_set(symbol_fset{}, std::vector<char>{}) == symbol_fset{});
    BOOST_CHECK((trim_symbol_set(symbol_fset{"x", "y", "z"}, {0, 0, 0}) == symbol_fset{"x", "y", "z"}));
    BOOST_CHECK((trim_symbol_set(symbol_fset{"x", "y", "z"}, {0, 1, 0}) == symbol_fset{"x", "z"}));
    BOOST_CHECK((trim_symbol_set(symbol_fset{"x", "y", "z"}, {1, 0, 0}) == symbol_fset{"y", "z"}));
    BOOST_CHECK((trim_symbol_set(symbol_fset{"x", "y", "z"}, {0, 0, 1}) == symbol_fset{"x", "y"}));
    BOOST_CHECK((trim_symbol_set(symbol_fset{"x", "y", "z"}, {1, 0, 1}) == symbol_fset{"y"}));
    BOOST_CHECK((trim_symbol_set(symbol_fset{"x", "y", "z"}, {1, 1, 0}) == symbol_fset{"z"}));
    BOOST_CHECK((trim_symbol_set(symbol_fset{"x", "y", "z"}, {0, 1, 1}) == symbol_fset{"x"}));
    BOOST_CHECK((trim_symbol_set(symbol_fset{"x", "y", "z"}, {1, 1, 1}) == symbol_fset{}));
    BOOST_CHECK_EXCEPTION(trim_symbol_set(symbol_fset{"x", "y", "z"}, {0, 0, 0, 0}), std::invalid_argument,
                          [](const std::invalid_argument &e) {
                              return boost::contains(e.what(), "invalid argument(s) for symbol set trimming: the size "
                                                               "of the original symbol set (3) differs from the size "
                                                               "of trimming mask (4)");
                          });
}
