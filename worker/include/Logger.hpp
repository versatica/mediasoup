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
 *   current log level is "debug".
 *
 * MS_HAS_DEBUG_TAG(tag)
 * MS_HAS_WARN_TAG(tag)
 *
 *   True if the current log level is satisfied and the given tag is enabled.
 *
 * MS_DEBUG_TAG(tag, ...)
 * MS_WARN_TAG(tag, ...)
 *
 *   Logs if the current log level is satisfied and the given tag is enabled.
 *
 *   Example:
 *     MS_WARN_TAG(ice, "ICE failed");
 *
 * MS_DEBUG_2TAGS(tag1, tag2, ...)
 * MS_WARN_2TAGS(tag1, tag2, ...)
 *
 *   Logs if the current log level is satisfied and any of the given two tags
 *   is enabled.
 *
 *   Example:
 *     MS_DEBUG_2TAGS(ice, dtls, "media connection established");
 *
 * MS_DEBUG_DEV(...)
 *
 * 	 Logs if the current source file defines the MS_LOG_DEV_LEVEL macro with
 * 	 value 3.
 *
 * 	 Example:
 * 	   MS_DEBUG_DEV("foo:%" PRIu32, foo);
 *
 * MS_WARN_DEV(...)
 *
 * 	 Logs if the current source file defines the MS_LOG_DEV_LEVEL macro with
 * 	 value >= 2.
 *
 * 	 Example:
 * 	   MS_WARN_DEV("foo:%" PRIu32, foo);
 *
 * MS_DUMP(...)
 *
 * 	 Logs always. Useful for Dump() methods.
 *
 * 	 Example:
 * 	   MS_DUMP("foo");
 *
 * MS_DUMP_DATA(const uint8_t* data, size_t len)
 *
 *   Logs always. Prints the given data in hexadecimal format (Wireshark friendly).
 *
 * MS_ERROR(...)
 *
 *   Logs an error if the current log level is satisfied (or if the current
 *   source file defines the MS_LOG_DEV_LEVEL macro with value >= 1). Must just
 *   be used for internal errors that should not happen.
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
#include "json.hpp"
#include "LogLevel.hpp"
#include "Settings.hpp"
#include "Channel/ChannelSocket.hpp"
#include "Utils.hpp"

#include <cstdio>  // std::snprintf(), std::fprintf(), stdout, stderr
#include <cstdlib> // std::abort()
#include <cstring>

// clang-format off

#define _MS_TAG_ENABLED(tag) Settings::configuration.logTags.tag
#define _MS_TAG_ENABLED_2(tag1, tag2) (Settings::configuration.logTags.tag1 || Settings::configuration.logTags.tag2)

#if !defined(MS_LOG_DEV_LEVEL)
	#define MS_LOG_DEV_LEVEL 0
#elif MS_LOG_DEV_LEVEL < 0 || MS_LOG_DEV_LEVEL > 3
	#error "invalid MS_LOG_DEV_LEVEL macro value"
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
	static bool MSlogopen(json& data);
	static void MSlogrotate();
	static void MSlogclose();
	static void MSlogwrite(int written); 

public:
  static std::string levelPrefix;
	static const int64_t pid;
	static const size_t bufferSize {50000};
	thread_local static char buffer[];
	thread_local static std::string backupBuffer;
	thread_local static std::string appdataBuffer;

	static std::string logfilename;
	static std::FILE*  logfd;
	static bool openLogFile;
};

/* Logging macros. */

#define _MS_LOG_SEPARATOR_CHAR_STD "\n"

// TBD: disabled file line option as we really don't need it much, for ease of code changes
//#ifdef MS_LOG_FILE_LINE
//	#define _MS_LOG_STR "%ld: %s:%d | %s::%s()"
//	#define _MS_LOG_STR_DESC _MS_LOG_STR " | "
//	#define _MS_FILE (std::strchr(__FILE__, '/') ? std::strchr(__FILE__, '/') + 1 : __FILE__)
//	#define _MS_LOG_ARG Logger::pid, _MS_FILE, __LINE__, MS_CLASS, __FUNCTION__
//#else
	#define _MS_LOG_STR  "%s level=\"%s\" pid=\"%ld\" message=\"%s::%s()"
	#define _MS_LOG_STR_DESC _MS_LOG_STR " | "
	#define _MS_LOG_ARG Utils::Time::currentStdTimestamp().c_str(), Logger::levelPrefix.c_str(), Logger::pid, MS_CLASS, __FUNCTION__
	
	#define _MS_LOG_STR_LIVELYAPP "%s level=\"%s\" pid=\"%ld\" %s message=\"%s::%s()"
	#define _MS_LOG_STR_DESC_LIVELYAPP _MS_LOG_STR_LIVELYAPP " | "
	#define _MS_LOG_ARG_LIVELYAPP Utils::Time::currentStdTimestamp().c_str(), Logger::levelPrefix.c_str(), Logger::pid, Logger::appdataBuffer.c_str(), MS_CLASS, __FUNCTION__
//#endif


#define MS_TRACE() \
	do \
	{ \
		if (Settings::configuration.logLevel == LogLevel::LOG_DEBUG && Settings::configuration.logTraceEnabled) \
		{ \
			Logger::levelPrefix = "trace"; \
			int loggerWritten = std::snprintf(Logger::buffer, Logger::bufferSize, _MS_LOG_STR, _MS_LOG_ARG); \
			Logger::MSlogwrite(loggerWritten); \
		} \
	} \
	while (false)


#define MS_TRACE_STD() \
	do \
	{ \
		if (Settings::configuration.logLevel == LogLevel::LOG_DEBUG && Settings::configuration.logTraceEnabled) \
		{ \
			std::fprintf(stdout, "(trace) " _MS_LOG_STR _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG); \
			std::fflush(stdout); \
		} \
	} \
	while (false)


#define MS_HAS_DEBUG_TAG(tag) \
	(Settings::configuration.logLevel == LogLevel::LOG_DEBUG && _MS_TAG_ENABLED(tag))

#define MS_HAS_WARN_TAG(tag) \
	(Settings::configuration.logLevel >= LogLevel::LOG_WARN && _MS_TAG_ENABLED(tag))


#define MS_DEBUG_TAG_LIVELYAPP(tag, appdatastr, desc, ...) \
	do \
	{ \
		if (Settings::configuration.logLevel == LogLevel::LOG_DEBUG && _MS_TAG_ENABLED(tag)) \
		{ \
			Logger::levelPrefix = "debug"; \
			Logger::appdataBuffer.assign(appdatastr); \
			int loggerWritten = std::snprintf(Logger::buffer, Logger::bufferSize, _MS_LOG_STR_DESC_LIVELYAPP desc, _MS_LOG_ARG_LIVELYAPP, ##__VA_ARGS__); \
			Logger::MSlogwrite(loggerWritten); \
		} \
	} \
	while (false)

#define MS_DEBUG_TAG(tag, desc, ...) \
	do \
	{ \
		if (Settings::configuration.logLevel == LogLevel::LOG_DEBUG && _MS_TAG_ENABLED(tag)) \
		{ \
			Logger::levelPrefix = "debug"; \
			int loggerWritten = std::snprintf(Logger::buffer, Logger::bufferSize, _MS_LOG_STR_DESC desc, _MS_LOG_ARG, ##__VA_ARGS__); \
			Logger::MSlogwrite(loggerWritten); \
		} \
	} \
	while (false)


#define MS_DEBUG_TAG_STD(tag, desc, ...) \
	do \
	{ \
		if (Settings::configuration.logLevel == LogLevel::LOG_DEBUG && _MS_TAG_ENABLED(tag)) \
		{ \
			std::fprintf(stdout, _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__); \
			std::fflush(stdout); \
		} \
	} \
	while (false)


#define MS_WARN_TAG(tag, desc, ...) \
	do \
	{ \
		if (Settings::configuration.logLevel >= LogLevel::LOG_WARN && _MS_TAG_ENABLED(tag)) \
		{ \
			Logger::levelPrefix = "warn"; \
			int loggerWritten = std::snprintf(Logger::buffer, Logger::bufferSize, _MS_LOG_STR_DESC desc, _MS_LOG_ARG, ##__VA_ARGS__); \
			Logger::MSlogwrite(loggerWritten); \
		} \
	} \
	while (false)


#define MS_WARN_TAG_STD(tag, desc, ...) \
	do \
	{ \
		if (Settings::configuration.logLevel >= LogLevel::LOG_WARN && _MS_TAG_ENABLED(tag)) \
		{ \
			std::fprintf(stderr, _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__); \
			std::fflush(stderr); \
		} \
	} \
	while (false)


#define MS_DEBUG_2TAGS(tag1, tag2, desc, ...) \
	do \
	{ \
		if (Settings::configuration.logLevel == LogLevel::LOG_DEBUG && _MS_TAG_ENABLED_2(tag1, tag2)) \
		{ \
			Logger::levelPrefix = "debug"; \
			int loggerWritten = std::snprintf(Logger::buffer, Logger::bufferSize, _MS_LOG_STR_DESC desc, _MS_LOG_ARG, ##__VA_ARGS__); \
			Logger::MSlogwrite(loggerWritten); \
		} \
	} \
	while (false)


#define MS_DEBUG_2TAGS_STD(tag1, tag2, desc, ...) \
	do \
	{ \
		if (Settings::configuration.logLevel == LogLevel::LOG_DEBUG && _MS_TAG_ENABLED_2(tag1, tag2)) \
		{ \
			std::fprintf(stdout, _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__); \
			std::fflush(stdout); \
		} \
	} \
	while (false)


#define MS_WARN_2TAGS(tag1, tag2, desc, ...) \
	do \
	{ \
		if (Settings::configuration.logLevel >= LogLevel::LOG_WARN && _MS_TAG_ENABLED_2(tag1, tag2)) \
		{ \
			Logger::levelPrefix = "warn"; \
			int loggerWritten = std::snprintf(Logger::buffer, Logger::bufferSize, _MS_LOG_STR_DESC desc, _MS_LOG_ARG, ##__VA_ARGS__); \
			Logger::MSlogwrite(loggerWritten); \
		} \
	} \
	while (false)


#define MS_WARN_2TAGS_STD(tag1, tag2, desc, ...) \
	do \
	{ \
		if (Settings::configuration.logLevel >= LogLevel::LOG_WARN && _MS_TAG_ENABLED_2(tag1, tag2)) \
		{ \
			std::fprintf(stderr, _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__); \
			std::fflush(stderr); \
		} \
	} \
	while (false)


	#define MS_DEBUG_DEV(desc, ...) \
		do \
		{ \
			if (Settings::configuration.logDevLevel == LogDevLevel::LOG_DEV_DEBUG) \
			{ \
				Logger::levelPrefix = "debug"; \
				int loggerWritten = std::snprintf(Logger::buffer, Logger::bufferSize, _MS_LOG_STR_DESC desc, _MS_LOG_ARG, ##__VA_ARGS__); \
				Logger::MSlogwrite(loggerWritten); \
			} \
		} \
		while (false)


	#define MS_DEBUG_DEV_STD(desc, ...) \
		do \
		{ \
			if (Settings::configuration.logDevLevel == LogDevLevel::LOG_DEV_DEBUG) \
			{ \
				std::fprintf(stdout, _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__); \
				std::fflush(stdout); \
			} \
		} \
		while (false)


	#define MS_WARN_DEV(desc, ...) \
		do \
		{ \
			if (Settings::configuration.logDevLevel >= LogDevLevel::LOG_DEV_WARN) \
			{ \
				Logger::levelPrefix = "warn"; \
				int loggerWritten = std::snprintf(Logger::buffer, Logger::bufferSize, _MS_LOG_STR_DESC desc, _MS_LOG_ARG, ##__VA_ARGS__); \
				Logger::MSlogwrite(loggerWritten); \
			} \
		} \
		while (false)


	#define MS_WARN_DEV_STD(desc, ...) \
		do \
		{ \
			if (Settings::configuration.logDevLevel >= LogDevLevel::LOG_DEV_WARN) \
			{ \
				std::fprintf(stderr, _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__); \
				std::fflush(stderr); \
			} \
		} \
		while (false)


#define MS_DUMP(desc, ...) \
	do \
	{ \
		Logger::levelPrefix = "dump"; \
		int loggerWritten = std::snprintf(Logger::buffer, Logger::bufferSize,  _MS_LOG_STR_DESC desc, _MS_LOG_ARG, ##__VA_ARGS__); \
		Logger::MSlogwrite(loggerWritten); \
	} \
	while (false)


#define MS_DUMP_STD(desc, ...) \
	do \
	{ \
		std::fprintf(stdout, _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__); \
		std::fflush(stdout); \
	} \
	while (false)


#define MS_DUMP_DATA(data, len) \
	do \
	{ \
		Logger::levelPrefix = "data"; \
		int loggerWritten = std::snprintf(Logger::buffer, Logger::bufferSize, _MS_LOG_STR, _MS_LOG_ARG); \
		Logger::MSlogwrite(loggerWritten); \
		size_t bufferDataLen{ 0 }; \
		for (size_t i{0}; i < len; ++i) \
		{ \
		  if (i % 8 == 0) \
		  { \
		  	if (bufferDataLen != 0) \
		  	{ \
					Logger::MSlogwrite(bufferDataLen); \
		  		bufferDataLen = 0; \
		  	} \
		    int loggerWritten = std::snprintf(Logger::buffer + bufferDataLen, Logger::bufferSize, "\n%06X ", static_cast<unsigned int>(i)); \
		    bufferDataLen += loggerWritten; \
		  } \
		  int loggerWritten = std::snprintf(Logger::buffer + bufferDataLen, Logger::bufferSize, "%02X ", static_cast<unsigned char>(data[i])); \
		  bufferDataLen += loggerWritten; \
		} \
		if (bufferDataLen != 0) \
		{ \
			Logger::MSlogwrite(bufferDataLen); \
		} \
	} \
	while (false)


#define MS_DUMP_DATA_STD(data, len) \
	do \
	{ \
		std::fprintf(stdout, "(data) " _MS_LOG_STR _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG); \
		size_t bufferDataLen{ 0 }; \
		for (size_t i{0}; i < len; ++i) \
		{ \
		  if (i % 8 == 0) \
		  { \
		  	if (bufferDataLen != 0) \
		  	{ \
		  		Logger::buffer[bufferDataLen] = '\0'; \
		  		std::fprintf(stdout, "%s", Logger::buffer); \
		  		bufferDataLen = 0; \
		  	} \
		    int loggerWritten = std::snprintf(Logger::buffer + bufferDataLen, Logger::bufferSize, "\n%06X ", static_cast<unsigned int>(i)); \
		    bufferDataLen += loggerWritten; \
		  } \
		  int loggerWritten = std::snprintf(Logger::buffer + bufferDataLen, Logger::bufferSize, "%02X ", static_cast<unsigned char>(data[i])); \
		  bufferDataLen += loggerWritten; \
		} \
		if (bufferDataLen != 0) \
		{ \
			Logger::buffer[bufferDataLen] = '\0'; \
			std::fprintf(stdout, "%s", Logger::buffer); \
		} \
		std::fflush(stdout); \
	} \
	while (false)


#define MS_ERROR(desc, ...) \
	do \
	{ \
		if (Settings::configuration.logLevel >= LogLevel::LOG_ERROR || MS_LOG_DEV_LEVEL >= 1) \
		{ \
			Logger::levelPrefix = "error"; \
			int loggerWritten = std::snprintf(Logger::buffer, Logger::bufferSize, _MS_LOG_STR_DESC desc, _MS_LOG_ARG, ##__VA_ARGS__); \
			Logger::MSlogwrite(loggerWritten); \
		} \
	} \
	while (false)


#define MS_ERROR_STD(desc, ...) \
	do \
	{ \
		if (Settings::configuration.logLevel >= LogLevel::LOG_ERROR || MS_LOG_DEV_LEVEL >= 1) \
		{ \
			std::fprintf(stderr, _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__); \
			std::fflush(stderr); \
		} \
	} \
	while (false)


#define MS_ABORT(desc, ...) \
	do \
	{ \
		std::fprintf(stderr, "(ABORT) " _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__); \
		std::fflush(stderr); \
		Logger::MSlogclose(); \
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
	#undef MS_DUMP
	#define MS_DUMP MS_DUMP_STD
	#undef MS_DUMP_DATA
	#define MS_DUMP_DATA MS_DUMP_DATA_STD
	#undef MS_ERROR
	#define MS_ERROR MS_ERROR_STD
#endif

// clang-format on

#endif
