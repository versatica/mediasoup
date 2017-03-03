#define MS_CLASS "RTC::RTCP::FeedbackPsVbcmPacket"
// #define MS_LOG_DEV

#include "RTC/RTCP/FeedbackPsVbcm.hpp"
#include "Logger.hpp"
#include <cstring>

namespace RTC { namespace RTCP
{
	/* Class methods. */

	VbcmItem* VbcmItem::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// data size must be >= header + length value.
		if (sizeof(Header) > len)
		{
			MS_WARN_TAG(rtcp, "not enough space for Vbcm item, discarded");

			return nullptr;
		}

		Header* header = const_cast<Header*>(reinterpret_cast<const Header*>(data));

		return new VbcmItem(header);
	}

	/* Instance methods. */
	VbcmItem::VbcmItem(uint32_t ssrc, uint8_t sequence_number, uint8_t payload_type, uint16_t length, uint8_t* value)
	{
		this->raw = new uint8_t[8 + length];
		this->header = reinterpret_cast<Header*>(this->raw);

		this->header->ssrc = htonl(ssrc);
		this->header->sequence_number = sequence_number;
		this->header->zero = 0;
		this->header->payload_type = payload_type;
		this->header->length = htons(length);
		std::memcpy(this->header->value, value, sizeof(length));
	}

	size_t VbcmItem::Serialize(uint8_t* buffer)
	{
		MS_TRACE();

		// Add minimum header.
		std::memcpy(buffer, this->header, 8);

		// Copy the content.
		std::memcpy(buffer+8, this->header->value, this->header->length);

		size_t offset = 8+this->header->length;

		// 32 bits padding.
		size_t padding = (-offset) & 3;
		for (size_t i = 0; i < padding; ++i)
		{
			buffer[offset+i] = 0;
		}

		return offset+padding;
	}

	void VbcmItem::Dump()
	{
		MS_TRACE();

		MS_DUMP("<VbcmItem>");
		MS_DUMP("  ssrc            : %" PRIu32, this->GetSsrc());
		MS_DUMP("  sequence number : %" PRIu8, this->GetSequenceNumber());
		MS_DUMP("  payload type    : %" PRIu8, this->GetPayloadType());
		MS_DUMP("  length          : %" PRIu16, this->GetLength());
		MS_DUMP("</VbcmItem>");
	}
}}
