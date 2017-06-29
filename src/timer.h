#ifndef TIMER_H
#define TIMER_H

const int min_time = 500; // Absolute minimum time to spend searching.

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
	int num_moves, target, time;
	float factor;

	num_moves = std::min(moves, 10);
	factor = 2 - num_moves / 10.0;
	target = time_left / (moves_to_go ? moves_to_go : 30);
	time = factor * target;
	return time > min_time ? time : min_time;
}

#endif