#ifndef MS_LOG_LEVEL_H
#define	MS_LOG_LEVEL_H

#include "common.h"

enum class LogLevel : uint8_t
{
	DEBUG = 2,
	WARN  = 1,
	ERROR = 0
};

#endif
