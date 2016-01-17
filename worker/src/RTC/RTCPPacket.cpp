#define MS_CLASS "RTC::RTCPPacket"

#include "RTC/RTCPPacket.h"
#include "Logger.h"

namespace RTC
{
	/* Class methods. */

	RTCPPacket* RTCPPacket::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (!RTCPPacket::IsRTCP(data, len))
			return nullptr;

		// TODO: Do it.

		// Get the header.
		CommonHeader* header = (CommonHeader*)data;

		return new RTCPPacket(header, data, len);
	}

	/* Instance methods. */

	RTCPPacket::RTCPPacket(CommonHeader* header, const uint8_t* raw, size_t length) :
		header(header),
		raw((uint8_t*)raw),
		length(length)
	{
		MS_TRACE();
	}

	RTCPPacket::~RTCPPacket()
	{
		MS_TRACE();
	}
}
