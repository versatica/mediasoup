#ifndef MS_LOG_LEVEL_HPP
#define	MS_LOG_LEVEL_HPP

#include "common.hpp"

enum class LogLevel : uint8_t
{
	LOG_DEBUG = 2,
	LOG_WARN  = 1,
	LOG_ERROR = 0
};

#endif
