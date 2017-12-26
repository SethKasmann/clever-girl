#ifndef TIMER_H
#define TIMER_H

#include <chrono>
//#include <sys/time.h>

const int min_time = 500; // Absolute minimum time to spend searching.

class Clock
{
public:
	void set()
	{
		mTime = std::chrono::high_resolution_clock::now();
	}
	template<typename T>
	int64_t elapsed()
	{
		std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
		return std::chrono::duration_cast<T>(now - mTime).count();
	}
private:
	std::chrono::high_resolution_clock::time_point mTime;
};

// Robert Hyatt's formula.
inline int64_t allocate_time(int time_left, int moves, int moves_to_go)
{
	int64_t num_moves, target, time;
	float factor;

	num_moves = std::min(moves, 10);
	factor = 2 - num_moves / 10.0;
	target = time_left / (moves_to_go ? moves_to_go : 30);
	time = factor * target;
	return time > min_time ? time : min_time;
}

#endif