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

#include <boost/iterator/counting_iterator.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <unordered_set>
#include <utility>

#include "config.hpp"
#include "exceptions.hpp"
#include "runtime_info.hpp"
#include "settings.hpp"
#include "thread_management.hpp"

#ifdef PIRANHA_THREAD_MODEL_PTHREADS
extern "C"
{
#include <pthread.h>
}
#endif

#ifdef _GNU_SOURCE
extern "C"
{
#include <sched.h>
}
#endif

namespace piranha
{

std::mutex thread_management::m_mutex;

std::mutex thread_management::binder::m_binder_mutex;
std::unordered_set<unsigned> thread_management::binder::m_used_procs;

/// Bind thread to specific processor.
/**
 * Upon successful completion of this method, the calling thread will be confined on the specified processor.
 * This method requires platform-specific functions and thus might not be available on all configurations.
 * 
 * @param[in] n index of the processor to which the thread will be bound (starting from index 0).
 * 
 * @throws std::invalid_argument if one of these conditions arises:
 * \li n is greater than an implementation-defined maximum value,
 * \li piranha::runtime_info::hardware_concurrency() returns a nonzero value \p m and <tt>n >= m</tt>.
 * @throws piranha::not_implemented_error if the method is not available on the current platform.
 * @throws std::system_error in case of failure(s) by threading primitives.
 * @throws std::runtime_error if the operation fails in an unspecified way.
 */
void thread_management::bind_to_proc(unsigned n)
{
	std::lock_guard<std::mutex> lock(m_mutex);
#if defined(PIRANHA_THREAD_MODEL_PTHREADS) && defined(_GNU_SOURCE)
	unsigned cpu_setsize;
	int n_int;
	try {
		cpu_setsize = boost::numeric_cast<unsigned>(CPU_SETSIZE);
		n_int = boost::numeric_cast<int>(n);
	} catch (const boost::numeric::bad_numeric_cast &) {
		piranha_throw(std::runtime_error,"numeric conversion error");
	}
	if (n >= cpu_setsize) {
		piranha_throw(std::invalid_argument,"processor index is larger than the maximum allowed value");
	}
	if (runtime_info::hardware_concurrency() != 0 && n >= runtime_info::hardware_concurrency()) {
		piranha_throw(std::invalid_argument,"processor index is larger than the detected hardware concurrency");
	}
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(n_int,&cpuset);
	const int errno_ = ::pthread_setaffinity_np(::pthread_self(),sizeof(cpuset),&cpuset);
	if (errno_ != 0) {
		piranha_throw(std::runtime_error,"the call to pthread_setaffinity_np() failed");
	}
#else
#warning No bind_to_proc implementation available.
	(void)n;
	piranha_throw(not_implemented_error,"bind_to_proc is not available on this platform")
#endif
}

/// Query if current thread is bound to a processor.
/**
 * The complexity of the operation is at most linear in the number of processors available on the system.
 * This method requires platform-specific functions and thus might not be available on all configurations.
 * 
 * @return the pair <tt>(true,n)</tt> if the calling thread is bound to a single processor with index \p n, <tt>(false,0)</tt>
 * otherwise.
 * 
 * @throws piranha::not_implemented_error if the method is not available on the current platform.
 * @throws std::runtime_error if the operation fails in an unspecified way.
 */
std::pair<bool,unsigned> thread_management::bound_proc()
{
	std::lock_guard<std::mutex> lock(m_mutex);
#if defined(PIRANHA_THREAD_MODEL_PTHREADS) && defined(_GNU_SOURCE)
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	const int errno_ = ::pthread_getaffinity_np(::pthread_self(),sizeof(cpuset),&cpuset);
	if (errno_ != 0) {
		piranha_throw(std::runtime_error,"the call to pthread_getaffinity_np() failed");
	}
	const int cpu_count = CPU_COUNT(&cpuset);
	if (cpu_count == 0 || cpu_count > 1) {
		return std::make_pair(false,static_cast<unsigned>(0));
	}
	int cpu_setsize;
	try {
		cpu_setsize = boost::numeric_cast<int>(CPU_SETSIZE);
	} catch (const boost::numeric::bad_numeric_cast &) {
		piranha_throw(std::runtime_error,"numeric conversion error");
	}
	for (int i = 0; i < cpu_setsize; ++i) {
		if (CPU_ISSET(i,&cpuset)) {
			// Cast is safe here (verified above that cpu_setsize is representable in int,
			// and, by extension, in unsigned).
			return std::make_pair(true,static_cast<unsigned>(i));
		}
	}
	piranha_throw(std::runtime_error,"operation failed");
#else
#warning No bound_proc implementation available.
	piranha_throw(not_implemented_error,"bound_proc is not available on this platform")
#endif
}

/// Default constructor.
/**
 * Will attempt to bind the calling thread to the first available processor.
 */
thread_management::binder::binder():m_result(false,0)
{
	// Do nothing if we are not in a different thread.
	if (std::this_thread::get_id() == runtime_info::get_main_thread_id()) {
		return;
	}
	std::lock_guard<std::mutex> lock(m_binder_mutex);
	unsigned candidate = 0, n_threads = settings::get_n_threads();
	for (; candidate < n_threads; ++candidate) {
		// If the processor is not taken, bail out and try to use it.
		if (m_used_procs.find(candidate) == m_used_procs.end()) {
			break;
		}
	}
	// If all procs were already taken, do not try to do any binding.
	if (candidate == n_threads) {
		return;
	}
	// Try to do the binding.
	try {
		bind_to_proc(candidate);
	} catch (...) {
		// Ignore any error and return.
		return;
	}
	// Bind was successful, record it.
	auto result = m_used_procs.insert(candidate);
	piranha_assert(result.second);
	m_result.first = true;
	m_result.second = candidate;
}

/// Destructor.
/**
 * Will remove the corresponding processor index from the internal list of used processors,
 * if the constructor resulted in a succesful bind operation.
 */
thread_management::binder::~binder()
{
	// Do nothing if we are not in a different thread.
	if (std::this_thread::get_id() == runtime_info::get_main_thread_id()) {
		return;
	}
	std::lock_guard<std::mutex> lock(m_binder_mutex);
	if (m_result.first) {
		auto it = m_used_procs.find(m_result.second);
		piranha_assert(it != m_used_procs.end());
		m_used_procs.erase(it);
	}
}

}
