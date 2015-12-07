#ifndef MS_MEDIASOUP_ERROR_H
#define MS_MEDIASOUP_ERROR_H

#include "Logger.h"
#include <stdexcept>
#include <string>
#include <cstdio>  // std::snprintf

class MediaSoupError : public std::runtime_error
{
public:
	MediaSoupError(const char* description);
};

/* Inline methods. */

inline
MediaSoupError::MediaSoupError(const char* description) :	std::runtime_error(description)
{}

#define MS_THROW_ERROR(desc, ...)  \
	do  \
	{  \
		MS_ERROR("throwing MediaSoupError | " desc, ##__VA_ARGS__);  \
		char buffer[1000];  \
		std::snprintf(buffer, 1000, desc, ##__VA_ARGS__);  \
		throw MediaSoupError(buffer);  \
	}  \
	while (0)

#endif
