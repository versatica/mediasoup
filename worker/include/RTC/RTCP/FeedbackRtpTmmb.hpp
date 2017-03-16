#ifndef MS_RTC_RTCP_FEEDBACK_TMMB_HPP
#define MS_RTC_RTCP_FEEDBACK_TMMB_HPP

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

namespace RTC { namespace RTCP
{
	template<typename T> class TmmbItem
		: public FeedbackItem
	{
	private:
		struct Header
		{
			uint32_t ssrc;
			uint32_t compact;
		};

	public:
		static const FeedbackRtp::MessageType MessageType;

	public:
		static TmmbItem* Parse(const uint8_t* data, size_t len);

	public:
		explicit TmmbItem(Header* header);
		explicit TmmbItem(TmmbItem* item);
		TmmbItem(uint32_t ssrc, uint64_t bitrate, uint32_t overhead);
		virtual ~TmmbItem() {};

		uint32_t GetSsrc() const;
		void SetSsrc(uint32_t ssrc);
		uint64_t GetBitrate() const;
		void SetBitrate(uint64_t bitrate);
		uint16_t GetOverhead() const;
		void SetOverhead(uint16_t overhead);

	/* Virtual methods inherited from FeedbackItem. */
	public:
		virtual void Dump() override;
		virtual size_t Serialize(uint8_t* buffer) override;
		virtual size_t GetSize() const override;

	private:
		Header* header = nullptr;
		uint64_t bitrate;
		uint16_t overhead;
	};

	// Tmmb types declaration.
	class Tmmbr {};
	class Tmmbn {};

	// Tmmbn classes declaration.
	typedef TmmbItem<Tmmbr> TmmbrItem;
	typedef TmmbItem<Tmmbn> TmmbnItem;

	// Tmmbn packets declaration.
	typedef FeedbackRtpItemPacket<TmmbrItem> FeedbackRtpTmmbrPacket;
	typedef FeedbackRtpItemPacket<TmmbnItem> FeedbackRtpTmmbnPacket;

	/* Inline instance methods. */

	template <typename T>
	size_t TmmbItem<T>::GetSize() const
	{
		return sizeof(Header);
	}

	template <typename T>
	uint32_t TmmbItem<T>::GetSsrc() const
	{
		return (uint32_t)ntohl(this->header->ssrc);
	}

	template <typename T>
	void TmmbItem<T>::SetSsrc(uint32_t ssrc)
	{
		this->header->ssrc = (uint32_t)htonl(ssrc);
	}

	template <typename T>
	uint64_t TmmbItem<T>::GetBitrate() const
	{
		return this->bitrate;
	}

	template <typename T>
	void TmmbItem<T>::SetBitrate(uint64_t bitrate)
	{
		this->bitrate = bitrate;
	}

	template <typename T>
	uint16_t TmmbItem<T>::GetOverhead() const
	{
		return this->overhead;
	}

	template <typename T>
	void TmmbItem<T>::SetOverhead(uint16_t overhead)
	{
		this->overhead = overhead;
	}
}}

#endif
