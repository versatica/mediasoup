#ifndef MS_LOGGER_H
#define	MS_LOGGER_H

#include "common.h"
#include "Settings.h"
#include "handles/UnixStreamSocket.h"
#include "Channel/UnixStreamSocket.h"
#include <string>
#include <cstdio>  // std::snprintf(), std::fprintf(), stdout, stderr
#include <cstring>
#include <cstdlib>  // std::abort()

#define MS_LOGGER_BUFFER_SIZE 10000

/*
 * Logger facility.
 *
 *   If the macro MS_DEVEL is set (see below) the output is even more verbose
 *   (it shows the file number in each log and enables the MS_TRACE() call).
 *
 * Usage:
 *
 * 	  - MS_TRACE()     Shows current file/line/class/function.
 * 	  - MS_DEBUG(...)
 * 	  - MS_WARN(...)
 * 	  - MS_ERROR(...)
 * 	  - MS_ABORT(...)
 *
 * 	  Arguments to those macros (except MS_TRACE) have the same format as printf.
 *
 *  Examples:
 *
 * 	  - MS_TRACE();
 * 	  - MS_DEBUG("starting worker %d", num);
 */

/*
 * Call the compiler with -DMS_DEVEL for more verbose logs.
 */
// #define MS_DEVEL

/*
 * This macro must point to a unsigned int function, method or variable that
 * returns the logging level.
 */
#define MS_CURRENT_LOG_LEVEL Settings::configuration.logLevel

class Logger
{
public:
	static void Init(const std::string id, Channel::UnixStreamSocket* channel);
	static bool HasDebugLevel();

public:
	static std::string id;
	static Channel::UnixStreamSocket* channel;
	static char buffer[];
};

/* Inline static methods. */

inline
bool Logger::HasDebugLevel()
{
	return (MS_LOG_LEVEL_DEBUG == MS_CURRENT_LOG_LEVEL);
}

// NOTE: Each file including Logger.h MUST define its own MS_CLASS macro.

#define _MS_LOG_SEPARATOR_CHAR_STD "\f"

#ifdef MS_DEVEL
	#define _MS_LOG_STR "[%s] %s:%d | %s::%s()"
	#define _MS_LOG_STR_DESC _MS_LOG_STR " | "
	#define _MS_FILE (std::strchr(__FILE__, '/') ? std::strchr(__FILE__, '/') + 1 : __FILE__)
	#define _MS_LOG_ARG ("id:" + Logger::id).c_str(), _MS_FILE, __LINE__, MS_CLASS, __FUNCTION__
#else
	#define _MS_LOG_STR "[%s] %s::%s()"
	#define _MS_LOG_STR_DESC _MS_LOG_STR " | "
	#define _MS_LOG_ARG ("id:" + Logger::id).c_str(), MS_CLASS, __FUNCTION__
#endif

#ifdef MS_DEVEL
#define MS_TRACE()  \
	do  \
	{  \
		int ms_logger_written = std::snprintf(Logger::buffer, MS_LOGGER_BUFFER_SIZE, "D(trace) " _MS_LOG_STR, _MS_LOG_ARG);  \
		Logger::channel->SendLog(Logger::buffer, ms_logger_written);  \
	}  \
	while (0)
#define MS_TRACE_STD()  \
	do  \
	{  \
		std::fprintf(stdout, "(trace) " _MS_LOG_STR _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG);  \
		std::fflush(stdout);  \
	}  \
	while (0)
#else
#define MS_TRACE()
#define MS_TRACE_STD()
#endif

#define MS_DEBUG(desc, ...)  \
	do  \
	{  \
		if (MS_LOG_LEVEL_DEBUG == MS_CURRENT_LOG_LEVEL)  \
		{  \
			int ms_logger_written = std::snprintf(Logger::buffer, MS_LOGGER_BUFFER_SIZE, "D" _MS_LOG_STR_DESC desc, _MS_LOG_ARG, ##__VA_ARGS__);  \
			Logger::channel->SendLog(Logger::buffer, ms_logger_written);  \
		}  \
	}  \
	while (0)

#define MS_DEBUG_STD(desc, ...)  \
	do  \
	{  \
		if (MS_LOG_LEVEL_DEBUG == MS_CURRENT_LOG_LEVEL)  \
		{  \
			std::fprintf(stdout, _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__);  \
			std::fflush(stdout);  \
		}  \
	}  \
	while (0)

#define MS_WARN(desc, ...)  \
	do  \
	{  \
		if (MS_LOG_LEVEL_WARN <= MS_CURRENT_LOG_LEVEL)  \
		{  \
			int ms_logger_written = std::snprintf(Logger::buffer, MS_LOGGER_BUFFER_SIZE, "W" _MS_LOG_STR_DESC desc, _MS_LOG_ARG, ##__VA_ARGS__);  \
			Logger::channel->SendLog(Logger::buffer, ms_logger_written);  \
		}  \
	}  \
	while (0)

#define MS_ERROR(desc, ...)  \
	do  \
	{  \
		int ms_logger_written = std::snprintf(Logger::buffer, MS_LOGGER_BUFFER_SIZE, "E" _MS_LOG_STR_DESC desc, _MS_LOG_ARG, ##__VA_ARGS__);  \
		Logger::channel->SendLog(Logger::buffer, ms_logger_written);  \
	}  \
	while (0)

#define MS_ERROR_STD(desc, ...)  \
	do  \
	{  \
		std::fprintf(stderr, _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__);  \
		std::fflush(stderr);  \
	}  \
	while (0)

#define MS_ABORT(desc, ...)  \
	do  \
	{  \
		std::fprintf(stderr, "ABORT | " desc, ##__VA_ARGS__);  \
		std::fflush(stderr);  \
		std::abort();  \
	}  \
	while (0)

#define MS_ASSERT(condition, desc, ...)  \
	if (!(condition))  \
	{  \
		MS_ABORT("failed assertion `%s': " desc, #condition, ##__VA_ARGS__);  \
	}

#endif
