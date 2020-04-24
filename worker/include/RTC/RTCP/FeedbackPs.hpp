#ifndef MS_RTC_RTCP_FEEDBACK_PS_HPP
#define MS_RTC_RTCP_FEEDBACK_PS_HPP

#include "common.hpp"
#include "RTC/RTCP/Feedback.hpp"
#include <vector>

namespace RTC
{
	namespace RTCP
	{
		template<typename Item>
		class FeedbackPsItemsPacket : public FeedbackPsPacket
		{
		public:
			using Iterator = typename std::vector<Item*>::iterator;

		public:
			static FeedbackPsItemsPacket<Item>* Parse(const uint8_t* data, size_t len);

		public:
			// Parsed Report. Points to an external data.
			explicit FeedbackPsItemsPacket(CommonHeader* commonHeader) : FeedbackPsPacket(commonHeader)
			{
			}
			explicit FeedbackPsItemsPacket(uint32_t senderSsrc, uint32_t mediaSsrc = 0)
			  : FeedbackPsPacket(Item::messageType, senderSsrc, mediaSsrc)
			{
			}
			~FeedbackPsItemsPacket()
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

			/* Pure virtual methods inherited from Packet. */
		public:
			void Dump() const override;
			size_t Serialize(uint8_t* buffer) override;
			size_t GetSize() const override
			{
				size_t size = FeedbackPsPacket::GetSize();

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
