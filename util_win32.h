/*
*/

#ifndef UTIL_WIN32_H
#define UTIL_WIN32_H

#include <condition_variable>
#include <mutex>

#if defined(_WIN32) && !defined(_MSC_VER)

#ifndef NOMINMAX
#  define NOMINMAX
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX

struct Mutex 
{
	Mutex() { InitializeCriticalSection(&cs); }
	~Mutex() { DeleteCriticalSection(&cs); }
	void lock() { EnterCriticalSection(&cs); }
	void unlock() { LeaveCriticalSection(&cs); }

private:
	CRITICAL_SECTION cs;
};

typedef std::condition_variable_any ConditionVariable;

#else

typedef std::mutex Mutex;
typedef std::condition_variable ConditionVariable;

#endif

#endif // #ifndef UTIL_WIN32_H
