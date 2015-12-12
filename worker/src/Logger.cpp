#define MS_CLASS "Logger"

#include "Logger.h"

/* Static variables. */

std::string Logger::id;

/* Static methods. */

void Logger::Init(const std::string id)
{
	Logger::id = id;

	MS_TRACE();
}
