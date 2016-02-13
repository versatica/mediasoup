#define MS_CLASS "RTC::RtcpPacket"

#include "RTC/RtcpPacket.h"
#include "Logger.h"

namespace RTC
{
	/* Class methods. */

	RtcpPacket* RtcpPacket::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (!RtcpPacket::IsRtcp(data, len))
			return nullptr;

		// TODO: Do it.

		// Get the header.
		CommonHeader* header = (CommonHeader*)data;

		return new RtcpPacket(header, data, len);
	}

	/* Instance methods. */

	RtcpPacket::RtcpPacket(CommonHeader* header, const uint8_t* raw, size_t length) :
		header(header),
		raw((uint8_t*)raw),
		length(length)
	{
		MS_TRACE();
	}

	RtcpPacket::~RtcpPacket()
	{
		MS_TRACE();
	}
}
