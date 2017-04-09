#define MS_CLASS "Logger"
// #define MS_LOG_DEV

#include "Logger.hpp"

/* Class variables. */

std::string Logger::id                     = "unset";
Channel::UnixStreamSocket* Logger::channel = nullptr;
char Logger::buffer[Logger::bufferSize];

/* Class methods. */

void Logger::Init(const std::string& id, Channel::UnixStreamSocket* channel)
{
	Logger::id      = id;
	Logger::channel = channel;

	MS_TRACE();
}

void Logger::Init(const std::string& id)
{
	Logger::id = id;

	MS_TRACE();
}
