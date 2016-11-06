#define MS_CLASS "RTC::RTCP::FeedbackPsPsLeiPacket"

#include "RTC/RTCP/FeedbackPsLei.h"
#include "Logger.h"

#include <cstring>  // std::memcmp(), std::memcpy()

namespace RTC { namespace RTCP
{

	/* PsLeiItem Class methods. */

	PsLeiItem* PsLeiItem::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE_STD();

		// data size must be >= header + length value.
		if (sizeof(Header) > len)
		{
				MS_WARN("not enough space for PsLei item, discarded");
				return nullptr;
		}

		Header* header = (Header*)data;
		return new PsLeiItem(header);
	}

	/* PsLeiItem Instance methods. */
	PsLeiItem::PsLeiItem(uint32_t ssrc)
	{
		this->raw = new uint8_t[sizeof(Header)];
		this->header = (Header*)this->raw;

		this->header->ssrc = htonl(ssrc);
	}

	size_t PsLeiItem::Serialize(uint8_t* data)
	{
		MS_TRACE_STD();

		// Add minimum header.
		std::memcpy(data, this->header, sizeof(Header));

		return sizeof(Header);
	}

	void PsLeiItem::Dump()
	{
		MS_TRACE_STD();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("\t\t<PsLei Item>");
		MS_WARN("\t\t\tssrc: %u", ntohl(this->header->ssrc));
		MS_WARN("\t\t</PsLei Item>");
	}

} } // RTP::RTCP

