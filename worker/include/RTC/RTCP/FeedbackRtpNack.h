#ifndef MS_RTC_RTCP_FEEDBACK_NACK_H
#define MS_RTC_RTCP_FEEDBACK_NACK_H

#include "common.h"
#include "RTC/RTCP/FeedbackRtp.h"

/* RFC 4585
 * Generic NACK message (NACK)
 *
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
0   |              PID              |             BPL               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

namespace RTC { namespace RTCP
{
	class NackItem
		: public FeedbackItem
	{
	public:
		struct Header
		{
			uint16_t packet_id;
			uint16_t lost_packet_bitmask;
		};

	public:
		static const FeedbackRtp::MessageType MessageType = FeedbackRtp::NACK;

	public:
		static NackItem* Parse(const uint8_t* data, size_t len);

	public:
		explicit NackItem(Header* header);
		explicit NackItem(NackItem* item);
		NackItem(uint16_t packetId, uint16_t lostPacketBitmask);
		virtual ~NackItem() {};

		uint16_t GetPacketId();
		uint16_t GetLostPacketBitmask();

	/* Virtual methods inherited from FeedbackItem. */
	public:
		virtual void Dump() override;
		virtual size_t Serialize(uint8_t* buffer) override;
		virtual size_t GetSize() override;

	private:
		Header* header = nullptr;
	};

	// Nack packet declaration.
	typedef FeedbackRtpItemPacket<NackItem> FeedbackRtpNackPacket;

	/* Inline instance methods. */

	inline
	NackItem::NackItem(Header* header):
		header(header)
	{}

	inline
	NackItem::NackItem(NackItem* item):
		header(item->header)
	{}

	inline
	size_t NackItem::GetSize()
	{
		return sizeof(Header);
	}

	inline
	uint16_t NackItem::GetPacketId()
	{
		return (uint16_t)ntohs(this->header->packet_id);
	}

	inline
	uint16_t NackItem::GetLostPacketBitmask()
	{
		return this->header->lost_packet_bitmask;
	}
}}

#endif
