#ifndef MS_RTC_RTCP_FEEDBACK_RTP_HPP
#define MS_RTC_RTCP_FEEDBACK_RTP_HPP

#include "common.hpp"
#include "RTC/RTCP/Feedback.hpp"
#include <vector>

namespace RTC { namespace RTCP
{
	template<typename Item> class FeedbackRtpItemPacket
		: public FeedbackRtpPacket
	{
	public:
		typedef typename std::vector<Item*>::iterator Iterator;

	public:
		static FeedbackRtpItemPacket<Item>* Parse(const uint8_t* data, size_t len);

	public:
		// Parsed Report. Points to an external data.
		explicit FeedbackRtpItemPacket(CommonHeader* commonHeader);
		explicit FeedbackRtpItemPacket(uint32_t sender_ssrc, uint32_t media_ssrc = 0);
		virtual ~FeedbackRtpItemPacket() {};

		void AddItem(Item* item);
		Iterator Begin();
		Iterator End();

	/* Virtual methods inherited from FeedbackItem. */
	public:
		virtual void Dump() const override;
		virtual size_t Serialize(uint8_t* buffer) override;
		virtual size_t GetSize() const override;

	private:
		std::vector<Item*> items;
	};

	/* Inline instance methods. */

	template<typename Item>
	FeedbackRtpItemPacket<Item>::FeedbackRtpItemPacket(CommonHeader* commonHeader):
		FeedbackRtpPacket(commonHeader)
	{}

	template<typename Item>
	FeedbackRtpItemPacket<Item>::FeedbackRtpItemPacket(uint32_t sender_ssrc, uint32_t media_ssrc):
		FeedbackRtpPacket(Item::MessageType, sender_ssrc, media_ssrc)
	{}

	template<typename Item>
	size_t FeedbackRtpItemPacket<Item>::GetSize() const
	{
		size_t size = FeedbackRtpPacket::GetSize();

		for (auto item : this->items)
		{
			size += item->GetSize();
		}

		return size;
	}

	template<typename Item>
	void FeedbackRtpItemPacket<Item>::AddItem(Item* item)
	{
		this->items.push_back(item);
	}

	template<typename Item>
	typename FeedbackRtpItemPacket<Item>::Iterator FeedbackRtpItemPacket<Item>::Begin()
	{
		return this->items.begin();
	}

	template<typename Item>
	typename FeedbackRtpItemPacket<Item>::Iterator FeedbackRtpItemPacket<Item>::End()
	{
		return this->items.end();
	}
}}

#endif
