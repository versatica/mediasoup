#ifndef MS_LOG_LEVEL_HPP
#define MS_LOG_LEVEL_HPP

#include "common.hpp"

enum class LogLevel : uint8_t
{
	LOG_DEBUG = 3,
	LOG_WARN  = 2,
	LOG_ERROR = 1,
	LOG_NONE  = 0
};

enum class LogDevLevel : uint8_t
{
	LOG_DEV_DEBUG = 3,
	LOG_DEV_WARN  = 2,
	LOG_DEV_NONE  = 0
};

#endif
