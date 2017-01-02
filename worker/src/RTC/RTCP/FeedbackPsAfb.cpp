#define MS_CLASS "RTC::RTCP::FeedbackPsAfbPacket"
// #define MS_LOG_DEV

#include "RTC/RTCP/FeedbackPsAfb.h"
#include "Logger.h"
#include <cstring>

namespace RTC { namespace RTCP
{
	/* Class methods. */

	FeedbackPsAfbPacket* FeedbackPsAfbPacket::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (sizeof(CommonHeader) + sizeof(FeedbackPacket::Header) > len)
		{
			MS_WARN_TAG(rtcp, "not enough space for Feedback packet, discarded");

			return nullptr;
		}

		CommonHeader* commonHeader = (CommonHeader*)data;
		std::auto_ptr<FeedbackPsAfbPacket> packet(new FeedbackPsAfbPacket(commonHeader));

		return packet.release();
	}

	size_t FeedbackPsAfbPacket::Serialize(uint8_t* data)
	{
		MS_TRACE();

		size_t offset = FeedbackPsPacket::Serialize(data);

		// Copy the content.
		std::memcpy(data+offset, this->data, this->size);

		return offset + this->size;
	}

	void FeedbackPsAfbPacket::Dump()
	{
		#ifdef MS_LOG_DEV

		MS_TRACE();

		MS_DEBUG_DEV("<FeedbackPsAfbPacket>");
		FeedbackPsPacket::Dump();
		MS_DEBUG_DEV("</FeedbackPsAfbPacket>");

		#endif
	}
}}
