#ifndef MS_RTC_RTCP_FEEDBACK_RTP_TLLEI_HPP
#define MS_RTC_RTCP_FEEDBACK_RTP_TLLEI_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackRtp.hpp"

/* RFC 4585
 * Transport-Layer Third-Party Loss Early Indication (TLLEI)
 *
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
0   |              PID              |             BPL               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

namespace RTC
{
	namespace RTCP
	{
		class FeedbackRtpTlleiItem : public FeedbackItem
		{
		public:
			struct Header
			{
				uint16_t packetId;
				uint16_t lostPacketBitmask;
			};

		public:
			static const FeedbackRtp::MessageType messageType{ FeedbackRtp::MessageType::TLLEI };

		public:
			explicit FeedbackRtpTlleiItem(Header* header);
			explicit FeedbackRtpTlleiItem(FeedbackRtpTlleiItem* item);
			FeedbackRtpTlleiItem(uint16_t packetId, uint16_t lostPacketBitmask);
			~FeedbackRtpTlleiItem() override = default;

			uint16_t GetPacketId() const;
			uint16_t GetLostPacketBitmask() const;

			/* Virtual methods inherited from FeedbackItem. */
		public:
			void Dump() const override;
			size_t Serialize(uint8_t* buffer) override;
			size_t GetSize() const override;

		private:
			Header* header{ nullptr };
		};

		// Tllei packet declaration.
		using FeedbackRtpTlleiPacket = FeedbackRtpItemsPacket<FeedbackRtpTlleiItem>;

		/* Inline instance methods. */

		inline FeedbackRtpTlleiItem::FeedbackRtpTlleiItem(Header* header) : header(header)
		{
		}

		inline FeedbackRtpTlleiItem::FeedbackRtpTlleiItem(FeedbackRtpTlleiItem* item)
		  : header(item->header)
		{
		}

		inline size_t FeedbackRtpTlleiItem::GetSize() const
		{
			return sizeof(Header);
		}

		inline uint16_t FeedbackRtpTlleiItem::GetPacketId() const
		{
			return uint16_t{ ntohs(this->header->packetId) };
		}

		inline uint16_t FeedbackRtpTlleiItem::GetLostPacketBitmask() const
		{
			return uint16_t{ ntohs(this->header->lostPacketBitmask) };
		}
	} // namespace RTCP
} // namespace RTC

#endif
