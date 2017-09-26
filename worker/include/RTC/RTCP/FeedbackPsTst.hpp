#ifndef MS_RTC_RTCP_FEEDBACK_PS_TST_HPP
#define MS_RTC_RTCP_FEEDBACK_PS_TST_HPP

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

namespace RTC
{
	namespace RTCP
	{
		template<typename T>
		class FeedbackPsTstItem : public FeedbackItem
		{
		private:
			struct Header
			{
				uint32_t ssrc;
				uint32_t sequenceNumber : 8;
				uint32_t reserved : 19;
				uint32_t index : 5;
			};

		public:
			static const FeedbackPs::MessageType messageType;

		public:
			static FeedbackPsTstItem* Parse(const uint8_t* data, size_t len);

		public:
			explicit FeedbackPsTstItem(Header* header);
			explicit FeedbackPsTstItem(FeedbackPsTstItem* item);
			FeedbackPsTstItem(uint32_t ssrc, uint8_t sequenceNumber, uint8_t index);
			~FeedbackPsTstItem() override = default;

			uint32_t GetSsrc() const;
			uint8_t GetSequenceNumber() const;
			uint8_t GetIndex() const;

			/* Virtual methods inherited from FeedbackItem. */
		public:
			void Dump() const override;
			size_t Serialize(uint8_t* buffer) override;
			size_t GetSize() const override;

		private:
			Header* header{ nullptr };
		};

		class Tstr
		{
		};

		class Tstn
		{
		};

		// Tst classes declaration.
		using FeedbackPsTstrItem = FeedbackPsTstItem<Tstr>;
		using FeedbackPsTstnItem = FeedbackPsTstItem<Tstn>;

		// Tst packets declaration.
		using FeedbackPsTstrPacket = FeedbackPsItemsPacket<FeedbackPsTstrItem>;
		using FeedbackPsTstnPacket = FeedbackPsItemsPacket<FeedbackPsTstnItem>;

		/* Inline instance methods. */

		template<typename T>
		inline FeedbackPsTstItem<T>::FeedbackPsTstItem(Header* header) : header(header)
		{
		}

		template<typename T>
		inline FeedbackPsTstItem<T>::FeedbackPsTstItem(FeedbackPsTstItem* item) : header(item->header)
		{
		}

		template<typename T>
		inline size_t FeedbackPsTstItem<T>::GetSize() const
		{
			return sizeof(Header);
		}

		template<typename T>
		inline uint32_t FeedbackPsTstItem<T>::GetSsrc() const
		{
			return uint32_t{ ntohl(this->header->ssrc) };
		}

		template<typename T>
		inline uint8_t FeedbackPsTstItem<T>::GetSequenceNumber() const
		{
			return static_cast<uint8_t>(this->header->sequenceNumber);
		}

		template<typename T>
		inline uint8_t FeedbackPsTstItem<T>::GetIndex() const
		{
			return static_cast<uint8_t>(this->header->index);
		}
	} // namespace RTCP
} // namespace RTC

#endif
