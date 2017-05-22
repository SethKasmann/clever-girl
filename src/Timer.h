#ifndef TIMER_H
#define TIMER_H

// Simple timer class - not chess related.

#include <iostream>
#include <map>
#include <string>
#include "sys/time.h"

class Timer
{
public:
	static void start_clock(const std::string &);
	static void reset_clock(const std::string &);
	static float clock_time(const std::string &);
private:
	Timer(){}
	static long get_current_time();
	static std::map<std::string, long> time_map_;
};

#endif