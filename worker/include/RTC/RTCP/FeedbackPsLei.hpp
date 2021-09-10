#ifndef MS_RTC_RTCP_FEEDBACK_PS_LEI_HPP
#define MS_RTC_RTCP_FEEDBACK_PS_LEI_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackPs.hpp"

/* RFC 6642
 * Payload-Specific Third-Party Loss Early Indication (PSLEI)

   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                              SSRC                             |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

namespace RTC
{
	namespace RTCP
	{
		class FeedbackPsLeiItem : public FeedbackItem
		{
		public:
			struct Header
			{
				uint32_t ssrc;
			};

		public:
			static const size_t HeaderSize{ 4 };
			static const FeedbackPs::MessageType messageType{ FeedbackPs::MessageType::PSLEI };

		public:
			explicit FeedbackPsLeiItem(Header* header) : header(header)
			{
			}
			explicit FeedbackPsLeiItem(FeedbackPsLeiItem* item) : header(item->header)
			{
			}
			explicit FeedbackPsLeiItem(uint32_t ssrc);
			~FeedbackPsLeiItem() override = default;

			uint32_t GetSsrc() const
			{
				return uint32_t{ ntohl(this->header->ssrc) };
			}

			/* Virtual methods inherited from FeedbackItem. */
		public:
			void Dump() const override;
			size_t Serialize(uint8_t* buffer) override;
			size_t GetSize() const override
			{
				return FeedbackPsLeiItem::HeaderSize;
			}

		private:
			Header* header{ nullptr };
		};

		// Lei packet declaration.
		using FeedbackPsLeiPacket = FeedbackPsItemsPacket<FeedbackPsLeiItem>;
	} // namespace RTCP
} // namespace RTC

#endif
