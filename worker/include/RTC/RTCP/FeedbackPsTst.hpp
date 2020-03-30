#ifndef MS_RTC_RTCP_FEEDBACK_PS_TST_HPP
#define MS_RTC_RTCP_FEEDBACK_PS_TST_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackPs.hpp"

/* RFC 5104
 * Temporal-Spatial Trade-off Request (TSTR)
 * Temporal-Spatial Trade-off Notification (TSTN)

   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                              SSRC                             |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |  Seq nr.      |
                  |  Reserved                           | Index   |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

namespace RTC
{
	namespace RTCP
	{
		template<typename T>
		class FeedbackPsTstItem : public FeedbackItem
		{
		public:
			struct Header
			{
				uint32_t ssrc;
				uint32_t sequenceNumber : 8;
				uint32_t reserved : 19;
				uint32_t index : 5;
			};

		public:
			static const FeedbackPs::MessageType messageType;

		public:
			explicit FeedbackPsTstItem(Header* header) : header(header)
			{
			}
			explicit FeedbackPsTstItem(FeedbackPsTstItem* item) : header(item->header)
			{
			}
			FeedbackPsTstItem(uint32_t ssrc, uint8_t sequenceNumber, uint8_t index);
			~FeedbackPsTstItem() override = default;

			uint32_t GetSsrc() const
			{
				return uint32_t{ ntohl(this->header->ssrc) };
			}
			uint8_t GetSequenceNumber() const
			{
				return static_cast<uint8_t>(this->header->sequenceNumber);
			}
			uint8_t GetIndex() const
			{
				return static_cast<uint8_t>(this->header->index);
			}

			/* Virtual methods inherited from FeedbackItem. */
		public:
			void Dump() const override;
			size_t Serialize(uint8_t* buffer) override;
			size_t GetSize() const override
			{
				return sizeof(Header);
			}

		private:
			Header* header{ nullptr };
		};

		class Tstr
		{
		};

		class Tstn
		{
		};

		// Tst classes declaration.
		using FeedbackPsTstrItem = FeedbackPsTstItem<Tstr>;
		using FeedbackPsTstnItem = FeedbackPsTstItem<Tstn>;

		// Tst packets declaration.
		using FeedbackPsTstrPacket = FeedbackPsItemsPacket<FeedbackPsTstrItem>;
		using FeedbackPsTstnPacket = FeedbackPsItemsPacket<FeedbackPsTstnItem>;
	} // namespace RTCP
} // namespace RTC

#endif
