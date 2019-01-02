#define MS_CLASS "Logger"
// #define MS_LOG_DEV

#include "Logger.hpp"
#include <unistd.h> // getpid()

/* Class variables. */

const long Logger::id{ static_cast<long>(getpid()) };
Channel::UnixStreamSocket* Logger::channel{ nullptr };
char Logger::buffer[Logger::bufferSize];

/* Class methods. */

void Logger::ClassInit(Channel::UnixStreamSocket* channel)
{
	Logger::channel = channel;

	MS_TRACE();
}
