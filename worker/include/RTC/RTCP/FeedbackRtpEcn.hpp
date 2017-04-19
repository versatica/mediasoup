#ifndef MS_RTC_RTCP_FEEDBACK_RTP_ECN_HPP
#define MS_RTC_RTCP_FEEDBACK_RTP_ECN_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackRtp.hpp"

/* RFC 6679
 * Explicit Congestion Notification (ECN) for RTP over UDP
 *

     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
0   | Extended Highest Sequence Number                              |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
4   | ECT (0) Counter                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
8   | ECT (1) Counter                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
12  | ECN-CE Counter                |
14                                  | not-ECT Counter               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
16  | Lost Packets Counter          |
18                                  | Duplication Counter           |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

namespace RTC
{
	namespace RTCP
	{
		class FeedbackRtpEcnItem : public FeedbackItem
		{
		private:
			struct Header
			{
				uint32_t sequenceNumber;
				uint32_t ect0Counter;
				uint32_t ect1Counter;
				uint16_t ecnCeCounter;
				uint16_t notEctCounter;
				uint16_t lostPackets;
				uint16_t duplicatedPackets;
			};

		public:
			static const FeedbackRtp::MessageType messageType{ FeedbackRtp::MessageType::ECN };

		public:
			static FeedbackRtpEcnItem* Parse(const uint8_t* data, size_t len);

		public:
			explicit FeedbackRtpEcnItem(Header* header);
			explicit FeedbackRtpEcnItem(FeedbackRtpEcnItem* item);
			~FeedbackRtpEcnItem() override = default;

			uint32_t GetSequenceNumber() const;
			uint32_t GetEct0Counter() const;
			uint32_t GetEct1Counter() const;
			uint16_t GetEcnCeCounter() const;
			uint16_t GetNotEctCounter() const;
			uint16_t GetLostPackets() const;
			uint16_t GetDuplicatedPackets() const;

			/* Virtual methods inherited from FeedbackItem. */
		public:
			void Dump() const override;
			size_t Serialize(uint8_t* buffer) override;
			size_t GetSize() const override;

		private:
			Header* header{ nullptr };
		};

		// Ecn packet declaration.
		using FeedbackRtpEcnPacket = FeedbackRtpItemsPacket<FeedbackRtpEcnItem>;

		/* Inline instance methods. */

		inline FeedbackRtpEcnItem::FeedbackRtpEcnItem(Header* header) : header(header)
		{
		}

		inline FeedbackRtpEcnItem::FeedbackRtpEcnItem(FeedbackRtpEcnItem* item) : header(item->header)
		{
		}

		inline size_t FeedbackRtpEcnItem::GetSize() const
		{
			return sizeof(Header);
		}

		inline uint32_t FeedbackRtpEcnItem::GetSequenceNumber() const
		{
			return uint32_t{ ntohl(this->header->sequenceNumber) };
		}

		inline uint32_t FeedbackRtpEcnItem::GetEct0Counter() const
		{
			return uint32_t{ ntohl(this->header->ect0Counter) };
		}

		inline uint32_t FeedbackRtpEcnItem::GetEct1Counter() const
		{
			return uint32_t{ ntohl(this->header->ect1Counter) };
		}

		inline uint16_t FeedbackRtpEcnItem::GetEcnCeCounter() const
		{
			return uint16_t{ ntohs(this->header->ecnCeCounter) };
		}

		inline uint16_t FeedbackRtpEcnItem::GetNotEctCounter() const
		{
			return uint16_t{ ntohs(this->header->notEctCounter) };
		}

		inline uint16_t FeedbackRtpEcnItem::GetLostPackets() const
		{
			return uint16_t{ ntohs(this->header->lostPackets) };
		}

		inline uint16_t FeedbackRtpEcnItem::GetDuplicatedPackets() const
		{
			return uint16_t{ ntohs(this->header->duplicatedPackets) };
		}
	} // namespace RTCP
} // namespace RTC

#endif
