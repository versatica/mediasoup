#define MS_CLASS "Logger"
// #define MS_LOG_DEV

#include "Logger.h"

/* Class variables. */

std::string Logger::id = "unset";
Channel::UnixStreamSocket* Logger::channel = nullptr;
char Logger::buffer[MS_LOGGER_BUFFER_SIZE];

/* Class methods. */

void Logger::Init(const std::string& id, Channel::UnixStreamSocket* channel)
{
	Logger::id = id;
	Logger::channel = channel;

	MS_TRACE();
}
