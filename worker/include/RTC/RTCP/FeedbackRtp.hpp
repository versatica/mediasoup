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
			using Iterator = typename std::vector<Item*>::iterator;

		public:
			static FeedbackRtpItemsPacket<Item>* Parse(const uint8_t* data, size_t len);

		public:
			// Parsed Report. Points to an external data.
			explicit FeedbackRtpItemsPacket(CommonHeader* commonHeader);
			explicit FeedbackRtpItemsPacket(uint32_t senderSsrc, uint32_t mediaSsrc = 0);
			~FeedbackRtpItemsPacket();

			void AddItem(Item* item);
			Iterator Begin();
			Iterator End();

			/* Virtual methods inherited from FeedbackItem. */
		public:
			void Dump() const override;
			size_t Serialize(uint8_t* buffer) override;
			size_t GetSize() const override;

		private:
			std::vector<Item*> items;
		};

		/* Inline instance methods. */

		template<typename Item>
		inline FeedbackRtpItemsPacket<Item>::FeedbackRtpItemsPacket(CommonHeader* commonHeader)
		  : FeedbackRtpPacket(commonHeader)
		{
		}

		template<typename Item>
		inline FeedbackRtpItemsPacket<Item>::FeedbackRtpItemsPacket(uint32_t senderSsrc, uint32_t mediaSsrc)
		  : FeedbackRtpPacket(Item::messageType, senderSsrc, mediaSsrc)
		{
		}

		template<typename Item>
		FeedbackRtpItemsPacket<Item>::~FeedbackRtpItemsPacket()
		{
			for (auto* item : this->items)
				delete item;
		}

		template<typename Item>
		inline size_t FeedbackRtpItemsPacket<Item>::GetSize() const
		{
			size_t size = FeedbackRtpPacket::GetSize();

			for (auto item : this->items)
			{
				size += item->GetSize();
			}

			return size;
		}

		template<typename Item>
		inline void FeedbackRtpItemsPacket<Item>::AddItem(Item* item)
		{
			this->items.push_back(item);
		}

		template<typename Item>
		inline typename FeedbackRtpItemsPacket<Item>::Iterator FeedbackRtpItemsPacket<Item>::Begin()
		{
			return this->items.begin();
		}

		template<typename Item>
		inline typename FeedbackRtpItemsPacket<Item>::Iterator FeedbackRtpItemsPacket<Item>::End()
		{
			return this->items.end();
		}
	} // namespace RTCP
} // namespace RTC

#endif
