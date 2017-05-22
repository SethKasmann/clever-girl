#include "Timer.h"
#include <map>

std::map<std::string, long> Timer::time_map_;

long Timer::get_current_time()
{
	timeval time;
	gettimeofday(&time, NULL);
	return (long) ((time.tv_sec * 1000) + (time.tv_usec / 1000));
}

void Timer::start_clock(const std::string & time_name)
{
	std::map<std::string, long>::const_iterator it = time_map_.find (time_name);
	if (it == time_map_.end())
	{
		time_map_[time_name] = get_current_time();
	}
	else return;
}

void Timer::reset_clock(const std::string & time_name)
{
	time_map_[time_name] = get_current_time();
}

float Timer::clock_time(const std::string & time_name)
{
	return (float) (get_current_time() - time_map_[time_name]);
}