#define MS_CLASS "RTC::RTCP::FeedbackRtpTllei"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTCP/FeedbackRtpTllei.hpp"
#include "Logger.hpp"
#include <cstring> // std::memcpy

namespace RTC
{
	namespace RTCP
	{
		/* Instance methods. */
		FeedbackRtpTlleiItem::FeedbackRtpTlleiItem(uint16_t packetId, uint16_t lostPacketBitmask)
		{
			this->raw    = new uint8_t[HeaderSize];
			this->header = reinterpret_cast<Header*>(this->raw);

			this->header->packetId          = uint16_t{ htons(packetId) };
			this->header->lostPacketBitmask = uint16_t{ htons(lostPacketBitmask) };
		}

		size_t FeedbackRtpTlleiItem::Serialize(uint8_t* buffer)
		{
			MS_TRACE();

			// Add minimum header.
			std::memcpy(buffer, this->header, HeaderSize);

			return HeaderSize;
		}

		void FeedbackRtpTlleiItem::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<FeedbackRtpTlleiItem>");
			MS_DUMP_CLEAN(indentation, "  pid: %" PRIu16, this->GetPacketId());
			MS_DUMP_CLEAN(indentation, "  bpl: %" PRIu16, this->GetLostPacketBitmask());
			MS_DUMP_CLEAN(indentation, "</FeedbackRtpTlleiItem>");
		}
	} // namespace RTCP
} // namespace RTC
