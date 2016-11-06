#define MS_CLASS "RTC::RTCP::FeedbackRtpPacket"

#include "RTC/RTCP/FeedbackRtp.h"
#include "RTC/RTCP/FeedbackRtpNack.h"
#include "RTC/RTCP/FeedbackRtpTmmb.h"
#include "RTC/RTCP/FeedbackRtpTllei.h"
#include "Logger.h"

namespace RTC { namespace RTCP
{

/* FeedbackRtpItemPacket Class methods. */
	template<typename Item>
	FeedbackRtpItemPacket<Item>* FeedbackRtpItemPacket<Item>::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE_STD();

		if (sizeof(CommonHeader) + sizeof(FeedbackPacket::Header) > len)
		{
				MS_WARN("not enough space for Feedback packet, discarded");
				return nullptr;
		}

		CommonHeader* commonHeader = (CommonHeader*)data;
		std::auto_ptr<FeedbackRtpItemPacket<Item> > packet(new FeedbackRtpItemPacket<Item>(commonHeader));

		size_t offset = sizeof(CommonHeader) + sizeof(FeedbackPacket::Header);

		while (len - offset > 0)
		{
			Item* item = Item::Parse(data+offset, len-offset);
			if (item) {
				packet->AddItem(item);
				offset += item->GetSize();
			} else {
				break;
			}
		}

		return packet.release();
	}

	/* FeedbackRtpItemPacket Instance methods. */

	template<typename Item>
	size_t FeedbackRtpItemPacket<Item>::Serialize(uint8_t* data)
	{
		MS_TRACE_STD();

		size_t offset = FeedbackPacket::Serialize(data);

		for(auto item : this->items) {
			offset += item->Serialize(data + offset);
		}

		return offset;
	}

	template<typename Item>
	void FeedbackRtpItemPacket<Item>::Dump()
	{
		MS_TRACE_STD();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("\t<%s>", FeedbackRtpPacket::MessageType2String(Item::MessageType).c_str());
		FeedbackRtpPacket::Dump();

		for (auto item : this->items) {
			item->Dump();
		}

		MS_WARN("\t<%s>", FeedbackRtpPacket::MessageType2String(Item::MessageType).c_str());
	}

	// explicit instantiation to have all FeedbackRtpPacket definitions in this file
	template class FeedbackRtpItemPacket<NackItem>;
	template class FeedbackRtpItemPacket<TmmbrItem>;
	template class FeedbackRtpItemPacket<TmmbnItem>;
	template class FeedbackRtpItemPacket<TlleiItem>;

} } // RTP::RTCP
