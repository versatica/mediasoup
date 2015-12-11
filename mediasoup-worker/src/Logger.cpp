#define MS_CLASS "Logger"

#include "Logger.h"

/* Static variables. */

std::string Logger::processName;
std::string Logger::processMinName;
bool Logger::isSyslogEnabled = false;

/* Static methods. */

void Logger::Init(const std::string id)
{
	Logger::processName = MS_PROCESS_NAME "@" + id;
	Logger::processMinName = MS_PROCESS_MIN_NAME "@" + id;

	MS_TRACE();
}

void Logger::EnableSyslog()
{
	MS_TRACE();

	MS_DEBUG("logging to Syslog");

	openlog(MS_PROCESS_NAME, (LOG_PID), Settings::configuration.syslogFacility);

	Logger::isSyslogEnabled = true;
}
