#define MS_CLASS "Logger"

#include "Logger.h"

/* Static variables. */

std::string Logger::processName;
bool Logger::isSyslogEnabled = false;

/* Static methods. */

void Logger::Init(const std::string name)
{
	Logger::processName = name;

	MS_TRACE();
}

void Logger::EnableSyslog()
{
	MS_TRACE();

	MS_DEBUG("logging to Syslog");

	openlog(MS_PROCESS_NAME, (LOG_PID), Settings::configuration.syslogFacility);

	Logger::isSyslogEnabled = true;
}
