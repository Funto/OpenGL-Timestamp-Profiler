// thread.h
// Thin interface over the Windows Threads API and pthread.

#ifndef __THREAD_H__
#define __THREAD_H__

// Basic types: ThreadId, Mutex
#ifdef WIN32
	#include <windows.h>
	typedef HANDLE				ThreadId;
	typedef CRITICAL_SECTION	Mutex;
	typedef HANDLE				Event;
#else
	#include <pthread.h>
	typedef pthread_t		ThreadId;
	typedef pthread_mutex_t	Mutex;
	struct Event
	{
		pthread_mutex_t	mutex;
		pthread_cond_t	cond;
		bool			triggered;
	};
#endif

typedef	void*	(*ThreadProc)(void* arg);

ThreadId	threadCreate(ThreadProc proc, void* arg);
ThreadId	threadGetId();
void		threadJoin(ThreadId id);

void		mutexCreate(Mutex* mutex);
void		mutexDestroy(Mutex* mutex);
void		mutexLock(Mutex* mutex);
void		mutexUnlock(Mutex* mutex);

void		eventCreate(Event* event);
void		eventDestroy(Event* event);
void		eventTrigger(Event* event);
void		eventReset(Event* event);
void		eventWait(Event* event);

// ----- Hash table support -----
class HashOpsThreadId
{
	static unsigned char getHash(ThreadId id)
	{
		unsigned int uint_id = (unsigned int)id;

		// FNV: http://www.isthe.com/chongo/tech/comp/fnv/index.html#FNV-source
		unsigned int hash = 2166136261u;

		hash *= 16777619;
		hash = hash ^ (uint_id & 0xff000000);
		hash *= 16777619;
		hash = hash ^ (uint_id & 0x00ff0000);
		hash *= 16777619;
		hash = hash ^ (uint_id & 0x0000ff00);
		hash *= 16777619;
		hash = hash ^ (uint_id & 0x000000ff);

		return (unsigned char)(hash & 0x000000ff);
	}

	static unsigned char hashIncrement(unsigned char hash)
	{
		return hash+1;	// loops in range 0-255
	}

	static bool equal(ThreadId	a, ThreadId b)
	{
		return (a == b);
	}
};

#endif // __THREAD_H__
