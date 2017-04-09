#define MS_CLASS "RTC::RTCP::FeedbackRtpNack"
// #define MS_LOG_DEV

#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "Logger.hpp"
#include <bitset> // std::bitset()
#include <cstring>

namespace RTC
{
	namespace RTCP
	{
		/* Class methods. */

		FeedbackRtpNackItem* FeedbackRtpNackItem::Parse(const uint8_t* data, size_t len)
		{
			MS_TRACE();

			// data size must be >= header + length value.
			if (sizeof(Header) > len)
			{
				MS_WARN_TAG(rtcp, "not enough space for Nack item, discarded");

				return nullptr;
			}

			Header* header = const_cast<Header*>(reinterpret_cast<const Header*>(data));

			return new FeedbackRtpNackItem(header);
		}

		/* Instance methods. */
		FeedbackRtpNackItem::FeedbackRtpNackItem(uint16_t packetId, uint16_t lostPacketBitmask)
		{
			this->raw    = new uint8_t[sizeof(Header)];
			this->header = reinterpret_cast<Header*>(this->raw);

			this->header->packetId          = htons(packetId);
			this->header->lostPacketBitmask = htons(lostPacketBitmask);
		}

		size_t FeedbackRtpNackItem::Serialize(uint8_t* buffer)
		{
			MS_TRACE();

			// Add minimum header.
			std::memcpy(buffer, this->header, sizeof(Header));

			return sizeof(Header);
		}

		void FeedbackRtpNackItem::Dump() const
		{
			MS_TRACE();

			std::bitset<16> nackBitset(this->GetLostPacketBitmask());

			MS_DUMP("<FeedbackRtpNackItem>");
			MS_DUMP("  pid : %" PRIu16, this->GetPacketId());
			MS_DUMP("  bpl : %s", nackBitset.to_string().c_str());
			MS_DUMP("</FeedbackRtpNackItem>");
		}
	}
}
