// thread.h
// Thin interface over the Windows Threads API and pthread.

#ifndef __THREAD_H__
#define __THREAD_H__

// Basic types: ThreadHandle, ThreadId, Mutex, Event
#ifdef WIN32
	#include <windows.h>
	typedef	HANDLE				ThreadHandle;
	typedef	DWORD				ThreadId;
	typedef	CRITICAL_SECTION	Mutex;
	typedef	HANDLE				Event;
#else
	#include <pthread.h>
	typedef	pthread_t			ThreadHandle;
	typedef	pthread_t			ThreadId;
	typedef	pthread_mutex_t		Mutex;
	struct						Event
	{
		pthread_mutex_t	mutex;
		pthread_cond_t	cond;
		bool			triggered;
	};
#endif

typedef	void*	(*ThreadProc)(void* arg);

ThreadHandle	threadCreate(ThreadProc proc, void* arg);
ThreadId		threadGetCurrentId();
void			threadJoin(ThreadHandle id);

void			mutexCreate(Mutex* mutex);
void			mutexDestroy(Mutex* mutex);
void			mutexLock(Mutex* mutex);
void			mutexUnlock(Mutex* mutex);

void			eventCreate(Event* event);
void			eventDestroy(Event* event);
void			eventTrigger(Event* event);
void			eventReset(Event* event);
void			eventWait(Event* event);

#endif // __THREAD_H__
