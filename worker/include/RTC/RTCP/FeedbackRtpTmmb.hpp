#ifndef MS_RTC_RTCP_FEEDBACK_RTP_TMMB_HPP
#define MS_RTC_RTCP_FEEDBACK_RTP_TMMB_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackRtp.hpp"

/* RFC 5104
 * Temporary Maximum Media Stream Bit Rate Request (TMMBR)
 * Temporary Maximum Media Stream Bit Rate Notification (TMMBN)

   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                              SSRC                             |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | MxTBR Exp |  MxTBR Mantissa                 |Measured Overhead|
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

namespace RTC
{
	namespace RTCP
	{
		template<typename T>
		class FeedbackRtpTmmbItem : public FeedbackItem
		{
		public:
			struct Header
			{
				uint32_t ssrc;
				uint32_t compact;
			};

		public:
			static const size_t HeaderSize{ 8 };
			static const FeedbackRtp::MessageType messageType;

		public:
			FeedbackRtpTmmbItem() = default;
			explicit FeedbackRtpTmmbItem(const uint8_t* data);
			explicit FeedbackRtpTmmbItem(const Header* header);
			~FeedbackRtpTmmbItem() override = default;

			uint32_t GetSsrc() const
			{
				return this->ssrc;
			}
			void SetSsrc(uint32_t ssrc)
			{
				this->ssrc = ssrc;
			}
			uint64_t GetBitrate() const
			{
				return this->bitrate;
			}
			void SetBitrate(uint64_t bitrate)
			{
				this->bitrate = bitrate;
			}
			uint16_t GetOverhead() const
			{
				return this->overhead;
			}
			void SetOverhead(uint16_t overhead)
			{
				this->overhead = overhead;
			}

			/* Virtual methods inherited from FeedbackItem. */
		public:
			void Dump() const override;
			size_t Serialize(uint8_t* buffer) override;
			size_t GetSize() const override
			{
				return FeedbackRtpTmmbItem::HeaderSize;
			}

		private:
			uint32_t ssrc{ 0 };
			uint64_t bitrate{ 0 };
			uint16_t overhead{ 0 };
		};

		// Tmmb types declaration.
		class FeedbackRtpTmmbr
		{
		};
		class FeedbackRtpTmmbn
		{
		};

		// Tmmbn classes declaration.
		using FeedbackRtpTmmbrItem = FeedbackRtpTmmbItem<FeedbackRtpTmmbr>;
		using FeedbackRtpTmmbnItem = FeedbackRtpTmmbItem<FeedbackRtpTmmbn>;

		// Tmmbn packets declaration.
		using FeedbackRtpTmmbrPacket = FeedbackRtpItemsPacket<FeedbackRtpTmmbrItem>;
		using FeedbackRtpTmmbnPacket = FeedbackRtpItemsPacket<FeedbackRtpTmmbnItem>;
	} // namespace RTCP
} // namespace RTC

#endif
