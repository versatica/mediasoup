#define MS_CLASS "RTC::RTCP::FeedbackPsAfb"
// #define MS_LOG_DEV

#include "RTC/RTCP/FeedbackPsAfb.hpp"
#include "RTC/RTCP/FeedbackPsRemb.hpp"
#include "Utils.hpp"
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

		std::unique_ptr<FeedbackPsAfbPacket> packet;

		constexpr size_t offset = sizeof(CommonHeader) + sizeof(FeedbackPacket::Header);
		if (Utils::Byte::Get4Bytes(data, offset) == FeedbackPsRembPacket::UniqueIdentifier)
		{
			packet.reset(FeedbackPsRembPacket::Parse(data, len));
		}

		else {
			packet.reset(new FeedbackPsAfbPacket(commonHeader));
		}

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

	void FeedbackPsAfbPacket::Dump() const
	{
		MS_TRACE();

		MS_DUMP("<FeedbackPsAfbPacket>");
		FeedbackPsPacket::Dump();
		MS_DUMP("</FeedbackPsAfbPacket>");
	}
}}
