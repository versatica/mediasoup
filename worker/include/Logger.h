/**
 * Logger facility.
 *
 * This include file defines logging macros for source files (.cpp). Each
 * source file including Logger.h MUST define its own MS_CLASS macro. Include
 * files (.h) MUST NOT include Logger.h.
 *
 * All the logging macros use the same format as printf(). The XXX_STD version
 * of a macro logs to stdoud/stderr instead of using the Channel instance.
 *
 * If the macro MS_LOG_FILE_LINE is defied, all the logging macros print more
 * verbose information, including current file and line.
 *
 * MS_TRACE()
 *
 *   Logs the current method/function if the current source file defines the
 *   MS_LOG_TRACE macro and the current debug level is "debug".
 *
 * MS_DEBUG_TAG(tag, ...)
 * MS_WARN_TAG(tag, ...)
 *
 *   Logs if the current debug level is satisfied and the given tag is enabled
 *   (or if the current source file defines the MS_LOG_DEV macro).
 *   Example:
 *     MS_WARN_TAG(ice, "ICE failed");
 *
 * MS_DEBUG_2TAGS(tag1, tag2, ...)
 * MS_WARN_2TAGS(tag1, tag2, ...)
 *
 *   Logs if the current debug level is satisfied and any of the given two tags
 *   is enabled (or if the current source file defines the MS_LOG_DEV macro).
 *   Example:
 *     MS_DEBUG_2TAGS(ice, dtls, "media connection established");
 *
 * MS_DEBUG_DEV(...)
 * MS_WARN_DEV(...)
 *
 * 	 Logs if the current source file defines the MS_LOG_DEV macro.
 * 	 Example:
 * 	   MS_DEBUG_DEV("Room closed [roomId:%" PRIu32 "]", roomId);
 *
 * MS_ERROR(...)
 *
 *   Logs an error. Must just be used for internal errors that should not
 *   happen.
 *
 * MS_ABORT(...)
 *
 *   Logs the given error to stderr and aborts the process.
 *
 * MS_ASSERT(condition, ...)
 *
 *   If the condition is not satisfied, it calls MS_ABORT().
 */

#ifndef MS_LOGGER_H
#define	MS_LOGGER_H

#include "LogLevel.h"
#include "Settings.h"
#include "handles/UnixStreamSocket.h"
#include "Channel/UnixStreamSocket.h"
#include <string>
#include <cstdio> // std::snprintf(), std::fprintf(), stdout, stderr
#include <cstring>
#include <cstdlib> // std::abort()

#define MS_LOGGER_BUFFER_SIZE 10000
#define _MS_TAG_ENABLED(tag) Settings::logTags.tag
#define _MS_TAG_ENABLED_2(tag1, tag2) (Settings::logTags.tag1 || Settings::logTags.tag2)
#ifdef MS_LOG_DEV
	#define _MS_LOG_DEV_ENABLED true
#else
	#define _MS_LOG_DEV_ENABLED false
#endif

class Logger
{
public:
	static void Init(const std::string& id, Channel::UnixStreamSocket* channel);

public:
	static std::string id;
	static Channel::UnixStreamSocket* channel;
	static char buffer[];
};

/* Logging macros. */

#define _MS_LOG_SEPARATOR_CHAR_STD "\n"

#ifdef MS_LOG_FILE_LINE
	#define _MS_LOG_STR "[%s] %s:%d | %s::%s()"
	#define _MS_LOG_STR_DESC _MS_LOG_STR " | "
	#define _MS_FILE (std::strchr(__FILE__, '/') ? std::strchr(__FILE__, '/') + 1 : __FILE__)
	#define _MS_LOG_ARG ("id:" + Logger::id).c_str(), _MS_FILE, __LINE__, MS_CLASS, __FUNCTION__
#else
	#define _MS_LOG_STR "[%s] %s::%s()"
	#define _MS_LOG_STR_DESC _MS_LOG_STR " | "
	#define _MS_LOG_ARG ("id:" + Logger::id).c_str(), MS_CLASS, __FUNCTION__
#endif

#ifdef MS_LOG_TRACE
	#define MS_TRACE() \
		do \
		{ \
			if (LogLevel::LOG_DEBUG == Settings::configuration.logLevel) \
			{ \
				int ms_logger_written = std::snprintf(Logger::buffer, MS_LOGGER_BUFFER_SIZE, "D(trace) " _MS_LOG_STR, _MS_LOG_ARG); \
				Logger::channel->SendLog(Logger::buffer, ms_logger_written); \
			} \
		} \
		while (0)

	#define MS_TRACE_STD() \
		do \
		{ \
			if (LogLevel::LOG_DEBUG == Settings::configuration.logLevel) \
			{ \
				std::fprintf(stdout, "(trace) " _MS_LOG_STR _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG); \
				std::fflush(stdout); \
			} \
		} \
		while (0)
#else
	#define MS_TRACE()
	#define MS_TRACE_STD()
#endif

#define MS_DEBUG_TAG(tag, desc, ...) \
	do \
	{ \
		if (LogLevel::LOG_DEBUG == Settings::configuration.logLevel && (_MS_TAG_ENABLED(tag) || _MS_LOG_DEV_ENABLED)) \
		{ \
			int ms_logger_written = std::snprintf(Logger::buffer, MS_LOGGER_BUFFER_SIZE, "D" _MS_LOG_STR_DESC desc, _MS_LOG_ARG, ##__VA_ARGS__); \
			Logger::channel->SendLog(Logger::buffer, ms_logger_written); \
		} \
	} \
	while (0)

#define MS_DEBUG_TAG_STD(tag, desc, ...) \
	do \
	{ \
		if (LogLevel::LOG_DEBUG == Settings::configuration.logLevel && (_MS_TAG_ENABLED(tag) || _MS_LOG_DEV_ENABLED)) \
		{ \
			std::fprintf(stdout, _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__); \
			std::fflush(stdout); \
		} \
	} \
	while (0)

#define MS_WARN_TAG(tag, desc, ...) \
	do \
	{ \
		if (LogLevel::LOG_WARN <= Settings::configuration.logLevel && (_MS_TAG_ENABLED(tag) || _MS_LOG_DEV_ENABLED)) \
		{ \
			int ms_logger_written = std::snprintf(Logger::buffer, MS_LOGGER_BUFFER_SIZE, "W" _MS_LOG_STR_DESC desc, _MS_LOG_ARG, ##__VA_ARGS__); \
			Logger::channel->SendLog(Logger::buffer, ms_logger_written); \
		} \
	} \
	while (0)

#define MS_WARN_TAG_STD(tag, desc, ...) \
	do \
	{ \
		if (LogLevel::LOG_WARN <= Settings::configuration.logLevel && (_MS_TAG_ENABLED(tag) || _MS_LOG_DEV_ENABLED)) \
		{ \
			std::fprintf(stderr, _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__); \
			std::fflush(stderr); \
		} \
	} \
	while (0)

#define MS_DEBUG_2TAGS(tag1, tag2, desc, ...) \
	do \
	{ \
		if (LogLevel::LOG_DEBUG == Settings::configuration.logLevel && (_MS_TAG_ENABLED_2(tag1, tag2) || _MS_LOG_DEV_ENABLED)) \
		{ \
			int ms_logger_written = std::snprintf(Logger::buffer, MS_LOGGER_BUFFER_SIZE, "D" _MS_LOG_STR_DESC desc, _MS_LOG_ARG, ##__VA_ARGS__); \
			Logger::channel->SendLog(Logger::buffer, ms_logger_written); \
		} \
	} \
	while (0)

#define MS_DEBUG_2TAGS_STD(tag, desc, ...) \
	do \
	{ \
		if (LogLevel::LOG_DEBUG == Settings::configuration.logLevel && (_MS_TAG_ENABLED_2(tag1, tag2) || _MS_LOG_DEV_ENABLED)) \
		{ \
			std::fprintf(stdout, _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__); \
			std::fflush(stdout); \
		} \
	} \
	while (0)

#define MS_WARN_2TAGS(tag1, tag2, desc, ...) \
	do \
	{ \
		if (LogLevel::LOG_WARN <= Settings::configuration.logLevel && (_MS_TAG_ENABLED_2(tag1, tag2) || _MS_LOG_DEV_ENABLED)) \
		{ \
			int ms_logger_written = std::snprintf(Logger::buffer, MS_LOGGER_BUFFER_SIZE, "W" _MS_LOG_STR_DESC desc, _MS_LOG_ARG, ##__VA_ARGS__); \
			Logger::channel->SendLog(Logger::buffer, ms_logger_written); \
		} \
	} \
	while (0)

#define MS_WARN_2TAGS_STD(tag1, tag2, desc, ...) \
	do \
	{ \
		if (LogLevel::LOG_WARN <= Settings::configuration.logLevel && (_MS_TAG_ENABLED_2(tag1, tag2) || _MS_LOG_DEV_ENABLED)) \
		{ \
			std::fprintf(stderr, _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__); \
			std::fflush(stderr); \
		} \
	} \
	while (0)

#ifdef MS_LOG_DEV
	#define MS_DEBUG_DEV(desc, ...) \
		do \
		{ \
			if (LogLevel::LOG_DEBUG == Settings::configuration.logLevel) \
			{ \
				int ms_logger_written = std::snprintf(Logger::buffer, MS_LOGGER_BUFFER_SIZE, "D" _MS_LOG_STR_DESC desc, _MS_LOG_ARG, ##__VA_ARGS__); \
				Logger::channel->SendLog(Logger::buffer, ms_logger_written); \
			} \
		} \
		while (0)

	#define MS_DEBUG_DEV_STD(desc, ...) \
		do \
		{ \
			if (LogLevel::LOG_DEBUG == Settings::configuration.logLevel) \
			{ \
				std::fprintf(stdout, _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__); \
				std::fflush(stdout); \
			} \
		} \
		while (0)

	#define MS_WARN_DEV(desc, ...) \
		do \
		{ \
			if (LogLevel::LOG_WARN <= Settings::configuration.logLevel) \
			{ \
				int ms_logger_written = std::snprintf(Logger::buffer, MS_LOGGER_BUFFER_SIZE, "W" _MS_LOG_STR_DESC desc, _MS_LOG_ARG, ##__VA_ARGS__); \
				Logger::channel->SendLog(Logger::buffer, ms_logger_written); \
			} \
		} \
		while (0)

	#define MS_WARN_DEV_STD(desc, ...) \
		do \
		{ \
			if (LogLevel::LOG_WARN <= Settings::configuration.logLevel) \
			{ \
				std::fprintf(stderr, _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__); \
				std::fflush(stderr); \
			} \
		} \
		while (0)
#else
	#define MS_DEBUG_DEV(desc, ...)
	#define MS_DEBUG_DEV_STD(desc, ...)
	#define MS_WARN_DEV(desc, ...)
	#define MS_WARN_DEV_STD(desc, ...)
#endif

#define MS_ERROR(desc, ...) \
	do \
	{ \
		int ms_logger_written = std::snprintf(Logger::buffer, MS_LOGGER_BUFFER_SIZE, "E" _MS_LOG_STR_DESC desc, _MS_LOG_ARG, ##__VA_ARGS__); \
		Logger::channel->SendLog(Logger::buffer, ms_logger_written); \
	} \
	while (0)

#define MS_ERROR_STD(desc, ...) \
	do \
	{ \
		std::fprintf(stderr, _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__); \
		std::fflush(stderr); \
	} \
	while (0)

#define MS_ABORT(desc, ...) \
	do \
	{ \
		std::fprintf(stderr, "ABORT" _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__); \
		std::fflush(stderr); \
		std::abort(); \
	} \
	while (0)

#define MS_ASSERT(condition, desc, ...) \
	if (!(condition)) \
	{ \
		MS_ABORT("failed assertion `%s': " desc, #condition, ##__VA_ARGS__); \
	}

#ifdef MS_LOG_STD
	#undef MS_TRACE
	#define MS_TRACE MS_TRACE_STD
	#undef MS_DEBUG_TAG
	#define MS_DEBUG_TAG MS_DEBUG_TAG_STD
	#undef MS_WARN_TAG
	#define MS_WARN_TAG MS_WARN_TAG_STD
	#undef MS_DEBUG_2TAGS
	#define MS_DEBUG_2TAGS MS_DEBUG_2TAGS_STD
	#undef MS_WARN_2TAGS
	#define MS_WARN_2TAGS MS_WARN_2TAGS_STD
	#undef MS_DEBUG_DEV
	#define MS_DEBUG_DEV MS_DEBUG_DEV_STD
	#undef MS_WARN_DEV
	#define MS_WARN_DEV MS_WARN_DEV_STD
	#undef MS_ERROR
	#define MS_ERROR MS_ERROR_STD
#endif

#endif
