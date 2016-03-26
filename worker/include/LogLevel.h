#ifndef MS_LOG_LEVEL_H
#define	MS_LOG_LEVEL_H

#include "common.h"

enum class LogLevel : uint8_t
{
	LOG_DEBUG = 2,
	LOG_WARN  = 1,
	LOG_ERROR = 0
};

#endif
