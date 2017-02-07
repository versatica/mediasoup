#define MS_CLASS "RTC::RTCP::FeedbackRtpGccPacket"
// #define MS_LOG_DEV

#include "RTC/RTCP/FeedbackRtpGcc.h"
#include "Logger.h"
#include <cstring>

namespace RTC { namespace RTCP
{
	/* Class methods. */

	FeedbackRtpGccPacket* FeedbackRtpGccPacket::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (sizeof(CommonHeader) + sizeof(FeedbackPacket::Header) > len)
		{
			MS_WARN_TAG(rtcp, "not enough space for Feedback packet, discarded");

			return nullptr;
		}

		CommonHeader* commonHeader = (CommonHeader*)data;
		std::unique_ptr<FeedbackRtpGccPacket> packet(new FeedbackRtpGccPacket(commonHeader));

		return packet.release();
	}

	size_t FeedbackRtpGccPacket::Serialize(uint8_t* buffer)
	{
		MS_TRACE();

		size_t offset = FeedbackRtpPacket::Serialize(buffer);

		// Copy the content.
		std::memcpy(buffer+offset, this->data, this->size);

		return offset + this->size;
	}

	void FeedbackRtpGccPacket::Dump()
	{
		#ifdef MS_LOG_DEV

		MS_TRACE();

		MS_DEBUG_DEV("<FeedbackRtpGccPacket>");
		FeedbackRtpPacket::Dump();
		MS_DEBUG_DEV("</FeedbackRtpGccPacket>");

		#endif
	}
}}
