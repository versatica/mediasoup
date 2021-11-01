#ifndef MS_RTC_RTCP_FEEDBACK_RTP_NACK_HPP
#define MS_RTC_RTCP_FEEDBACK_RTP_NACK_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/RTCP/FeedbackRtp.hpp"

/* RFC 4585
 * Generic NACK message (NACK)

   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |              PID              |             BPL               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

namespace RTC
{
	namespace RTCP
	{
		class FeedbackRtpNackItem : public FeedbackItem
		{
		public:
			struct Header
			{
				uint16_t packetId;
				uint16_t lostPacketBitmask;
			};

		public:
			static const size_t HeaderSize{ 4 };
			static const FeedbackRtp::MessageType messageType{ FeedbackRtp::MessageType::NACK };

		public:
			explicit FeedbackRtpNackItem(Header* header) : header(header)
			{
			}
			explicit FeedbackRtpNackItem(FeedbackRtpNackItem* item) : header(item->header)
			{
			}
			FeedbackRtpNackItem(uint16_t packetId, uint16_t lostPacketBitmask);
			~FeedbackRtpNackItem() override = default;

			uint16_t GetPacketId() const
			{
				return uint16_t{ ntohs(this->header->packetId) };
			}
			uint16_t GetLostPacketBitmask() const
			{
				return uint16_t{ ntohs(this->header->lostPacketBitmask) };
			}
			size_t CountRequestedPackets() const
			{
				return Utils::Bits::CountSetBits(this->header->lostPacketBitmask) + 1;
			}

			/* Virtual methods inherited from FeedbackItem. */
		public:
			void Dump() const override;
			size_t Serialize(uint8_t* buffer) override;
			size_t GetSize() const override
			{
				return FeedbackRtpNackItem::HeaderSize;
			}

		private:
			Header* header{ nullptr };
		};

		// Nack packet declaration.
		using FeedbackRtpNackPacket = FeedbackRtpItemsPacket<FeedbackRtpNackItem>;
	} // namespace RTCP
} // namespace RTC

#endif
