// hp_timer.cpp

#include "hp_timer.h"
#include <stdio.h>

#ifdef _WIN32
	uint64_t __freq;

	void initTimer()
	{
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		__freq = freq.QuadPart;
	}

#elif defined(__MACH__)

	clock_serv_t __clock_rt;

	void initTimer()
	{
		host_get_clock_service(mach_host_self(), REALTIME_CLOCK, &__clock_rt);

/*		natural_t				attributes[4];
		mach_msg_type_number_t	count;
		clock_get_attributes(__clock_rt, CLOCK_GET_TIME_RES,
							  (clock_attr_t)&attributes, &count);

		printf("MacOS X: realtime clock resolution: %u ns\n", attributes[0]);
*/
	}

	void shutTimer()
	{
		mach_port_deallocate(mach_task_self(), __clock_rt);
	}

#else
	time_t	__tv_sec_at_init = 0;

	void initTimer()
	{
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		__tv_sec_at_init = ts.tv_sec;
	}

#endif
