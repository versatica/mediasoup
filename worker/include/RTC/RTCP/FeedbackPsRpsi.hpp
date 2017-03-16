#ifndef MS_RTC_RTCP_FEEDBACK_RPSI_HPP
#define MS_RTC_RTCP_FEEDBACK_RPSI_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackPs.hpp"

/* RFC 4585
 * Reference Picture Selection Indication (RPSI)
 *

    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
0  |      PB       |
1                  |0| Payload Type|
2                                  |    Native RPSI bit string     |
4  |   defined per codec          ...                | Padding (0) |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

namespace RTC { namespace RTCP
{
	class RpsiItem
		: public FeedbackItem
	{
		const static size_t MaxBitStringSize = 6;
		const static size_t BitStringOffset = 2;

	private:
		struct Header
		{
			uint8_t padding_bits;
			uint8_t zero:1;
			uint8_t payload_type:7;
			uint8_t bit_string[MaxBitStringSize];
		};

	public:
		static const FeedbackPs::MessageType MessageType = FeedbackPs::RPSI;

	public:
		static RpsiItem* Parse(const uint8_t* data, size_t len);

	public:
		explicit RpsiItem(Header* header);
		explicit RpsiItem(RpsiItem* item);
		RpsiItem(uint8_t payload_type, uint8_t* bit_string, size_t length);
		virtual ~RpsiItem() {};

		bool IsCorrect() const;
		uint8_t GetPayloadType() const;
		uint8_t* GetBitString() const;
		size_t GetLength() const;

	/* Virtual methods inherited from FeedbackItem. */
	public:
		virtual void Dump() const override;
		virtual size_t Serialize(uint8_t* buffer) override;
		virtual size_t GetSize() const override;

	private:
		Header* header = nullptr;
		size_t length = 0;
	};

	// Rpsi packet declaration.
	typedef FeedbackPsItemPacket<RpsiItem> FeedbackPsRpsiPacket;

	/* Inline instance methods. */

	inline
	RpsiItem::RpsiItem(RpsiItem* item) :
		header(item->header)
	{}

	inline
	size_t RpsiItem::GetSize() const
	{
		return sizeof(Header);
	}

	inline
	uint8_t RpsiItem::GetPayloadType() const
	{
		return this->header->payload_type;
	}

	inline
	uint8_t* RpsiItem::GetBitString() const
	{
		return this->header->bit_string;
	}

	inline
	size_t RpsiItem::GetLength() const
	{
		return this->length;
	}

	inline
	bool RpsiItem::IsCorrect() const
	{
		return this->isCorrect;
	}
}}

#endif
