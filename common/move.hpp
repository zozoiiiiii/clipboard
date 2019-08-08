// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007-8 Anthony Williams

#ifndef BOOST_THREAD_MOVE_HPP
#define BOOST_THREAD_MOVE_HPP


namespace detail
{
	template<typename T>
	struct thread_move_t
	{
		T& t;
		explicit thread_move_t(T& t_):
		t(t_)
		{}

		T& operator*() const
		{
			return t;
		}

		T* operator->() const
		{
			return &t;
		}
	private:
		void operator=(thread_move_t&);
	};
}

template<typename T>
detail::thread_move_t<T> move(detail::thread_move_t<T> t)
{
	return t;
}

#endif
