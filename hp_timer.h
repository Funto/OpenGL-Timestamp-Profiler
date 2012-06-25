// hp_timer.h

#ifndef __HP_TIMER_H__
#define __HP_TIMER_H__

#include <stdint.h>

#ifdef _WIN32	// Windows 32 bits and 64 bits: use QueryPerformanceCounter()
	#include <windows.h>

	extern uint64_t __freq;
	void initTimer();
	inline void shutTimer() {}

	// TODO
	inline uint64_t	getTimeNs()
	{
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		return (uint64_t)(now.QuadPart / __freq);
	}


#elif defined(__MACH__)	// OSX: use clock_get_time
	#include <sys/time.h>
	#include <mach/mach.h>
	#include <mach/clock.h>

	void initTimer();
	void shutTimer();

	extern clock_serv_t __clock_rt;

	// TODO: change to getTimeNs()
	inline double getTimeMs()
	{
		// http://pastebin.com/89qJQsCw
		// http://www.opensource.apple.com/source/xnu/xnu-344/osfmk/i386/rtclock.c
		// http://opensource.apple.com/source/xnu/xnu-1456.1.26/osfmk/man/host_get_clock_service.html -> HIGHRES_CLOCK

		mach_timespec_t mts;
		clock_get_time(__clock_rt, &mts);

		return (double)(mts.tv_sec) * 1000.0 + (double)(mts.tv_nsec) / 1000000.0;
	}

#elif defined(__linux__)	// Linux: use clock_gettime()

	#include <time.h>

	extern time_t	__tv_sec_at_init;

	void initTimer();
	inline void shutTimer() {}

	inline uint64_t getTimeNs()
	{
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		uint64_t	time_sec = (uint64_t)(ts.tv_sec - __tv_sec_at_init);
		uint64_t	time_ns = time_sec * (uint64_t)(1000000000);
		time_ns += (uint64_t)(ts.tv_nsec);
		return time_ns;
	}

#else	// Fallback for other UN*X systems: use gettimeofday()
	// http://stackoverflow.com/questions/275004/c-timer-function-to-provide-time-in-nano-seconds

	#include <sys/time.h>
	inline uint64_t getTimeNs()
	{
		struct timeval tv;
		gettimeofday(&tv, NULL);
		uint64_t	time_sec = (uint64_t)(ts.tv_sec - __tv_sec_at_init);
		uint64_t	time_ns = time_sec * (uint64_t)(1000000000);
		time_ns += (uint64_t)(ts.tv_nsec);
		return time_ns;
	}
#endif

#endif // __HP_TIMER_H__
