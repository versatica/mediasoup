#ifndef MS_RTC_RTCP_FEEDBACK_RTP_HPP
#define MS_RTC_RTCP_FEEDBACK_RTP_HPP

#include "common.hpp"
#include "RTC/RTCP/Feedback.hpp"
#include <vector>

namespace RTC
{
	namespace RTCP
	{
		template<typename Item>
		class FeedbackRtpItemsPacket : public FeedbackRtpPacket
		{
		public:
			typedef typename std::vector<Item*>::iterator Iterator;

		public:
			static FeedbackRtpItemsPacket<Item>* Parse(const uint8_t* data, size_t len);

		public:
			// Parsed Report. Points to an external data.
			explicit FeedbackRtpItemsPacket(CommonHeader* commonHeader);
			explicit FeedbackRtpItemsPacket(uint32_t senderSsrc, uint32_t mediaSsrc = 0);
			virtual ~FeedbackRtpItemsPacket(){};

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
		FeedbackRtpItemsPacket<Item>::FeedbackRtpItemsPacket(CommonHeader* commonHeader)
		    : FeedbackRtpPacket(commonHeader)
		{
		}

		template<typename Item>
		FeedbackRtpItemsPacket<Item>::FeedbackRtpItemsPacket(uint32_t senderSsrc, uint32_t mediaSsrc)
		    : FeedbackRtpPacket(Item::MessageType, senderSsrc, mediaSsrc)
		{
		}

		template<typename Item>
		size_t FeedbackRtpItemsPacket<Item>::GetSize() const
		{
			size_t size = FeedbackRtpPacket::GetSize();

			for (auto item : this->items)
			{
				size += item->GetSize();
			}

			return size;
		}

		template<typename Item>
		void FeedbackRtpItemsPacket<Item>::AddItem(Item* item)
		{
			this->items.push_back(item);
		}

		template<typename Item>
		typename FeedbackRtpItemsPacket<Item>::Iterator FeedbackRtpItemsPacket<Item>::Begin()
		{
			return this->items.begin();
		}

		template<typename Item>
		typename FeedbackRtpItemsPacket<Item>::Iterator FeedbackRtpItemsPacket<Item>::End()
		{
			return this->items.end();
		}
	}
}

#endif
