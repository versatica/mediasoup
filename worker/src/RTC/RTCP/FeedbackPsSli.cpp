#define MS_CLASS "RTC::RTCP::FeedbackPsSliPacket"

#include "RTC/RTCP/FeedbackPsSli.h"
#include "Logger.h"

#include <cstring>  // std::memcmp(), std::memcpy()

namespace RTC
{
namespace RTCP
{

	/* SliItem Class methods. */

	SliItem* SliItem::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE_STD();

		// Get the header.
		Header* header = (Header*)data;

		// data size must be >= header + length value.
		if (sizeof(Header) > len)
		{
				MS_WARN("not enough space for Tmmbn item, discarded");
				return nullptr;
		}

		std::auto_ptr<SliItem> item(new SliItem(header));
		return item.release();
	}

	/* SliItem Instance methods. */
	SliItem::SliItem(Header* header)
	{
		this->header = header;

		uint32_t compact = ntohl(header->compact);

		this->first = compact >> 19;                  /* first 13 bits */
		this->number = (compact >> 6) & 0x1fff;       /* next  13 bits */
		this->pictureId = compact & 0x3f;             /* last   6 bits */
	}

	size_t SliItem::Serialize(uint8_t* data)
	{
		uint32_t compact = (this->first << 19) | (this->number << 6) | this->pictureId;

		Header* header = (Header*) data;

		header->compact = htonl(compact);

		memcpy(data, header, sizeof(Header));

		return sizeof(Header);
	}

	void SliItem::Dump()
	{
		MS_TRACE_STD();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("\t\t<Sli Item>");
		MS_WARN("\t\t\tfirst: %u", this->first);
		MS_WARN("\t\t\tnumber: %u", this->number);
		MS_WARN("\t\t\tpictureId: %u", this->pictureId);
		MS_WARN("\t\t</Sli Item>");
	}

/* FeedbackPsSliPacket Class methods. */

	FeedbackPsSliPacket* FeedbackPsSliPacket::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE_STD();

		if (sizeof(CommonHeader) + sizeof(FeedbackPacket::Header) > len)
		{
				MS_WARN("not enough space for Feedback packet, discarded");
				return nullptr;
		}

		CommonHeader* commonHeader = (CommonHeader*)data;
		std::auto_ptr<FeedbackPsSliPacket> packet(new FeedbackPsSliPacket(commonHeader));

		size_t offset = sizeof(CommonHeader) + sizeof(FeedbackPacket::Header);

		while (len - offset > 0)
		{
			SliItem* item = SliItem::Parse(data+offset, len-offset);
			if (item) {
				packet->AddItem(item);
				offset += item->GetSize();
			} else {
				break;;
			}
		}

		// There must be at least one Item
		if (packet->Begin() != packet->End())
			return packet.release();
		else
			return nullptr;
	}

	/* FeedbackPsSliPacket Instance methods. */

	FeedbackPsSliPacket::FeedbackPsSliPacket(CommonHeader* commonHeader):
		FeedbackPsPacket(commonHeader)
	{
	}

	FeedbackPsSliPacket::FeedbackPsSliPacket(uint32_t sender_ssrc, uint32_t media_ssrc):
		FeedbackPsPacket(FeedbackPs::PLI, sender_ssrc, media_ssrc)
	{
	}

	size_t FeedbackPsSliPacket::Serialize(uint8_t* data)
	{
		MS_TRACE_STD();

		size_t offset = FeedbackPacket::Serialize(data);

		for(auto item : this->items) {
			offset += item->Serialize(data + offset);
		}

		return offset;
	}

	void FeedbackPsSliPacket::Dump()
	{
		MS_TRACE_STD();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("\t<FeedbackPsSliPacket>");
		FeedbackPsPacket::Dump();

		for (auto item : this->items) {
			item->Dump();
		}

		MS_WARN("\t</FeedbackPsSliPacket>");
	}
}
}

