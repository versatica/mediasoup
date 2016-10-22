#define MS_CLASS "RTC::RTCP::FeedbackPsFirPacket"

#include "RTC/RTCP/FeedbackPsFir.h"
#include "Logger.h"

#include <cstring>  // std::memcmp(), std::memcpy()
#include <string.h> // memset()

namespace RTC
{
namespace RTCP
{

	/* FirItem Class methods. */

	FirItem* FirItem::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE_STD();

		// Get the header.
		Header* header = (Header*)data;

		// data size must be >= header + length value.
		if (sizeof(Header) > len)
		{
				MS_WARN("not enough space for Fir item, discarded");
				return nullptr;
		}

		std::auto_ptr<FirItem> item(new FirItem(header));
		return item.release();
	}

	/* FirItem Instance methods. */
	FirItem::FirItem(Header* header):
		header(header)
	{
		MS_TRACE_STD();

	}

	FirItem::FirItem(uint32_t ssrc, uint8_t seqnr)
	{
		MS_TRACE_STD();

		this->raw = new uint8_t[sizeof(Header)];
		this->header = (Header*) this->raw;

		// set reserved bits to zero
		std::memset(this->header, 0, sizeof(Header));

		this->header->ssrc = htonl(ssrc);
		this->header->seqnr = seqnr;
	}

	FirItem::~FirItem()
	{
		if (this->raw)
			delete this->raw;
	}

	void FirItem::Serialize()
	{
		MS_TRACE_STD();

		if (!this->raw)
			this->raw = new uint8_t[sizeof(Header)];

		this->Serialize(this->raw);
	}

	size_t FirItem::Serialize(uint8_t* data)
	{
		MS_TRACE_STD();

		memcpy(data, this->header, sizeof(Header));
		return sizeof(Header);
	}

	void FirItem::Dump()
	{
		MS_TRACE_STD();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("\t\t<Fir Item>");
		MS_WARN("\t\t\tssrc: %u", ntohl(this->header->ssrc));
		MS_WARN("\t\t\tseqnr: %u", this->header->seqnr);
		MS_WARN("\t\t</Fir Item>");
	}

/* FeedbackPsFirPacket Class methods. */

	FeedbackPsFirPacket* FeedbackPsFirPacket::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE_STD();

		if (sizeof(CommonHeader) + sizeof(FeedbackPacket::Header) > len)
		{
				MS_WARN("not enough space for Feedback packet, discarded");
				return nullptr;
		}

		CommonHeader* commonHeader = (CommonHeader*)data;
		std::auto_ptr<FeedbackPsFirPacket> packet(new FeedbackPsFirPacket(commonHeader));

		size_t offset = sizeof(CommonHeader) + sizeof(FeedbackPacket::Header);

		while (len - offset > 0)
		{
			FirItem* item = FirItem::Parse(data+offset, len-offset);
			if (item) {
				packet->AddItem(item);
				offset += item->GetSize();
			} else {
				return packet.release();
			}
		}

		return packet.release();
	}

	/* FeedbackPsFirPacket Instance methods. */

	FeedbackPsFirPacket::FeedbackPsFirPacket(CommonHeader* commonHeader):
		FeedbackPsPacket(commonHeader)
	{
	}

	FeedbackPsFirPacket::FeedbackPsFirPacket(uint32_t sender_ssrc, uint32_t media_ssrc):
		FeedbackPsPacket(FeedbackPs::FIR, sender_ssrc, media_ssrc)
	{
	}

	size_t FeedbackPsFirPacket::Serialize(uint8_t* data)
	{
		MS_TRACE_STD();

		size_t offset = FeedbackPacket::Serialize(data);

		for (auto item : this->items) {
			offset += item->Serialize(data + offset);
		}

		return offset;
	}

	void FeedbackPsFirPacket::Dump()
	{
		MS_TRACE_STD();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("\t<FeedbackPsFirPacket>");
		FeedbackPsPacket::Dump();

		for (auto item : this->items) {
			item->Dump();
		}

		MS_WARN("\t</FeedbackPsFirPacket>");
	}
}
}

