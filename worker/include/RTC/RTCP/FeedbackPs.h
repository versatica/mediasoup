#ifndef MS_RTC_RTCP_FEEDBACK_PS_H
#define MS_RTC_RTCP_FEEDBACK_PS_H

#include "common.h"
#include "RTC/RTCP/Feedback.h"
#include <vector>

namespace RTC { namespace RTCP
{
	template<typename Item> class FeedbackPsItemPacket
		: public FeedbackPsPacket
	{
	public:
		typedef typename std::vector<Item*>::iterator Iterator;

	public:
		static FeedbackPsItemPacket<Item>* Parse(const uint8_t* data, size_t len);

	public:
		// Parsed Report. Points to an external data.
		explicit FeedbackPsItemPacket(CommonHeader* commonHeader);
		FeedbackPsItemPacket(uint32_t sender_ssrc, uint32_t media_ssrc = 0);

		void AddItem(Item* item);
		Iterator Begin();
		Iterator End();

	/* Pure virtual methods inherited from Packet. */
	public:
		virtual void Dump() override;
		virtual size_t Serialize(uint8_t* buffer) override;
		virtual size_t GetSize() override;

	private:
		std::vector<Item*> items;
	};

	/* Inline instance methods. */

	template<typename Item>
	FeedbackPsItemPacket<Item>::FeedbackPsItemPacket(CommonHeader* commonHeader):
		FeedbackPsPacket(commonHeader)
	{}

	template<typename Item>
	FeedbackPsItemPacket<Item>::FeedbackPsItemPacket(uint32_t sender_ssrc, uint32_t media_ssrc):
		FeedbackPsPacket(Item::MessageType, sender_ssrc, media_ssrc)
	{}

	template<typename Item>
	size_t FeedbackPsItemPacket<Item>::GetSize()
	{
		size_t size = FeedbackPsPacket::GetSize();

		for (auto item : this->items)
		{
			size += item->GetSize();
		}

		return size;
	}

	template<typename Item>
	void FeedbackPsItemPacket<Item>::AddItem(Item* item)
	{
		this->items.push_back(item);
	}

	template<typename Item>
	typename FeedbackPsItemPacket<Item>::Iterator FeedbackPsItemPacket<Item>::Begin()
	{
		return this->items.begin();
	}

	template<typename Item>
	typename FeedbackPsItemPacket<Item>::Iterator FeedbackPsItemPacket<Item>::End()
	{
		return this->items.end();
	}
}}

#endif
