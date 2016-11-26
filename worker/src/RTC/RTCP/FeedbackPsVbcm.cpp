#define MS_CLASS "RTC::RTCP::FeedbackPsVbcmPacket"

#include "RTC/RTCP/FeedbackPsVbcm.h"
#include "Logger.h"

#include <cstring>  // std::memcmp(), std::memcpy()

namespace RTC { namespace RTCP
{

	/* VbcmItem Class methods. */

	VbcmItem* VbcmItem::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE_STD();

		// data size must be >= header + length value.
		if (sizeof(Header) > len)
		{
				MS_WARN("not enough space for Vbcm item, discarded");
				return nullptr;
		}

		Header* header = (Header*)data;
		return new VbcmItem(header);
	}

	/* VbcmItem Instance methods. */
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
		MS_TRACE_STD();

		// Add minimum header.
		std::memcpy(data, this->header, 8);

		// Copy the content.
		std::memcpy(data+8, this->header->value, this->header->length);

		size_t offset = 8+this->header->length;

		// 32 bits padding
		size_t padding = (-offset) & 3;
		for (size_t i = 0; i < padding; i++)
		{
			data[offset+i] = 0;
		}

		return offset+padding;
	}

	void VbcmItem::Dump()
	{
		MS_TRACE_STD();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("\t\t<Vbcm Item>");
		MS_WARN("\t\t\tssrc: %u", ntohl(this->header->ssrc));
		MS_WARN("\t\t\tsequence_number: %u", this->header->sequence_number);
		MS_WARN("\t\t\tpayload type: %u", this->header->payload_type);
		MS_WARN("\t\t\tlength: %u", ntohs(this->header->length));
		MS_WARN("\t\t</Vbcm Item>");
	}

} } // RTP::RTCP

