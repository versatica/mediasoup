#ifndef MS_RTC_RTCP_FEEDBACK_RPSI_H
#define MS_RTC_RTCP_FEEDBACK_RPSI_H

#include "common.h"
#include "RTC/RTCP/Feedback.h"

#include <vector>

namespace RTC { namespace RTCP
{
	class RpsiItem
	{
		const static size_t MaxBitStringSize = 6;
		const static size_t BitStringOffset = 2;

	private:
		struct Header
		{
			uint8_t pb;
			uint8_t zero:1;
			uint8_t payloadType:7;
			uint8_t bitString[MaxBitStringSize];
		};

	public:
		static RpsiItem* Parse(const uint8_t* data, size_t len);

	public:
		RpsiItem(Header* header);
		RpsiItem(RpsiItem* item);
		RpsiItem(uint8_t payloadType, uint8_t* bitString, size_t length);
		~RpsiItem();

		void Dump();
		void Serialize();
		size_t Serialize(uint8_t* data);
		size_t GetSize();

		bool IsCorrect();
		uint8_t GetPayloadType();
		uint8_t* GetBitString();
		size_t GetLength();

	private:
		// Passed by argument.
		Header* header = nullptr;
		uint8_t* raw = nullptr;
		size_t length;
		bool isCorrect = true;
	};

	class FeedbackPsRpsiPacket
		: public FeedbackPsPacket
	{

	public:
		static FeedbackPsRpsiPacket* Parse(const uint8_t* data, size_t len);

	public:
		// Parsed Report. Points to an external data.
		FeedbackPsRpsiPacket(CommonHeader* commonHeader);
		FeedbackPsRpsiPacket(uint32_t sender_ssrc, uint32_t media_ssrc = 0);

		void Dump() override;
		size_t Serialize(uint8_t* data) override;
		size_t GetSize() override;

		void SetItem(RpsiItem* item);

	private:
		RpsiItem* item = nullptr;
	};

	/* RpsiItem inline instance methods */

	inline
	RpsiItem::RpsiItem(RpsiItem* item) :
		header(item->header)
	{
	}

	inline
	uint8_t RpsiItem::GetPayloadType()
	{
		return this->header->payloadType;
	}

	inline
	uint8_t* RpsiItem::GetBitString()
	{
		return this->header->bitString;
	}

	inline
	size_t RpsiItem::GetLength()
	{
		return this->length;
	}

	inline
	bool RpsiItem::IsCorrect()
	{
		return this->isCorrect;
	}

	/* FeedbackPsRpsiPacket inline instance methods */

	inline
	size_t FeedbackPsRpsiPacket::GetSize()
	{
		return FeedbackPsPacket::GetSize() + sizeof(Header);
	}

	inline
	void FeedbackPsRpsiPacket::SetItem(RpsiItem* item)
	{
		this->item = item;
	}

}
}

#endif
