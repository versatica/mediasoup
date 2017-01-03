#define MS_CLASS "RTC::RTCP::FeedbackPsFirPacket"
// #define MS_LOG_DEV

#include "RTC/RTCP/FeedbackPsFir.h"
#include "Logger.h"
#include <cstring>
#include <string.h> // std::memset()

namespace RTC { namespace RTCP
{
	/* Class methods. */

	FirItem* FirItem::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// data size must be >= header.
		if (sizeof(Header) > len)
		{
			MS_WARN_TAG(rtcp, "not enough space for Fir item, discarded");

			return nullptr;
		}

		Header* header = (Header*)data;

		return new FirItem(header);
	}

	/* Instance methods. */

	FirItem::FirItem(uint32_t ssrc, uint8_t sequence_number)
	{
		MS_TRACE();

		this->raw = new uint8_t[sizeof(Header)];
		this->header = (Header*)this->raw;

		// set reserved bits to zero
		std::memset(this->header, 0, sizeof(Header));

		this->header->ssrc = htonl(ssrc);
		this->header->sequence_number = sequence_number;
	}

	size_t FirItem::Serialize(uint8_t* data)
	{
		MS_TRACE();

		std::memcpy(data, this->header, sizeof(Header));

		return sizeof(Header);
	}

	void FirItem::Dump()
	{
		MS_TRACE();

		MS_DEBUG_DEV("<FirItem>");
		MS_DEBUG_DEV("  ssrc            : %" PRIu32, ntohl(this->header->ssrc));
		MS_DEBUG_DEV("  sequence number : %" PRIu8, this->header->sequence_number);
		MS_DEBUG_DEV("\t\t</FirItem>");
	}
}}
