#ifndef MS_RTC_RTCP_FEEDBACK_PS_VBCM_HPP
#define MS_RTC_RTCP_FEEDBACK_PS_VBCM_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackPs.hpp"

/* RFC 5104
 * H.271 Video Back Channel Message (VBCM)

   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                              SSRC                             |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | Seq nr.       |
                  |0| Payload Vbcm|
                                  | Length                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                    VBCM Octet String....      |    Padding    |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

namespace RTC
{
	namespace RTCP
	{
		class FeedbackPsVbcmItem : public FeedbackItem
		{
		public:
			struct Header
			{
				uint32_t ssrc;
				uint8_t sequenceNumber;
#if defined(MS_LITTLE_ENDIAN)
				uint8_t payloadType : 7;
				uint8_t zero : 1;
#elif defined(MS_BIG_ENDIAN)
				uint8_t zero : 1;
				uint8_t payloadType : 7;
#endif
				uint16_t length;
				uint8_t value[];
			};

		public:
			static const size_t HeaderSize{ 8 };
			static const FeedbackPs::MessageType messageType{ FeedbackPs::MessageType::FIR };

		public:
			explicit FeedbackPsVbcmItem(Header* header) : header(header)
			{
			}
			explicit FeedbackPsVbcmItem(FeedbackPsVbcmItem* item) : header(item->header)
			{
			}
			FeedbackPsVbcmItem(
			  uint32_t ssrc, uint8_t sequenceNumber, uint8_t payloadType, uint16_t length, uint8_t* value);
			~FeedbackPsVbcmItem() override = default;

			uint32_t GetSsrc() const
			{
				return uint32_t{ ntohl(this->header->ssrc) };
			}
			uint8_t GetSequenceNumber() const
			{
				return uint8_t{ this->header->sequenceNumber };
			}
			uint8_t GetPayloadType() const
			{
				return uint8_t{ this->header->payloadType };
			}
			uint16_t GetLength() const
			{
				return uint16_t{ ntohs(this->header->length) };
			}
			uint8_t* GetValue() const
			{
				return this->header->value;
			}

			/* Virtual methods inherited from FeedbackItem. */
		public:
			void Dump() const override;
			size_t Serialize(uint8_t* buffer) override;
			size_t GetSize() const override
			{
				size_t size = FeedbackPsVbcmItem::HeaderSize + static_cast<size_t>(GetLength());

				// Consider pading to 32 bits (4 bytes) boundary.
				return (size + 3) & ~3;
			}

		private:
			Header* header{ nullptr };
		};

		// Vbcm packet declaration.
		using FeedbackPsVbcmPacket = FeedbackPsItemsPacket<FeedbackPsVbcmItem>;
	} // namespace RTCP
} // namespace RTC

#endif
