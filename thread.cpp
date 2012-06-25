// thread.cpp

#include "thread.h"
#include <assert.h>

// ------------------------- Windows API implementation-----------------------
// http://www.flipcode.com/archives/Simple_Win32_Thread_Class.shtml
#ifdef WIN32

ThreadId threadCreate(ThreadProc proc, void* arg)
{
	return CreateThread(0, 0, (LPTHREAD_START_ROUTINE)proc, arg, 0, 0);
}

ThreadId threadGetId()
{
	return (ThreadId)GetCurrentThread();
}

void threadJoin(ThreadId id)
{
	DWORD	dwWaitResult = WaitForSingleObject(id, INFINITE);
	assert(dwWaitResult == WAIT_OBJECT_0);
}

void mutexCreate(Mutex* mutex)
{
	InitializeCriticalSection((LPCRITICAL_SECTION)mutex);
}

void mutexDestroy(Mutex* mutex)
{
	DeleteCriticalSection((LPCRITICAL_SECTION)mutex);
}

void mutexLock(Mutex* mutex)
{
	EnterCriticalSection((LPCRITICAL_SECTION)mutex);
}

void mutexUnlock(Mutex* mutex)
{
	LeaveCriticalSection((LPCRITICAL_SECTION)mutex);
}

void eventCreate(Event* event)
{
	*event = CreateEvent(
			NULL,	// security attributes
			TRUE,	// manual reset event
			FALSE,	// initial state is non-signaled
			NULL);	// no name
}

void eventDestroy(Event* event)
{
	CloseHandle(*event);
}

void eventTrigger(Event* event)
{
	SetEvent(*event);
}

void eventReset(Event *event)
{
	ResetEvent(*event);
}

void eventWait(Event* event)
{
	DWORD	dwWaitResult = WaitForSingleObject(*event, INFINITE);
	assert(dwWaitResult == WAIT_OBJECT_0);
}
// ---------------- pthread implementation: MacOS X, Linux, BSD... --------
#else

// --- Thread ---
ThreadId threadCreate(ThreadProc proc, void* arg)
{
	ThreadId	thread_id;
	pthread_create((pthread_t*)(&thread_id), NULL, proc, arg);
	return thread_id;
}

ThreadId threadGetId()
{
	return (ThreadId)pthread_self();
}

void threadJoin(ThreadId id)
{
	pthread_join(id, NULL);
}

// --- Mutex ---
void mutexCreate(Mutex* mutex)
{
	pthread_mutex_init((pthread_mutex_t*)mutex, NULL);
}

void mutexDestroy(Mutex* mutex)
{
	pthread_mutex_destroy((pthread_mutex_t*)mutex);
}

void mutexLock(Mutex* mutex)
{
	pthread_mutex_lock((pthread_mutex_t*)mutex);
}

void mutexUnlock(Mutex* mutex)
{
	pthread_mutex_unlock((pthread_mutex_t*)mutex);
}

// --- Events ---

void eventCreate(Event* event)
{
	pthread_mutex_init(&event->mutex, NULL);
	pthread_cond_init(&event->cond, NULL);
	event->triggered = false;
}

void eventDestroy(Event* event)
{
	pthread_cond_destroy(&event->cond);
	pthread_mutex_destroy(&event->mutex);
}

void eventTrigger(Event* event)
{
	pthread_mutex_lock(&event->mutex);
	event->triggered = true;
	pthread_cond_signal(&event->cond);
	pthread_mutex_unlock(&event->mutex);
}

void eventReset(Event *event)
{
	pthread_mutex_lock(&event->mutex);
	event->triggered = false;
	pthread_mutex_unlock(&event->mutex);
}

void eventWait(Event* event)
{
	pthread_mutex_lock(&event->mutex);
	while (!event->triggered)
		pthread_cond_wait(&event->cond, &event->mutex);
	pthread_mutex_unlock(&event->mutex);
}

#endif
