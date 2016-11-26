#define MS_CLASS "RTC::RTCP::FeedbackRtpNackPacket"

#include "RTC/RTCP/FeedbackRtpNack.h"
#include "Logger.h"

#include <cstring>  // std::memcmp(), std::memcpy()

namespace RTC { namespace RTCP
{

	/* NackItem Class methods. */

	NackItem* NackItem::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE_STD();

		// data size must be >= header + length value.
		if (sizeof(Header) > len)
		{
				MS_WARN("not enough space for Nack item, discarded");
				return nullptr;
		}

		Header* header = (Header*)data;
		return new NackItem(header);
	}

	/* NackItem Instance methods. */
	NackItem::NackItem(uint16_t packetId, uint16_t lostPacketBitmask)
	{
		this->raw = new uint8_t[sizeof(Header)];
		this->header = (Header*)this->raw;

		this->header->packet_id = htons(packetId);
		this->header->lost_packet_bitmask = htons(lostPacketBitmask);
	}

	size_t NackItem::Serialize(uint8_t* data)
	{
		MS_TRACE_STD();

		// Add minimum header.
		std::memcpy(data, this->header, sizeof(Header));

		return sizeof(Header);
	}

	void NackItem::Dump()
	{
		MS_TRACE_STD();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("\t\t<Nack Item>");
		MS_WARN("\t\t\tpid: %u", ntohl(this->header->packet_id));
		MS_WARN("\t\t\tbpl: %u", ntohl(this->header->lost_packet_bitmask));
		MS_WARN("\t\t</Nack Item>");
	}

} } // RTP::RTCP

