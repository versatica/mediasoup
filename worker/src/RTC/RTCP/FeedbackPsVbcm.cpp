#define MS_CLASS "RTC::RTCP::FeedbackPsVbcmPacket"
// #define MS_LOG_DEV

#include "RTC/RTCP/FeedbackPsVbcm.h"
#include "Logger.h"
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

		Header* header = (Header*)data;

		return new VbcmItem(header);
	}

	/* Instance methods. */
	VbcmItem::VbcmItem(uint32_t ssrc, uint8_t sequence_number, uint8_t payload_type, uint16_t length, uint8_t* value)
	{
		this->raw = new uint8_t[8 + length];
		this->header = (Header*)this->raw;

		this->header->ssrc = htonl(ssrc);
		this->header->sequence_number = sequence_number;
		this->header->zero = 0;
		this->header->payload_type = payload_type;
		this->header->length = htons(length);
		this->header->value = value;
	}

	size_t VbcmItem::Serialize(uint8_t* data)
	{
		MS_TRACE();

		// Add minimum header.
		std::memcpy(data, this->header, 8);

		// Copy the content.
		std::memcpy(data+8, this->header->value, this->header->length);

		size_t offset = 8+this->header->length;

		// 32 bits padding.
		size_t padding = (-offset) & 3;
		for (size_t i = 0; i < padding; i++)
		{
			data[offset+i] = 0;
		}

		return offset+padding;
	}

	void VbcmItem::Dump()
	{
		MS_TRACE();

		MS_DEBUG_DEV("<VbcmItem>");
		MS_DEBUG_DEV("  ssrc            : %" PRIu32, ntohl(this->header->ssrc));
		MS_DEBUG_DEV("  sequence number : %" PRIu8, this->header->sequence_number);
		MS_DEBUG_DEV("  payload type    : %" PRIu8, this->header->payload_type);
		MS_DEBUG_DEV("  length          : %" PRIu16, ntohs(this->header->length));
		MS_DEBUG_DEV("</Vbcm Item>");
	}
}}
