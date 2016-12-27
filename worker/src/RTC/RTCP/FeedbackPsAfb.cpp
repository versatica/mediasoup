#define MS_CLASS "RTC::RTCP::FeedbackPsAfbPacket"

#include "RTC/RTCP/FeedbackPsAfb.h"
#include "Logger.h"

#include <cstring>  // std::memcmp(), std::memcpy()

namespace RTC { namespace RTCP
{

/* FeedbackPsAfbPacket Class methods. */

	FeedbackPsAfbPacket* FeedbackPsAfbPacket::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE_STD();

		if (sizeof(CommonHeader) + sizeof(FeedbackPacket::Header) > len)
		{
				MS_WARN("not enough space for Feedback packet, discarded");
				return nullptr;
		}

		CommonHeader* commonHeader = (CommonHeader*)data;
		std::auto_ptr<FeedbackPsAfbPacket> packet(new FeedbackPsAfbPacket(commonHeader));
		return packet.release();
	}

	size_t FeedbackPsAfbPacket::Serialize(uint8_t* data)
	{
		MS_TRACE_STD();

		size_t offset = FeedbackPsPacket::Serialize(data);

		// Copy the content.
		std::memcpy(data+offset, this->data, this->size);

		return offset + this->size;
	}


	void FeedbackPsAfbPacket::Dump()
	{
		MS_TRACE_STD();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("\t<FeedbackPsAfbPacket>");
		FeedbackPsPacket::Dump();
		MS_WARN("\t</FeedbackPsAfbPacket>");
	}

} } // RTP::RTCP
