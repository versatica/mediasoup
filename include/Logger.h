#ifndef MS_LOGGER_H
#define	MS_LOGGER_H

#include "common.h"
#include "Settings.h"
#include <string>
#include <cstdio>  // stdout, stderr, fflush()
#include <cstring>
#include <cstdlib>  // std::_Exit(), std::abort()
#include <uv.h>
extern "C"
{
	#include <syslog.h>
}

/*
 * Logger facility for MediaSoup.
 *
 *   If the macro MS_DEVEL is set (see below) the output is even more verbose
 *   (it shows the file number in each log and enables the MS_TRACE() call).
 *
 * Usage:
 *
 * 	  - MS_TRACE()     Shows current file/line/class/function.
 * 	  - MS_DEBUG(...)
 * 	  - MS_INFO(...)
 * 	  - MS_NOTICE(...)
 * 	  - MS_WARN(...)
 * 	  - MS_ERROR(...)
 * 	  - MS_CRIT(...)
 * 	  - MS_ABORT(...)
 *
 * 	  Arguments to those macros (except MS_TRACE) have the same format as printf.
 *
 *  Examples:
 *
 * 	  - MS_TRACE();
 * 	  - MS_DEBUG("starting worker %d", num);
 *
 *     Output:
 *       TRACE:  [worker #1] Worker.cpp:93 | void Worker::Run()
 *       DEBUG:  [worker #1] Worker.cpp:94 | void Worker::Run() | starting worker 1
 */


/*
 * Call the compiler with -DMS_DEVEL for more verbose logs.
 */
// #define MS_DEVEL

/*
 * This macro must point to a unsigned int function, method or variable that
 * returns Syslog logging level.
 */
#define MS_GET_LOG_LEVEL  Settings::configuration.logLevel

class Logger
{
public:
	static void ThreadInit(const std::string name);
	static const char* GetThreadName();
	static void EnableSyslog();
	static bool IsSyslogEnabled();
	static bool HasDebugLevel();

private:
	// NOTE:
	//  __thread implemented in GCC >= 4.7 and Clang.
	//  local_thread implemented in GCC >= 4.8.
	static __thread std::string* threadName;
	static __thread const char* threadNamePtr;
	static bool isSyslogEnabled;
};

/* Inline static methods. */

inline
const char* Logger::GetThreadName()
{
	return Logger::threadNamePtr;
}

inline
bool Logger::IsSyslogEnabled()
{
	return Logger::isSyslogEnabled;
}

inline
bool Logger::HasDebugLevel()
{
	return (LOG_DEBUG == MS_GET_LOG_LEVEL);
}

// NOTE: Each file including Logger.h MUST define its own MS_CLASS macro.

#ifdef MS_DEVEL
	#define _MS_LOG_STR  "[%s] %s:%d | %s::%s()"
	#define _MS_LOG_STR_DESC  _MS_LOG_STR " | "
	#define _MS_FILE  (std::strchr(__FILE__, '/') ? std::strchr(__FILE__, '/') + 1 : __FILE__)
	#define _MS_LOG_ARG  Logger::GetThreadName(), _MS_FILE, __LINE__, MS_CLASS, __FUNCTION__
#else
	#define _MS_LOG_STR  "[%s] %s::%s()"
	#define _MS_LOG_STR_DESC  _MS_LOG_STR " | "
	#define _MS_LOG_ARG  Logger::GetThreadName(), MS_CLASS, __FUNCTION__
#endif

#define _MS_TO_STDOUT(prefix)  \
	if (!Logger::IsSyslogEnabled())  \
	{  \
		std::fprintf(stdout, prefix _MS_LOG_STR "\n", _MS_LOG_ARG);  \
		fflush(stdout);  \
	}

#define _MS_TO_STDOUT_DESC(prefix, desc, ...)  \
	if (!Logger::IsSyslogEnabled())  \
	{  \
		std::fprintf(stdout, prefix _MS_LOG_STR_DESC desc "\n", _MS_LOG_ARG, ##__VA_ARGS__);  \
		fflush(stdout);  \
	}

#define _MS_TO_STDERR_DESC(prefix, desc, ...)  \
	if (!Logger::IsSyslogEnabled())  \
	{  \
		std::fprintf(stderr, prefix _MS_LOG_STR_DESC desc "\n", _MS_LOG_ARG, ##__VA_ARGS__);  \
		fflush(stderr);  \
	}

#define _MS_TO_SYSLOG(severity, prefix)  \
	if (Logger::IsSyslogEnabled())  \
		syslog(severity, prefix _MS_LOG_STR, _MS_LOG_ARG);

#define _MS_TO_SYSLOG_DESC(severity, prefix, desc, ...)  \
	if (Logger::IsSyslogEnabled())  \
		syslog(severity, prefix _MS_LOG_STR_DESC desc, _MS_LOG_ARG, ##__VA_ARGS__);

/*
 * Log levels in syslog.h:
 *
 * LOG_EMERG       0       system is unusable
 * LOG_ALERT       1       action must be taken immediately
 * LOG_CRIT        2       critical conditions
 * LOG_ERR         3       error conditions
 * LOG_WARNING     4       warning conditions
 * LOG_NOTICE      5       normal but significant condition
 * LOG_INFO        6       informational
 * LOG_DEBUG       7       debug-level messages
 */


#ifdef MS_DEVEL
#define MS_TRACE()  \
	do  \
	{  \
		_MS_TO_STDOUT("TRACE:  ");  \
		_MS_TO_SYSLOG(LOG_DEBUG, "TRACE:  ");  \
	}  \
	while (0)
#else
#define MS_TRACE()
#endif

#define MS_DEBUG(desc, ...)  \
	do  \
	{  \
		if (LOG_DEBUG == MS_GET_LOG_LEVEL)  \
		{  \
			_MS_TO_STDOUT_DESC("DEBUG:  ", desc, ##__VA_ARGS__);  \
			_MS_TO_SYSLOG_DESC(LOG_DEBUG, "DEBUG:  ", desc, ##__VA_ARGS__);  \
		}  \
	}  \
	while (0)

#define MS_INFO(desc, ...)  \
	do  \
	{  \
		if (LOG_INFO <= MS_GET_LOG_LEVEL)  \
		{  \
			_MS_TO_STDOUT_DESC("INFO:   ", desc, ##__VA_ARGS__);  \
			_MS_TO_SYSLOG_DESC(LOG_INFO, "INFO:   ", desc, ##__VA_ARGS__);  \
		}  \
	}  \
	while (0)

#define MS_NOTICE(desc, ...)  \
	do  \
	{  \
		if (LOG_NOTICE <= MS_GET_LOG_LEVEL)  \
		{  \
			_MS_TO_STDOUT_DESC("NOTICE: ", desc, ##__VA_ARGS__);  \
			_MS_TO_SYSLOG_DESC(LOG_NOTICE, "NOTICE: ", desc, ##__VA_ARGS__);  \
		}  \
	}  \
	while (0)

#define MS_WARN(desc, ...)  \
	do  \
	{  \
		if (LOG_WARNING <= MS_GET_LOG_LEVEL)  \
		{  \
			_MS_TO_STDERR_DESC("WARN:   ", desc, ##__VA_ARGS__);  \
			_MS_TO_SYSLOG_DESC(LOG_WARNING, "WARN:   ", desc, ##__VA_ARGS__);  \
		}  \
	}  \
	while (0)

#define MS_ERROR(desc, ...)  \
	do  \
	{  \
		_MS_TO_STDERR_DESC("ERROR:  ", desc, ##__VA_ARGS__);  \
		_MS_TO_SYSLOG_DESC(LOG_ERR, "ERROR:  ", desc, ##__VA_ARGS__);  \
	}  \
	while (0)

#define MS_CRIT(desc, ...)  \
	do  \
	{  \
		_MS_TO_STDERR_DESC("CRIT:   ", desc, ##__VA_ARGS__);  \
		_MS_TO_SYSLOG_DESC(LOG_ERR, "CRIT:   ", desc, ##__VA_ARGS__);  \
	}  \
	while (0)

#define MS_EXIT_SUCCESS(desc, ...)  \
	do  \
	{  \
		MS_NOTICE("SUCCESS EXIT | " desc, ##__VA_ARGS__);  \
		std::_Exit(EXIT_SUCCESS);  \
	}  \
	while (0)

#define MS_EXIT_FAILURE(desc, ...)  \
	do  \
	{  \
		MS_CRIT("FAILURE EXIT | " desc, ##__VA_ARGS__);  \
		std::_Exit(EXIT_FAILURE);  \
	}  \
	while (0)

#define MS_ABORT(desc, ...)  \
	do  \
	{  \
		MS_CRIT("ABORT | " desc, ##__VA_ARGS__);  \
		std::abort();  \
	}  \
	while (0)

#define MS_ASSERT(condition, desc, ...)  \
	if (!(condition))  \
	{  \
		MS_ABORT("failed assertion `%s': " desc, #condition, ##__VA_ARGS__);  \
	}

#endif
