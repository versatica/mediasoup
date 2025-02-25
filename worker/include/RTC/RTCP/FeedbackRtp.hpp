#ifndef MS_RTC_RTCP_FEEDBACK_RTP_HPP
#define MS_RTC_RTCP_FEEDBACK_RTP_HPP

#include "common.hpp"
#include "RTC/RTCP/Feedback.hpp"
#include "RTC/RTCP/FeedbackItem.hpp"
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
			explicit FeedbackRtpItemsPacket(CommonHeader* commonHeader) : FeedbackRtpPacket(commonHeader)
			{
			}
			explicit FeedbackRtpItemsPacket(uint32_t senderSsrc, uint32_t mediaSsrc = 0)
			  : FeedbackRtpPacket(Item::messageType, senderSsrc, mediaSsrc)
			{
			}
			~FeedbackRtpItemsPacket()
			{
				for (auto* item : this->items)
				{
					delete item;
				}
			}

			void AddItem(Item* item)
			{
				this->items.push_back(item);
			}
			Iterator Begin()
			{
				return this->items.begin();
			}
			Iterator End()
			{
				return this->items.end();
			}

			/* Virtual methods inherited from FeedbackItem. */
		public:
			void Dump() const override;
			size_t Serialize(uint8_t* buffer) override;
			size_t GetSize() const override
			{
				size_t size = FeedbackRtpPacket::GetSize();

				for (auto* item : this->items)
				{
					size += item->GetSize();
				}

				return size;
			}

		private:
			std::vector<Item*> items;
		};
	} // namespace RTCP
} // namespace RTC

#endif
