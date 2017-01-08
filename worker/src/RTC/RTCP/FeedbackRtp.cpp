#define MS_CLASS "RTC::RTCP::FeedbackRtpPacket"
// #define MS_LOG_DEV

#include "RTC/RTCP/FeedbackRtp.h"
#include "RTC/RTCP/FeedbackRtpNack.h"
#include "RTC/RTCP/FeedbackRtpTmmb.h"
#include "RTC/RTCP/FeedbackRtpTllei.h"
#include "RTC/RTCP/FeedbackRtpEcn.h"
#include "Logger.h"

namespace RTC { namespace RTCP
{
	/* Class methods. */

	template<typename Item>
	FeedbackRtpItemPacket<Item>* FeedbackRtpItemPacket<Item>::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (sizeof(CommonHeader) + sizeof(FeedbackPacket::Header) > len)
		{
			MS_WARN_TAG(rtcp, "not enough space for Feedback packet, discarded");

			return nullptr;
		}

		CommonHeader* commonHeader = (CommonHeader*)data;
		std::unique_ptr<FeedbackRtpItemPacket<Item> > packet(new FeedbackRtpItemPacket<Item>(commonHeader));

		size_t offset = sizeof(CommonHeader) + sizeof(FeedbackPacket::Header);

		while (len - offset > 0)
		{
			Item* item = Item::Parse(data+offset, len-offset);

			if (item)
			{
				packet->AddItem(item);
				offset += item->GetSize();
			}
			else
			{
				break;
			}
		}

		return packet.release();
	}

	/* Instance methods. */

	template<typename Item>
	size_t FeedbackRtpItemPacket<Item>::Serialize(uint8_t* buffer)
	{
		MS_TRACE();

		size_t offset = FeedbackPacket::Serialize(buffer);

		for (auto item : this->items)
		{
			offset += item->Serialize(buffer + offset);
		}

		return offset;
	}

	template<typename Item>
	void FeedbackRtpItemPacket<Item>::Dump()
	{
		#ifdef MS_LOG_DEV

		MS_TRACE();

		MS_DEBUG_DEV("<%s>", FeedbackRtpPacket::MessageType2String(Item::MessageType).c_str());
		FeedbackRtpPacket::Dump();
		for (auto item : this->items)
		{
			item->Dump();
		}
		MS_DEBUG_DEV("</%s>", FeedbackRtpPacket::MessageType2String(Item::MessageType).c_str());

		#endif
	}

	// Explicit instantiation to have all FeedbackRtpPacket definitions in this file.
	template class FeedbackRtpItemPacket<NackItem>;
	template class FeedbackRtpItemPacket<TmmbrItem>;
	template class FeedbackRtpItemPacket<TmmbnItem>;
	template class FeedbackRtpItemPacket<TlleiItem>;
	template class FeedbackRtpItemPacket<EcnItem>;
}}
