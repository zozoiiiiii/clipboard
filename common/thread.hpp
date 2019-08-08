#ifndef BOOST_THREAD_THREAD_COMMON_HPP
#define BOOST_THREAD_THREAD_COMMON_HPP
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007-10 Anthony Williams

#ifdef _WIN32
#include <windows.h>
#include <memory>
#include <functional>
#elif defined(_BB10_)
#include <c++/4.6.3/tr1/memory>
#include <c++/4.6.3/tr1/functional>
#include <pthread.h>
#elif defined(__IOS__) || defined(__MACOS__)
#include <memory>
#include <functional>
#include <pthread.h>
#else
#include <tr1/memory>
#include <tr1/functional>
#include <pthread.h>
#endif

#include <string>

using namespace std;

#if defined (__IOS__) || defined(__MACOS__)
#define tr1_ std
#else
#define tr1_ std::tr1
using namespace tr1;
#endif

#include "move.hpp"
#include "thread_heap_alloc.hpp"

#ifdef __LINUX__
#pragma GCC diagnostic ignored "-Wunused-parameter"
#else
#pragma warning(push)
#pragma warning(disable:4251)
#endif

struct thread_data_base
{
    void* thread_handle;
#ifdef _WIN32
    typedef void* native_handle_type;
#else
    pthread_t thread_t;
    typedef pthread_t native_handle_type;
#endif
    unsigned int id;
    string thread_name;
    
    thread_data_base():thread_handle(0),id(0)
    {
        thread_name[0] = 0;
    }
    
    virtual ~thread_data_base()
    {}
    
    void interrupt()
    {
    }
    
    virtual void run()=0;
};

template<typename F>
class thread_data: public thread_data_base
{
public:
#ifndef BOOST_NO_RVALUE_REFERENCES
    thread_data(F&& f_):
    f(static_cast<F&&>(f_))
    {}
    thread_data(F& f_):
    f(f_)
    {}
#else
    thread_data(F f_):
    f(f_)
    {}
    thread_data(detail::thread_move_t<F> f_):
    f(f_)
    {}
#endif
    void run()
    {
        f();
    }
private:
    F f;
    
    void operator=(thread_data&);
    thread_data(thread_data&);
};

template<typename F>
class thread_data<tr1_::reference_wrapper<F> >:public thread_data_base
{
private:
    F& f;
    
    void operator=(thread_data&);
    thread_data(thread_data&);
public:
    thread_data(tr1_::reference_wrapper<F> f_):
    f(f_)
    {}
    
    void run()
    {
        f();
    }
};

template<typename F>
class thread_data<const tr1_::reference_wrapper<F> >:
public thread_data_base
{
private:
    F& f;
    void operator=(thread_data&);
    thread_data(thread_data&);
public:
    thread_data(const tr1_::reference_wrapper<F> f_):
    f(f_)
    {}
    
    void run()
    {
        f();
    }
};

typedef tr1_::shared_ptr<thread_data_base> thread_data_ptr;

class thread
{
private:
    thread(thread&);
    thread& operator=(thread&);
    
    void release_handle();
    
    thread_data_ptr thread_info;
    
    bool start_thread(char* thread_name);
    
    explicit thread(thread_data_ptr data);
    
#ifndef BOOST_NO_RVALUE_REFERENCES
    template<typename F>
    static inline thread_data_ptr make_thread_info(F&& f)
    {
        return thread_data_ptr(detail::heap_new<thread_data<typename std::remove_reference<F>::type>>(static_cast<F&&>(f)), std::tr1::bind(&detail::heap_delete<thread_data<typename std::remove_reference<F>::type>>, std::tr1::placeholders::_1));
    }
    static inline thread_data_ptr make_thread_info(void (*f)())
    {
        thread_data_ptr ptr(detail::heap_new<thread_data<void(*)()>>(static_cast<void(*&&)()>(f)), std::tr1::bind(&detail::heap_delete<thread_data<void(*)()>>, std::tr1::placeholders::_1));
        return ptr;
    }
#else
    template<typename F>
    static inline thread_data_ptr make_thread_info(F f)
    {
        return thread_data_ptr(detail::heap_new<thread_data<F> >(f), tr1_::bind(&detail::heap_delete<thread_data<F> >, tr1_::placeholders::_1));
    }
    template<typename F>
    static inline thread_data_ptr make_thread_info(detail::thread_move_t<F> f)
    {
        return thread_data_ptr(detail::heap_new<thread_data<F> >(f),tr1_::bind(&detail::heap_delete<thread_data<F> >, tr1_::placeholders::_1));
    }
    
#endif
    struct dummy;
public:
    thread();
    ~thread();
    
#ifndef BOOST_NO_RVALUE_REFERENCES
#ifdef BOOST_MSVC
    template <class F>
    explicit thread(F f,typename disable_if<boost::is_convertible<F&,detail::thread_move_t<F> >, dummy* >::type=0):
    thread_info(make_thread_info(static_cast<F&&>(f)))
    {
        start_thread();
    }
#else
    template <class F>
    thread(F&& f):
    thread_info(make_thread_info(static_cast<F&&>(f)))
    {
        start_thread();
    }
#endif
    
    thread(thread&& other)
    {
        thread_info.swap(other.thread_info);
    }
    
    thread& operator=(thread&& other)
    {
        thread_info=other.thread_info;
        other.thread_info.reset();
        return *this;
    }
    
    thread&& move()
    {
        return static_cast<thread&&>(*this);
    }
    
#else
#ifdef BOOST_NO_SFINAE
    template <class F>
    explicit thread(F f):
    thread_info(make_thread_info(f))
    {
        start_thread(NULL);
    }
#else
    template <class F>
    explicit thread(F f,typename disable_if<boost::is_convertible<F&,detail::thread_move_t<F> >, dummy* >::type=0):
    thread_info(make_thread_info(f))
    {
        start_thread();
    }
#endif
    
    template <class F>
    explicit thread(detail::thread_move_t<F> f):
    thread_info(make_thread_info(f))
    {
        start_thread(NULL);
    }
    
    thread(detail::thread_move_t<thread> x)
    {
        thread_info=x->thread_info;
        x->thread_info.reset();
    }
    
    thread& operator=(detail::thread_move_t<thread> x)
    {
        thread new_thread(x);
        swap(new_thread);
        return *this;
    }
    //#endif
    operator detail::thread_move_t<thread>()
    {
        return move();
    }
    
    detail::thread_move_t<thread> move()
    {
        detail::thread_move_t<thread> x(*this);
        return x;
    }
    
#endif
    
    template <class F,class A1>
    thread(F f,A1 a1, char* thread_name):
    thread_info(make_thread_info(tr1_::bind(f,a1)))
    {
        start_thread(thread_name);
    }
    
    template <class F,class A1,class A2>
    thread(F f,A1 a1,A2 a2, char* thread_name):
    thread_info(make_thread_info(tr1_::bind(f,a1,a2)))
    {
        start_thread(thread_name);
    }
    
#ifdef _WIN32
    /*    template <class F,class A1,class A2,class A3>
     thread(F f,A1 a1,A2 a2,A3 a3):
     thread_info(make_thread_info(boost::bind(std::type<void>(),f,a1,a2,a3)))
     {
     start_thread();
     }
     */
    template <class F,class A1,class A2,class A3,class A4>
    thread(F f,A1 a1,A2 a2,A3 a3,A4 a4):
    thread_info(make_thread_info(boost::bind(std::type<void>(),f,a1,a2,a3,a4)))
    {
        start_thread();
    }
    
    template <class F,class A1,class A2,class A3,class A4,class A5>
    thread(F f,A1 a1,A2 a2,A3 a3,A4 a4,A5 a5):
    thread_info(make_thread_info(boost::bind(std::type<void>(),f,a1,a2,a3,a4,a5)))
    {
        start_thread();
    }
    
    template <class F,class A1,class A2,class A3,class A4,class A5,class A6>
    thread(F f,A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6):
    thread_info(make_thread_info(boost::bind(std::type<void>(),f,a1,a2,a3,a4,a5,a6)))
    {
        start_thread();
    }
    
    template <class F,class A1,class A2,class A3,class A4,class A5,class A6,class A7>
    thread(F f,A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7):
    thread_info(make_thread_info(boost::bind(std::type<void>(),f,a1,a2,a3,a4,a5,a6,a7)))
    {
        start_thread();
    }
    
    template <class F,class A1,class A2,class A3,class A4,class A5,class A6,class A7,class A8>
    thread(F f,A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7,A8 a8):
    thread_info(make_thread_info(boost::bind(std::type<void>(),f,a1,a2,a3,a4,a5,a6,a7,a8)))
    {
        start_thread();
    }
    
    template <class F,class A1,class A2,class A3,class A4,class A5,class A6,class A7,class A8,class A9>
    thread(F f,A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7,A8 a8,A9 a9):
    thread_info(make_thread_info(boost::bind(std::type<void>(),f,a1,a2,a3,a4,a5,a6,a7,a8,a9)))
    {
        start_thread();
    }
#endif
    void swap(thread& x)
    {
        thread_info.swap(x.thread_info);
    }
    
    unsigned int get_id() const{return thread_info->id;}
    
    int join();
    void detach();
    bool post_msg(int msg, int* param1, int* param2);
    
    thread_data_ptr get_thread_info () const;
    static unsigned hardware_concurrency();
    
    typedef thread_data_base::native_handle_type native_handle_type;
    native_handle_type native_handle();
};

#ifndef BOOST_NO_RVALUE_REFERENCES
inline thread&& move(thread& t)
{
    return static_cast<thread&&>(t);
}
inline thread&& move(thread&& t)
{
    return static_cast<thread&&>(t);
}
#else
inline detail::thread_move_t<thread> move(detail::thread_move_t<thread> t)
{
    return t;
}
#endif

#ifndef __LINUX__
#pragma warning(pop)
#endif

#endif

