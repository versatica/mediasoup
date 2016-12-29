#define MS_CLASS "RTC::RTCP::FeedbackPsRpsiPacket"

#include "RTC/RTCP/FeedbackPsRpsi.h"
#include "Logger.h"
#include <cstring>

namespace RTC { namespace RTCP
{
	/* Class methods. */

	RpsiItem* RpsiItem::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// data size must be >= header.
		if (sizeof(Header) > len)
		{
			MS_WARN("not enough space for Rpsi item, discarded");
			return nullptr;
		}

		Header* header = (Header*)data;
		std::auto_ptr<RpsiItem> item(new RpsiItem(header));

		if (item->IsCorrect())
			return item.release();
		else
			return nullptr;
	}

	/* Instance methods. */

	RpsiItem::RpsiItem(Header* header)
	{
		MS_TRACE();

		this->header = header;

		// Calculate bit_string length.
		if (this->header->padding_bits % 8 != 0) {
			MS_WARN("invalid Rpsi packet with fractional padding bytes value");
			isCorrect = false;
		}

		size_t paddingBytes = this->header->padding_bits / 8;

		if (paddingBytes > RpsiItem::MaxBitStringSize) {
			MS_WARN("invalid Rpsi packet with too many padding bytes");
			isCorrect = false;
		}

		this->length = RpsiItem::MaxBitStringSize - paddingBytes;
	}

	RpsiItem::RpsiItem(uint8_t payload_type, uint8_t* bit_string, size_t length)
	{
		MS_TRACE();

		MS_ASSERT(payload_type <= 0x7f, "rpsi payload type exceeds the maximum value");
		MS_ASSERT(length <= RpsiItem::MaxBitStringSize, "rpsi bit string length exceeds the maximum value");

		this->raw = new uint8_t[sizeof(Header)];
		this->header = (Header*) this->raw;

		// 32 bits padding.
		size_t padding = (-length) & 3;

		this->header->padding_bits = padding * 8;
		this->header->zero = 0;
		std::memcpy(this->header->bit_string, bit_string, length);

		// Fill padding.
		for (size_t i = 0; i < padding; i++)
		{
			this->raw[sizeof(Header)+i-1] = 0;
		}
	}

	size_t RpsiItem::Serialize(uint8_t* data)
	{
		MS_TRACE();

		std::memcpy(data, this->header, sizeof(Header));

		return sizeof(Header);
	}

	void RpsiItem::Dump()
	{
		MS_TRACE();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("\t\t<Rpsi Item>");
		MS_WARN("\t\t\tpadding bits: %u", this->header->padding_bits);
		MS_WARN("\t\t\tpayload_type: %u", this->header->payload_type);
		MS_WARN("\t\t\tlength: %zu", this->length);
		MS_WARN("\t\t</Rpsi Item>");
	}
}}
