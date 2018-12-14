#ifndef MS_LOGGER_HPP
#define MS_LOGGER_HPP

#include "common.hpp"
#include <cstdio>  // std::snprintf(), std::fprintf(), stdout, stderr
#include <cstdlib> // std::abort()
#include <cstring>
#include <string>

#ifdef MS_LOG_DEV
	#define _MS_LOG_DEV_ENABLED true
#else
	#define _MS_LOG_DEV_ENABLED false
#endif

class Logger
{
public:
	static const size_t bufferSize {10000};
	static char buffer[];
};

/* Logging macros. */

#define _MS_LOG_SEPARATOR_CHAR_STD "\n"
#define _MS_LOG_STR "%s::%s()"
#define _MS_LOG_STR_DESC _MS_LOG_STR " | "
#define _MS_LOG_ARG MS_CLASS, __FUNCTION__

#ifdef MS_LOG_DEV
	#define MS_TRACE() \
		do \
		{ \
			std::fprintf(stdout, "(trace) " _MS_LOG_STR _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG); \
			std::fflush(stdout); \
		} \
		while (false)

	#define MS_DEBUG_DEV(desc, ...) \
		do \
		{ \
			std::fprintf(stdout, _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__); \
			std::fflush(stdout); \
		} \
		while (false)

	#define MS_WARN_DEV(desc, ...) \
		do \
		{ \
			std::fprintf(stderr, _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__); \
			std::fflush(stderr); \
		} \
		while (false)
#else
	#define MS_TRACE() ;
	#define MS_DEBUG_DEV(desc, ...) ;
	#define MS_WARN_DEV(desc, ...) ;
#endif

#define MS_ERROR(desc, ...) \
	do \
	{ \
		std::fprintf(stderr, _MS_LOG_STR_DESC desc _MS_LOG_SEPARATOR_CHAR_STD, _MS_LOG_ARG, ##__VA_ARGS__); \
		std::fflush(stderr); \
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

#endif
