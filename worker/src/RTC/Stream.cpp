#define MS_CLASS "RTC::Stream"

#include "RTC/Stream.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	Stream::Stream()
	{
		MS_TRACE();
	}

	Stream::~Stream()
	{
		MS_TRACE();
	}

	void Stream::Close()
	{
		MS_TRACE();

		delete this;
	}
}
