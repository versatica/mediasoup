#ifndef MS_RTC_RTCP_FEEDBACK_TMMB_H
#define MS_RTC_RTCP_FEEDBACK_TMMB_H

#include "common.h"
#include "RTC/RTCP/Feedback.h"

#include <vector>

namespace RTC { namespace RTCP
{
	class TmmbItem
	{

	private:
		struct Header
		{
			uint32_t ssrc;
			uint32_t compact;
		};

	public:
		static TmmbItem* Parse(const uint8_t* data, size_t len);

	public:
		TmmbItem(Header* header);
		TmmbItem(TmmbItem* item);
		TmmbItem(uint32_t ssrc, uint64_t bitrate, uint32_t overhead);

		void Dump();
		void Serialize();
		size_t Serialize(uint8_t* data);
		size_t GetSize();

		bool IsCorrect();
		uint32_t GetSsrc();
		void SetSsrc(uint32_t ssrc);
		uint64_t GetBitrate();
		void SetBitrate(uint64_t bitrate);
		uint16_t GetOverhead();
		void SetOverhead(uint16_t overhead);

	private:
		// Passed by argument.
		Header* header = nullptr;
		uint8_t* raw = nullptr;

		uint64_t bitrate;
		uint16_t overhead;
		bool isCorrect = true;
	};

	template <typename T> class FeedbackRtpTmmbPacket
		: public FeedbackRtpPacket
	{
	public:
		typedef std::vector<TmmbItem*>::iterator Iterator;

	public:
		static const std::string Name;
		static const FeedbackRtp::MessageType MessageType;
		static FeedbackRtpTmmbPacket<T>* Parse(const uint8_t* data, size_t len);

	public:
		// Parsed Report. Points to an external data.
		FeedbackRtpTmmbPacket(CommonHeader* commonHeader);
		FeedbackRtpTmmbPacket(uint32_t sender_ssrc, uint32_t media_ssrc = 0);

		void Dump() override;
		size_t Serialize(uint8_t* data) override;
		size_t GetSize() override;

		void AddItem(TmmbItem* item);
		Iterator Begin();
		Iterator End();

	private:
		std::vector<TmmbItem*> items;
	};

	class Tmmbr {};
	class Tmmbn {};

	typedef FeedbackRtpTmmbPacket<Tmmbr> FeedbackRtpTmmbrPacket;
	typedef FeedbackRtpTmmbPacket<Tmmbn> FeedbackRtpTmmbnPacket;

	/* TmmbItem inline instance methods */

	inline
	size_t TmmbItem::GetSize()
	{
		return sizeof(Header);
	}

	inline
	bool TmmbItem::IsCorrect()
	{
		return this->isCorrect;
	}

	inline
	uint32_t TmmbItem::GetSsrc()
	{
		return (uint32_t)ntohl(this->header->ssrc);
	}

	inline
	void TmmbItem::SetSsrc(uint32_t ssrc)
	{
		this->header->ssrc = (uint32_t)htonl(ssrc);
	}

	inline
	uint64_t TmmbItem::GetBitrate()
	{
		return this->bitrate;
	}

	inline
	void TmmbItem::SetBitrate(uint64_t bitrate)
	{
		this->bitrate = bitrate;
	}

	inline
	uint16_t TmmbItem::GetOverhead()
	{
		return this->overhead;
	}

	inline
	void TmmbItem::SetOverhead(uint16_t overhead)
	{
		this->overhead = overhead;
	}

	/* FeedbackRtpTmmbPacket inline instance methods */

	template <typename T>
	size_t FeedbackRtpTmmbPacket<T>::GetSize()
	{
		size_t size = FeedbackRtpPacket::GetSize();

		for (auto item : this->items) {
			size += item->GetSize();
		}

		return size;
	}

	template <typename T>
	void FeedbackRtpTmmbPacket<T>::AddItem(TmmbItem* item)
	{
		this->items.push_back(item);
	}
}
}

#endif
