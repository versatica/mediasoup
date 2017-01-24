#ifndef MS_RTC_RTCP_FEEDBACK_VBCM_H
#define MS_RTC_RTCP_FEEDBACK_VBCM_H

#include "common.h"
#include "RTC/RTCP/FeedbackPs.h"

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
	class VbcmItem
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
		static const FeedbackPs::MessageType MessageType = FeedbackPs::FIR;

	public:
		static VbcmItem* Parse(const uint8_t* data, size_t len);

	public:
		explicit VbcmItem(Header* header);
		explicit VbcmItem(VbcmItem* item);
		VbcmItem(uint32_t ssrc, uint8_t sequence_number, uint8_t payload_type, uint16_t length, uint8_t* value);
		virtual ~VbcmItem() {};

		uint32_t GetSsrc();
		uint8_t  GetSequenceNumber();
		uint8_t  GetPayloadType();
		uint16_t GetLength();
		uint8_t* GetValue();

	/* Virtual methods inherited from FeedbackItem. */
	public:
		virtual void Dump() override;
		virtual size_t Serialize(uint8_t* buffer) override;
		virtual size_t GetSize() override;

	private:
		Header* header = nullptr;
	};

	// Vbcm packet declaration
	typedef FeedbackPsItemPacket<VbcmItem> FeedbackPsVbcmPacket;

	/* Inline instance methods. */

	inline
	VbcmItem::VbcmItem(Header* header):
		header(header)
	{}

	inline
	VbcmItem::VbcmItem(VbcmItem* item):
		header(item->header)
	{}

	inline
	size_t VbcmItem::GetSize()
	{
		size_t size =  8 + size_t(this->header->length);

		// Consider pading to 32 bits (4 bytes) boundary.
		return (size + 3) & ~3;
	}

	inline
	uint32_t VbcmItem::GetSsrc()
	{
		return (uint32_t)ntohl(this->header->ssrc);
	}

	inline
	uint8_t VbcmItem::GetSequenceNumber()
	{
		return (uint8_t)this->header->sequence_number;
	}

	inline
	uint8_t VbcmItem::GetPayloadType()
	{
		return (uint8_t)this->header->payload_type;
	}

	inline
	uint16_t VbcmItem::GetLength()
	{
		return (uint16_t)ntohs(this->header->length);
	}

	inline
	uint8_t* VbcmItem::GetValue()
	{
		return this->header->value;
	}
}}

#endif
