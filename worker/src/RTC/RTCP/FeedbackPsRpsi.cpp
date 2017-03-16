#define MS_CLASS "RTC::RTCP::FeedbackPsRpsi"
// #define MS_LOG_DEV

#include "RTC/RTCP/FeedbackPsRpsi.hpp"
#include "Logger.hpp"
#include <cstring>

namespace RTC { namespace RTCP
{
	/* Class methods. */

	FeedbackPsRpsiItem* FeedbackPsRpsiItem::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// data size must be >= header.
		if (sizeof(Header) > len)
		{
			MS_WARN_TAG(rtcp, "not enough space for Rpsi item, discarded");

			return nullptr;
		}

		Header* header = const_cast<Header*>(reinterpret_cast<const Header*>(data));
		std::unique_ptr<FeedbackPsRpsiItem> item(new FeedbackPsRpsiItem(header));

		if (item->IsCorrect())
			return item.release();
		else
			return nullptr;
	}

	/* Instance methods. */

	FeedbackPsRpsiItem::FeedbackPsRpsiItem(Header* header)
	{
		MS_TRACE();

		this->header = header;

		// Calculate bit_string length.
		if (this->header->padding_bits % 8 != 0)
		{
			MS_WARN_TAG(rtcp, "invalid Rpsi packet with fractional padding bytes value");

			isCorrect = false;
		}

		size_t paddingBytes = this->header->padding_bits / 8;

		if (paddingBytes > FeedbackPsRpsiItem::MaxBitStringSize)
		{
			MS_WARN_TAG(rtcp, "invalid Rpsi packet with too many padding bytes");

			isCorrect = false;
		}

		this->length = FeedbackPsRpsiItem::MaxBitStringSize - paddingBytes;
	}

	FeedbackPsRpsiItem::FeedbackPsRpsiItem(uint8_t payload_type, uint8_t* bit_string, size_t length)
	{
		MS_TRACE();

		MS_ASSERT(payload_type <= 0x7f, "rpsi payload type exceeds the maximum value");
		MS_ASSERT(length <= FeedbackPsRpsiItem::MaxBitStringSize, "rpsi bit string length exceeds the maximum value");

		this->raw = new uint8_t[sizeof(Header)];
		this->header = reinterpret_cast<Header*>(this->raw);

		// 32 bits padding.
		size_t padding = (-length) & 3;

		this->header->padding_bits = padding * 8;
		this->header->zero = 0;
		std::memcpy(this->header->bit_string, bit_string, length);

		// Fill padding.
		for (size_t i = 0; i < padding; ++i)
		{
			this->raw[sizeof(Header)+i-1] = 0;
		}
	}

	size_t FeedbackPsRpsiItem::Serialize(uint8_t* buffer)
	{
		MS_TRACE();

		std::memcpy(buffer, this->header, sizeof(Header));

		return sizeof(Header);
	}

	void FeedbackPsRpsiItem::Dump() const
	{
		MS_TRACE();

		MS_DUMP("<FeedbackPsRpsiItem>");
		MS_DUMP("  padding bits : %" PRIu8, this->header->padding_bits);
		MS_DUMP("  payload type : %" PRIu8, this->GetPayloadType());
		MS_DUMP("  length       : %zu", this->GetLength());
		MS_DUMP("</FeedbackPsRpsiItem>");
	}
}}
