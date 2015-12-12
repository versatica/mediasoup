#ifndef MS_LOGGER_H
#define	MS_LOGGER_H

#include "common.h"
#include "Settings.h"
#include <string>
#include <cstdio>  // stdout, stderr, fflush()
#include <cstring>
#include <cstdlib>  // std::_Exit(), std::abort()
#include <uv.h>

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
 * 	  - MS_INFO(...)
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
#define MS_GET_LOG_LEVEL Settings::configuration.logLevel

class Logger
{
public:
	static void Init(const std::string id);
	static const char* GetProcessName();
	static const char* GetProcessMinName();
	static bool HasDebugLevel();

private:
	static std::string processName;
	static std::string processMinName;
};

/* Inline static methods. */

inline
const char* Logger::GetProcessName()
{
	return Logger::processName.c_str();
}

inline
const char* Logger::GetProcessMinName()
{
	return Logger::processMinName.c_str();
}

inline
bool Logger::HasDebugLevel()
{
	return (MS_LOG_LEVEL_DEBUG == MS_GET_LOG_LEVEL);
}

// NOTE: Each file including Logger.h MUST define its own MS_CLASS macro.

#ifdef MS_DEVEL
	#define _MS_LOG_STR "[%s] %s:%d | %s::%s()"
	#define _MS_LOG_STR_DESC _MS_LOG_STR " | "
	#define _MS_FILE (std::strchr(__FILE__, '/') ? std::strchr(__FILE__, '/') + 1 : __FILE__)
	#define _MS_LOG_ARG Logger::GetProcessName(), _MS_FILE, __LINE__, MS_CLASS, __FUNCTION__
#else
	#define _MS_LOG_STR "[%s] %s::%s()"
	#define _MS_LOG_STR_DESC _MS_LOG_STR " | "
	#define _MS_LOG_ARG Logger::GetProcessMinName(), MS_CLASS, __FUNCTION__
#endif

#define _MS_TO_STDOUT()  \
	std::fprintf(stdout, _MS_LOG_STR "\n", _MS_LOG_ARG);  \
	fflush(stdout);

#define _MS_TO_STDOUT_DESC(desc, ...)  \
	std::fprintf(stdout, _MS_LOG_STR_DESC desc "\n", _MS_LOG_ARG, ##__VA_ARGS__);  \
	fflush(stdout);  \

#define _MS_TO_STDERR_DESC(desc, ...)  \
	std::fprintf(stderr, _MS_LOG_STR_DESC desc "\n", _MS_LOG_ARG, ##__VA_ARGS__);  \
	fflush(stderr);  \

#ifdef MS_DEVEL
#define MS_TRACE()  \
	do  \
	{  \
		_MS_TO_STDOUT();  \
	}  \
	while (0)
#else
#define MS_TRACE()
#endif

#define MS_DEBUG(desc, ...)  \
	do  \
	{  \
		if (MS_LOG_LEVEL_DEBUG == MS_GET_LOG_LEVEL)  \
		{  \
			_MS_TO_STDOUT_DESC(desc, ##__VA_ARGS__);  \
		}  \
	}  \
	while (0)

#define MS_INFO(desc, ...)  \
	do  \
	{  \
		if (MS_LOG_LEVEL_INFO <= MS_GET_LOG_LEVEL)  \
		{  \
			_MS_TO_STDOUT_DESC(desc, ##__VA_ARGS__);  \
		}  \
	}  \
	while (0)

#define MS_WARN(desc, ...)  \
	do  \
	{  \
		if (MS_LOG_LEVEL_WARN <= MS_GET_LOG_LEVEL)  \
		{  \
			_MS_TO_STDERR_DESC(desc, ##__VA_ARGS__);  \
		}  \
	}  \
	while (0)

#define MS_ERROR(desc, ...)  \
	do  \
	{  \
		_MS_TO_STDERR_DESC(desc, ##__VA_ARGS__);  \
	}  \
	while (0)

#define MS_EXIT_SUCCESS(desc, ...)  \
	do  \
	{  \
		MS_INFO("SUCCESS EXIT | " desc, ##__VA_ARGS__);  \
		std::_Exit(EXIT_SUCCESS);  \
	}  \
	while (0)

#define MS_EXIT_FAILURE(desc, ...)  \
	do  \
	{  \
		MS_ERROR("FAILURE EXIT | " desc, ##__VA_ARGS__);  \
		std::_Exit(EXIT_FAILURE);  \
	}  \
	while (0)

#define MS_ABORT(desc, ...)  \
	do  \
	{  \
		MS_ERROR("ABORT | " desc, ##__VA_ARGS__);  \
		std::abort();  \
	}  \
	while (0)

#define MS_ASSERT(condition, desc, ...)  \
	if (!(condition))  \
	{  \
		MS_ABORT("failed assertion `%s': " desc, #condition, ##__VA_ARGS__);  \
	}

#endif
