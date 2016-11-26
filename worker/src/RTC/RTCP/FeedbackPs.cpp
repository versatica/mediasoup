#define MS_CLASS "RTC::RTCP::FeedbackPsPacket"

#include "RTC/RTCP/FeedbackPs.h"
#include "RTC/RTCP/FeedbackPsSli.h"
#include "RTC/RTCP/FeedbackPsRpsi.h"
#include "RTC/RTCP/FeedbackPsFir.h"
#include "RTC/RTCP/FeedbackPsTst.h"
#include "RTC/RTCP/FeedbackPsVbcm.h"
#include "RTC/RTCP/FeedbackPsLei.h"
#include "Logger.h"

namespace RTC { namespace RTCP
{

/* FeedbackPsItemPacket Class methods. */
	template<typename Item>
	FeedbackPsItemPacket<Item>* FeedbackPsItemPacket<Item>::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE_STD();

		if (sizeof(CommonHeader) + sizeof(FeedbackPacket::Header) > len)
		{
				MS_WARN("not enough space for Feedback packet, discarded");
				return nullptr;
		}

		CommonHeader* commonHeader = (CommonHeader*)data;
		std::auto_ptr<FeedbackPsItemPacket<Item> > packet(new FeedbackPsItemPacket<Item>(commonHeader));

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

	/* FeedbackPsItemPacket Instance methods. */

	template<typename Item>
	size_t FeedbackPsItemPacket<Item>::Serialize(uint8_t* data)
	{
		MS_TRACE_STD();

		size_t offset = FeedbackPacket::Serialize(data);

		for(auto item : this->items)
		{
			offset += item->Serialize(data + offset);
		}

		return offset;
	}

	template<typename Item>
	void FeedbackPsItemPacket<Item>::Dump()
	{
		MS_TRACE_STD();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("\t<%s>", FeedbackPsPacket::MessageType2String(Item::MessageType).c_str());
		FeedbackPsPacket::Dump();

		for (auto item : this->items)
		{
			item->Dump();
		}

		MS_WARN("\t<%s>", FeedbackPsPacket::MessageType2String(Item::MessageType).c_str());
	}

	// explicit instantiation to have all FeedbackRtpPacket definitions in this file
	template class FeedbackPsItemPacket<FirItem>;
	template class FeedbackPsItemPacket<SliItem>;
	template class FeedbackPsItemPacket<RpsiItem>;
	template class FeedbackPsItemPacket<TstrItem>;
	template class FeedbackPsItemPacket<TstnItem>;
	template class FeedbackPsItemPacket<VbcmItem>;
	template class FeedbackPsItemPacket<PsLeiItem>;

} } // RTP::RTCP

