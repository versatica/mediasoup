#define MS_CLASS "Logger"

#include "Logger.h"

/* Class variables. */

std::string Logger::id;
int Logger::fd;

/* Class methods. */

void Logger::Init(const std::string id)
{
	Logger::id = id;
	Logger::fd = std::stoi(std::getenv("MEDIASOUP_LOGGER_FD"));

	MS_TRACE();
}
