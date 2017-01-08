#define MS_CLASS "RTC::RTCP::SenderReport"
// #define MS_LOG_DEV

#include "RTC/RTCP/SenderReport.h"
#include "Logger.h"
#include <cstring>

namespace RTC { namespace RTCP
{
	/* Class methods. */

	SenderReport* SenderReport::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// Get the header.
		Header* header = (Header*)data;

			// Packet size must be >= header size.
		if (sizeof(Header) > len)
		{
			MS_WARN_TAG(rtcp, "not enough space for sender report, packet discarded");

			return nullptr;
		}

		return new SenderReport(header);
	}

	/* Instance methods. */

	void SenderReport::Dump()
	{
		MS_TRACE();

		MS_DEBUG_DEV("<SenderReport>");
		MS_DEBUG_DEV("  ssrc         : %" PRIu32, (uint32_t)ntohl(this->header->ssrc));
		MS_DEBUG_DEV("  ntp sec      : %" PRIu32, (uint32_t)ntohl(this->header->ntp_sec));
		MS_DEBUG_DEV("  ntp frac     : %" PRIu32, (uint32_t)ntohl(this->header->ntp_frac));
		MS_DEBUG_DEV("  rtp ts       : %" PRIu32, (uint32_t)ntohl(this->header->rtp_ts));
		MS_DEBUG_DEV("  packet count : %" PRIu32, (uint32_t)ntohl(this->header->packet_count));
		MS_DEBUG_DEV("  octet count  : %" PRIu32, (uint32_t)ntohl(this->header->octet_count));
		MS_DEBUG_DEV("</SenderReport>");
	}

	void SenderReport::Serialize()
	{
		MS_TRACE();

		// Allocate internal data.
		if (!this->raw)
		{
			this->raw = new uint8_t[sizeof(Header)];

			// Copy the header.
			std::memcpy(this->raw, this->header, sizeof(Header));

			// Update the header pointer.
			this->header = (Header*)(this->raw);
		}
	}

	size_t SenderReport::Serialize(uint8_t* data)
	{
		MS_TRACE();

		// Copy the header.
		std::memcpy(data, this->header, sizeof(Header));

		return sizeof(Header);
	}

	/* Class methods. */

	SenderReportPacket* SenderReportPacket::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		std::unique_ptr<SenderReportPacket> packet(new SenderReportPacket());
		size_t offset = sizeof(Packet::CommonHeader);

		SenderReport* report = SenderReport::Parse(data+offset, len-offset);
		if (report)
			packet->AddReport(report);
		else
			return packet.release();

		return packet.release();
	}

	/* Instance methods. */

	size_t SenderReportPacket::Serialize(uint8_t* data)
	{
		MS_TRACE();

		MS_ASSERT(this->reports.size() == 1, "invalid number of sender reports");

		size_t offset = Packet::Serialize(data);

		// Serialize reports.
		for (auto report : this->reports)
		{
			offset += report->Serialize(data + offset);
		}

		return offset;
	}

	void SenderReportPacket::Dump()
	{
		#ifdef MS_LOG_DEV

		MS_TRACE();

		MS_DEBUG_DEV("<SenderReportPacket>");
		for (auto report : this->reports)
		{
			report->Dump();
		}
		MS_DEBUG_DEV("</SenderReportPacket>");

		#endif
	}
}}
