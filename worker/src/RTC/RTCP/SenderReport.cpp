#define MS_CLASS "RTC::RTCP::SenderReport"

#include "RTC/RTCP/SenderReport.h"
#include "Logger.h"
#include <cstring>  // std::memcmp(), std::memcpy()

namespace RTC { namespace RTCP
{
	/* SenderReport Class methods. */

	SenderReport* SenderReport::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE_STD();

		// Get the header.
		Header* header = (Header*)data;

			// Packet size must be >= header size.
		if (sizeof(Header) > len)
		{
				MS_WARN("not enough space for sender report, packet discarded");

				return nullptr;
		}

		return new SenderReport(header);
	}

	/* SenderReport Instance methods. */

	void SenderReport::Dump()
	{
		MS_TRACE_STD();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("\t<SenderReport>");
		MS_WARN("\t\tssrc: %u", ntohl(this->header->ssrc));
		MS_WARN("\t\tntp_sec: %u", ntohl(this->header->ntp_sec));
		MS_WARN("\t\tntp_frac: %u", ntohl(this->header->ntp_frac));
		MS_WARN("\t\trtp_ts: %u", ntohl(this->header->rtp_ts));
		MS_WARN("\t\tpacket_count: %u", ntohl(this->header->packet_count));
		MS_WARN("\t\toctet_count: %u", ntohl(this->header->octet_count));
		MS_WARN("\t</SenderReport>");
	}

	void SenderReport::Serialize()
	{
		MS_TRACE_STD();

		// Allocate internal data.
		if (!this->raw) {
			this->raw = new uint8_t[sizeof(Header)];

			// Copy the header.
			std::memcpy(this->raw, this->header, sizeof(Header));

			// Update the header pointer.
			this->header = (Header*)(this->raw);
		}
	}

	size_t SenderReport::Serialize(uint8_t* data)
	{
		MS_TRACE_STD();

		// Copy the header.
		std::memcpy(data, this->header, sizeof(Header));

		return sizeof(Header);
	}

	/* SenderReportPacket Class methods. */

	SenderReportPacket* SenderReportPacket::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE_STD();

		std::auto_ptr<SenderReportPacket> packet(new SenderReportPacket());

		size_t offset = sizeof(Packet::CommonHeader);

		SenderReport* report = SenderReport::Parse(data+offset, len-offset);
		if (report) {
			packet->AddReport(report);
		} else {
			return packet.release();
		}

		return packet.release();
	}

	/* SenderReportPacket Instance methods. */

	size_t SenderReportPacket::Serialize(uint8_t* data)
	{
		MS_TRACE_STD();

		MS_ASSERT(this->reports.size() == 1, "invalid number of sender reports");

		size_t offset = Packet::Serialize(data);

		// Serialize reports
		for(auto report : this->reports) {
			offset += report->Serialize(data + offset);
		}

		return offset;
	}

	void SenderReportPacket::Dump()
	{
		MS_TRACE_STD();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("<SenderReportPacket>");

		for(auto report : this->reports) {
			report->Dump();
		}

		MS_WARN("</SenderReportPacket>");
	}

} } // RTP::RTCP
