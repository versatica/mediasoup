#ifndef MS_RTC_RTCP_FEEDBACK_ECN_HPP
#define MS_RTC_RTCP_FEEDBACK_ECN_HPP

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

namespace RTC { namespace RTCP
{
	class EcnItem
		: public FeedbackItem
	{
	private:
		struct Header
		{
			uint32_t sequence_number;
			uint32_t ect0_counter;
			uint32_t ect1_counter;
			uint16_t ecn_ce_counter;
			uint16_t not_ect_counter;
			uint16_t lost_packets;
			uint16_t duplicated_packets;
		};

	public:
		static const FeedbackRtp::MessageType MessageType = FeedbackRtp::ECN;

	public:
		static EcnItem* Parse(const uint8_t* data, size_t len);

	public:
		explicit EcnItem(Header* header);
		explicit EcnItem(EcnItem* item);
		virtual ~EcnItem() {};

		uint32_t GetSequenceNumber() const;
		uint32_t GetEct0Counter() const;
		uint32_t GetEct1Counter() const;
		uint16_t GetEcnCeCounter() const;
		uint16_t GetNotEctCounter() const;
		uint16_t GetLostPackets() const;
		uint16_t GetDuplicatedPackets() const;

	/* Virtual methods inherited from FeedbackItem. */
	public:
		virtual void Dump() override;
		virtual size_t Serialize(uint8_t* buffer) override;
		virtual size_t GetSize() const override;

	private:
		Header* header = nullptr;
	};

	// Ecn packet declaration.
	typedef FeedbackRtpItemPacket<EcnItem> FeedbackRtpEcnPacket;

	/* Inline instance methods. */

	inline
	EcnItem::EcnItem(Header* header):
		header(header)
	{}

	inline
	EcnItem::EcnItem(EcnItem* item):
		header(item->header)
	{}

	inline
	size_t EcnItem::GetSize() const
	{
		return sizeof(Header);
	}

	inline
	uint32_t EcnItem::GetSequenceNumber() const
	{
		return ntohl(this->header->sequence_number);
	}

	inline
	uint32_t EcnItem::GetEct0Counter() const
	{
		return ntohl(this->header->ect0_counter);
	}

	inline
	uint32_t EcnItem::GetEct1Counter() const
	{
		return ntohl(this->header->ect1_counter);
	}

	inline
	uint16_t EcnItem::GetEcnCeCounter() const
	{
		return ntohs(this->header->ecn_ce_counter);
	}

	inline
	uint16_t EcnItem::GetNotEctCounter() const
	{
		return ntohs(this->header->not_ect_counter);
	}

	inline
	uint16_t EcnItem::GetLostPackets() const
	{
		return ntohs(this->header->lost_packets);
	}

	inline
	uint16_t EcnItem::GetDuplicatedPackets() const
	{
		return ntohs(this->header->duplicated_packets);
	}
}}

#endif
