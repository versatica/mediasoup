#define MS_CLASS "RTC::RTCP::FeedbackPsPliPacket"

#include "RTC/RTCP/FeedbackPsPli.h"
#include "Logger.h"

#include <cstring>  // std::memcmp(), std::memcpy()

namespace RTC
{
namespace RTCP
{

/* FeedbackPsPliPacket Class methods. */

	FeedbackPsPliPacket* FeedbackPsPliPacket::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE_STD();

		if (sizeof(CommonHeader) + sizeof(FeedbackPacket::Header) > len)
		{
				MS_WARN("not enough space for Feedback packet, discarded");
				return nullptr;
		}

		CommonHeader* commonHeader = (CommonHeader*)data;
		std::auto_ptr<FeedbackPsPliPacket> packet(new FeedbackPsPliPacket(commonHeader));
		return packet.release();
	}

	/* FeedbackPsPliPacket Instance methods. */

	FeedbackPsPliPacket::FeedbackPsPliPacket(CommonHeader* commonHeader):
		FeedbackPsPacket(commonHeader)
	{
	}

	FeedbackPsPliPacket::FeedbackPsPliPacket(uint32_t sender_ssrc, uint32_t media_ssrc):
		FeedbackPsPacket(FeedbackPs::PLI, sender_ssrc, media_ssrc)
	{
	}

	void FeedbackPsPliPacket::Dump()
	{
		MS_TRACE_STD();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("\t<FeedbackPsPliPacket>");
		FeedbackPsPacket::Dump();
		MS_WARN("\t</FeedbackPsPliPacket>");
	}
}
}

