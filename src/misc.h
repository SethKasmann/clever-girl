#ifndef MISC_H
#define MICSC_H

#include <string>
#include <sstream>

// Credit to Oliver Brausch
inline int input_waiting()
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
	#include <windows.h>
	static int init = 0, pipe;
	static HANDLE inh;
	DWORD dw;

	if (!init)
	{
		init = 1;
		inh = GetStdHandle(STD_INPUT_HANDLE);
		pipe = !GetConsoleMode(inh, &dw);
		if (!pipe)
		{
			SetConsoleMode(inh, dw & ~(ENABLE_MOUSE_INPUT|ENABLE_WINDOW_INPUT));
			FlushConsoleInputBuffer(inh);
		}
	}
	if (pipe)
	{
		if (!PeekNamedPipe(inh, NULL, 0, NULL, &dw, NULL))
			return 1;
		return dw;
	}
	else
	{
		GetNumberOfConsoleInputEvents(inh, &dw);
		return dw <= 1 ? 0 : dw;
	}
#else
	#include <sys/time.h>
	fd_set readfds;
	timeval tv;
	FD_ZERO(&readfds);
	FD_SET(fileno(stdin), &readfds);
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	select(16, &readfds, 0, 0, &tv);
	return (FD_ISSET(fileno(stdin), &readfds));
#endif
}

inline std::string get_input()
{
	std::string command;
	std::getline(std::cin, command);
	return command;
}

#endif