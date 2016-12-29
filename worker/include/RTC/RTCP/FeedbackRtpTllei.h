#ifndef MS_RTC_RTCP_FEEDBACK_TLLEI_H
#define MS_RTC_RTCP_FEEDBACK_TLLEI_H

#include "common.h"
#include "RTC/RTCP/FeedbackRtp.h"

/* RFC 4585
 * Transport-Layer Third-Party Loss Early Indication (TLLEI)
 *
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
0   |              PID              |             BPL               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

namespace RTC { namespace RTCP
{
	class TlleiItem
		: public FeedbackItem
	{
	private:
		struct Header
		{
			uint16_t packet_id;
			uint16_t lost_packet_bitmask;
		};

	public:
		static const FeedbackRtp::MessageType MessageType = FeedbackRtp::TLLEI;

	public:
		static TlleiItem* Parse(const uint8_t* data, size_t len);

	public:
		explicit TlleiItem(Header* header);
		explicit TlleiItem(TlleiItem* item);
		TlleiItem(uint16_t packetId, uint16_t lostPacketBitmask);

		uint16_t GetPacketId();
		uint16_t GetLostPacketBitmask();

	/* Virtual methods inherited from FeedbackItem. */
	public:
		virtual void Dump() override;
		virtual size_t Serialize(uint8_t* data) override;
		virtual size_t GetSize() override;

	private:
		Header* header = nullptr;
	};

	// Nack packet declaration.
	typedef FeedbackRtpItemPacket<TlleiItem> FeedbackRtpTlleiPacket;

	/* Inline instance methods. */

	inline
	TlleiItem::TlleiItem(Header* header):
		header(header)
	{}

	inline
	TlleiItem::TlleiItem(TlleiItem* item):
		header(item->header)
	{}

	inline
	size_t TlleiItem::GetSize()
	{
		return sizeof(Header);
	}

	inline
	uint16_t TlleiItem::GetPacketId()
	{
		return (uint16_t)ntohs(this->header->packet_id);
	}

	inline
	uint16_t TlleiItem::GetLostPacketBitmask()
	{
		return (uint16_t)ntohs(this->header->lost_packet_bitmask);
	}
}}

#endif
