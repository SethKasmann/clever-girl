#ifndef TIMER_H
#define TIMER_H

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include <windows.h>
inline int64_t system_time()
{
	SYSTEMTIME time;
	GetSystemTime(&time);
	return (time.wSecond * 1000LL) + time.wMilliseconds;
}
#else
#include <sys/time.h>
inline int64_t system_time()
{
	timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec * 1000LL + t.tv_usec / 1000LL;
}
#endif

// Robert Hyatt's formula.
inline int allocate_time(int time_left, int moves, int moves_to_go)
{
	int num_moves = std::min(moves, 10);
	float factor = 2 - num_moves / 10.0;
	int target = time_left / (moves_to_go ? moves_to_go : 30);
	return factor * target;
}

#endif