#define MS_CLASS "RTC::RTCP::FeedbackRtpTmmbPacket"

#include "RTC/RTCP/FeedbackRtpTmmb.h"
#include "Logger.h"

#include <cstring>  // std::memcmp(), std::memcpy()

namespace RTC
{
namespace RTCP
{

	/* TmmbItem Class methods. */

	TmmbItem* TmmbItem::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE_STD();

		// Get the header.
		Header* header = (Header*)data;

		// data size must be >= header + length value.
		if (sizeof(Header) > len)
		{
				MS_WARN("not enough space for Tmmb item, discarded");
				return nullptr;
		}

		std::auto_ptr<TmmbItem> item(new TmmbItem(header));

		if (!item->IsCorrect())
			return nullptr;

		return item.release();
	}

	/* TmmbItem Instance methods. */
	TmmbItem::TmmbItem(Header* header)
	{
		this->header = header;

		uint32_t compact = ntohl(header->compact);
		uint8_t exponent = compact >> 26;             /* first 6 bits */
		uint32_t mantissa = (compact >> 9) & 0x1ffff; /* next 17 bits */

		this->bitrate = (mantissa << exponent);
		this->overhead = compact & 0x1ff;             /* last 9 bits */

		if ((this->bitrate >> exponent) != mantissa) {
			MS_WARN("invalid TMMB bitrate value : %u *2^%u", mantissa, exponent);
			this->isCorrect = false;
		}
	}

	size_t TmmbItem::Serialize(uint8_t* data)
	{
		uint64_t mantissa = this->bitrate;
		uint32_t exponent = 0;
		while (mantissa > 0x1ffff /* max mantissa (17 bits) */) {
			mantissa >>= 1;
			++exponent;
		}

		uint32_t compact = (exponent << 26) | (mantissa << 9) | this->overhead;

		Header* header = (Header*) data;

		header->ssrc = this->header->ssrc;
		header->compact = htonl(compact);

		memcpy(data, header, sizeof(Header));

		return sizeof(Header);
	}

	void TmmbItem::Dump()
	{
		MS_TRACE_STD();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("\t\t<Tmmb Item>");
		MS_WARN("\t\t\tssrc: %u", ntohl(this->header->ssrc));
		MS_WARN("\t\t\tbitrate: %" PRIu64, this->bitrate);
		MS_WARN("\t\t\toverhead: %u", this->overhead);
		MS_WARN("\t\t</Tmmb Item>");
	}

/* FeedbackRtpTmmbPacket Class methods. */

	template <typename T>
	FeedbackRtpTmmbPacket<T>* FeedbackRtpTmmbPacket<T>::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE_STD();

		if (sizeof(CommonHeader) + sizeof(FeedbackPacket::Header) > len)
		{
				MS_WARN("not enough space for Feedback packet, discarded");
				return nullptr;
		}

		CommonHeader* commonHeader = (CommonHeader*)data;
		std::auto_ptr<FeedbackRtpTmmbPacket<T> > packet(new FeedbackRtpTmmbPacket<T>(commonHeader));

		size_t offset = sizeof(CommonHeader) + sizeof(FeedbackPacket::Header);

		while (len - offset > 0)
		{
			TmmbItem* item = TmmbItem::Parse(data+offset, len-offset);
			if (item) {
				packet->AddItem(item);
				offset += item->GetSize();
			} else {
				return packet.release();
			}
		}

		return packet.release();
	}

	/* FeedbackRtpTmmbPacket Instance methods. */

	template <typename T>
	FeedbackRtpTmmbPacket<T>::FeedbackRtpTmmbPacket(CommonHeader* commonHeader):
		FeedbackRtpPacket(commonHeader)
	{
	}

	template <typename T>
	FeedbackRtpTmmbPacket<T>::FeedbackRtpTmmbPacket(uint32_t sender_ssrc, uint32_t media_ssrc):
		FeedbackRtpPacket(MessageType, sender_ssrc, media_ssrc)
	{
	}

	template <typename T>
	size_t FeedbackRtpTmmbPacket<T>::Serialize(uint8_t* data)
	{
		MS_TRACE_STD();

		size_t offset = FeedbackPacket::Serialize(data);

		for(auto item : this->items) {
			offset += item->Serialize(data + offset);
		}

		return offset;
	}

	template <typename T>
	void FeedbackRtpTmmbPacket<T>::Dump()
	{
		MS_TRACE_STD();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("\t<%s>", Name.c_str());
		FeedbackRtpPacket::Dump();

		for (auto item : this->items) {
			item->Dump();
		}

		MS_WARN("\t</%s>", Name.c_str());
	}

	/* FeedbackRtpTmmbPacket specialization for Tmmbr class. */

	template<>
	const std::string FeedbackRtpTmmbPacket<Tmmbr>::Name = "FeedbackRtpTmmbrPacket";

	template<>
	const FeedbackRtp::MessageType FeedbackRtpTmmbPacket<Tmmbr>::MessageType = FeedbackRtp::TMMBR;

	/* FeedbackRtpTmmbPacket specialization for Tmmbn class. */

	template<>
	const std::string FeedbackRtpTmmbPacket<Tmmbn>::Name = "FeedbackRtpTmmbnPacket";

	template<>
	const FeedbackRtp::MessageType FeedbackRtpTmmbPacket<Tmmbn>::MessageType = FeedbackRtp::TMMBN;

	// explicit instantiation to have all FeedbackRtpTmmbPacket definitions in this file
	template class FeedbackRtpTmmbPacket<Tmmbr>;
	template class FeedbackRtpTmmbPacket<Tmmbn>;
}
}

