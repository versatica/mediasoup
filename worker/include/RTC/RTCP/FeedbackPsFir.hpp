#ifndef MS_RTC_RTCP_FEEDBACK_PS_FIR_HPP
#define MS_RTC_RTCP_FEEDBACK_PS_FIR_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackPs.hpp"

/* RFC 5104
 * Full Intra Request (FIR)
 *
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
0  |                               SSRC                            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
4  | Seq nr.       |
5                  | Reserved                                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

namespace RTC
{
	namespace RTCP
	{
		class FeedbackPsFirItem : public FeedbackItem
		{
		private:
			struct Header
			{
				uint32_t ssrc;
				uint8_t sequenceNumber;
				uint32_t reserved : 24;
			};

		public:
			static const FeedbackPs::MessageType MessageType = FeedbackPs::MessageType::FIR;

		public:
			static FeedbackPsFirItem* Parse(const uint8_t* data, size_t len);

		public:
			explicit FeedbackPsFirItem(Header* header);
			explicit FeedbackPsFirItem(FeedbackPsFirItem* item);
			FeedbackPsFirItem(uint32_t ssrc, uint8_t sequenceNumber);
			virtual ~FeedbackPsFirItem(){};

			uint32_t GetSsrc() const;
			uint8_t GetSequenceNumber() const;

			/* Virtual methods inherited from FeedbackItem. */
		public:
			virtual void Dump() const override;
			virtual size_t Serialize(uint8_t* buffer) override;
			virtual size_t GetSize() const override;

		private:
			Header* header = nullptr;
		};

		// Fir packet declaration.
		typedef FeedbackPsItemsPacket<FeedbackPsFirItem> FeedbackPsFirPacket;

		/* Inline instance methods. */

		inline FeedbackPsFirItem::FeedbackPsFirItem(Header* header)
		    : header(header)
		{
		}

		inline FeedbackPsFirItem::FeedbackPsFirItem(FeedbackPsFirItem* item)
		    : header(item->header)
		{
		}

		inline size_t FeedbackPsFirItem::GetSize() const
		{
			return sizeof(Header);
		}

		inline uint32_t FeedbackPsFirItem::GetSsrc() const
		{
			return (uint32_t)ntohl(this->header->ssrc);
		}

		inline uint8_t FeedbackPsFirItem::GetSequenceNumber() const
		{
			return (uint8_t)this->header->sequenceNumber;
		}
	}
}

#endif
