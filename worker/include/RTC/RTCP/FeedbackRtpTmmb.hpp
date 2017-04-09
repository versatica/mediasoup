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
		private:
			static constexpr size_t HeaderSize = 8;

		public:
			static const FeedbackRtp::MessageType MessageType;

		public:
			static FeedbackRtpTmmbItem* Parse(const uint8_t* data, size_t len);

		public:
			explicit FeedbackRtpTmmbItem(const uint8_t* data);
			FeedbackRtpTmmbItem();

			uint32_t GetSsrc() const;
			void SetSsrc(uint32_t ssrc);
			uint64_t GetBitrate() const;
			void SetBitrate(uint64_t bitrate);
			uint16_t GetOverhead() const;
			void SetOverhead(uint16_t overhead);

			/* Virtual methods inherited from FeedbackItem. */
		public:
			virtual void Dump() const override;
			virtual size_t Serialize(uint8_t* buffer) override;
			virtual size_t GetSize() const override;

		private:
			uint32_t ssrc     = 0;
			uint64_t bitrate  = 0;
			uint16_t overhead = 0;
		};

		// Tmmb types declaration.
		class FeedbackRtpTmmbr
		{
		};
		class FeedbackRtpTmmbn
		{
		};

		// Tmmbn classes declaration.
		typedef FeedbackRtpTmmbItem<FeedbackRtpTmmbr> FeedbackRtpTmmbrItem;
		typedef FeedbackRtpTmmbItem<FeedbackRtpTmmbn> FeedbackRtpTmmbnItem;

		// Tmmbn packets declaration.
		typedef FeedbackRtpItemsPacket<FeedbackRtpTmmbrItem> FeedbackRtpTmmbrPacket;
		typedef FeedbackRtpItemsPacket<FeedbackRtpTmmbnItem> FeedbackRtpTmmbnPacket;

		/* Inline instance methods. */

		template<typename T>
		FeedbackRtpTmmbItem<T>::FeedbackRtpTmmbItem()
		{
		}

		template<typename T>
		size_t FeedbackRtpTmmbItem<T>::GetSize() const
		{
			return HeaderSize;
		}

		template<typename T>
		uint32_t FeedbackRtpTmmbItem<T>::GetSsrc() const
		{
			return this->ssrc;
		}

		template<typename T>
		void FeedbackRtpTmmbItem<T>::SetSsrc(uint32_t ssrc)
		{
			this->ssrc = ssrc;
		}

		template<typename T>
		uint64_t FeedbackRtpTmmbItem<T>::GetBitrate() const
		{
			return this->bitrate;
		}

		template<typename T>
		void FeedbackRtpTmmbItem<T>::SetBitrate(uint64_t bitrate)
		{
			this->bitrate = bitrate;
		}

		template<typename T>
		uint16_t FeedbackRtpTmmbItem<T>::GetOverhead() const
		{
			return this->overhead;
		}

		template<typename T>
		void FeedbackRtpTmmbItem<T>::SetOverhead(uint16_t overhead)
		{
			this->overhead = overhead;
		}
	}
}

#endif
