#define MS_CLASS "RTC::RTCP::FeedbackPsTstPacket"

#include "RTC/RTCP/FeedbackPsTst.h"
#include "Logger.h"

#include <cstring>  // std::memcmp(), std::memcpy()

namespace RTC
{
namespace RTCP
{

	/* TstItem Class methods. */

	TstItem* TstItem::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE_STD();

		// Get the header.
		Header* header = (Header*)data;

		// data size must be >= header + length value.
		if (sizeof(Header) > len)
		{
				MS_WARN("not enough space for Tst item, discarded");
				return nullptr;
		}

		std::auto_ptr<TstItem> item(new TstItem(header));
		return item.release();
	}

	/* TstItem Instance methods. */
	TstItem::TstItem(Header* header):
		header(header)
	{
		MS_TRACE_STD();

	}

	TstItem::TstItem(uint32_t ssrc, uint8_t seqnr, uint8_t index)
	{
		MS_TRACE_STD();

		this->raw = new uint8_t[sizeof(Header)];
		this->header = (Header*) this->raw;

		// set reserved bits to zero
		std::memset(this->header, 0, sizeof(Header));

		this->header->ssrc = htonl(ssrc);
		this->header->seqnr = seqnr;
		this->header->index = index;
	}

	TstItem::~TstItem()
	{
		if (this->raw)
			delete this->raw;
	}

	void TstItem::Serialize()
	{
		MS_TRACE_STD();

		if (!this->raw)
			this->raw = new uint8_t[sizeof(Header)];

		this->Serialize(this->raw);
	}

	size_t TstItem::Serialize(uint8_t* data)
	{
		MS_TRACE_STD();

		memcpy(data, this->header, sizeof(Header));
		return sizeof(Header);
	}

	void TstItem::Dump()
	{
		MS_TRACE_STD();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("\t\t<Tst Item>");
		MS_WARN("\t\t\tssrc: %u", ntohl(this->header->ssrc));
		MS_WARN("\t\t\tseqnr: %u", this->header->seqnr);
		MS_WARN("\t\t\tindex: %u", this->header->index);
		MS_WARN("\t\t</Tst Item>");
	}

/* FeedbackPsTstPacket Class methods. */

	template <typename T>
	FeedbackPsTstPacket<T>* FeedbackPsTstPacket<T>::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE_STD();

		if (sizeof(CommonHeader) + sizeof(FeedbackPacket::Header) > len)
		{
				MS_WARN("not enough space for Feedback packet, discarded");
				return nullptr;
		}

		CommonHeader* commonHeader = (CommonHeader*)data;
		std::auto_ptr<FeedbackPsTstPacket<T> > packet(new FeedbackPsTstPacket<T>(commonHeader));

		size_t offset = sizeof(CommonHeader) + sizeof(FeedbackPacket::Header);

		while (len - offset > 0)
		{
			TstItem* item = TstItem::Parse(data+offset, len-offset);
			if (item) {
				packet->AddItem(item);
				offset += item->GetSize();
			} else {
				return packet.release();
			}
		}

		return packet.release();
	}

	/* FeedbackPsTstPacket Instance methods. */

	template <typename T>
	FeedbackPsTstPacket<T>::FeedbackPsTstPacket(CommonHeader* commonHeader):
		FeedbackPsPacket(commonHeader)
	{
	}

	template <typename T>
	FeedbackPsTstPacket<T>::FeedbackPsTstPacket(uint32_t sender_ssrc, uint32_t media_ssrc):
		FeedbackPsPacket(MessageType, sender_ssrc, media_ssrc)
	{
	}

	template <typename T>
	size_t FeedbackPsTstPacket<T>::Serialize(uint8_t* data)
	{
		MS_TRACE_STD();

		size_t offset = FeedbackPacket::Serialize(data);

		for(auto item : this->items) {
			offset += item->Serialize(data + offset);
		}

		return offset;
	}

	template <typename T>
	void FeedbackPsTstPacket<T>::Dump()
	{
		MS_TRACE_STD();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("\t<%s>", Name.c_str());
		FeedbackPsPacket::Dump();

		for (auto item : this->items) {
			item->Dump();
		}

		MS_WARN("\t</%s>", Name.c_str());
	}

	/* FeedbackPsTstPacket specialization for Tstr class. */

	template<>
	const std::string FeedbackPsTstPacket<Tstr>::Name = "FeedbackPsTstrPacket";

	template<>
	const FeedbackPs::MessageType FeedbackPsTstPacket<Tstr>::MessageType = FeedbackPs::TSTR;

	/* FeedbackPsTstPacket specialization for Tstn class. */

	template<>
	const std::string FeedbackPsTstPacket<Tstn>::Name = "FeedbackPsTstnPacket";

	template<>
	const FeedbackPs::MessageType FeedbackPsTstPacket<Tstn>::MessageType = FeedbackPs::TSTN;

	// explicit instantiation to have all FeedbackPsTstPacket definitions in this file
	template class FeedbackPsTstPacket<Tstr>;
	template class FeedbackPsTstPacket<Tstn>;
}
}

