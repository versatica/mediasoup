#define MS_CLASS "RTC::RTCP::FeedbackPsRpsiPacket"

#include "RTC/RTCP/FeedbackPsRpsi.h"
#include "Logger.h"

#include <cstring>  // std::memcmp(), std::memcpy()

namespace RTC
{
namespace RTCP
{

	/* RpsiItem Class methods. */

	RpsiItem* RpsiItem::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE_STD();

		// Get the header.
		Header* header = (Header*)data;

		// data size must be >= header + length value.
		if (sizeof(Header) > len)
		{
				MS_WARN("not enough space for Rpsi item, discarded");
				return nullptr;
		}

		std::auto_ptr<RpsiItem> item(new RpsiItem(header));

		if (item->IsCorrect())
			return item.release();
		else
			return nullptr;
	}

	/* RpsiItem Instance methods. */
	RpsiItem::RpsiItem(Header* header)
	{
		MS_TRACE_STD();

		this->header = header;

		// calculate bitString length
		if (this->header->pb % 8 != 0) {
			MS_WARN("invalid Rpsi packet with fractional padding bytes value");
			isCorrect = false;
		}

		size_t paddingBytes = this->header->pb / 8;
		if (paddingBytes > MaxBitStringSize) {
			MS_WARN("invalid Rpsi packet with too many padding bytes");
			isCorrect = false;
		}

		this->length = MaxBitStringSize - paddingBytes;
	}

	RpsiItem::RpsiItem(uint8_t payloadType, uint8_t* bitString, size_t length)
	{
		MS_TRACE_STD();

		MS_ASSERT(payloadType <= 0x7f, "rpsi payload type exceeds the maximum value");
		MS_ASSERT(length <= MaxBitStringSize, "rpsi bit string length exceeds the maximum value");

		this->raw = new uint8_t[sizeof(Header)];
		this->header = (Header*) this->raw;

		// 32 bits padding
		size_t padding = (-length) & 3;

		this->header->pb = padding * 8;
		this->header->zero = 0;
		memcpy(this->header->bitString, bitString, length);

		// fill padding
		for (size_t i = 0; i < padding; i++)
		{
			this->raw[sizeof(Header)+i-1] = 0;
		}
	}

	RpsiItem::~RpsiItem()
	{
		if (this->raw)
			delete this->raw;
	}

	void RpsiItem::Serialize()
	{
		MS_TRACE_STD();

		if (!this->raw)
			this->raw = new uint8_t[sizeof(Header)];

		this->Serialize(this->raw);
	}

	size_t RpsiItem::Serialize(uint8_t* data)
	{
		MS_TRACE_STD();

		memcpy(data, this->header, sizeof(Header));
		return sizeof(Header);
	}

	void RpsiItem::Dump()
	{
		MS_TRACE_STD();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("\t\t<Rpsi Item>");
		MS_WARN("\t\t\tpadding bits: %u", this->header->pb);
		MS_WARN("\t\t\tpayloadType: %u", this->header->payloadType);
		MS_WARN("\t\t\tlength: %zu", this->length);
		MS_WARN("\t\t</Rpsi Item>");
	}

/* FeedbackPsRpsiPacket Class methods. */

	FeedbackPsRpsiPacket* FeedbackPsRpsiPacket::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE_STD();

		if (sizeof(CommonHeader) + sizeof(FeedbackPacket::Header) > len)
		{
				MS_WARN("not enough space for Feedback packet, discarded");
				return nullptr;
		}

		CommonHeader* commonHeader = (CommonHeader*)data;
		std::auto_ptr<FeedbackPsRpsiPacket> packet(new FeedbackPsRpsiPacket(commonHeader));

		size_t offset = sizeof(CommonHeader) + sizeof(FeedbackPacket::Header);

		RpsiItem* item = RpsiItem::Parse(data+offset, len-offset);
		if (item) {
			packet->SetItem(item);
			return packet.release();
		} else {
			return nullptr;
		}
	}

	/* FeedbackPsRpsiPacket Instance methods. */

	FeedbackPsRpsiPacket::FeedbackPsRpsiPacket(CommonHeader* commonHeader):
		FeedbackPsPacket(commonHeader)
	{
	}

	FeedbackPsRpsiPacket::FeedbackPsRpsiPacket(uint32_t sender_ssrc, uint32_t media_ssrc):
		FeedbackPsPacket(FeedbackPs::RPSI, sender_ssrc, media_ssrc)
	{
	}

	size_t FeedbackPsRpsiPacket::Serialize(uint8_t* data)
	{
		MS_TRACE_STD();

		size_t offset = FeedbackPacket::Serialize(data);

		offset += item->Serialize(data + offset);

		return offset;
	}

	void FeedbackPsRpsiPacket::Dump()
	{
		MS_TRACE_STD();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("\t<FeedbackPsRpsiPacket>");
		FeedbackPsPacket::Dump();

		item->Dump();

		MS_WARN("\t</FeedbackPsRpsiPacket>");
	}
}
}

