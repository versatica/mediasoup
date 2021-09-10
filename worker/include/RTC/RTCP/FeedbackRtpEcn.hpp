#ifndef MS_RTC_RTCP_FEEDBACK_RTP_ECN_HPP
#define MS_RTC_RTCP_FEEDBACK_RTP_ECN_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackRtp.hpp"

/* RFC 6679
 * Explicit Congestion Notification (ECN) for RTP over UDP

   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | Extended Highest Sequence Number                              |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | ECT (0) Counter                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | ECT (1) Counter                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | ECN-CE Counter                |
                                  | not-ECT Counter               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | Lost Packets Counter          |
                                  | Duplication Counter           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

namespace RTC
{
	namespace RTCP
	{
		class FeedbackRtpEcnItem : public FeedbackItem
		{
		public:
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
			static const size_t HeaderSize{ 20 };
			static const FeedbackRtp::MessageType messageType{ FeedbackRtp::MessageType::ECN };

		public:
			explicit FeedbackRtpEcnItem(Header* header) : header(header)
			{
			}
			explicit FeedbackRtpEcnItem(FeedbackRtpEcnItem* item) : header(item->header)
			{
			}
			~FeedbackRtpEcnItem() override = default;

			uint32_t GetSequenceNumber() const
			{
				return uint32_t{ ntohl(this->header->sequenceNumber) };
			}
			uint32_t GetEct0Counter() const
			{
				return uint32_t{ ntohl(this->header->ect0Counter) };
			}
			uint32_t GetEct1Counter() const
			{
				return uint32_t{ ntohl(this->header->ect1Counter) };
			}
			uint16_t GetEcnCeCounter() const
			{
				return uint16_t{ ntohs(this->header->ecnCeCounter) };
			}
			uint16_t GetNotEctCounter() const
			{
				return uint16_t{ ntohs(this->header->notEctCounter) };
			}
			uint16_t GetLostPackets() const
			{
				return uint16_t{ ntohs(this->header->lostPackets) };
			}
			uint16_t GetDuplicatedPackets() const
			{
				return uint16_t{ ntohs(this->header->duplicatedPackets) };
			}

			/* Virtual methods inherited from FeedbackItem. */
		public:
			void Dump() const override;
			size_t Serialize(uint8_t* buffer) override;
			size_t GetSize() const override
			{
				return FeedbackRtpEcnItem::HeaderSize;
			}

		private:
			Header* header{ nullptr };
		};

		// Ecn packet declaration.
		using FeedbackRtpEcnPacket = FeedbackRtpItemsPacket<FeedbackRtpEcnItem>;
	} // namespace RTCP
} // namespace RTC

#endif
