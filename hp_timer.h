// hp_timer.h

#ifndef __HP_TIMER_H__
#define __HP_TIMER_H__

#include <stdint.h>

void initTimer();
void shutTimer();

#ifdef _WIN32	// Windows 32 bits and 64 bits: use QueryPerformanceCounter()
	#include <windows.h>

	extern int64_t __freq;
	extern int64_t __time_at_init;

	inline uint64_t	getTimeNs()
	{
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		static const uint64_t factor = 1000000000;
		return (uint64_t)( factor*(now.QuadPart-__time_at_init) / __freq );
	}

#elif defined(__MACH__)	// OSX: use clock_get_time
	#include <sys/time.h>
	#include <mach/mach.h>
	#include <mach/clock.h>

	extern clock_serv_t	__clock_rt;
	extern unsigned int	__tv_sec_at_init;

	inline uint64_t getTimeNs()
	{
		// http://pastebin.com/89qJQsCw
		// http://www.opensource.apple.com/source/xnu/xnu-344/osfmk/i386/rtclock.c
		// http://opensource.apple.com/source/xnu/xnu-1456.1.26/osfmk/man/host_get_clock_service.html -> HIGHRES_CLOCK

		mach_timespec_t mts;
		clock_get_time(__clock_rt, &mts);

		uint64_t	time_sec = (uint64_t)(mts.tv_sec - __tv_sec_at_init);
		uint64_t	time_ns = time_sec * (uint64_t)(1000000000);
		time_ns += (uint64_t)(mts.tv_nsec);
		return time_ns;
	}

#elif defined(__linux__)	// Linux: use clock_gettime()

	#include <time.h>

	extern time_t	__tv_sec_at_init;

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

	extern time_t	__tv_sec_at_init;

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
