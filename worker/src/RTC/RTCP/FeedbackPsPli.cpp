#define MS_CLASS "RTC::RTCP::FeedbackPsPliPacket"
// #define MS_LOG_DEV

#include "RTC/RTCP/FeedbackPsPli.hpp"
#include "Logger.hpp"

namespace RTC { namespace RTCP
{
	/* Class methods. */

	FeedbackPsPliPacket* FeedbackPsPliPacket::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (sizeof(CommonHeader) + sizeof(FeedbackPacket::Header) > len)
		{
			MS_WARN_TAG(rtcp, "not enough space for Feedback packet, discarded");

			return nullptr;
		}

		CommonHeader* commonHeader = const_cast<CommonHeader*>(reinterpret_cast<const CommonHeader*>(data));
		std::unique_ptr<FeedbackPsPliPacket> packet(new FeedbackPsPliPacket(commonHeader));

		return packet.release();
	}

	void FeedbackPsPliPacket::Dump()
	{
		MS_TRACE();

		MS_DUMP("<FeedbackPsPliPacket>");
		FeedbackPsPacket::Dump();
		MS_DUMP("</FeedbackPsPliPacket>");
	}
}}
