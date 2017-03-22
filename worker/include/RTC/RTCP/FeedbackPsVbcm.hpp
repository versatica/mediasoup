#ifndef MS_RTC_RTCP_FEEDBACK_PS_VBCM_HPP
#define MS_RTC_RTCP_FEEDBACK_PS_VBCM_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackPs.hpp"

/* RFC 5104
 * H.271 Video Back Channel Message (VBCM)
 *
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
0   |                              SSRC                             |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
4   | Seq nr.       |
5                   |0| Payload Vbcm|
6                                   | Length                        |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
8   |                    VBCM Octet String....      |    Padding    |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

namespace RTC { namespace RTCP
{
	class FeedbackPsVbcmItem
		: public FeedbackItem
	{
	private:
		struct Header
		{
			uint32_t ssrc;
			uint8_t sequence_number;
			uint8_t zero:1;
			uint8_t payload_type:7;
			uint16_t length;
			uint8_t value[];
		};

	public:
		static const FeedbackPs::MessageType MessageType = FeedbackPs::MessageType::FIR;

	public:
		static FeedbackPsVbcmItem* Parse(const uint8_t* data, size_t len);

	public:
		explicit FeedbackPsVbcmItem(Header* header);
		explicit FeedbackPsVbcmItem(FeedbackPsVbcmItem* item);
		FeedbackPsVbcmItem(uint32_t ssrc, uint8_t sequence_number, uint8_t payload_type, uint16_t length, uint8_t* value);
		virtual ~FeedbackPsVbcmItem() {};

		uint32_t GetSsrc() const;
		uint8_t  GetSequenceNumber() const;
		uint8_t  GetPayloadType() const;
		uint16_t GetLength() const;
		uint8_t* GetValue() const;

	/* Virtual methods inherited from FeedbackItem. */
	public:
		virtual void Dump() const override;
		virtual size_t Serialize(uint8_t* buffer) override;
		virtual size_t GetSize() const override;

	private:
		Header* header = nullptr;
	};

	// Vbcm packet declaration
	typedef FeedbackPsItemsPacket<FeedbackPsVbcmItem> FeedbackPsVbcmPacket;

	/* Inline instance methods. */

	inline
	FeedbackPsVbcmItem::FeedbackPsVbcmItem(Header* header):
		header(header)
	{}

	inline
	FeedbackPsVbcmItem::FeedbackPsVbcmItem(FeedbackPsVbcmItem* item):
		header(item->header)
	{}

	inline
	size_t FeedbackPsVbcmItem::GetSize() const
	{
		size_t size =  8 + size_t(this->header->length);

		// Consider pading to 32 bits (4 bytes) boundary.
		return (size + 3) & ~3;
	}

	inline
	uint32_t FeedbackPsVbcmItem::GetSsrc() const
	{
		return (uint32_t)ntohl(this->header->ssrc);
	}

	inline
	uint8_t FeedbackPsVbcmItem::GetSequenceNumber() const
	{
		return (uint8_t)this->header->sequence_number;
	}

	inline
	uint8_t FeedbackPsVbcmItem::GetPayloadType() const
	{
		return (uint8_t)this->header->payload_type;
	}

	inline
	uint16_t FeedbackPsVbcmItem::GetLength() const
	{
		return (uint16_t)ntohs(this->header->length);
	}

	inline
	uint8_t* FeedbackPsVbcmItem::GetValue() const
	{
		return this->header->value;
	}
}}

#endif
