/**
 * Logger facility.
 *
 * This include file defines logging macros for source files (.cpp). Each
 * source file including Logger.hpp MUST define its own MS_CLASS macro. Include
 * files (.hpp) MUST NOT include Logger.hpp.
 *
 * All the logging macros use the same format as printf(). The XXX_STD() version
 * of a macro logs to stdoud/stderr instead of using the Channel instance.
 * However some macros such as MS_ABORT() and MS_ASSERT() always log to stderr.
 *
 * If the macro MS_LOG_STD is defined, all the macros log to stdout/stderr.
 *
 * If the macro MS_LOG_FILE_LINE is defied, all the logging macros print more
 * verbose information, including current file and line.
 *
 * MS_TRACE()
 *
 *   Logs the current method/function if MS_LOG_TRACE macro is defined and the
 *   current debug level is "debug".
 *
 * MS_HAS_DEBUG_TAG(tag)
 * MS_HAS_WARN_TAG(tag)
 *
 *   True if the current debug level is satisfied and the given tag is enabled
 *   (or if the current source file defines the MS_LOG_DEV macro).
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
 * 	 Logs if the current source file defines the MS_LOG_DEV macro and the current
 * 	 debug level is satisfied.
 * 	 Example:
 * 	   MS_DEBUG_DEV("Producer closed [producerId:%" PRIu32 "]", producerId);
 *
 * MS_ERROR(...)
 *
 *   Logs an error if the current debug level is satisfied (or if the current
 *   source file defines the MS_LOG_DEV macro). Must just be used for internal
 *   errors that should not happen.
 *
 * MS_ABORT(...)
 *
 *   Logs the given error to stderr and aborts the process.
 *
 * MS_ASSERT(condition, ...)
 *
 *   If the condition is not satisfied, it calls MS_ABORT().
 */

#ifndef MS_LOGGER_HPP
#define MS_LOGGER_HPP

#include "common.hpp"
#include "LogLevel.hpp"
#include "Settings.hpp"
#include "Channel/UnixStreamSocket.hpp"
#include <cstdio>  // std::snprintf(), std::fprintf(), stdout, stderr
#include <cstdlib> // std::abort(), std::getenv()
#include <cstring>

// clang-format off

#define _MS_TAG_ENABLED(tag) Settings::configuration.logTags.tag
#define _MS_TAG_ENABLED_2(tag1, tag2) (Settings::configuration.logTags.tag1 || Settings::configuration.logTags.tag2)
#ifdef MS_LOG_DEV
	#define _MS_LOG_DEV_ENABLED true
#else
	#define _MS_LOG_DEV_ENABLED false
#endif

// Usage:
//   MS_DEBUG_DEV("Leading text "MS_UINT16_TO_BINARY_PATTERN, MS_UINT16_TO_BINARY(value));
#define MS_UINT16_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"
#define MS_UINT16_TO_BINARY(value) \
	((value & 0x8000) ? '1' : '0'), \
	((value & 0x4000) ? '1' : '0'), \
	((value & 0x2000) ? '1' : '0'), \
	((value & 0x1000) ? '1' : '0'), \
	((value & 0x800) ? '1' : '0'), \
	((value & 0x400) ? '1' : '0'), \
	((value & 0x200) ? '1' : '0'), \
	((value & 0x100) ? '1' : '0'), \
	((value & 0x80) ? '1' : '0'), \
	((value & 0x40) ? '1' : '0'), \
	((value & 0x20) ? '1' : '0'), \
	((value & 0x10) ? '1' : '0'), \
	((value & 0x08) ? '1' : '0'), \
	((value & 0x04) ? '1' : '0'), \
	((value & 0x02) ? '1' : '0'), \
	((value & 0x01) ? '1' : '0')

class Logger
{
public:
	static void ClassInit(Channel::UnixStreamSocket* channel);

public:
	static const int64_t pid;
	static Channel::UnixStreamSocket* channel;
	static const size_t bufferSize {10000};
	static char buffer[];
};

/* Logging macros. */

#define _MS_LOG_SEPARATOR_CHAR_STD "\n"

#ifdef MS_LOG_FILE_LINE
	#define _MS_LOG_STR "%s:%d | %s::%s()"
	#define _MS_LOG_STR_DESC _MS_LOG_STR " | "
	#define _MS_FILE (std::strchr(__FILE__, '/') ? std::strchr(__FILE__, '/') + 1 : __FILE__)
	#define _MS_LOG_ARG _MS_FILE, __LINE__, MS_CLASS, __FUNCTION__
#else
	#define _MS_LOG_STR "%s::%s()"
	#define _MS_LOG_STR_DESC _MS_LOG_STR " | "
	#define _MS_LOG_ARG MS_CLASS, __FUNCTION__
#endif

#ifdef MS_LOG_TRACE
	#define MS_TRACE() \
		do \
		{ \
			if (Settings::configuration.logLevel == LogLevel::LOG_DEBUG) \
			{ \
				int loggerWritten = std::snprintf(Logger::buffer, Logger::bufferSize, "D(trace) " _MS_LOG_STR, _MS_LOG_ARG); \
				Logger::channel->SendLog(Logger::buffer, loggerWritten); \
			} \
		} \
		while (false)

	#define MS_TRACE_STD() \
		do \
		{ \
			if (Settings::configuration.logLevel == LogLevel::LOG_DEBUG) \
			{ \
				std::fprintf(stdout, "(trace) " _MS_LOG_STR _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG); \
				std::fflush(stdout); \
			} \
		} \
		while (false)
#else
	#define MS_TRACE() {}
	#define MS_TRACE_STD() {}
#endif

#define MS_HAS_DEBUG_TAG(tag) \
	(Settings::configuration.logLevel == LogLevel::LOG_DEBUG && (_MS_TAG_ENABLED(tag) || _MS_LOG_DEV_ENABLED))

#define MS_HAS_WARN_TAG(tag) \
	(Settings::configuration.logLevel >= LogLevel::LOG_WARN && (_MS_TAG_ENABLED(tag) || _MS_LOG_DEV_ENABLED))

#define MS_DEBUG_TAG(tag, desc, ...) \
	do \
	{ \
		if (Settings::configuration.logLevel == LogLevel::LOG_DEBUG && (_MS_TAG_ENABLED(tag) || _MS_LOG_DEV_ENABLED)) \
		{ \
			int loggerWritten = std::snprintf(Logger::buffer, Logger::bufferSize, "D" _MS_LOG_STR_DESC desc, _MS_LOG_ARG, ##__VA_ARGS__); \
			Logger::channel->SendLog(Logger::buffer, loggerWritten); \
		} \
	} \
	while (false)

#define MS_DEBUG_TAG_STD(tag, desc, ...) \
	do \
	{ \
		if (Settings::configuration.logLevel == LogLevel::LOG_DEBUG && (_MS_TAG_ENABLED(tag) || _MS_LOG_DEV_ENABLED)) \
		{ \
			std::fprintf(stdout, _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__); \
			std::fflush(stdout); \
		} \
	} \
	while (false)

#define MS_WARN_TAG(tag, desc, ...) \
	do \
	{ \
		if (Settings::configuration.logLevel >= LogLevel::LOG_WARN && (_MS_TAG_ENABLED(tag) || _MS_LOG_DEV_ENABLED)) \
		{ \
			int loggerWritten = std::snprintf(Logger::buffer, Logger::bufferSize, "W" _MS_LOG_STR_DESC desc, _MS_LOG_ARG, ##__VA_ARGS__); \
			Logger::channel->SendLog(Logger::buffer, loggerWritten); \
		} \
	} \
	while (false)

#define MS_WARN_TAG_STD(tag, desc, ...) \
	do \
	{ \
		if (Settings::configuration.logLevel >= LogLevel::LOG_WARN && (_MS_TAG_ENABLED(tag) || _MS_LOG_DEV_ENABLED)) \
		{ \
			std::fprintf(stderr, _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__); \
			std::fflush(stderr); \
		} \
	} \
	while (false)

#define MS_DEBUG_2TAGS(tag1, tag2, desc, ...) \
	do \
	{ \
		if (Settings::configuration.logLevel == LogLevel::LOG_DEBUG && (_MS_TAG_ENABLED_2(tag1, tag2) || _MS_LOG_DEV_ENABLED)) \
		{ \
			int loggerWritten = std::snprintf(Logger::buffer, Logger::bufferSize, "D" _MS_LOG_STR_DESC desc, _MS_LOG_ARG, ##__VA_ARGS__); \
			Logger::channel->SendLog(Logger::buffer, loggerWritten); \
		} \
	} \
	while (false)

#define MS_DEBUG_2TAGS_STD(tag1, tag2, desc, ...) \
	do \
	{ \
		if (Settings::configuration.logLevel == LogLevel::LOG_DEBUG && (_MS_TAG_ENABLED_2(tag1, tag2) || _MS_LOG_DEV_ENABLED)) \
		{ \
			std::fprintf(stdout, _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__); \
			std::fflush(stdout); \
		} \
	} \
	while (false)

#define MS_WARN_2TAGS(tag1, tag2, desc, ...) \
	do \
	{ \
		if (Settings::configuration.logLevel >= LogLevel::LOG_WARN && (_MS_TAG_ENABLED_2(tag1, tag2) || _MS_LOG_DEV_ENABLED)) \
		{ \
			int loggerWritten = std::snprintf(Logger::buffer, Logger::bufferSize, "W" _MS_LOG_STR_DESC desc, _MS_LOG_ARG, ##__VA_ARGS__); \
			Logger::channel->SendLog(Logger::buffer, loggerWritten); \
		} \
	} \
	while (false)

#define MS_WARN_2TAGS_STD(tag1, tag2, desc, ...) \
	do \
	{ \
		if (Settings::configuration.logLevel >= LogLevel::LOG_WARN && (_MS_TAG_ENABLED_2(tag1, tag2) || _MS_LOG_DEV_ENABLED)) \
		{ \
			std::fprintf(stderr, _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__); \
			std::fflush(stderr); \
		} \
	} \
	while (false)

#ifdef MS_LOG_DEV
	#define MS_DEBUG_DEV(desc, ...) \
		do \
		{ \
			if (Settings::configuration.logLevel == LogLevel::LOG_DEBUG) \
			{ \
				int loggerWritten = std::snprintf(Logger::buffer, Logger::bufferSize, "D" _MS_LOG_STR_DESC desc, _MS_LOG_ARG, ##__VA_ARGS__); \
				Logger::channel->SendLog(Logger::buffer, loggerWritten); \
			} \
		} \
		while (false)

	#define MS_DEBUG_DEV_STD(desc, ...) \
		do \
		{ \
			if (Settings::configuration.logLevel == LogLevel::LOG_DEBUG) \
			{ \
				std::fprintf(stdout, _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__); \
				std::fflush(stdout); \
			} \
		} \
		while (false)

	#define MS_WARN_DEV(desc, ...) \
		do \
		{ \
			if (Settings::configuration.logLevel >= LogLevel::LOG_WARN) \
			{ \
				int loggerWritten = std::snprintf(Logger::buffer, Logger::bufferSize, "W" _MS_LOG_STR_DESC desc, _MS_LOG_ARG, ##__VA_ARGS__); \
				Logger::channel->SendLog(Logger::buffer, loggerWritten); \
			} \
		} \
		while (false)

	#define MS_WARN_DEV_STD(desc, ...) \
		do \
		{ \
			if (Settings::configuration.logLevel >= LogLevel::LOG_WARN) \
			{ \
				std::fprintf(stderr, _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__); \
				std::fflush(stderr); \
			} \
		} \
		while (false)
#else
	#define MS_DEBUG_DEV(desc, ...) {}
	#define MS_DEBUG_DEV_STD(desc, ...) {}
	#define MS_WARN_DEV(desc, ...) {}
	#define MS_WARN_DEV_STD(desc, ...) {}
#endif

#define MS_ERROR(desc, ...) \
	do \
	{ \
		if (Settings::configuration.logLevel >= LogLevel::LOG_ERROR || _MS_LOG_DEV_ENABLED) \
		{ \
			int loggerWritten = std::snprintf(Logger::buffer, Logger::bufferSize, "E" _MS_LOG_STR_DESC desc, _MS_LOG_ARG, ##__VA_ARGS__); \
			Logger::channel->SendLog(Logger::buffer, loggerWritten); \
		} \
	} \
	while (false)

#define MS_ERROR_STD(desc, ...) \
	do \
	{ \
		if (Settings::configuration.logLevel >= LogLevel::LOG_ERROR || _MS_LOG_DEV_ENABLED) \
		{ \
			std::fprintf(stderr, _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__); \
			std::fflush(stderr); \
		} \
	} \
	while (false)

#define MS_ABORT(desc, ...) \
	do \
	{ \
		std::fprintf(stderr, "ABORT" _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__); \
		std::fflush(stderr); \
		std::abort(); \
	} \
	while (false)

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

// clang-format on

#endif
