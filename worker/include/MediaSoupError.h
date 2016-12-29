#ifndef MS_MEDIASOUP_ERROR_H
#define MS_MEDIASOUP_ERROR_H

#include "Logger.h"
#include <stdexcept>
#include <string>
#include <cstdio>  // std::snprintf

class MediaSoupError :
	public std::runtime_error
{
public:
	explicit MediaSoupError(const char* description);
};

/* Inline methods. */

inline
MediaSoupError::MediaSoupError(const char* description) :
	std::runtime_error(description)
{}

#define MS_THROW_ERROR(desc, ...)  \
	do  \
	{  \
		MS_ERROR("throwing MediaSoupError | " desc, ##__VA_ARGS__);  \
		static char buffer[2000];  \
		std::snprintf(buffer, 2000, desc, ##__VA_ARGS__);  \
		throw MediaSoupError(buffer);  \
	}  \
	while (0)

#define MS_THROW_ERROR_STD(desc, ...)  \
	do  \
	{  \
		MS_ERROR_STD("throwing MediaSoupError | " desc, ##__VA_ARGS__);  \
		static char buffer[2000];  \
		std::snprintf(buffer, 2000, desc, ##__VA_ARGS__);  \
		throw MediaSoupError(buffer);  \
	}  \
	while (0)

#endif
