#define MS_CLASS "RTC::RTCP::FeedbackRtpNackPacket"
// #define MS_LOG_DEV

#include "RTC/RTCP/FeedbackRtpNack.h"
#include "Logger.h"
#include <cstring>

namespace RTC { namespace RTCP
{
	/* Class methods. */

	NackItem* NackItem::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// data size must be >= header + length value.
		if (sizeof(Header) > len)
		{
			MS_WARN_TAG(rtcp, "not enough space for Nack item, discarded");

			return nullptr;
		}

		Header* header = (Header*)data;

		return new NackItem(header);
	}

	/* Instance methods. */
	NackItem::NackItem(uint16_t packetId, uint16_t lostPacketBitmask)
	{
		this->raw = new uint8_t[sizeof(Header)];
		this->header = (Header*)this->raw;

		this->header->packet_id = htons(packetId);
		this->header->lost_packet_bitmask = htons(lostPacketBitmask);
	}

	size_t NackItem::Serialize(uint8_t* data)
	{
		MS_TRACE();

		// Add minimum header.
		std::memcpy(data, this->header, sizeof(Header));

		return sizeof(Header);
	}

	void NackItem::Dump()
	{
		MS_TRACE();

		MS_DEBUG_DEV("<NackItem>");
		MS_DEBUG_DEV("  pid : %" PRIu16, (uint16_t)ntohs(this->header->packet_id));
		MS_DEBUG_DEV("  bpl : %" PRIu16, (uint16_t)ntohs(this->header->lost_packet_bitmask));
		MS_DEBUG_DEV("</NackItem>");
	}
}}
