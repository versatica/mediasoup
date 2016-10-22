#ifndef MS_RTC_RTCP_FEEDBACK_FIR_H
#define MS_RTC_RTCP_FEEDBACK_FIR_H

#include "common.h"
#include "RTC/RTCP/Feedback.h"

#include <vector>

namespace RTC { namespace RTCP
{
	class FirItem
	{
	private:
		struct Header
		{
			uint32_t ssrc;
			uint8_t seqnr;
			uint32_t reserved:24;
		};

	public:
		static FirItem* Parse(const uint8_t* data, size_t len);

	public:
		FirItem(Header* header);
		FirItem(FirItem* item);
		FirItem(uint32_t ssrc, uint8_t seqnr);
		~FirItem();

		void Dump();
		void Serialize();
		size_t Serialize(uint8_t* data);
		size_t GetSize();

		uint32_t GetSsrc();
		uint8_t GetSeqNr();

	private:
		// Passed by argument.
		Header* header = nullptr;
		uint8_t* raw = nullptr;
	};

	class FeedbackPsFirPacket
		: public FeedbackPsPacket
	{

	public:
		static FeedbackPsFirPacket* Parse(const uint8_t* data, size_t len);

	public:
		// Parsed Report. Points to an external data.
		FeedbackPsFirPacket(CommonHeader* commonHeader);
		FeedbackPsFirPacket(uint32_t sender_ssrc, uint32_t media_ssrc = 0);

		void Dump() override;
		size_t Serialize(uint8_t* data) override;
		size_t GetSize() override;

		void AddItem(FirItem* item);

	private:
		std::vector<FirItem*> items;
	};

	/* FirItem inline instance methods */

	inline
	FirItem::FirItem(FirItem* item) :
		header(item->header)
	{
	}

	inline
	size_t FirItem::GetSize()
	{
		return sizeof(Header);
	}

	inline
	uint32_t FirItem::GetSsrc()
	{
		return (uint32_t)ntohl(this->header->ssrc);
	}

	inline
	uint8_t FirItem::GetSeqNr()
	{
		return (uint8_t)this->header->seqnr;
	}


	/* FeedbackPsFirPacket inline instance methods */

	inline
	size_t FeedbackPsFirPacket::GetSize()
	{
		size_t size = FeedbackPsPacket::GetSize();

		for (auto item : this->items) {
			size += item->GetSize();
		}

		return size;
	}

	inline
	void FeedbackPsFirPacket::AddItem(FirItem* item)
	{
		this->items.push_back(item);
	}

}
}

#endif
