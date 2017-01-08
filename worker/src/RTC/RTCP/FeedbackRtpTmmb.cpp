#define MS_CLASS "RTC::RTCP::FeedbackRtpTmmbPacket"
// #define MS_LOG_DEV

#include "RTC/RTCP/FeedbackRtpTmmb.h"
#include "Logger.h"
#include <cstring>

namespace RTC { namespace RTCP
{
	/* Class methods. */
	template <typename T>
	TmmbItem<T>* TmmbItem<T>::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// Get the header.
		Header* header = (Header*)data;

		// data size must be >= header + length value.
		if (sizeof(Header) > len)
		{
			MS_WARN_TAG(rtcp, "not enough space for Tmmb item, discarded");

			return nullptr;
		}

		std::unique_ptr<TmmbItem> item(new TmmbItem(header));

		if (!item->IsCorrect())
			return nullptr;

		return item.release();
	}

	/* Instance methods. */
	template <typename T>
	TmmbItem<T>::TmmbItem(Header* header)
	{
		this->header = header;

		uint32_t compact = (uint32_t)ntohl(header->compact);
		uint8_t exponent = compact >> 26;             /* first 6 bits */
		uint32_t mantissa = (compact >> 9) & 0x1ffff; /* next 17 bits */

		this->bitrate = (mantissa << exponent);
		this->overhead = compact & 0x1ff;             /* last 9 bits */

		if ((this->bitrate >> exponent) != mantissa)
		{
			MS_WARN_TAG(rtcp, "invalid TMMB bitrate value : %u *2^%u", mantissa, exponent);

			this->isCorrect = false;
		}
	}

	template <typename T>
	size_t TmmbItem<T>::Serialize(uint8_t* buffer)
	{
		uint64_t mantissa = this->bitrate;
		uint32_t exponent = 0;

		while (mantissa > 0x1ffff /* max mantissa (17 bits) */)
		{
			mantissa >>= 1;
			++exponent;
		}

		uint32_t compact = (exponent << 26) | (mantissa << 9) | this->overhead;
		Header* header = (Header*)buffer;

		header->ssrc = this->header->ssrc;
		header->compact = htonl(compact);

		std::memcpy(buffer, header, sizeof(Header));

		return sizeof(Header);
	}

	template <typename T>
	void TmmbItem<T>::Dump()
	{
		MS_TRACE();

		MS_DEBUG_DEV("<TmmbItem>");
		MS_DEBUG_DEV("  ssrc     : %" PRIu32, (uint32_t)ntohl(this->header->ssrc));
		MS_DEBUG_DEV("  bitrate  : %" PRIu64, this->bitrate);
		MS_DEBUG_DEV("  overhead : %" PRIu16, this->overhead);
		MS_DEBUG_DEV("</TmmbItem>");
	}

	/* Specialization for Tmmbr class. */

	template<>
	const FeedbackRtp::MessageType TmmbItem<Tmmbr>::MessageType = FeedbackRtp::TMMBR;

	/* Specialization for Tmmbn class. */

	template<>
	const FeedbackRtp::MessageType TmmbItem<Tmmbn>::MessageType = FeedbackRtp::TMMBN;

	// Explicit instantiation to have all TmmbItem definitions in this file.
	template class TmmbItem<Tmmbr>;
	template class TmmbItem<Tmmbn>;
}}
