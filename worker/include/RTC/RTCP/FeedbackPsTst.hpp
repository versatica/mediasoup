#ifndef MS_RTC_RTCP_FEEDBACK_TST_HPP
#define MS_RTC_RTCP_FEEDBACK_TST_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackPs.hpp"

/* RFC 5104
 * Temporal-Spatial Trade-off Request (TSTR)
 * Temporal-Spatial Trade-off Notification (TSTN)

    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
0  |                              SSRC                             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
4  |  Seq nr.      |
5                  |  Reserved                           | Index   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

namespace RTC { namespace RTCP
{
	template<typename T> class TstItem
		: public FeedbackItem
	{
	private:
		struct Header
		{
			uint32_t ssrc;
			uint32_t sequence_number:8;
			uint32_t reserved:19;
			uint32_t index:5;
		};

	public:
		static const FeedbackPs::MessageType MessageType;

	public:
		static TstItem* Parse(const uint8_t* data, size_t len);

	public:
		explicit TstItem(Header* header);
		explicit TstItem(TstItem* item);
		TstItem(uint32_t ssrc, uint8_t sequenceNumber, uint8_t index);
		virtual ~TstItem() {};

		uint32_t GetSsrc() const;
		uint8_t GetSequenceNumber() const;
		uint8_t GetIndex() const;

	/* Virtual methods inherited from FeedbackItem. */
	public:
		virtual void Dump() override;
		virtual size_t Serialize(uint8_t* buffer) override;
		virtual size_t GetSize() const override;

	private:
		Header* header = nullptr;
	};

	class Tstr {};
	class Tstn {};

	// Tst classes declaration.
	typedef TstItem<Tstr> TstrItem;
	typedef TstItem<Tstn> TstnItem;

	// Tst packets declaration.
	typedef FeedbackPsItemPacket<TstrItem> FeedbackPsTstrPacket;
	typedef FeedbackPsItemPacket<TstnItem> FeedbackPsTstnPacket;

	/* Inline instance methods. */

	template <typename T>
	TstItem<T>::TstItem(Header* header):
		header(header)
	{}

	template <typename T>
	TstItem<T>::TstItem(TstItem* item) :
		header(item->header)
	{}

	template <typename T>
	size_t TstItem<T>::GetSize() const
	{
		return sizeof(Header);
	}

	template <typename T>
	uint32_t TstItem<T>::GetSsrc() const
	{
		return (uint32_t)ntohl(this->header->ssrc);
	}

	template <typename T>
	uint8_t TstItem<T>::GetSequenceNumber() const
	{
		return (uint8_t)this->header->sequence_number;
	}

	template <typename T>
	uint8_t TstItem<T>::GetIndex() const
	{
		return (uint8_t)this->header->index;
	}
}}

#endif
