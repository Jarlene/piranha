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

#include <piranha/kronecker_monomial.hpp>

#define BOOST_TEST_MODULE kronecker_monomial_01_test
#include <boost/test/included/unit_test.hpp>

#include <array>
#include <boost/algorithm/string/predicate.hpp>
#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <list>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <piranha/exceptions.hpp>
#include <piranha/init.hpp>
#include <piranha/is_key.hpp>
#include <piranha/key_is_convertible.hpp>
#include <piranha/key_is_multipliable.hpp>
#include <piranha/kronecker_array.hpp>
#include <piranha/math.hpp>
#include <piranha/mp_integer.hpp>
#include <piranha/mp_rational.hpp>
#include <piranha/pow.hpp>
#include <piranha/real.hpp>
#include <piranha/s11n.hpp>
#include <piranha/symbol_utils.hpp>
#include <piranha/term.hpp>
#include <piranha/type_traits.hpp>

using namespace piranha;

using int_types = std::tuple<signed char, int, long, long long>;

// Constructors, assignments, getters, setters, etc.
struct constructor_tester {
    template <typename T>
    void operator()(const T &) const
    {
        typedef kronecker_monomial<T> k_type;
        typedef kronecker_array<T> ka;
        k_type k1;
        BOOST_CHECK_EQUAL(k1.get_int(), 0);
        k_type k2({-1, -1});
        std::vector<T> v2(2);
        ka::decode(v2, k2.get_int());
        BOOST_CHECK_EQUAL(v2[0], -1);
        BOOST_CHECK_EQUAL(v2[1], -1);
        k_type k3({});
        BOOST_CHECK_EQUAL(k3.get_int(), 0);
        k_type k4({10});
        BOOST_CHECK_EQUAL(k4.get_int(), 10);
        // Ctor from container.
        k1 = k_type(std::vector<int>{});
        BOOST_CHECK_EQUAL(k1.get_int(), 0);
        k1 = k_type(std::vector<int>{12});
        BOOST_CHECK_EQUAL(k1.get_int(), 12);
        k1 = k_type(std::vector<int>{-1, 2});
        ka::decode(v2, k1.get_int());
        BOOST_CHECK_EQUAL(v2[0], -1);
        BOOST_CHECK_EQUAL(v2[1], 2);
        k1 = k_type(std::list<int>{});
        BOOST_CHECK_EQUAL(k1.get_int(), 0);
        k1 = k_type(std::list<int>{12});
        BOOST_CHECK_EQUAL(k1.get_int(), 12);
        k1 = k_type(std::list<int>{-1, 2});
        ka::decode(v2, k1.get_int());
        BOOST_CHECK_EQUAL(v2[0], -1);
        BOOST_CHECK_EQUAL(v2[1], 2);
        // Ctor from symbol set.
        k_type k5(symbol_fset{});
        BOOST_CHECK_EQUAL(k5.get_int(), 0);
        k_type k6(symbol_fset{"a"});
        BOOST_CHECK_EQUAL(k6.get_int(), 0);
        k_type k7(symbol_fset{"a", "b"});
        BOOST_CHECK_EQUAL(k7.get_int(), 0);
        k_type k8(0);
        BOOST_CHECK_EQUAL(k8.get_int(), 0);
        k_type k9(1);
        BOOST_CHECK_EQUAL(k9.get_int(), 1);
        k_type k10;
        k10.set_int(10);
        BOOST_CHECK_EQUAL(k10.get_int(), 10);
        k_type k11;
        k11 = k10;
        BOOST_CHECK_EQUAL(k11.get_int(), 10);
        k11 = std::move(k9);
        BOOST_CHECK_EQUAL(k9.get_int(), 1);
        // Constructor from iterators.
        v2 = {};
        k_type k12(v2.begin(), v2.end());
        BOOST_CHECK_EQUAL(k12.get_int(), 0);
        v2 = {21};
        k_type k13(v2.begin(), v2.end());
        BOOST_CHECK_EQUAL(k13.get_int(), 21);
        v2 = {-21};
        k_type k14(v2.begin(), v2.end());
        BOOST_CHECK_EQUAL(k14.get_int(), -21);
        v2 = {1, -2};
        k_type k15(v2.begin(), v2.end());
        auto v = k15.unpack(symbol_fset{"a", "b"});
        BOOST_CHECK(v.size() == 2u);
        BOOST_CHECK(v[0u] == 1);
        BOOST_CHECK(v[1u] == -2);
        BOOST_CHECK((std::is_constructible<k_type, T *, T *>::value));
        // Ctor from range and symbol set.
        v2 = {};
        k1 = k_type(v2.begin(), v2.end(), symbol_fset{});
        BOOST_CHECK_EQUAL(k1.get_int(), 0);
        v2 = {-3};
        k1 = k_type(v2.begin(), v2.end(), symbol_fset{"x"});
        BOOST_CHECK_EQUAL(k1.get_int(), -3);
        BOOST_CHECK_EXCEPTION(
            k1 = k_type(v2.begin(), v2.end(), symbol_fset{}), std::invalid_argument,
            [](const std::invalid_argument &e) {
                return boost::contains(
                    e.what(),
                    "the Kronecker monomial constructor from range and symbol set "
                    "yielded an invalid monomial: the range length (1) differs from the size of the symbol set (0)");
            });
        v2 = {-1, 0};
        k1 = k_type(v2.begin(), v2.end(), symbol_fset{"x", "y"});
        ka::decode(v2, k1.get_int());
        BOOST_CHECK_EQUAL(v2[0], -1);
        BOOST_CHECK_EQUAL(v2[1], 0);
        std::list<int> l2;
        k1 = k_type(l2.begin(), l2.end(), symbol_fset{});
        BOOST_CHECK_EQUAL(k1.get_int(), 0);
        l2 = {-3};
        k1 = k_type(l2.begin(), l2.end(), symbol_fset{"x"});
        BOOST_CHECK_EQUAL(k1.get_int(), -3);
        BOOST_CHECK_EXCEPTION(
            k1 = k_type(l2.begin(), l2.end(), symbol_fset{}), std::invalid_argument,
            [](const std::invalid_argument &e) {
                return boost::contains(
                    e.what(),
                    "the Kronecker monomial constructor from range and symbol set "
                    "yielded an invalid monomial: the range length (1) differs from the size of the symbol set (0)");
            });
        l2 = {-1, 0};
        k1 = k_type(l2.begin(), l2.end(), symbol_fset{"x", "y"});
        ka::decode(v2, k1.get_int());
        BOOST_CHECK_EQUAL(v2[0], -1);
        BOOST_CHECK_EQUAL(v2[1], 0);
        struct foo {
        };
        BOOST_CHECK((std::is_constructible<k_type, int *, int *, symbol_fset>::value));
        BOOST_CHECK((!std::is_constructible<k_type, foo *, foo *, symbol_fset>::value));
        // Iterators have to be of homogeneous type.
        BOOST_CHECK((!std::is_constructible<k_type, T *, T const *>::value));
        BOOST_CHECK((std::is_constructible<k_type, typename std::vector<T>::iterator,
                                           typename std::vector<T>::iterator>::value));
        BOOST_CHECK((!std::is_constructible<k_type, typename std::vector<T>::const_iterator,
                                            typename std::vector<T>::iterator>::value));
        BOOST_CHECK((!std::is_constructible<k_type, typename std::vector<T>::iterator, int>::value));
        // Converting constructor.
        k_type k16, k17(k16, symbol_fset{});
        BOOST_CHECK(k16 == k17);
        k16.set_int(10);
        k_type k18(k16, symbol_fset{"a"});
        BOOST_CHECK(k16 == k18);
    }
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_constructor_test)
{
    init();
    tuple_for_each(int_types{}, constructor_tester{});
}

struct compatibility_tester {
    template <typename T>
    void operator()(const T &) const
    {
        typedef kronecker_monomial<T> k_type;
        typedef kronecker_array<T> ka;
        const auto &limits = ka::get_limits();
        k_type k1;
        BOOST_CHECK(k1.is_compatible(symbol_fset{}));
        k1.set_int(1);
        BOOST_CHECK(!k1.is_compatible(symbol_fset{}));
        if (limits.size() < 255u) {
            symbol_fset v2;
            for (auto i = 0u; i < 255; ++i) {
                v2.insert(std::string(1u, (char)i));
            }
            BOOST_CHECK(!k1.is_compatible(v2));
        }
        k1.set_int(std::numeric_limits<T>::max());
        BOOST_CHECK(!k1.is_compatible(symbol_fset{"a", "b"}));
        k1.set_int(-1);
        BOOST_CHECK(k1.is_compatible(symbol_fset{"a", "b"}));
    }
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_compatibility_test)
{
    tuple_for_each(int_types{}, compatibility_tester{});
}

struct merge_args_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using k_type = kronecker_monomial<T>;
        k_type k1;
        BOOST_CHECK_EXCEPTION(
            k1.merge_symbols({}, symbol_fset{}), std::invalid_argument, [](const std::invalid_argument &e) {
                return boost::contains(e.what(),
                                       "invalid argument(s) for symbol set merging: the insertion map cannot be empty");
            });
        BOOST_CHECK_EXCEPTION(
            k1.merge_symbols({}, symbol_fset{"d"}), std::invalid_argument, [](const std::invalid_argument &e) {
                return boost::contains(e.what(),
                                       "invalid argument(s) for symbol set merging: the insertion map cannot be empty");
            });
        BOOST_CHECK((k1.merge_symbols({{0, {"a", "b"}}}, symbol_fset{"d"}) == k_type{0, 0, 0}));
        BOOST_CHECK((k_type{1}.merge_symbols({{0, {"a", "b"}}}, symbol_fset{"d"}) == k_type{0, 0, 1}));
        BOOST_CHECK((k_type{1}.merge_symbols({{1, {"e", "f"}}}, symbol_fset{"d"}) == k_type{1, 0, 0}));
        BOOST_CHECK((k_type{1, 1}.merge_symbols({{0, {"a", "b"}}}, symbol_fset{"d", "n"}) == k_type{0, 0, 1, 1}));
        BOOST_CHECK((k_type{1, 1}.merge_symbols({{1, {"e", "f"}}}, symbol_fset{"d", "n"}) == k_type{1, 0, 0, 1}));
        BOOST_CHECK((k_type{1, 1}.merge_symbols({{2, {"f", "g"}}}, symbol_fset{"d", "e"}) == k_type{1, 1, 0, 0}));
        BOOST_CHECK(
            (k_type{-1, -1}.merge_symbols({{0, {"a"}}, {2, {"f"}}}, symbol_fset{"d", "e"}) == k_type{0, -1, -1, 0}));
        BOOST_CHECK((k_type{-1, -1}.merge_symbols({{0, {"a"}}, {1, std::initializer_list<std::string>{}}, {2, {"f"}}},
                                                  symbol_fset{"d", "e"})
                     == k_type{0, -1, -1, 0}));
        BOOST_CHECK_EXCEPTION((k_type{1, 1}.merge_symbols({{3, {"f", "g"}}}, symbol_fset{"d", "e"})),
                              std::invalid_argument, [](const std::invalid_argument &e) {
                                  return boost::contains(e.what(), "invalid argument(s) for symbol set merging: the "
                                                                   "last index of the insertion map (3) must not be "
                                                                   "greater than the key's size (2)");
                              });
        if (std::numeric_limits<T>::max() >= std::numeric_limits<int>::max()) {
            BOOST_CHECK((k_type{-1, -1}.merge_symbols({{0, {"a"}}, {2, {"f"}}, {1, {"b"}}}, symbol_fset{"d", "e"})
                         == k_type{0, -1, 0, -1, 0}));
            BOOST_CHECK(
                (k_type{-1, -1, 3}.merge_symbols({{0, {"a"}}, {3, {"f"}}, {1, {"b"}}}, symbol_fset{"d", "e1", "e2"})
                 == k_type{0, -1, 0, -1, 3, 0}));
        }
    }
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_merge_args_test)
{
    tuple_for_each(int_types{}, merge_args_tester{});
}

struct is_unitary_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using k_type = kronecker_monomial<T>;
        k_type k1;
        BOOST_CHECK(k1.is_unitary(symbol_fset{}));
        k_type k2({-1});
        BOOST_CHECK(!k2.is_unitary(symbol_fset{"a"}));
        k_type k3({0});
        BOOST_CHECK(k3.is_unitary(symbol_fset{"a"}));
        k_type k4({0, 0});
        BOOST_CHECK(k4.is_unitary(symbol_fset{"a", "b"}));
        k_type k5({0, 1});
        BOOST_CHECK(!k5.is_unitary(symbol_fset{"a", "b"}));
    }
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_is_unitary_test)
{
    tuple_for_each(int_types{}, is_unitary_tester{});
}

struct degree_tester {
    template <typename T>
    void operator()(const T &) const
    {
        using k_type = kronecker_monomial<T>;
        k_type k1;
        if (std::is_same<T, signed char>::value) {
            BOOST_CHECK((std::is_same<decltype(k1.degree(symbol_fset{})), int>::value));
        } else {
            BOOST_CHECK((std::is_same<decltype(k1.degree(symbol_fset{})), T>::value));
        }
        BOOST_CHECK(k1.degree(symbol_fset{}) == 0);
        BOOST_CHECK(k1.ldegree(symbol_fset{}) == 0);
        k_type k2({0});
        BOOST_CHECK(k2.degree(symbol_fset{"a"}) == 0);
        BOOST_CHECK(k2.ldegree(symbol_fset{"a"}) == 0);
        k_type k3({-1});
        BOOST_CHECK(k3.degree(symbol_fset{"a"}) == -1);
        BOOST_CHECK(k3.ldegree(symbol_fset{"a"}) == -1);
        k_type k4({0, 0});
        BOOST_CHECK(k4.degree(symbol_fset{"a", "b"}) == 0);
        BOOST_CHECK(k4.ldegree(symbol_fset{"a", "b"}) == 0);
        k_type k5({-1, -1});
        BOOST_CHECK(k5.degree(symbol_fset{"a", "b"}) == -2);
        if (std::is_same<T, signed char>::value) {
            BOOST_CHECK((std::is_same<decltype(k5.degree(symbol_idx_fset{}, symbol_fset{})), int>::value));
        } else {
            BOOST_CHECK((std::is_same<decltype(k5.degree(symbol_idx_fset{}, symbol_fset{})), T>::value));
        }
        BOOST_CHECK(k5.degree(symbol_idx_fset{0}, symbol_fset{"a", "b"}) == -1);
        BOOST_CHECK(k5.degree(symbol_idx_fset{}, symbol_fset{"a", "b"}) == 0);
        BOOST_CHECK(k5.degree(symbol_idx_fset{0, 1}, symbol_fset{"a", "b"}) == -2);
        BOOST_CHECK(k5.degree(symbol_idx_fset{1}, symbol_fset{"a", "b"}) == -1);
        BOOST_CHECK(k5.ldegree(symbol_fset{"a", "b"}) == -2);
        BOOST_CHECK(k5.ldegree(symbol_idx_fset{0}, symbol_fset{"a", "b"}) == -1);
        BOOST_CHECK(k5.ldegree(symbol_idx_fset{}, symbol_fset{"a", "b"}) == 0);
        BOOST_CHECK(k5.ldegree(symbol_idx_fset{0, 1}, symbol_fset{"a", "b"}) == -2);
        BOOST_CHECK(k5.ldegree(symbol_idx_fset{1}, symbol_fset{"a", "b"}) == -1);
        // Try partials with bogus positions.
        BOOST_CHECK_EXCEPTION(
            (k5.degree(symbol_idx_fset{2}, symbol_fset{"a", "b"})), std::invalid_argument,
            [](const std::invalid_argument &e) {
                return boost::contains(
                    e.what(), "the largest value in the positions set for the computation of the "
                              "partial degree of a Kronecker monomial is 2, but the monomial has a size of only 2");
            });
        BOOST_CHECK_EXCEPTION(
            (k5.ldegree(symbol_idx_fset{4}, symbol_fset{"a", "b"})), std::invalid_argument,
            [](const std::invalid_argument &e) {
                return boost::contains(
                    e.what(), "the largest value in the positions set for the computation of the "
                              "partial degree of a Kronecker monomial is 4, but the monomial has a size of only 2");
            });
    }
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_degree_test)
{
    tuple_for_each(int_types{}, degree_tester{});
}

// Mock cf with wrong specialisation of mul3.
struct mock_cf3 {
    mock_cf3();
    explicit mock_cf3(const int &);
    mock_cf3(const mock_cf3 &);
    mock_cf3(mock_cf3 &&) noexcept;
    mock_cf3 &operator=(const mock_cf3 &);
    mock_cf3 &operator=(mock_cf3 &&) noexcept;
    friend std::ostream &operator<<(std::ostream &, const mock_cf3 &);
    mock_cf3 operator-() const;
    bool operator==(const mock_cf3 &) const;
    bool operator!=(const mock_cf3 &) const;
    mock_cf3 &operator+=(const mock_cf3 &);
    mock_cf3 &operator-=(const mock_cf3 &);
    mock_cf3 operator+(const mock_cf3 &) const;
    mock_cf3 operator-(const mock_cf3 &) const;
    mock_cf3 &operator*=(const mock_cf3 &);
    mock_cf3 operator*(const mock_cf3 &)const;
};

namespace piranha
{
namespace math
{

template <typename T>
struct mul3_impl<T, typename std::enable_if<std::is_same<T, mock_cf3>::value>::type> {
};
}
}

struct multiply_tester {
    template <typename T>
    void operator()(const T &) const
    {
        typedef kronecker_monomial<T> k_type;
        typedef kronecker_array<T> ka;
        using term_type = term<integer, k_type>;
        BOOST_CHECK((key_is_multipliable<int, k_type>::value));
        BOOST_CHECK((key_is_multipliable<integer, k_type>::value));
        BOOST_CHECK((!key_is_multipliable<mock_cf3, k_type>::value));
        BOOST_CHECK(is_key<k_type>::value);
        term_type t1, t2;
        std::array<term_type, 1u> result;
        k_type::multiply(result, t1, t2, symbol_fset{});
        BOOST_CHECK(result[0u].m_cf == 0);
        BOOST_CHECK(result[0u].m_key.get_int() == 0);
        t1.m_cf = 2;
        t2.m_cf = 3;
        t1.m_key = k_type({0});
        t2.m_key = k_type({0});
        k_type::multiply(result, t1, t2, symbol_fset{"a"});
        BOOST_CHECK(result[0u].m_cf == 6);
        BOOST_CHECK(result[0u].m_key.get_int() == 0);
        t1.m_key = k_type({1});
        t2.m_key = k_type({2});
        k_type::multiply(result, t1, t2, symbol_fset{"a"});
        BOOST_CHECK(result[0u].m_cf == 6);
        BOOST_CHECK(result[0u].m_key.get_int() == 3);
        t1.m_cf = 2;
        t2.m_cf = -4;
        t1.m_key = k_type({1, -1});
        t2.m_key = k_type({2, 0});
        k_type::multiply(result, t1, t2, symbol_fset{"a", "b"});
        BOOST_CHECK(result[0u].m_cf == -8);
        std::vector<int> tmp(2u);
        ka::decode(tmp, result[0u].m_key.get_int());
        BOOST_CHECK(tmp[0u] == 3);
        BOOST_CHECK(tmp[1u] == -1);
        // Check special handling of rational coefficients.
        using term_type2 = term<rational, k_type>;
        term_type2 ta, tb;
        std::array<term_type2, 1u> result2;
        ta.m_cf = 2 / 3_q;
        tb.m_cf = -4 / 5_q;
        ta.m_key = k_type({1, -1});
        tb.m_key = k_type({2, 0});
        k_type::multiply(result2, ta, tb, symbol_fset{"a", "b"});
        BOOST_CHECK(result2[0u].m_cf == -8);
        ka::decode(tmp, result2[0u].m_key.get_int());
        BOOST_CHECK(tmp[0u] == 3);
        BOOST_CHECK(tmp[1u] == -1);
    }
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_multiply_test)
{
    tuple_for_each(int_types{}, multiply_tester{});
}

struct monomial_multiply_tester {
    template <typename T>
    void operator()(const T &) const
    {
        typedef kronecker_monomial<T> k_type;
        k_type k1, k2, res;
        k_type::multiply(res, k1, k2, symbol_fset{});
        BOOST_CHECK_EQUAL(res.get_int(), 0);
        k1 = k_type{-5};
        k2 = k_type{7};
        k_type::multiply(res, k1, k2, symbol_fset{});
        BOOST_CHECK_EQUAL(res.get_int(), 2);
        k_type::multiply(res, k1, k2, symbol_fset{"x", "y"});
        BOOST_CHECK_EQUAL(res.get_int(), 2);
    }
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_monomial_multiply_test)
{
    tuple_for_each(int_types{}, monomial_multiply_tester{});
}

struct equality_tester {
    template <typename T>
    void operator()(const T &) const
    {
        typedef kronecker_monomial<T> k_type;
        k_type k1, k2;
        BOOST_CHECK(k1 == k2);
        BOOST_CHECK(!(k1 != k2));
        k1 = k_type({0});
        k2 = k_type({0});
        BOOST_CHECK(k1 == k2);
        BOOST_CHECK(!(k1 != k2));
        k2 = k_type({1});
        BOOST_CHECK(!(k1 == k2));
        BOOST_CHECK(k1 != k2);
        k1 = k_type({0, 0});
        k2 = k_type({0, 0});
        BOOST_CHECK(k1 == k2);
        BOOST_CHECK(!(k1 != k2));
        k1 = k_type({1, 0});
        k2 = k_type({1, 0});
        BOOST_CHECK(k1 == k2);
        BOOST_CHECK(!(k1 != k2));
        k1 = k_type({1, 0});
        k2 = k_type({0, 1});
        BOOST_CHECK(!(k1 == k2));
        BOOST_CHECK(k1 != k2);
    }
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_equality_test)
{
    tuple_for_each(int_types{}, equality_tester{});
}

struct hash_tester {
    template <typename T>
    void operator()(const T &) const
    {
        typedef kronecker_monomial<T> k_type;
        k_type k1;
        BOOST_CHECK(k1.hash() == static_cast<std::size_t>(k1.get_int()));
        k1 = k_type({0});
        BOOST_CHECK(k1.hash() == static_cast<std::size_t>(k1.get_int()));
        k1 = k_type({0, 1});
        BOOST_CHECK(k1.hash() == static_cast<std::size_t>(k1.get_int()));
        k1 = k_type({0, 1, -1});
        BOOST_CHECK(k1.hash() == static_cast<std::size_t>(k1.get_int()));
        BOOST_CHECK(std::hash<k_type>()(k1) == static_cast<std::size_t>(k1.get_int()));
    }
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_hash_test)
{
    tuple_for_each(int_types{}, hash_tester{});
}

struct unpack_tester {
    template <typename T>
    void operator()(const T &) const
    {
        typedef kronecker_monomial<T> k_type;
        k_type k1({0});
        auto t1 = k1.unpack(symbol_fset{});
        typedef decltype(t1) s_vector_type;
        BOOST_CHECK(!t1.size());
        k1.set_int(-1);
        auto t2 = k1.unpack(symbol_fset{"a"});
        BOOST_CHECK(t2.size());
        BOOST_CHECK(t2[0u] == -1);
        // Check for overflow condition.
        symbol_fset vs1{"a"};
        std::string tmp = "";
        for (integer i(0u); i < integer(s_vector_type::max_size) + 1; ++i) {
            tmp += "b";
            vs1.insert(vs1.end(), tmp);
        }
        BOOST_CHECK_THROW(k1.unpack(vs1), std::invalid_argument);
    }
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_unpack_test)
{
    tuple_for_each(int_types{}, unpack_tester{});
}

struct print_tester {
    template <typename T>
    void operator()(const T &) const
    {
        typedef kronecker_monomial<T> k_type;
        k_type k1;
        std::ostringstream oss;
        k1.print(oss, symbol_fset{});
        BOOST_CHECK(oss.str().empty());
        k_type k2(symbol_fset{"x"});
        k2.print(oss, symbol_fset{"x"});
        BOOST_CHECK(oss.str().empty());
        k_type k3({T(-1)});
        k3.print(oss, symbol_fset{"x"});
        BOOST_CHECK(oss.str() == "x**-1");
        k_type k4({T(1)});
        oss.str("");
        k4.print(oss, symbol_fset{"x"});
        BOOST_CHECK(oss.str() == "x");
        k_type k5({T(-1), T(1)});
        oss.str("");
        k5.print(oss, symbol_fset{"x", "y"});
        BOOST_CHECK(oss.str() == "x**-1*y");
        k_type k6({T(-1), T(-2)});
        oss.str("");
        k6.print(oss, symbol_fset{"x", "y"});
        BOOST_CHECK(oss.str() == "x**-1*y**-2");
    }
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_print_test)
{
    tuple_for_each(int_types{}, print_tester{});
}
#if 0
struct linear_argument_tester {
    template <typename T>
    void operator()(const T &)
    {
        typedef kronecker_monomial<T> k_type;
        symbol_set vs;
        BOOST_CHECK_THROW(k_type().linear_argument(vs), std::invalid_argument);
        vs.add("x");
        BOOST_CHECK_THROW(k_type().linear_argument(vs), std::invalid_argument);
        k_type k({T(1)});
        BOOST_CHECK_EQUAL(k.linear_argument(vs), "x");
        k = k_type({T(0), T(1)});
        vs.add("y");
        BOOST_CHECK_EQUAL(k.linear_argument(vs), "y");
        k = k_type({T(0), T(2)});
        BOOST_CHECK_THROW(k.linear_argument(vs), std::invalid_argument);
        k = k_type({T(2), T(0)});
        BOOST_CHECK_THROW(k.linear_argument(vs), std::invalid_argument);
        k = k_type({T(1), T(1)});
        BOOST_CHECK_THROW(k.linear_argument(vs), std::invalid_argument);
    }
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_linear_argument_test)
{
    boost::mpl::for_each<int_types>(linear_argument_tester());
}

struct pow_tester {
    template <typename T>
    void operator()(const T &)
    {
        typedef kronecker_monomial<T> k_type;
        typedef kronecker_array<T> ka;
        const auto &limits = ka::get_limits();
        k_type k1;
        k1.set_int(1);
        symbol_set vs;
        BOOST_CHECK_THROW(k1.pow(42, vs), std::invalid_argument);
        vs.add("x");
        BOOST_CHECK_THROW(k1.pow(42.5, vs), std::invalid_argument);
        k1 = k_type{2};
        k_type k2({4});
        BOOST_CHECK(k1.pow(2, vs) == k2);
        BOOST_CHECK_THROW(k1.pow(std::numeric_limits<T>::max(), vs), std::invalid_argument);
        k1 = k_type{1};
        if (std::get<0u>(limits[1u])[0u] < std::numeric_limits<T>::max()) {
            BOOST_CHECK_THROW(k1.pow(std::get<0u>(limits[1u])[0u] + T(1), vs), std::invalid_argument);
        }
        k1 = k_type{2};
        BOOST_CHECK(k1.pow(0, vs) == k_type{});
    }
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_pow_test)
{
    boost::mpl::for_each<int_types>(pow_tester());
}

struct partial_tester {
    template <typename T>
    void operator()(const T &)
    {
        typedef kronecker_monomial<T> k_type;
        BOOST_CHECK(key_is_differentiable<k_type>::value);
        using positions = symbol_set::positions;
        auto s_to_pos = [](const symbol_set &v, const symbol &s) {
            symbol_set tmp{s};
            return positions(v, tmp);
        };
        symbol_set vs;
        k_type k1;
        k1.set_int(T(1));
        // An empty symbol set must always be related to a zero encoded value.
        BOOST_CHECK_THROW(k1.partial(s_to_pos(vs, symbol("x")), vs), std::invalid_argument);
        vs.add("x");
        k1 = k_type({T(2)});
        auto ret = k1.partial(s_to_pos(vs, symbol("x")), vs);
        BOOST_CHECK_EQUAL(ret.first, 2);
        BOOST_CHECK(ret.second == k_type({T(1)}));
        // y is not in the monomial.
        ret = k1.partial(s_to_pos(vs, symbol("y")), vs);
        BOOST_CHECK_EQUAL(ret.first, 0);
        BOOST_CHECK(ret.second == k_type(vs));
        // x is in the monomial but it is zero.
        k1 = k_type({T(0)});
        ret = k1.partial(s_to_pos(vs, symbol("x")), vs);
        BOOST_CHECK_EQUAL(ret.first, 0);
        BOOST_CHECK(ret.second == k_type(vs));
        // y in the monomial but zero.
        vs.add("y");
        k1 = k_type({T(-1), T(0)});
        ret = k1.partial(s_to_pos(vs, symbol("y")), vs);
        BOOST_CHECK_EQUAL(ret.first, 0);
        BOOST_CHECK(ret.second == k_type(vs));
        ret = k1.partial(s_to_pos(vs, symbol("x")), vs);
        BOOST_CHECK_EQUAL(ret.first, -1);
        BOOST_CHECK(ret.second == k_type({T(-2), T(0)}));
        // Check limits violation.
        typedef kronecker_array<T> ka;
        const auto &limits = ka::get_limits();
        k1 = k_type{-std::get<0u>(limits[2u])[0u], -std::get<0u>(limits[2u])[0u]};
        BOOST_CHECK_THROW(ret = k1.partial(s_to_pos(vs, symbol("x")), vs), std::invalid_argument);
        // Check with bogus positions.
        symbol_set vs2;
        vs2.add("x");
        vs2.add("y");
        vs2.add("z");
        // The z variable is in position 2, which is outside the size of the monomial.
        BOOST_CHECK_THROW(k1.partial(s_to_pos(vs2, symbol("z")), vs), std::invalid_argument);
        // Derivative wrt multiple variables.
        BOOST_CHECK_THROW(k1.partial(symbol_set::positions(vs2, symbol_set({symbol("x"), symbol("y")})), vs),
                          std::invalid_argument);
    }
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_partial_test)
{
    boost::mpl::for_each<int_types>(partial_tester());
}

struct evaluate_tester {
    template <typename T>
    void operator()(const T &)
    {
        using k_type = kronecker_monomial<T>;
        using dict_type1 = std::unordered_map<symbol, integer>;
        using pmap_type1 = symbol_set::positions_map<integer>;
        BOOST_CHECK((key_is_evaluable<k_type, integer>::value));
        symbol_set vs;
        k_type k1;
        BOOST_CHECK_EQUAL(k1.evaluate(pmap_type1(vs, dict_type1{}), vs), integer(1));
        vs.add("x");
        BOOST_CHECK_THROW(k1.evaluate(pmap_type1(vs, dict_type1{}), vs), std::invalid_argument);
        k1 = k_type({T(1)});
        BOOST_CHECK_THROW(k1.evaluate(pmap_type1(vs, dict_type1{}), vs), std::invalid_argument);
        BOOST_CHECK_EQUAL(k1.evaluate(pmap_type1(vs, dict_type1{{symbol("x"), integer(1)}}), vs), 1);
        // pmap with invalid position, 1, where the monomial has only 1 element.
        BOOST_CHECK_THROW(
            k1.evaluate(pmap_type1(symbol_set{symbol{"a"}, symbol{"b"}}, dict_type1{{symbol{"b"}, integer(4)}}), vs),
            std::invalid_argument);
        k1 = k_type({T(2)});
        BOOST_CHECK_EQUAL(k1.evaluate(pmap_type1(vs, dict_type1{{symbol("x"), integer(3)}}), vs), 9);
        BOOST_CHECK_EQUAL(
            k1.evaluate(pmap_type1(vs, dict_type1{{symbol("x"), integer(3)}, {symbol("y"), integer(4)}}), vs), 9);
        k1 = k_type({T(2), T(3)});
        vs.add("y");
        BOOST_CHECK_EQUAL(
            k1.evaluate(pmap_type1(vs, dict_type1{{symbol("x"), integer(3)}, {symbol("y"), integer(4)}}), vs), 576);
        BOOST_CHECK_EQUAL(
            k1.evaluate(pmap_type1(vs, dict_type1{{symbol("y"), integer(4)}, {symbol("x"), integer(3)}}), vs), 576);
        // pmap has correctly 2 elements, but they refer to indices 0 and 2.
        BOOST_CHECK_THROW(k1.evaluate(pmap_type1(symbol_set{symbol{"a"}, symbol{"b"}, symbol{"c"}},
                                                 dict_type1{{symbol{"a"}, integer(4)}, {symbol{"c"}, integer(4)}}),
                                      vs),
                          std::invalid_argument);
        // Same with indices 1 and 2.
        BOOST_CHECK_THROW(k1.evaluate(pmap_type1(symbol_set{symbol{"a"}, symbol{"b"}, symbol{"c"}},
                                                 dict_type1{{symbol{"b"}, integer(4)}, {symbol{"c"}, integer(4)}}),
                                      vs),
                          std::invalid_argument);
        using dict_type2 = std::unordered_map<symbol, double>;
        using pmap_type2 = symbol_set::positions_map<double>;
        BOOST_CHECK_EQUAL(k1.evaluate(pmap_type2(vs, dict_type2{{symbol("y"), -4.3}, {symbol("x"), 3.2}}), vs),
                          math::pow(3.2, 2) * math::pow(-4.3, 3));
        using dict_type3 = std::unordered_map<symbol, rational>;
        using pmap_type3 = symbol_set::positions_map<rational>;
        BOOST_CHECK_EQUAL(
            k1.evaluate(pmap_type3(vs, dict_type3{{symbol("y"), rational(1, 2)}, {symbol("x"), rational(-4, 3)}}), vs),
            math::pow(rational(4, -3), 2) * math::pow(rational(-1, -2), 3));
        k1 = k_type({T(-2), T(-3)});
        BOOST_CHECK_EQUAL(
            k1.evaluate(pmap_type3(vs, dict_type3{{symbol("y"), rational(1, 2)}, {symbol("x"), rational(-4, 3)}}), vs),
            math::pow(rational(4, -3), -2) * math::pow(rational(-1, -2), -3));
        using dict_type4 = std::unordered_map<symbol, real>;
        using pmap_type4 = symbol_set::positions_map<real>;
        BOOST_CHECK_EQUAL(
            k1.evaluate(pmap_type4(vs, dict_type4{{symbol("y"), real(1.234)}, {symbol("x"), real(5.678)}}), vs),
            math::pow(real(5.678), -2) * math::pow(real(1.234), -3));
    }
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_evaluate_test)
{
    boost::mpl::for_each<int_types>(evaluate_tester());
    BOOST_CHECK((!key_is_evaluable<kronecker_monomial<>, std::vector<int>>::value));
    BOOST_CHECK((!key_is_evaluable<kronecker_monomial<>, char *>::value));
    BOOST_CHECK((!key_is_evaluable<kronecker_monomial<>, std::string>::value));
    BOOST_CHECK((!key_is_evaluable<kronecker_monomial<>, void *>::value));
}

struct subs_tester {
    template <typename T>
    void operator()(const T &)
    {
        typedef kronecker_monomial<T> k_type;
        symbol_set vs;
        // Test the type trait.
        BOOST_CHECK((key_has_subs<k_type, integer>::value));
        BOOST_CHECK((key_has_subs<k_type, rational>::value));
        BOOST_CHECK((key_has_subs<k_type, real>::value));
        BOOST_CHECK((key_has_subs<k_type, double>::value));
        BOOST_CHECK((!key_has_subs<k_type, std::string>::value));
        BOOST_CHECK((!key_has_subs<k_type, std::vector<std::string>>::value));
        k_type k1;
        auto ret = k1.subs("x", integer(4), vs);
        BOOST_CHECK_EQUAL(ret.size(), 1u);
        BOOST_CHECK_EQUAL(ret[0u].first, 1);
        BOOST_CHECK((std::is_same<integer, decltype(ret[0u].first)>::value));
        BOOST_CHECK(ret[0u].second == k1);
        k1 = k_type({T(1)});
        BOOST_CHECK_THROW(k1.subs("x", integer(4), vs), std::invalid_argument);
        vs.add("x");
        k1 = k_type({T(2)});
        ret = k1.subs("y", integer(4), vs);
        BOOST_CHECK_EQUAL(ret.size(), 1u);
        BOOST_CHECK_EQUAL(ret[0u].first, 1);
        BOOST_CHECK(ret[0u].second == k1);
        ret = k1.subs("x", integer(4), vs);
        BOOST_CHECK_EQUAL(ret.size(), 1u);
        BOOST_CHECK_EQUAL(ret[0u].first, math::pow(integer(4), T(2)));
        BOOST_CHECK(ret[0u].second == k_type{T(0)});
        k1 = k_type({T(2), T(3)});
        vs.add("y");
        ret = k1.subs("y", integer(-2), vs);
        BOOST_CHECK_EQUAL(ret.size(), 1u);
        BOOST_CHECK_EQUAL(ret[0u].first, math::pow(integer(-2), T(3)));
        BOOST_CHECK((ret[0u].second == k_type{T(2), T(0)}));
        auto ret2 = k1.subs("x", real(-2.345), vs);
        BOOST_CHECK_EQUAL(ret2.size(), 1u);
        BOOST_CHECK_EQUAL(ret2[0u].first, math::pow(real(-2.345), T(2)));
        BOOST_CHECK((ret2[0u].second == k_type{T(0), T(3)}));
        BOOST_CHECK((std::is_same<real, decltype(ret2[0u].first)>::value));
        auto ret3 = k1.subs("x", rational(-1, 2), vs);
        BOOST_CHECK_EQUAL(ret3.size(), 1u);
        BOOST_CHECK_EQUAL(ret3[0u].first, rational(1, 4));
        BOOST_CHECK((ret3[0u].second == k_type{T(0), T(3)}));
        BOOST_CHECK((std::is_same<rational, decltype(ret3[0u].first)>::value));
    }
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_subs_test)
{
    boost::mpl::for_each<int_types>(subs_tester());
}

struct print_tex_tester {
    template <typename T>
    void operator()(const T &)
    {
        typedef kronecker_monomial<T> k_type;
        symbol_set vs;
        k_type k1;
        std::ostringstream oss;
        k1.print_tex(oss, vs);
        BOOST_CHECK(oss.str().empty());
        k1 = k_type({T(1)});
        BOOST_CHECK_THROW(k1.print_tex(oss, vs), std::invalid_argument);
        k1 = k_type({T(0)});
        vs.add("x");
        k1.print_tex(oss, vs);
        BOOST_CHECK_EQUAL(oss.str(), "");
        k1 = k_type({T(1)});
        k1.print_tex(oss, vs);
        BOOST_CHECK_EQUAL(oss.str(), "{x}");
        oss.str("");
        k1 = k_type({T(-1)});
        k1.print_tex(oss, vs);
        BOOST_CHECK_EQUAL(oss.str(), "\\frac{1}{{x}}");
        oss.str("");
        k1 = k_type({T(2)});
        k1.print_tex(oss, vs);
        BOOST_CHECK_EQUAL(oss.str(), "{x}^{2}");
        oss.str("");
        k1 = k_type({T(-2)});
        k1.print_tex(oss, vs);
        BOOST_CHECK_EQUAL(oss.str(), "\\frac{1}{{x}^{2}}");
        vs.add("y");
        oss.str("");
        k1 = k_type({T(-2), T(1)});
        k1.print_tex(oss, vs);
        BOOST_CHECK_EQUAL(oss.str(), "\\frac{{y}}{{x}^{2}}");
        oss.str("");
        k1 = k_type({T(-2), T(3)});
        k1.print_tex(oss, vs);
        BOOST_CHECK_EQUAL(oss.str(), "\\frac{{y}^{3}}{{x}^{2}}");
        oss.str("");
        k1 = k_type({T(-2), T(-3)});
        k1.print_tex(oss, vs);
        BOOST_CHECK_EQUAL(oss.str(), "\\frac{1}{{x}^{2}{y}^{3}}");
        oss.str("");
        k1 = k_type({T(2), T(3)});
        k1.print_tex(oss, vs);
        BOOST_CHECK_EQUAL(oss.str(), "{x}^{2}{y}^{3}");
        oss.str("");
        k1 = k_type({T(1), T(3)});
        k1.print_tex(oss, vs);
        BOOST_CHECK_EQUAL(oss.str(), "{x}{y}^{3}");
        oss.str("");
        k1 = k_type({T(0), T(3)});
        k1.print_tex(oss, vs);
        BOOST_CHECK_EQUAL(oss.str(), "{y}^{3}");
        oss.str("");
        k1 = k_type({T(0), T(0)});
        k1.print_tex(oss, vs);
        BOOST_CHECK_EQUAL(oss.str(), "");
        oss.str("");
        k1 = k_type({T(0), T(1)});
        k1.print_tex(oss, vs);
        BOOST_CHECK_EQUAL(oss.str(), "{y}");
        oss.str("");
        k1 = k_type({T(0), T(-1)});
        k1.print_tex(oss, vs);
        BOOST_CHECK_EQUAL(oss.str(), "\\frac{1}{{y}}");
    }
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_print_tex_test)
{
    boost::mpl::for_each<int_types>(print_tex_tester());
}

struct integrate_tester {
    template <typename T>
    void operator()(const T &)
    {
        typedef kronecker_monomial<T> k_type;
        BOOST_CHECK(key_is_integrable<k_type>::value);
        symbol_set vs;
        k_type k1;
        auto ret = k1.integrate(symbol("a"), vs);
        BOOST_CHECK_EQUAL(ret.first, T(1));
        BOOST_CHECK(ret.second == k_type({T(1)}));
        k1 = k_type{T(1)};
        BOOST_CHECK_THROW(k1.integrate(symbol("b"), vs), std::invalid_argument);
        vs.add("b");
        ret = k1.integrate(symbol("b"), vs);
        BOOST_CHECK_EQUAL(ret.first, T(2));
        BOOST_CHECK(ret.second == k_type({T(2)}));
        k1 = k_type{T(2)};
        ret = k1.integrate(symbol("c"), vs);
        BOOST_CHECK_EQUAL(ret.first, T(1));
        BOOST_CHECK(ret.second == k_type({T(2), T(1)}));
        ret = k1.integrate(symbol("a"), vs);
        BOOST_CHECK_EQUAL(ret.first, T(1));
        BOOST_CHECK(ret.second == k_type({T(1), T(2)}));
        k1 = k_type{T(0), T(1)};
        vs.add("d");
        ret = k1.integrate(symbol("a"), vs);
        BOOST_CHECK_EQUAL(ret.first, T(1));
        BOOST_CHECK(ret.second == k_type({T(1), T(0), T(1)}));
        ret = k1.integrate(symbol("b"), vs);
        BOOST_CHECK_EQUAL(ret.first, T(1));
        BOOST_CHECK(ret.second == k_type({T(1), T(1)}));
        ret = k1.integrate(symbol("c"), vs);
        BOOST_CHECK_EQUAL(ret.first, T(1));
        BOOST_CHECK(ret.second == k_type({T(0), T(1), T(1)}));
        ret = k1.integrate(symbol("d"), vs);
        BOOST_CHECK_EQUAL(ret.first, T(2));
        BOOST_CHECK(ret.second == k_type({T(0), T(2)}));
        ret = k1.integrate(symbol("e"), vs);
        BOOST_CHECK_EQUAL(ret.first, T(1));
        BOOST_CHECK(ret.second == k_type({T(0), T(1), T(1)}));
        k1 = k_type{T(-1), T(0)};
        BOOST_CHECK_THROW(k1.integrate(symbol("b"), vs), std::invalid_argument);
        k1 = k_type{T(0), T(-1)};
        BOOST_CHECK_THROW(k1.integrate(symbol("d"), vs), std::invalid_argument);
        // Check limits violation.
        typedef kronecker_array<T> ka;
        const auto &limits = ka::get_limits();
        k1 = k_type{std::get<0u>(limits[2u])[0u], std::get<0u>(limits[2u])[0u]};
        BOOST_CHECK_THROW(ret = k1.integrate(symbol("b"), vs), std::invalid_argument);
    }
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_integrate_test)
{
    boost::mpl::for_each<int_types>(integrate_tester());
}

struct trim_identify_tester {
    template <typename T>
    void operator()(const T &)
    {
        typedef kronecker_monomial<T> k_type;
        k_type k0;
        symbol_set v1, v2;
        k0.set_int(1);
        BOOST_CHECK_THROW(k0.trim_identify(v2, v2), std::invalid_argument);
        v1.add("x");
        v2.add("y");
        v2.add("x");
        k0 = k_type({T(1), T(2)});
        k0.trim_identify(v1, v2);
        BOOST_CHECK(v1 == symbol_set());
        k0 = k_type({T(0), T(2)});
        v1.add("x");
        v1.add("y");
        k0.trim_identify(v1, v2);
        BOOST_CHECK(v1 == symbol_set({symbol("x")}));
        k0 = k_type({T(0), T(0)});
        v1.add("y");
        k0.trim_identify(v1, v2);
        BOOST_CHECK(v1 == symbol_set({symbol("x"), symbol("y")}));
        k0 = k_type({T(1), T(0)});
        k0.trim_identify(v1, v2);
        BOOST_CHECK(v1 == symbol_set({symbol("y")}));
    }
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_trim_identify_test)
{
    boost::mpl::for_each<int_types>(trim_identify_tester());
}

struct trim_tester {
    template <typename T>
    void operator()(const T &)
    {
        typedef kronecker_monomial<T> k_type;
        k_type k0;
        symbol_set v1, v2;
        k0.set_int(1);
        BOOST_CHECK_THROW(k0.trim(v1, v2), std::invalid_argument);
        v1.add("x");
        v1.add("y");
        v1.add("z");
        k0 = k_type{T(1), T(0), T(-1)};
        v2.add("x");
        BOOST_CHECK((k0.trim(v2, v1) == k_type{T(0), T(-1)}));
        v2.add("z");
        v2.add("a");
        BOOST_CHECK((k0.trim(v2, v1) == k_type{T(0)}));
        v2.add("y");
        BOOST_CHECK((k0.trim(v2, v1) == k_type{}));
        v2 = symbol_set();
        BOOST_CHECK((k0.trim(v2, v1) == k0));
    }
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_trim_test)
{
    boost::mpl::for_each<int_types>(trim_tester());
}

struct ipow_subs_tester {
    template <typename T>
    void operator()(const T &)
    {
        typedef kronecker_monomial<T> k_type;
        // Test the type trait.
        BOOST_CHECK((key_has_ipow_subs<k_type, integer>::value));
        BOOST_CHECK((key_has_ipow_subs<k_type, double>::value));
        BOOST_CHECK((key_has_ipow_subs<k_type, real>::value));
        BOOST_CHECK((key_has_ipow_subs<k_type, rational>::value));
        BOOST_CHECK((!key_has_ipow_subs<k_type, std::string>::value));
        symbol_set vs;
        k_type k1;
        auto ret = k1.ipow_subs("x", integer(45), integer(4), vs);
        BOOST_CHECK_EQUAL(ret.size(), 1u);
        BOOST_CHECK_EQUAL(ret[0u].first, 1);
        BOOST_CHECK((std::is_same<integer, decltype(ret[0u].first)>::value));
        BOOST_CHECK(ret[0u].second == k1);
        k1.set_int(1);
        BOOST_CHECK_THROW(k1.ipow_subs("x", integer(35), integer(4), vs), std::invalid_argument);
        vs.add("x");
        k1 = k_type({T(2)});
        ret = k1.ipow_subs("y", integer(2), integer(4), vs);
        BOOST_CHECK_EQUAL(ret.size(), 1u);
        BOOST_CHECK_EQUAL(ret[0u].first, 1);
        BOOST_CHECK(ret[0u].second == k1);
        ret = k1.ipow_subs("x", integer(1), integer(4), vs);
        BOOST_CHECK_EQUAL(ret.size(), 1u);
        BOOST_CHECK_EQUAL(ret[0u].first, math::pow(integer(4), T(2)));
        BOOST_CHECK(ret[0u].second == k_type({T(0)}));
        ret = k1.ipow_subs("x", integer(2), integer(4), vs);
        BOOST_CHECK_EQUAL(ret.size(), 1u);
        BOOST_CHECK_EQUAL(ret[0u].first, math::pow(integer(4), T(1)));
        BOOST_CHECK(ret[0u].second == k_type({T(0)}));
        ret = k1.ipow_subs("x", integer(-1), integer(4), vs);
        BOOST_CHECK_EQUAL(ret.size(), 1u);
        BOOST_CHECK_EQUAL(ret[0u].first, 1);
        BOOST_CHECK(ret[0u].second == k_type({T(2)}));
        ret = k1.ipow_subs("x", integer(4), integer(4), vs);
        BOOST_CHECK_EQUAL(ret.size(), 1u);
        BOOST_CHECK_EQUAL(ret[0u].first, 1);
        BOOST_CHECK(ret[0u].second == k_type({T(2)}));
        if (std::is_same<T, signed char>::value) {
            // Values below are too large for signed char.
            return;
        }
        k1 = k_type({T(7), T(2)});
        vs.add("y");
        ret = k1.ipow_subs("x", integer(3), integer(2), vs);
        BOOST_CHECK_EQUAL(ret.size(), 1u);
        BOOST_CHECK_EQUAL(ret[0u].first, math::pow(integer(2), T(2)));
        BOOST_CHECK((ret[0u].second == k_type{T(1), T(2)}));
        ret = k1.ipow_subs("x", integer(4), integer(2), vs);
        BOOST_CHECK_EQUAL(ret.size(), 1u);
        BOOST_CHECK_EQUAL(ret[0u].first, math::pow(integer(2), T(1)));
        BOOST_CHECK((ret[0u].second == k_type{T(3), T(2)}));
        ret = k1.ipow_subs("x", integer(-4), integer(2), vs);
        BOOST_CHECK_EQUAL(ret.size(), 1u);
        BOOST_CHECK_EQUAL(ret[0u].first, 1);
        BOOST_CHECK((ret[0u].second == k_type{T(7), T(2)}));
        k1 = k_type({T(-7), T(2)});
        ret = k1.ipow_subs("x", integer(4), integer(2), vs);
        BOOST_CHECK_EQUAL(ret.size(), 1u);
        BOOST_CHECK_EQUAL(ret[0u].first, 1);
        BOOST_CHECK((ret[0u].second == k_type{T(-7), T(2)}));
        ret = k1.ipow_subs("x", integer(-4), integer(2), vs);
        BOOST_CHECK_EQUAL(ret.size(), 1u);
        BOOST_CHECK_EQUAL(ret[0u].first, math::pow(integer(2), T(1)));
        BOOST_CHECK((ret[0u].second == k_type{T(-3), T(2)}));
        ret = k1.ipow_subs("x", integer(-3), integer(2), vs);
        BOOST_CHECK_EQUAL(ret.size(), 1u);
        BOOST_CHECK_EQUAL(ret[0u].first, math::pow(integer(2), T(2)));
        BOOST_CHECK((ret[0u].second == k_type{T(-1), T(2)}));
        k1 = k_type({T(2), T(-7)});
        ret = k1.ipow_subs("y", integer(-3), integer(2), vs);
        BOOST_CHECK_EQUAL(ret.size(), 1u);
        BOOST_CHECK_EQUAL(ret[0u].first, math::pow(integer(2), T(2)));
        BOOST_CHECK((ret[0u].second == k_type{T(2), T(-1)}));
        BOOST_CHECK_THROW(k1.ipow_subs("y", integer(0), integer(2), vs), zero_division_error);
        k1 = k_type({T(-7), T(2)});
        auto ret2 = k1.ipow_subs("x", integer(-4), real(-2.345), vs);
        BOOST_CHECK_EQUAL(ret2.size(), 1u);
        BOOST_CHECK_EQUAL(ret2[0u].first, math::pow(real(-2.345), T(1)));
        BOOST_CHECK((ret2[0u].second == k_type{T(-3), T(2)}));
        BOOST_CHECK((std::is_same<real, decltype(ret2[0u].first)>::value));
        auto ret3 = k1.ipow_subs("x", integer(-3), rational(-1, 2), vs);
        BOOST_CHECK_EQUAL(ret3.size(), 1u);
        BOOST_CHECK_EQUAL(ret3[0u].first, math::pow(rational(-1, 2), T(2)));
        BOOST_CHECK((ret3[0u].second == k_type{T(-1), T(2)}));
        BOOST_CHECK((std::is_same<rational, decltype(ret3[0u].first)>::value));
    }
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_ipow_subs_test)
{
    boost::mpl::for_each<int_types>(ipow_subs_tester());
}

struct tt_tester {
    template <typename T>
    void operator()(const T &)
    {
        typedef kronecker_monomial<T> k_type;
        BOOST_CHECK((!key_has_t_subs<k_type, int, int>::value));
        BOOST_CHECK((!key_has_t_subs<k_type &, int, int>::value));
        BOOST_CHECK((!key_has_t_subs<k_type const &, int, int>::value));
        BOOST_CHECK(is_hashable<k_type>::value);
        BOOST_CHECK(key_has_degree<k_type>::value);
        BOOST_CHECK(key_has_ldegree<k_type>::value);
        BOOST_CHECK(!key_has_t_degree<k_type>::value);
        BOOST_CHECK(!key_has_t_ldegree<k_type>::value);
        BOOST_CHECK(!key_has_t_order<k_type>::value);
        BOOST_CHECK(!key_has_t_lorder<k_type>::value);
    }
};

BOOST_AUTO_TEST_CASE(kronecker_monomial_type_traits_test)
{
    boost::mpl::for_each<int_types>(tt_tester());
}

BOOST_AUTO_TEST_CASE(kronecker_monomial_kic_test)
{
    BOOST_CHECK((key_is_convertible<k_monomial, k_monomial>::value));
    BOOST_CHECK((!key_is_convertible<kronecker_monomial<int>, kronecker_monomial<long>>::value));
    BOOST_CHECK((!key_is_convertible<k_monomial, monomial<int>>::value));
    BOOST_CHECK((!key_is_convertible<monomial<int>, k_monomial>::value));
}

BOOST_AUTO_TEST_CASE(kronecker_monomial_comparison_test)
{
    BOOST_CHECK((is_less_than_comparable<k_monomial>::value));
    BOOST_CHECK(!(k_monomial{} < k_monomial{}));
    BOOST_CHECK(!(k_monomial{1} < k_monomial{1}));
    BOOST_CHECK(!(k_monomial{2} < k_monomial{1}));
    BOOST_CHECK(k_monomial{1} < k_monomial{2});
}
#endif