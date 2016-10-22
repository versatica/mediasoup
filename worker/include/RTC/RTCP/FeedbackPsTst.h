#ifndef MS_RTC_RTCP_FEEDBACK_TST_H
#define MS_RTC_RTCP_FEEDBACK_TST_H

#include "common.h"
#include "RTC/RTCP/Feedback.h"

#include <vector>

namespace RTC { namespace RTCP
{
	class TstItem
	{

	private:
		struct Header
		{
			uint32_t ssrc;
			uint32_t seqnr:8;
			uint32_t reserved:19;
			uint32_t index:5;
		};

	public:
		static TstItem* Parse(const uint8_t* data, size_t len);

	public:
		TstItem(Header* header);
		TstItem(TstItem* item);
		TstItem(uint32_t ssrc, uint8_t seqnr, uint8_t index);
		~TstItem();

		void Dump();
		void Serialize();
		size_t Serialize(uint8_t* data);
		size_t GetSize();

		uint32_t GetSsrc();
		uint8_t GetSeqNr();
		uint8_t GetIndex();

	private:
		// Passed by argument.
		Header* header = nullptr;
		uint8_t* raw = nullptr;
	};

	template <typename T> class FeedbackPsTstPacket
		: public FeedbackPsPacket
	{
	public:
		typedef std::vector<TstItem*>::iterator Iterator;

	public:
		static const std::string Name;
		static const FeedbackPs::MessageType MessageType;
		static FeedbackPsTstPacket<T>* Parse(const uint8_t* data, size_t len);

	public:
		// Parsed Report. Points to an external data.
		FeedbackPsTstPacket(CommonHeader* commonHeader);
		FeedbackPsTstPacket(uint32_t sender_ssrc, uint32_t media_ssrc = 0);

		void Dump() override;
		size_t Serialize(uint8_t* data) override;
		size_t GetSize() override;

		void AddItem(TstItem* item);
		Iterator Begin();
		Iterator End();

	private:
		std::vector<TstItem*> items;
	};

	class Tstr {};
	class Tstn {};

	typedef FeedbackPsTstPacket<Tstr> FeedbackPsTstrPacket;
	typedef FeedbackPsTstPacket<Tstn> FeedbackPsTstnPacket;

	/* TstItem inline instance methods */

	inline
	TstItem::TstItem(TstItem* item) :
		header(item->header)
	{
	}

	inline
	size_t TstItem::GetSize()
	{
		return sizeof(Header);
	}

	inline
	uint32_t TstItem::GetSsrc()
	{
		return (uint32_t)ntohl(this->header->ssrc);
	}

	inline
	uint8_t TstItem::GetSeqNr()
	{
		return (uint8_t)this->header->seqnr;
	}

	inline
	uint8_t TstItem::GetIndex()
	{
		return (uint8_t)ntohl(this->header->index);
	}


	/* FeedbackPsTstPacket inline instance methods */

	template <typename T>
	size_t FeedbackPsTstPacket<T>::GetSize()
	{
		size_t size = FeedbackPsPacket::GetSize();

		for (auto item : this->items) {
			size += item->GetSize();
		}

		return size;
	}

	template <typename T>
	void FeedbackPsTstPacket<T>::AddItem(TstItem* item)
	{
		this->items.push_back(item);
	}
}
}

#endif
