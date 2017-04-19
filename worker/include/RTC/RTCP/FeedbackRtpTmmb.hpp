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
		template<typename t>
		class FeedbackRtpTmmbItem : public FeedbackItem
		{
		private:
			static constexpr size_t HeaderSize{ 8 };

		public:
			static const FeedbackRtp::MessageType messageType;

		public:
			static FeedbackRtpTmmbItem* Parse(const uint8_t* data, size_t len);

		public:
			explicit FeedbackRtpTmmbItem(const uint8_t* data);
			FeedbackRtpTmmbItem();
			~FeedbackRtpTmmbItem() override = default;

			uint32_t GetSsrc() const;
			void SetSsrc(uint32_t ssrc);
			uint64_t GetBitrate() const;
			void SetBitrate(uint64_t bitrate);
			uint16_t GetOverhead() const;
			void SetOverhead(uint16_t overhead);

			/* Virtual methods inherited from FeedbackItem. */
		public:
			void Dump() const override;
			size_t Serialize(uint8_t* buffer) override;
			size_t GetSize() const override;

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

		/* Inline instance methods. */

		template<typename T>
		inline FeedbackRtpTmmbItem<T>::FeedbackRtpTmmbItem() = default;

		template<typename T>
		inline size_t FeedbackRtpTmmbItem<T>::GetSize() const
		{
			return HeaderSize;
		}

		template<typename T>
		inline uint32_t FeedbackRtpTmmbItem<T>::GetSsrc() const
		{
			return this->ssrc;
		}

		template<typename T>
		inline void FeedbackRtpTmmbItem<T>::SetSsrc(uint32_t ssrc)
		{
			this->ssrc = ssrc;
		}

		template<typename T>
		inline uint64_t FeedbackRtpTmmbItem<T>::GetBitrate() const
		{
			return this->bitrate;
		}

		template<typename T>
		inline void FeedbackRtpTmmbItem<T>::SetBitrate(uint64_t bitrate)
		{
			this->bitrate = bitrate;
		}

		template<typename T>
		inline uint16_t FeedbackRtpTmmbItem<T>::GetOverhead() const
		{
			return this->overhead;
		}

		template<typename T>
		inline void FeedbackRtpTmmbItem<T>::SetOverhead(uint16_t overhead)
		{
			this->overhead = overhead;
		}
	} // namespace RTCP
} // namespace RTC

#endif
