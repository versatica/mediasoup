#define MS_CLASS "Logger"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#ifndef _WIN32
#include <unistd.h> // getpid()
#endif

/* Class variables. */

const int64_t Logger::pid{ static_cast<int64_t>(getpid()) };
Channel::UnixStreamSocket* Logger::channel{ nullptr };
char Logger::buffer[Logger::bufferSize];

/* Class methods. */

void Logger::ClassInit(Channel::UnixStreamSocket* channel)
{
	Logger::channel = channel;

	MS_TRACE();
}
