#define MS_CLASS "Logger"

#include "Logger.h"
#include "Version.h"


/* Static variables. */

__thread std::string* Logger::threadName = nullptr;
__thread const char* Logger::threadNamePtr = nullptr;
bool Logger::isSyslogEnabled = false;


/* Static methods. */

void Logger::ThreadInit(const std::string name) {
	// Store the name for this thread in thread local storage.
	if (Logger::threadName)
		delete Logger::threadName;

	// NOTE: this std::string pointer is never deallocated.
	Logger::threadName = new std::string(name);
	Logger::threadNamePtr = Logger::threadName->c_str();

	MS_TRACE();
}


void Logger::EnableSyslog() {
	MS_TRACE();

	openlog(Version::command.c_str(), (LOG_PID), Settings::configuration.syslogFacility);

	Logger::isSyslogEnabled = true;
}
