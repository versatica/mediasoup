#define MS_CLASS "Logger"
// #define MS_LOG_DEV

#include "Logger.hpp"
#include <unistd.h> // getpid()

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
