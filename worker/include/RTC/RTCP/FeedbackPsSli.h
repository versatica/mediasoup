#ifndef MS_RTC_RTCP_FEEDBACK_SLI_H
#define MS_RTC_RTCP_FEEDBACK_SLI_H

#include "common.h"
#include "RTC/RTCP/Feedback.h"

#include <vector>

namespace RTC { namespace RTCP
{
	class SliItem
	{

	private:
		struct Header
		{
			uint32_t compact;
		};

	public:
		static SliItem* Parse(const uint8_t* data, size_t len);

	public:
		SliItem(Header* header);
		SliItem(SliItem* item);
		SliItem(uint16_t first, uint16_t number, uint8_t pictureId);

		void Dump();
		void Serialize();
		size_t Serialize(uint8_t* data);
		size_t GetSize();

		uint16_t GetFirst();
		void SetFirst(uint16_t first);
		uint16_t GetNumber();
		void SetNumber(uint16_t number);
		uint8_t GetPictureId();
		void SetPictureId(uint8_t pictureId);

	private:
		// Passed by argument.
		Header* header = nullptr;
		uint8_t* raw = nullptr;

		uint16_t first;
		uint16_t number;
		uint8_t pictureId;
	};

	class FeedbackPsSliPacket
		: public FeedbackPsPacket
	{
	public:
		typedef std::vector<SliItem*>::iterator Iterator;

	public:
		static FeedbackPsSliPacket* Parse(const uint8_t* data, size_t len);

	public:
		// Parsed Report. Points to an external data.
		FeedbackPsSliPacket(CommonHeader* commonHeader);
		FeedbackPsSliPacket(uint32_t sender_ssrc, uint32_t media_ssrc = 0);

		void Dump() override;
		size_t Serialize(uint8_t* data) override;
		size_t GetSize() override;

		void AddItem(SliItem* item);
		Iterator Begin();
		Iterator End();

	private:
		std::vector<SliItem*> items;
	};

	/* SliItem inline instance methods */

	inline
	size_t SliItem::GetSize()
	{
		return sizeof(Header);
	}

	inline
	uint16_t SliItem::GetFirst()
	{
		return this->first;
	}

	inline
	void SliItem::SetFirst(uint16_t first)
	{
		this->first = first;
	}

	inline
	uint16_t SliItem::GetNumber()
	{
		return this->number;
	}

	inline
	void SliItem::SetNumber(uint16_t number)
	{
		this->number = number;
	}

	inline
	uint8_t SliItem::GetPictureId()
	{
		return this->pictureId;
	}

	inline
	void SliItem::SetPictureId(uint8_t pictureId)
	{
		this->pictureId = pictureId;
	}

	/* FeedbackPsSliPacket inline instance methods */

	inline
	size_t FeedbackPsSliPacket::GetSize()
	{
		size_t size = FeedbackPsPacket::GetSize();

		for (auto item : this->items) {
			size += item->GetSize();
		}

		return size;
	}

	inline
	void FeedbackPsSliPacket::AddItem(SliItem* item)
	{
		this->items.push_back(item);
	}

	inline
	FeedbackPsSliPacket::Iterator FeedbackPsSliPacket::Begin()
	{
		return this->items.begin();
	}

	inline
	FeedbackPsSliPacket::Iterator FeedbackPsSliPacket::End()
	{
		return this->items.end();
	}
}
}

#endif
