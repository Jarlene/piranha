/***************************************************************************
 *   Copyright (C) 2009-2011 by Francesco Biscani                          *
 *   bluescarni@gmail.com                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "../src/dynamic_aligning_allocator.hpp"

#define BOOST_TEST_MODULE dynamic_aligning_allocator_test
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <boost/integer_traits.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <cstddef>
#include <iterator>
#include <list>
#include <memory>
#include <new>
#include <string>
#include <vector>

#include "../src/environment.hpp"

typedef boost::mpl::vector<double,std::string> types;

using namespace piranha;

BOOST_AUTO_TEST_CASE(dynamic_aligning_allocator_general_test)
{
	environment env;
	dynamic_aligning_allocator<int> daa1, daa2;
	BOOST_CHECK(daa1 == daa2);
	BOOST_CHECK(!(daa1 != daa2));
	BOOST_CHECK(daa1.alignment() == 0);
	BOOST_CHECK(daa2.alignment() == 0);
	dynamic_aligning_allocator<int> daa3, daa4(alignof(int));
	BOOST_CHECK(daa3 != daa4);
	BOOST_CHECK(daa3.alignment() == 0);
	BOOST_CHECK(daa4.alignment() == alignof(int));
	daa3 = daa4;
	BOOST_CHECK(daa3.alignment() == alignof(int));
	BOOST_CHECK(daa4.alignment() == alignof(int));
	daa3 = std::move(daa4);
	BOOST_CHECK(daa3.alignment() == alignof(int));
	BOOST_CHECK(daa4.alignment() == alignof(int));
	if (sizeof(int) > 1u) {
		BOOST_CHECK_THROW(daa4.allocate(boost::integer_traits<std::size_t>::const_max),std::bad_alloc);
	}
	// Constructors from different instances.
	dynamic_aligning_allocator<long> daa1a(daa4), daa2a(std::move(daa3));
	BOOST_CHECK(daa1a.alignment() == daa4.alignment());
	BOOST_CHECK(daa2a.alignment() == daa4.alignment());
}

struct std_container_tester
{
	template <typename T>
	void operator()(const T &)
	{
		// Test rebinding as well.
		using a_type = dynamic_aligning_allocator<int>;
		using a_type2 = dynamic_aligning_allocator<T>;
		std::vector<T,a_type> v;
		std::generate_n(std::back_inserter(v),100,[](){return boost::lexical_cast<T>(1);});
		BOOST_CHECK(std::all_of(v.begin(),v.end(),[](const T &x) {return x == boost::lexical_cast<T>(1);}));
		std::vector<T,a_type> l;
		std::generate_n(std::back_inserter(l),100,[](){return boost::lexical_cast<T>(1);});
		BOOST_CHECK(std::all_of(l.begin(),l.end(),[](const T &x) {return x == boost::lexical_cast<T>(1);}));
		std::vector<T,a_type2> va(a_type2(alignof(T)));
		std::generate_n(std::back_inserter(va),100,[](){return boost::lexical_cast<T>(2);});
		BOOST_CHECK(std::all_of(va.begin(),va.end(),[](const T &x) {return x == boost::lexical_cast<T>(2);}));
		// NOTE: there seems to be a problem currently here with GCC's stdlib - it tries to use directly
		// the allocator's typedefs instead of using allocator_traits.
		//std::list<T,a_type2> la(a_type2(alignof(T)));
		//std::generate_n(std::back_inserter(la),100,[](){return boost::lexical_cast<T>(2);});
		//BOOST_CHECK(std::all_of(la.begin(),la.end(),[](const T &x) {return x == boost::lexical_cast<T>(2);}));
		if (alignof(T) > boost::integer_traits<std::size_t>::const_max / 2u) {
			return;
		}
		std::vector<T,a_type2> va2(a_type2(alignof(T) * 2u));
		std::generate_n(std::back_inserter(va2),100,[](){return boost::lexical_cast<T>(3);});
		auto va3(va2);
		va2 = va;
		BOOST_CHECK(std::all_of(va2.begin(),va2.end(),[](const T &x) {return x == boost::lexical_cast<T>(2);}));
		va3 = std::move(va);
		BOOST_CHECK(std::all_of(va3.begin(),va3.end(),[](const T &x) {return x == boost::lexical_cast<T>(2);}));
		BOOST_CHECK(std::allocator_traits<a_type2>::propagate_on_container_move_assignment::value);
	}
};

BOOST_AUTO_TEST_CASE(dynamic_aligning_allocator_std_container_test)
{
	boost::mpl::for_each<types>(std_container_tester());
}