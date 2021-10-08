#ifndef MS_RTC_RTCP_FEEDBACK_PS_FIR_HPP
#define MS_RTC_RTCP_FEEDBACK_PS_FIR_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackPs.hpp"

/* RFC 5104
 * Full Intra Request (FIR)

   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                               SSRC                            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | Seq nr.       |
                  | Reserved                                      |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

namespace RTC
{
	namespace RTCP
	{
		class FeedbackPsFirItem : public FeedbackItem
		{
		public:
			struct Header
			{
				uint32_t ssrc;
				uint8_t sequenceNumber;
#ifdef _WIN32
				uint8_t reserved[3]; // Alignment.
#else
				uint32_t reserved : 24;
#endif
			};

		public:
			static const size_t HeaderSize = 8;
			static const FeedbackPs::MessageType messageType{ FeedbackPs::MessageType::FIR };

		public:
			explicit FeedbackPsFirItem(Header* header) : header(header)
			{
			}
			explicit FeedbackPsFirItem(FeedbackPsFirItem* item) : header(item->header)
			{
			}
			FeedbackPsFirItem(uint32_t ssrc, uint8_t sequenceNumber);
			~FeedbackPsFirItem() override = default;

			uint32_t GetSsrc() const
			{
				return uint32_t{ ntohl(this->header->ssrc) };
			}
			uint8_t GetSequenceNumber() const
			{
				return this->header->sequenceNumber;
			}

			/* Virtual methods inherited from FeedbackItem. */
		public:
			void Dump() const override;
			size_t Serialize(uint8_t* buffer) override;
			size_t GetSize() const override
			{
				return FeedbackPsFirItem::HeaderSize;
			}

		private:
			Header* header{ nullptr };
		};

		// Fir packet declaration.
		using FeedbackPsFirPacket = FeedbackPsItemsPacket<FeedbackPsFirItem>;
	} // namespace RTCP
} // namespace RTC

#endif
