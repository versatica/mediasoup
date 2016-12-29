#define MS_CLASS "RTC::RTCP::FeedbackRtpEcnPacket"

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
			MS_WARN("not enough space for Ecn item, discarded");
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

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("\t\t<Ecn Item>");
		MS_WARN("\t\tsequence_number: %d",    ntohl(this->header->sequence_number));
		MS_WARN("\t\tect0_counter: %d",       ntohl(this->header->ect0_counter));
		MS_WARN("\t\tect1_counter: %d",       ntohl(this->header->ect1_counter));
		MS_WARN("\t\tecn_ce_counter: %d",     ntohs(this->header->ecn_ce_counter));
		MS_WARN("\t\tnot_ect_counter: %d",    ntohs(this->header->not_ect_counter));
		MS_WARN("\t\tlost_packets: %d",       ntohs(this->header->lost_packets));
		MS_WARN("\t\tduplicated_packets: %d", ntohs(this->header->duplicated_packets));
		MS_WARN("\t\t</Ecn Item>");
	}
}}
