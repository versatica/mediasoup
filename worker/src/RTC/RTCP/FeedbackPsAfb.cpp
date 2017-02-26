#define MS_CLASS "RTC::RTCP::FeedbackPsAfbPacket"
// #define MS_LOG_DEV

#include "RTC/RTCP/FeedbackPsAfb.hpp"
#include "Logger.hpp"
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

		CommonHeader* commonHeader = const_cast<CommonHeader*>(reinterpret_cast<const CommonHeader*>(data));
		std::unique_ptr<FeedbackPsAfbPacket> packet(new FeedbackPsAfbPacket(commonHeader));

		return packet.release();
	}

	size_t FeedbackPsAfbPacket::Serialize(uint8_t* buffer)
	{
		MS_TRACE();

		size_t offset = FeedbackPsPacket::Serialize(buffer);

		// Copy the content.
		std::memcpy(buffer+offset, this->data, this->size);

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
