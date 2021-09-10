#ifndef MS_RTC_RTCP_FEEDBACK_PS_SLI_HPP
#define MS_RTC_RTCP_FEEDBACK_PS_SLI_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackPs.hpp"

/* RFC 4585
 * Slice Loss Indication (SLI)

   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |            First        |        Number           | PictureID |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

namespace RTC
{
	namespace RTCP
	{
		class FeedbackPsSliItem : public FeedbackItem
		{
		public:
			struct Header
			{
				uint32_t compact;
			};

		public:
			static const size_t HeaderSize{ 4 };
			static const FeedbackPs::MessageType messageType{ FeedbackPs::MessageType::SLI };

		public:
			explicit FeedbackPsSliItem(Header* header);
			explicit FeedbackPsSliItem(FeedbackPsSliItem* item);
			FeedbackPsSliItem(uint16_t first, uint16_t number, uint8_t pictureId);
			~FeedbackPsSliItem() override = default;

			uint16_t GetFirst() const
			{
				return this->first;
			}
			void SetFirst(uint16_t first)
			{
				this->first = first;
			}
			uint16_t GetNumber() const
			{
				return this->number;
			}
			void SetNumber(uint16_t number)
			{
				this->number = number;
			}
			uint8_t GetPictureId() const
			{
				return this->pictureId;
			}
			void SetPictureId(uint8_t pictureId)
			{
				this->pictureId = pictureId;
			}

			/* Virtual methods inherited from FeedbackItem. */
		public:
			void Dump() const override;
			size_t Serialize(uint8_t* buffer) override;
			size_t GetSize() const override
			{
				return FeedbackPsSliItem::HeaderSize;
			}

		private:
			Header* header{ nullptr };
			uint16_t first{ 0 };
			uint16_t number{ 0 };
			uint8_t pictureId{ 0 };
		};

		// Sli packet declaration.
		using FeedbackPsSliPacket = FeedbackPsItemsPacket<FeedbackPsSliItem>;
	} // namespace RTCP
} // namespace RTC

#endif
