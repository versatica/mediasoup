#define MS_CLASS "RTC::RTCP::FeedbackRtpEcnPacket"
// #define MS_LOG_DEV

#include "RTC/RTCP/FeedbackRtpEcn.h"
#include "Logger.h"
#include <cstring>

namespace RTC { namespace RTCP
{
	/* Class methods. */

	EcnItem* EcnItem::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// data size must be >= header + length value.
		if (sizeof(Header) > len)
		{
			MS_WARN_TAG(rtcp, "not enough space for Ecn item, discarded");

			return nullptr;
		}

		Header* header = (Header*)data;

		return new EcnItem(header);
	}

	size_t EcnItem::Serialize(uint8_t* data)
	{
		MS_TRACE();

		// Add minimum header.
		std::memcpy(data, this->header, sizeof(Header));

		return sizeof(Header);
	}

	void EcnItem::Dump()
	{
		MS_TRACE();

		MS_DEBUG_DEV("<EcnItem>");
		MS_DEBUG_DEV("  sequence number    : %" PRIu32, ntohl(this->header->sequence_number));
		MS_DEBUG_DEV("  ect0 counter       : %" PRIu32, ntohl(this->header->ect0_counter));
		MS_DEBUG_DEV("  ect1 counter       : %" PRIu32, ntohl(this->header->ect1_counter));
		MS_DEBUG_DEV("  ecn ce counter     : %" PRIu16, ntohs(this->header->ecn_ce_counter));
		MS_DEBUG_DEV("  not ect counter    : %" PRIu16, ntohs(this->header->not_ect_counter));
		MS_DEBUG_DEV("  lost packets       : %" PRIu16, ntohs(this->header->lost_packets));
		MS_DEBUG_DEV("  duplicated packets : %" PRIu16, ntohs(this->header->duplicated_packets));
		MS_DEBUG_DEV("</EcnItem>");
	}
}}
