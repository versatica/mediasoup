#ifndef MS_RTC_RTCP_FEEDBACK_PS_TST_HPP
#define MS_RTC_RTCP_FEEDBACK_PS_TST_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackPs.hpp"

/* RFC 5104
 * Temporal-Spatial Trade-off Request (TSTR)
 * Temporal-Spatial Trade-off Notification (TSTN)

   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                              SSRC                             |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |  Seq nr.      |  Reserved                           | Index   |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

namespace RTC
{
	namespace RTCP
	{
		template<typename T>
		class FeedbackPsTstItem : public FeedbackItem
		{
		public:
#ifdef _WIN32
#pragma pack(push, 1)
#define MEDIASOUP_PACKED
#else
#define MEDIASOUP_PACKED __attribute__((packed))
#endif
			struct Header
			{
				uint32_t ssrc;
				uint8_t sequenceNumber;
				uint16_t reserved1;
#if defined(MS_LITTLE_ENDIAN)
				uint8_t index : 5;
				uint8_t reserved2 : 3;
#elif defined(MS_BIG_ENDIAN)
				uint8_t reserved2 : 3;
				uint8_t index : 5;
#endif
			} MEDIASOUP_PACKED;
#ifdef _WIN32
#pragma pack(pop)
#endif
#undef MEDIASOUP_PACKED

		public:
			static const size_t HeaderSize{ 8 };
			static const FeedbackPs::MessageType messageType;

		public:
			explicit FeedbackPsTstItem(Header* header) : header(header)
			{
			}
			explicit FeedbackPsTstItem(FeedbackPsTstItem* item) : header(item->header)
			{
			}
			FeedbackPsTstItem(uint32_t ssrc, uint8_t sequenceNumber, uint8_t index);
			~FeedbackPsTstItem() override = default;

			uint32_t GetSsrc() const
			{
				return uint32_t{ ntohl(this->header->ssrc) };
			}
			uint8_t GetSequenceNumber() const
			{
				return this->header->sequenceNumber;
			}
			uint8_t GetIndex() const
			{
				return this->header->index;
			}

			/* Virtual methods inherited from FeedbackItem. */
		public:
			void Dump() const override;
			size_t Serialize(uint8_t* buffer) override;
			size_t GetSize() const override
			{
				return FeedbackPsTstItem::HeaderSize;
			}

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
	} // namespace RTCP
} // namespace RTC

#endif
