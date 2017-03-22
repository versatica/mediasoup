#ifndef MS_RTC_RTCP_FEEDBACK_RTP_NACK_HPP
#define MS_RTC_RTCP_FEEDBACK_RTP_NACK_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackRtp.hpp"

/* RFC 4585
 * Generic NACK message (NACK)
 *
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |              PID              |             BPL               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

namespace RTC { namespace RTCP
{
	class FeedbackRtpNackItem
		: public FeedbackItem
	{
	public:
		struct Header
		{
			uint16_t packet_id;
			uint16_t lost_packet_bitmask;
		};

	public:
		static const FeedbackRtp::MessageType MessageType = FeedbackRtp::MessageType::NACK;

	public:
		static FeedbackRtpNackItem* Parse(const uint8_t* data, size_t len);

	public:
		explicit FeedbackRtpNackItem(Header* header);
		explicit FeedbackRtpNackItem(FeedbackRtpNackItem* item);
		FeedbackRtpNackItem(uint16_t packetId, uint16_t lostPacketBitmask);
		virtual ~FeedbackRtpNackItem() {};

		uint16_t GetPacketId() const;
		uint16_t GetLostPacketBitmask() const;

	/* Virtual methods inherited from FeedbackItem. */
	public:
		virtual void Dump() const override;
		virtual size_t Serialize(uint8_t* buffer) override;
		virtual size_t GetSize() const override;

	private:
		Header* header = nullptr;
	};

	// Nack packet declaration.
	typedef FeedbackRtpItemsPacket<FeedbackRtpNackItem> FeedbackRtpNackPacket;

	/* Inline instance methods. */

	inline
	FeedbackRtpNackItem::FeedbackRtpNackItem(Header* header):
		header(header)
	{}

	inline
	FeedbackRtpNackItem::FeedbackRtpNackItem(FeedbackRtpNackItem* item):
		header(item->header)
	{}

	inline
	size_t FeedbackRtpNackItem::GetSize() const
	{
		return sizeof(Header);
	}

	inline
	uint16_t FeedbackRtpNackItem::GetPacketId() const
	{
		return (uint16_t)ntohs(this->header->packet_id);
	}

	inline
	uint16_t FeedbackRtpNackItem::GetLostPacketBitmask() const
	{
		return (uint16_t)ntohs(this->header->lost_packet_bitmask);
	}
}}

#endif
