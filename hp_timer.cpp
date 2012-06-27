// hp_timer.cpp

#include "hp_timer.h"
#include <stdio.h>

#ifdef _WIN32
	int64_t __freq;
	int64_t __time_at_init;

	void initTimer()
	{
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		__freq = freq.QuadPart;

		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		__time_at_init = now.QuadPart;
	}

	void shutTimer() {}

#elif defined(__MACH__)

	clock_serv_t	__clock_rt;
	unsigned int	__tv_sec_at_init;

	void initTimer()
	{
		host_get_clock_service(mach_host_self(), REALTIME_CLOCK, &__clock_rt);

		//		natural_t				attributes[4];
		//		mach_msg_type_number_t	count;
		//		clock_get_attributes(__clock_rt, CLOCK_GET_TIME_RES,
		//							  (clock_attr_t)&attributes, &count);
		//		printf("MacOS X: realtime clock resolution: %u ns\n", attributes[0]);

		mach_timespec_t mts;
		clock_get_time(__clock_rt, &mts);
		__tv_sec_at_init = mts.tv_sec;
	}

	void shutTimer()
	{
		mach_port_deallocate(mach_task_self(), __clock_rt);
	}

#elif defined(__linux__)
	time_t	__tv_sec_at_init = 0;

	void initTimer()
	{
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		__tv_sec_at_init = ts.tv_sec;
	}

	void shutTimer() {}

#else
	time_t	__tv_sec_at_init = 0;

	void initTimer()
	{
		struct timeval tv;
		gettimeofday(&tv, NULL);
		__tv_sec_at_init = ts.tv_sec;
	}

	void shutTimer() {}
#endif
