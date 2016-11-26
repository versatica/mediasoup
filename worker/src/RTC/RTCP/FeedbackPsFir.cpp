#define MS_CLASS "RTC::RTCP::FeedbackPsFirPacket"

#include "RTC/RTCP/FeedbackPsFir.h"
#include "Logger.h"

#include <cstring>  // std::memcmp(), std::memcpy()
#include <string.h> // memset()

namespace RTC { namespace RTCP
{

	/* FirItem Class methods. */

	FirItem* FirItem::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE_STD();

		// data size must be >= header.
		if (sizeof(Header) > len)
		{
				MS_WARN("not enough space for Fir item, discarded");
				return nullptr;
		}

		Header* header = (Header*)data;
		return new FirItem(header);
	}

	/* FirItem Instance methods. */

	FirItem::FirItem(uint32_t ssrc, uint8_t sequence_number)
	{
		MS_TRACE_STD();

		this->raw = new uint8_t[sizeof(Header)];
		this->header = (Header*)this->raw;

		// set reserved bits to zero
		std::memset(this->header, 0, sizeof(Header));

		this->header->ssrc = htonl(ssrc);
		this->header->sequence_number = sequence_number;
	}

	size_t FirItem::Serialize(uint8_t* data)
	{
		MS_TRACE_STD();

		memcpy(data, this->header, sizeof(Header));
		return sizeof(Header);
	}

	void FirItem::Dump()
	{
		MS_TRACE_STD();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("\t\t<Fir Item>");
		MS_WARN("\t\t\tssrc: %u", ntohl(this->header->ssrc));
		MS_WARN("\t\t\tsequence_number: %u", this->header->sequence_number);
		MS_WARN("\t\t</Fir Item>");
	}

} } // RTP::RTCP

