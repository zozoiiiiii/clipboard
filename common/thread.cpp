// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007 Anthony Williams
// (C) Copyright 2007 David Deakins

const int _WIN32_WINNT = 0x0502;
const int  WINVER = 0x0502;

#include "thread.hpp"

#ifdef _WIN32
#include <process.h>
//for debug
static int term_thread_id = 0;

struct thread_info{
	char thread_name[64];
	int thread_id;
};

class debug_thread
{
public:
	debug_thread(){memset(_info, 0, sizeof(_info));}
	void add_thread(const char* name, int id)
	{
		for(int i = 0; i<4; i++)
		{
			if(_info[i].thread_id == 0)
			{
				strcpy_s(_info[i].thread_name, name);
				_info[i].thread_id = id;
				break;
			}
		}
	}

	void remove_thread(int id)
	{
		for(int i = 0; i<4; i++)
		{
			if(_info[i].thread_id == id)
			{
				_info[i].thread_name[0] = 0;
				_info[i].thread_id = 0;
				break;
			}
		}
	}
private:
	struct thread_info _info[4];
};

static debug_thread dt;

#else
#include <sys/sysctl.h>
#endif

void set_thread_name(unsigned int thread_id, const char* thread_name);
void set_thread_name(unsigned int thread_id, const char* thread_name)
{
#ifdef _WIN32

UNREFERENCED_PARAMETER(thread_id);
UNREFERENCED_PARAMETER(thread_name);

#if !defined(_LOG_TO_FILE)
	return;
#else

    const DWORD MS_VC_EXCEPTION=0x406D1388;
    
#pragma pack(push,8)
    typedef struct tagTHREADNAME_INFO
    {
        DWORD dwType; // Must be 0x1000.
        LPCSTR szName; // Pointer to name (in user addr space).
        DWORD dwThreadID; // Thread ID (-1=caller thread).
        DWORD dwFlags; // Reserved for future use, must be zero.
    } THREADNAME_INFO;
#pragma pack(pop)
    
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = thread_name;
	info.dwThreadID = thread_id;
	info.dwFlags = 0;

	__try
	{
		RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	}
#endif
#elif defined(__LINUX__)
    pthread_setname_np(pthread_self(), thread_name);
#else
    pthread_setname_np(thread_name);
#endif
}

#ifdef _WIN32
unsigned __stdcall thread_start_function(void* param);
unsigned __stdcall thread_start_function(void* param)
#else
void* thread_start_function(void* param);
void* thread_start_function(void* param)
#endif
{
	thread_data_base* const thread_info(reinterpret_cast<thread_data_base*>(param));

	set_thread_name(thread_info->id, thread_info->thread_name.c_str());
	thread_info->run();

	return 0;
}

thread::thread()
{}

bool thread::start_thread(char* thread_name)
{
#ifdef _WIN32
	uintptr_t const new_thread=_beginthreadex(0,0,&thread_start_function,thread_info.get(),CREATE_SUSPENDED,&thread_info->id);
	if(!new_thread)
	{
		return false;
	}
    
    thread_info->thread_handle=(void*)(new_thread);
    thread_info->thread_name.assign(thread_name);

	dt.add_thread(thread_name, thread_info->id);
    
    ResumeThread(thread_info->thread_handle);
#else
    int  ret = pthread_create(&thread_info->thread_t, NULL, thread_start_function, (void*)thread_info.get());
    thread_info->thread_handle = (void*)thread_info->thread_t;
    
    if (ret != 0)
    {
        return false;
    }
    
    thread_info->thread_name.assign(thread_name);
#endif
	return true;
}

thread::thread(thread_data_ptr data):thread_info(data)
{}

thread::~thread()
{
	detach();
}

int thread::join()
{
	thread_data_ptr local_thread_info=(get_thread_info)();
	if(local_thread_info && local_thread_info->thread_handle)
	{
#ifdef _WIN32
		if(WaitForSingleObject(local_thread_info->thread_handle, 120000)==WAIT_TIMEOUT)
		{
			term_thread_id = local_thread_info->id;
            TerminateThread(local_thread_info->thread_handle, 1);
            return 1;
        }
        else
        {
            DWORD exit_code;
            GetExitCodeThread(native_handle(), &exit_code);
            return exit_code;
        }
#else
        long res = 0;
        pthread_join(local_thread_info->thread_t, (void**)&res);
        return (int)res;
#endif
	}
    
    return 1;
}

void thread::detach()
{
	release_handle();
}

bool thread::post_msg(int msg, int* param1, int* param2)
{
#ifdef _WIN32
	return PostThreadMessage(get_id(), msg, (WPARAM)param1, (LPARAM)param2) == TRUE;
#else
	return true;
#endif
}

void thread::release_handle()
{
#ifdef _WIN32
	if(thread_info->thread_handle)
		CloseHandle(thread_info->thread_handle);
	dt.remove_thread(thread_info->id);
#elif !defined(__LINUX__)
    pthread_detach(thread_info->thread_t);
#endif

	thread_info.reset();
}

unsigned thread::hardware_concurrency()
{
#ifdef _WIN32
	SYSTEM_INFO info={{0}};
	GetSystemInfo(&info);
	return info.dwNumberOfProcessors;
#elif defined(__LINUX__)
    return 0;
#else
    size_t numCPU = 0;
    int mib[4];
    size_t len = sizeof(numCPU); 
    
    mib[0] = CTL_HW;
    mib[1] = HW_AVAILCPU;
    
    sysctl(mib, 2, &numCPU, &len, NULL, 0);
    
    if( numCPU < 1 ) 
    {
        mib[1] = HW_NCPU;
        sysctl( mib, 2, &numCPU, &len, NULL, 0 );
        
        if( numCPU < 1 )
        {
            numCPU = 1;
        }
    }
    return (unsigned)numCPU;
#endif
}

thread::native_handle_type thread::native_handle()
{
	thread_data_ptr local_thread_info=(get_thread_info)();
	if(local_thread_info)
	{
#ifdef _WIN32
		return local_thread_info->thread_handle;
#else
        return local_thread_info->thread_t;
#endif
	}

	return 0;
}

thread_data_ptr thread::get_thread_info () const
{
	return thread_info;
}


