#define MS_CLASS "RTC::RTCP::FeedbackPsPacket"
// #define MS_LOG_DEV

#include "RTC/RTCP/FeedbackPs.hpp"
#include "RTC/RTCP/FeedbackPsSli.hpp"
#include "RTC/RTCP/FeedbackPsRpsi.hpp"
#include "RTC/RTCP/FeedbackPsFir.hpp"
#include "RTC/RTCP/FeedbackPsTst.hpp"
#include "RTC/RTCP/FeedbackPsVbcm.hpp"
#include "RTC/RTCP/FeedbackPsLei.hpp"
#include "Logger.hpp"

namespace RTC { namespace RTCP
{
	/* Class methods. */

	template<typename Item>
	FeedbackPsItemPacket<Item>* FeedbackPsItemPacket<Item>::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (sizeof(CommonHeader) + sizeof(FeedbackPacket::Header) > len)
		{
			MS_WARN_TAG(rtcp, "not enough space for Feedback packet, discarded");

			return nullptr;
		}

		CommonHeader* commonHeader = (CommonHeader*)data;
		std::unique_ptr<FeedbackPsItemPacket<Item>> packet(new FeedbackPsItemPacket<Item>(commonHeader));

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
	size_t FeedbackPsItemPacket<Item>::Serialize(uint8_t* buffer)
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
	void FeedbackPsItemPacket<Item>::Dump()
	{
		#ifdef MS_LOG_DEV

		MS_TRACE();

		MS_DEBUG_DEV("<%s>", FeedbackPsPacket::MessageType2String(Item::MessageType).c_str());
		FeedbackPsPacket::Dump();
		for (auto item : this->items)
		{
			item->Dump();
		}
		MS_DEBUG_DEV("</%s>", FeedbackPsPacket::MessageType2String(Item::MessageType).c_str());

		#endif
	}

	// explicit instantiation to have all FeedbackRtpPacket definitions in this file.
	template class FeedbackPsItemPacket<FirItem>;
	template class FeedbackPsItemPacket<SliItem>;
	template class FeedbackPsItemPacket<RpsiItem>;
	template class FeedbackPsItemPacket<TstrItem>;
	template class FeedbackPsItemPacket<TstnItem>;
	template class FeedbackPsItemPacket<VbcmItem>;
	template class FeedbackPsItemPacket<PsLeiItem>;
}}
